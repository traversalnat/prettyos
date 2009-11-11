#include "pci.h"

uint32_t pci_config_read( uint8_t bus, uint8_t device, uint8_t func, uint16_t content )
{
    // example: PCI_VENDOR_ID 0x0200 ==> length: 0x02 reg: 0x00 offset: 0x00
    uint8_t length  = content >> 8;
    uint8_t reg_off = content & 0x00FF;
    uint8_t reg     = reg_off & 0xFC;     // bit mask: 11111100b
    uint8_t offset  = reg_off % 0x04;     // remainder of modulo operation provides offset

    outportl(PCI_CONFIGURATION_ADDRESS,
        0x80000000
        | (bus    << 16)
        | (device << 11)
        | (func   <<  8)
        | (reg         ));

    // use offset to find searched content
    uint32_t readVal = inportl(PCI_CONFIGURATION_DATA) >> (8 * offset);

    switch(length)
    {
        case 1:
            readVal &= 0x000000FF;
        break;
        case 2:
            readVal &= 0x0000FFFF;
        break;
        case 4:
            readVal &= 0xFFFFFFFF;
        break;
    }
    return readVal;
}

void pci_config_write_dword( uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val )
{
    outportl(PCI_CONFIGURATION_ADDRESS,
        0x80000000
        | (bus     << 16)
        | (device  << 11)
        | (func    <<  8)
        | (reg & 0xFC   ));

    outportl(PCI_CONFIGURATION_DATA, val);
}

 void pciScan()
 {
    uint32_t i;
    settextcolor(15,0);
    uint8_t  bus                = 0; // max. 256
    uint8_t  device             = 0; // max.  32
    uint8_t  func               = 0; // max.   8

    uint32_t pciBar             = 0; // helper variable for memory size
    uint32_t EHCI_data          = 0; // helper variable for EHCI_data


    // array of devices, 50 for first tests
    for(i=0;i<50;++i)
    {
        pciDev_Array[i].number = i;
    }

    int number=0;
    for(bus=0;bus<8;++bus)
    {
        for(device=0;device<32;++device)
        {
            for(func=0;func<8;++func)
            {
                uint16_t vendorID = pci_config_read( bus, device, func, PCI_VENDOR_ID);
                if( vendorID && (vendorID != 0xFFFF) )
                {
                    pciDev_Array[number].vendorID     = vendorID;
                    pciDev_Array[number].deviceID     = pci_config_read( bus, device, func, PCI_DEVICE_ID  );
                    pciDev_Array[number].classID      = pci_config_read( bus, device, func, PCI_CLASS      );
                    pciDev_Array[number].subclassID   = pci_config_read( bus, device, func, PCI_SUBCLASS   );
                    pciDev_Array[number].interfaceID  = pci_config_read( bus, device, func, PCI_INTERFACE  );
                    pciDev_Array[number].revID        = pci_config_read( bus, device, func, PCI_REVISION   );
                    pciDev_Array[number].irq          = pci_config_read( bus, device, func, PCI_IRQLINE    );
                    pciDev_Array[number].bar[0].baseAddress = pci_config_read( bus, device, func, PCI_BAR0 );
                    pciDev_Array[number].bar[1].baseAddress = pci_config_read( bus, device, func, PCI_BAR1 );
                    pciDev_Array[number].bar[2].baseAddress = pci_config_read( bus, device, func, PCI_BAR2 );
                    pciDev_Array[number].bar[3].baseAddress = pci_config_read( bus, device, func, PCI_BAR3 );
                    pciDev_Array[number].bar[4].baseAddress = pci_config_read( bus, device, func, PCI_BAR4 );
                    pciDev_Array[number].bar[5].baseAddress = pci_config_read( bus, device, func, PCI_BAR5 );

                    // Valid Device
                    pciDev_Array[number].bus    = bus;
                    pciDev_Array[number].device = device;
                    pciDev_Array[number].func   = func;

                    // output to screen
                    printformat("%d:%d.%d\t dev:%x vend:%x",
                         pciDev_Array[number].bus, pciDev_Array[number].device, pciDev_Array[number].func,
                         pciDev_Array[number].deviceID, pciDev_Array[number].vendorID );

                    if(pciDev_Array[number].irq!=255)
                    {
                        printformat(" IRQ:%d ", pciDev_Array[number].irq );
                    }
                    else // "255 means "unknown" or "no connection" to the interrupt controller"
                    {
                        printformat(" IRQ:-- ");
                    }

                    // test on USB
                    if( (pciDev_Array[number].classID==0x0C) && (pciDev_Array[number].subclassID==0x03) )
                    {
                        printformat(" USB ");
                        if( pciDev_Array[number].interfaceID==0x00 ) { printformat("UHCI ");   }
                        if( pciDev_Array[number].interfaceID==0x10 ) { printformat("OHCI ");   }
                        if( pciDev_Array[number].interfaceID==0x20 ) { printformat("EHCI ");   }
                        if( pciDev_Array[number].interfaceID==0x80 ) { printformat("no HCI "); }
                        if( pciDev_Array[number].interfaceID==0xFE ) { printformat("any ");    }

                        for(i=0;i<6;++i) // check USB BARs
                        {
                            pciDev_Array[number].bar[i].memoryType = pciDev_Array[number].bar[i].baseAddress & 0x01;

                            if(pciDev_Array[number].bar[i].baseAddress) // check valid BAR
                            {
                                if(pciDev_Array[number].bar[i].memoryType == 0)
                                {
                                    printformat("%d:%X MEM ", i, pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0 );
                                }
                                if(pciDev_Array[number].bar[i].memoryType == 1)
                                {
                                    printformat("%d:%X I/O ", i, pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFFC );
                                }

                                /// TEST Memory Size Begin
                                cli();
                                pci_config_write_dword  ( bus, device, func, PCI_BAR0 + 4*i, 0xFFFFFFFF );
                                pciBar = pci_config_read( bus, device, func, PCI_BAR0 + 4*i             );
                                pci_config_write_dword  ( bus, device, func, PCI_BAR0 + 4*i,
                                                          pciDev_Array[number].bar[i].baseAddress       );
                                sti();
                                pciDev_Array[number].bar[i].memorySize = (~pciBar | 0x0F) + 1;
                                printformat("sz:%d ", pciDev_Array[number].bar[i].memorySize );
                                /// TEST Memory Size End

                                /// TEST EHCI Data Begin
                                if(  (pciDev_Array[number].interfaceID==0x20)   // EHCI
                                   && pciDev_Array[number].bar[i].baseAddress ) // valid BAR
                                {
                                    /*
                                    Offset Size Mnemonic    Power Well   Register Name
                                    00h     1   CAPLENGTH      Core      Capability Register Length
                                    01h     1   Reserved       Core      N/A
                                    02h     2   HCIVERSION     Core      Interface Version Number
                                    04h     4   HCSPARAMS      Core      Structural Parameters
                                    08h     4   HCCPARAMS      Core      Capability Parameters
                                    0Ch     8   HCSP-PORTROUTE Core      Companion Port Route Description
                                    */

                                    uint32_t bar = pciDev_Array[number].bar[i].baseAddress & 0xFFFFFFF0;

                                    EHCI_data = *((volatile uint8_t* )(bar + 0x00));
                                    printformat("\nBAR%d CAPLENGTH:  %x \t\t",i, EHCI_data);

                                    EHCI_data = *((volatile uint16_t*)(bar + 0x02));
                                    printformat(  "BAR%d HCIVERSION: %x \n",i, EHCI_data);

                                    EHCI_data = *((volatile uint32_t*)(bar + 0x04));
                                    printformat(  "BAR%d HCSPARAMS:  %X \t",i, EHCI_data);

                                    EHCI_data = *((volatile uint32_t*)(bar + 0x08));
                                    printformat(  "BAR%d HCCPARAMS:  %X \n",i, EHCI_data);
                                }
                                /// TEST EHCI Data End
                            } // if
                        } // for
                    } // if
                    printformat("\n");
                    ++number;
                } // if pciVendor

                // Bit 7 in header type (Bit 23-16) --> multifunctional
                if( !(pci_config_read(bus, device, 0, PCI_HEADERTYPE) & 0x80) )
                {
                    break; // --> not multifunctional, only function 0 used
                }
            } // for function
        } // for device
    } // for bus
    printformat("\n");
}

