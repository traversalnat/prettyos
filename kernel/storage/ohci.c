/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f?r die Verwendung dieses Sourcecodes siehe unten
*/

#include "ohci.h"
#include "util.h"
#include "timer.h"
#include "kheap.h"
#include "task.h"
#include "irq.h"
#include "keyboard.h"

//#define OHCI_SCENARIO // ed/td experiments


static uint8_t index   = 0;
static ohci_t* curOHCI = 0;
static ohci_t* ohci[OHCIMAX];
static bool    OHCI_USBtransferFlag = false;


static void ohci_handler(registers_t* r, pciDev_t* device);
static void ohci_start();
static void showPortstatus(ohci_t* o);


void ohci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize)
{
  #ifdef _OHCI_DIAGNOSIS_
    printf("\n>>>ohci_install<<<\n");
  #endif

    curOHCI = ohci[index]   = malloc(sizeof(ohci_t), 0, "ohci");
    ohci[index]->PCIdevice  = PCIdev;
    ohci[index]->PCIdevice->data = ohci[index];
    ohci[index]->bar        = (uintptr_t)paging_acquirePciMemory(bar_phys,1);
    uint16_t offset         = bar_phys % PAGESIZE;
    ohci[index]->memSize    = memorySize;
    ohci[index]->num        = index;

  #ifdef _OHCI_DIAGNOSIS_
    printf("\nOHCI_MMIO %Xh mapped to virt addr %Xh, offset: %xh", bar_phys, ohci[index]->bar, offset);
  #endif

    ohci[index]->bar+= offset;
    ohci[index]->OpRegs = (ohci_OpRegs_t*) (ohci[index]->bar);

    char str[10];
    snprintf(str, 10, "OHCI %u", index+1);

    scheduler_insertTask(create_cthread(&ohci_start, str));

    index++;
    sleepMilliSeconds(20); // HACK: Avoid race condition between ohci_install and the thread just created. Problem related to curOHCI global variable
}

static void ohci_start()
{
    ohci_t* o = curOHCI;

  #ifdef _OHCI_DIAGNOSIS_
    printf("\n>>>startOHCI<<<\n");
  #endif

    ohci_initHC(o);
    printf("\n\n>>> Press key to close this console. <<<");
    getch();
}

void ohci_initHC(ohci_t* o)
{
  #ifdef _OHCI_DIAGNOSIS_
    printf("\n>>>initOHCIHostController<<<\n");
  #endif

    textColor(HEADLINE);
    printf("Initialize OHCI Host Controller:");
    textColor(TEXT);

    // pci bus data
    uint8_t bus  = o->PCIdevice->bus;
    uint8_t dev  = o->PCIdevice->device;
    uint8_t func = o->PCIdevice->func;

    // prepare PCI command register
    // bit 9: Fast Back-to-Back Enable // not necessary
    // bit 2: Bus Master               // cf. http://forum.osdev.org/viewtopic.php?f=1&t=20255&start=0
    uint16_t pciCommandRegister = pci_config_read(bus, dev, func, PCI_COMMAND, 2);
    pci_config_write_dword(bus, dev, func, PCI_COMMAND, pciCommandRegister | PCI_CMD_IO | PCI_CMD_MMIO | PCI_CMD_BUSMASTER); // resets status register, sets command register
    //uint8_t pciCapabilitiesList = pci_config_read(bus, dev, func, PCI_CAPLIST, 1);

  #ifdef _OHCI_DIAGNOSIS_
    printf("\nPCI Command Register before:          %xh", pciCommandRegister);
    printf("\nPCI Command Register plus bus master: %xh", pci_config_read(bus, dev, func, PCI_COMMAND, 2));
    //printf("\nPCI Capabilities List: first Pointer: %yh", pciCapabilitiesList);
 #endif
    irq_installPCIHandler(o->PCIdevice->irq, ohci_handler, o->PCIdevice);

    OHCI_USBtransferFlag = true;
    o->enabledPorts      = false;

    ohci_resetHC(o);
}

void ohci_resetHC(ohci_t* o)
{
  #ifdef _OHCI_DIAGNOSIS_
    printf("\n\n>>>ohci_resetHostController<<<\n");
  #endif

    // Revision and Number Downstream Ports (NDP)
    /*
    When checking the Revision, the HC Driver must mask the rest of the bits in the HcRevision register
    as they are used to specify which optional features that are supported by the HC.
    */
    textColor(IMPORTANT);
    printf("\nOHCI: Revision %u.%u, Number Downstream Ports: %u\n",
        BYTE1(o->OpRegs->HcRevision) >> 4,
        BYTE1(o->OpRegs->HcRevision) & 0xF,
        BYTE1(o->OpRegs->HcRhDescriptorA)); // bits 7:0 provide Number Downstream Ports (NDP)
    textColor(TEXT);

    if (!((BYTE1(o->OpRegs->HcRevision)) == 0x10 || BYTE1(o->OpRegs->HcRevision) == 0x11))
    {
        textColor(ERROR);
        printf("Revision not valid!");
        textColor(TEXT);
    }

    o->OpRegs->HcInterruptDisable = OHCI_INT_MIE;

    if (o->OpRegs->HcControl & OHCI_CTRL_IR) // SMM driver is active because the InterruptRouting bit is set
    {
        o->OpRegs->HcCommandStatus |= OHCI_STATUS_OCR; // ownership change request

        // monitor the IR bit to determine when the ownership change has taken effect
        uint16_t i;
        for (i=0; (o->OpRegs->HcControl & OHCI_CTRL_IR) && (i < 1000); i++)
        {
             sleepMilliSeconds(1);
        }

        if (i < 1000)
        {
            // Once the IR bit is cleared, the HC driver may proceed to the setup of the HC.
            textColor(SUCCESS);
            printf("\nOHCI takes control from SMM after %u loops.", i);
            textColor(TEXT);
        }
        else
        {
            textColor(ERROR);
            printf("\nOwnership change request did not work. SMM has still control.");
            textColor(TEXT);

            o->OpRegs->HcControl &= ~OHCI_CTRL_IR; // we try to reset the IR bit
            sleepMilliSeconds(200);

            if (o->OpRegs->HcControl & OHCI_CTRL_IR) // SMM driver is still active
            {
                textColor(ERROR);
                printf("\nOHCI taking control from SMM did not work."); // evil
                textColor(TEXT);
            }
            else
            {
                textColor(SUCCESS);
                printf("\nSuccess in taking control from SMM.");
                textColor(TEXT);
            }
        }
    }
    else // InterruptRouting bit is not set
    {
        if ((o->OpRegs->HcControl & OHCI_CTRL_HCFS) != OHCI_USB_RESET)
        {
            // there is an active BIOS driver, if the InterruptRouting bit is not set
            // and the HostControllerFunctionalState (HCFS) is not USBRESET
            printf("\nThere is an active BIOS OHCI driver");

            if ((o->OpRegs->HcControl & OHCI_CTRL_HCFS) != OHCI_USB_OPERATIONAL)
            {
                // If the HostControllerFunctionalState is not USBOPERATIONAL, the OS driver should set the HCFS to USBRESUME
                printf("\nActivate RESUME");
                o->OpRegs->HcControl &= ~OHCI_CTRL_HCFS; // clear HCFS bits
                o->OpRegs->HcControl |= OHCI_USB_RESUME; // set specific HCFS bit

                // and wait the minimum time specified in the USB Specification for assertion of resume on the USB
                sleepMilliSeconds(10);
            }
        }
        else // HCFS is USBRESET
        {
            // Neither SMM nor BIOS
            sleepMilliSeconds(10);
        }
    }

    // setup of the Host Controller
    printf("\n\nSetup of the HC\n");

    // The HC Driver should now save the contents of the HcFmInterval register ...
    uint32_t saveHcFmInterval = o->OpRegs->HcFmInterval;

    // ... and then issue a software reset
    o->OpRegs->HcCommandStatus |= OHCI_STATUS_RESET;
    sleepMilliSeconds(20);

    // After the software reset is complete (a maximum of 10 ms), the Host Controller Driver
    // should restore the value of the HcFmInterval register
    o->OpRegs->HcFmInterval = saveHcFmInterval;

    /*
    The HC is now in the USBSUSPEND state; it must not stay in this state more than 2 ms
    or the USBRESUME state will need to be entered for the minimum time specified
    in the USB Specification for the assertion of resume on the USB.
    */

    if ((o->OpRegs->HcControl & OHCI_CTRL_HCFS) == OHCI_USB_SUSPEND)
    {
        o->OpRegs->HcControl &= ~OHCI_CTRL_HCFS; // clear HCFS bits
        o->OpRegs->HcControl |= OHCI_USB_RESUME; // set specific HCFS bit
        sleepMilliSeconds(10);
    }

    /////////////////////
    // initializations //
    /////////////////////

    // HCCA
    /*
    Initialize the device data HCCA block to match the current device data state;
    i.e., all virtual queues are run and constructed into physical queues on the HCCA block
    and other fields initialized accordingly.
    */
    void* hccaVirt = malloc(sizeof(ohci_HCCA_t), OHCI_HCCA_ALIGN, "ohci HCCA"); // HCCA must be minimum 256-byte aligned
    memset(hccaVirt, 0, sizeof(ohci_HCCA_t));
    o->hcca = (ohci_HCCA_t*)hccaVirt;
    // TODO: ...

  #ifdef _OHCI_DIAGNOSIS_
    printf("\nHCCA (phys. address): %X", o->OpRegs->HcHCCA);
  #endif

    /*
    Initialize the Operational Registers to match the current device data state;
    i.e., all virtual queues are run and constructed into physical queues for HcControlHeadED and HcBulkHeadED
    */
    // ED Pool: 64 EDs (size: ED)
    // TD Pool: 56 TDs (size: TD+1024)    
    // ED and TD are part of ohci_t

    // Set the HcHCCA to the physical address of the HCCA block
    o->OpRegs->HcHCCA = paging_getPhysAddr(hccaVirt);

    // Set HcInterruptEnable to have all interrupt enabled except Start-of-Frame detect
    o->OpRegs->HcInterruptDisable = OHCI_INT_MIE;
    o->OpRegs->HcInterruptStatus  = ~0;
    o->OpRegs->HcInterruptEnable  = OHCI_INT_SO   | // scheduling overrun
                                    OHCI_INT_WDH  | // write back done head
                                  //OHCI_INT_SF   | // start of frame
                                    OHCI_INT_RD   | // resume detected
                                    OHCI_INT_UE   | // unrecoverable error
                                    OHCI_INT_FNO  | // frame number overflow
                                    OHCI_INT_RHSC | // root hub status change
                                    OHCI_INT_OC   | // ownership change
                                    OHCI_INT_MIE;   // (de)activates interrupts

    // Set HcControl to have �all queues on�
    o->OpRegs->HcControl |=   OHCI_CTRL_CLE | OHCI_CTRL_BLE; // activate control and bulk transfers
    o->OpRegs->HcControl &= ~(OHCI_CTRL_PLE | OHCI_CTRL_IE); // de-activate periodical and isochronous transfers

    // Set HcPeriodicStart to a value that is 90% of the value in FrameInterval field of the HcFmInterval register
    // When HcFmRemaining reaches this value, periodic lists gets priority over control/bulk processing
    o->OpRegs->HcPeriodicStart = (o->OpRegs->HcFmInterval & 0x3FFF) * 90/100;

    /*
    The HCD then begins to send SOF tokens on the USB by writing to the HcControl register with
    the HostControllerFunctionalState set to USBOPERATIONAL and the appropriate enable bits set.
    The Host Controller begins sending SOF tokens within one ms
    (if the HCD needs to know when the SOFs it may unmask the StartOfFrame interrupt).
    */

    printf("\n\nHC will be activated.\n");

    o->OpRegs->HcControl &= ~OHCI_CTRL_HCFS;      // clear HCFS bits
    o->OpRegs->HcControl |= OHCI_USB_OPERATIONAL; // set specific HCFS bit

    o->OpRegs->HcRhStatus |= OHCI_RHS_LPSC;           // SetGlobalPower: turn on power to all ports
    o->rootPorts = BYTE1(o->OpRegs->HcRhDescriptorA); // NumberDownstreamPorts

    // duration HCD has to wait before accessing a powered-on port of the Root Hub.
    // It is implementation-specific. Duration is calculated as POTPGT * 2 ms.
    sleepMilliSeconds(2 * BYTE4(o->OpRegs->HcRhDescriptorA));

    textColor(IMPORTANT);
    printf("\n\nFound %i Rootports.\n", o->rootPorts);
    textColor(TEXT);

    for (uint8_t j=0; j < o->rootPorts; j++)
    {
        o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PRS; 
        sleepMilliSeconds(20);

        o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_CCS; 
        sleepMilliSeconds(20);

        o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PES; 
        sleepMilliSeconds(20);
    }

    //
    //
}


/*******************************************************************************************************
*                                                                                                      *
*                                              PORTS                                                   *
*                                                                                                      *
*******************************************************************************************************/

void showPortstatus(ohci_t* o)
{
    for (uint8_t j=0; j<o->rootPorts; j++)
    {
        if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_CSC)
        {
            textColor(IMPORTANT);
            printf("\nport[%u]:", j+1);
            textColor(TEXT);
        
            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_LSDA) { printf(" LowSpeed");            }
            else                                               { printf(" FullSpeed");           }
            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_CCS)  { textColor(SUCCESS);   printf(" dev. attached -");     
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PES;                                   }
            
            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PES)  { textColor(SUCCESS);   printf(" enabled  -");  }
            else                                               { textColor(IMPORTANT); printf(" disabled -");  } 
            textColor(TEXT);
        
            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PSS)  { printf(" susp.     -");         }
            else                                               { printf(" not susp. -");         } 
        
            textColor(ERROR);
            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_POCI) { printf(" overcurrent -");       }
            textColor(TEXT);

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PRS)  { printf(" reset -");             }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PPS)  { printf(" pow on -  ");          }
            else                                               { printf(" pow off - ");          } 

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_CSC)  { printf(" CSC -");              
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_CSC;                                   }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PESC) { printf(" enable Change -");    
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PESC;                                  }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PSSC) { printf(" resume compl. -");     
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PSSC;                                  }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_OCIC) { printf(" overcurrent Change -");
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_OCIC;                                  }

            if (o->OpRegs->HcRhPortStatus[j] & OHCI_PORT_PRSC) { printf(" Reset Complete -");    
                o->OpRegs->HcRhPortStatus[j] |= OHCI_PORT_PRSC;                                  }
        }
    }
}


/*******************************************************************************************************
*                                                                                                      *
*                                              ohci handler                                            *
*                                                                                                      *
*******************************************************************************************************/

static void ohci_handler(registers_t* r, pciDev_t* device)
{
    // Check if an OHCI controller issued this interrupt
    ohci_t* o = device->data;
    bool found = false;
    uint8_t i;
    for (i=0; i<OHCIMAX; i++)
    {
        if (o == ohci[i])
        {
            textColor(TEXT);
            found = true;
            break;
        }
    }

    volatile uint32_t val = o->OpRegs->HcInterruptStatus;
    uint32_t handled = 0;
    uintptr_t phys;

    if(!found || o==0 || val==0) // No interrupt from corresponding ohci device found
    {
      #ifdef _OHCI_DIAGNOSIS_
        textColor(ERROR);
        printf("interrupt did not come from ohci device!\n");
        textColor(TEXT);
      #endif
        return;
    }

    printf("\nUSB OHCI %u: ", i);

    if (val & OHCI_INT_SO) // scheduling overrun
    {
        printf("\nScheduling overrun.");
        handled |= OHCI_INT_SO;
    }

    if (val & OHCI_INT_WDH) // write back done head
    {
        printf("\nWrite back done head.");
        phys = o->hcca->doneHead;
        // TODO: handle ready transfer (ED, TD)
        handled |= OHCI_INT_WDH;
    }

    if (val & OHCI_INT_SF) // start of frame
    {
        printf("\nStart of frame.");
        handled |= OHCI_INT_SF;
    }

    if (val & OHCI_INT_RD) // resume detected
    {
        printf("\nResume detected.");
        handled |= OHCI_INT_RD;
    }

    if (val & OHCI_INT_UE) // unrecoverable error
    {
        printf("\nUnrecoverable HC error.");
        o->OpRegs->HcCommandStatus |= OHCI_STATUS_RESET;
        handled |= OHCI_INT_UE;
    }

    if (val & OHCI_INT_FNO) // frame number overflow
    {
        printf("\nFrame number overflow.");
        handled |= OHCI_INT_FNO;
    }

    if (val & OHCI_INT_RHSC) // root hub status change
    {
        printf("\nRoot hub status change.");
        handled |= OHCI_INT_RHSC;
        showPortstatus(o);
    }

    if (val & OHCI_INT_OC) // ownership change
    {
        printf("\nOwnership change.");
        handled |= OHCI_INT_OC;
    }

    if (val & ~handled)
    {
        printf("\nUnhandled interrupt: %X", val & ~handled);
    }

    o->OpRegs->HcInterruptStatus = val; // reset interrupts
}


/*******************************************************************************************************
*                                                                                                      *
*                                          Setup USB-Device                                            *
*                                                                                                      *
*******************************************************************************************************/

// TODO


/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
