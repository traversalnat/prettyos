#ifndef UHCI_H
#define UHCI_H

#include "os.h"
#include "pci.h"
#include "synchronisation.h"
#include "devicemanager.h"

#define UHCIMAX      8  // max number of UHCI devices
#define UHCIPORTMAX  4  // max number of UHCI device ports 

#define UHCI_USBCMD         0x00
#define UHCI_USBSTS         0x02
#define UHCI_USBINTR        0x04
#define UHCI_FRNUM          0x06
#define UHCI_FRBASEADD      0x08
#define UHCI_SOFMOD         0x0C
#define UHCI_PORTSC1        0x10
#define UHCI_PORTSC2        0x12


/* ****** */
/* USBCMD */
/* ****** */

#define UHCI_CMD_MAXP     BIT(7)
#define UHCI_CMD_CF       BIT(6)
#define UHCI_CMD_SWDBG    BIT(5)
#define UHCI_CMD_FGR      BIT(4)
#define UHCI_CMD_EGSM     BIT(3)
#define UHCI_CMD_GRESET   BIT(2)
#define UHCI_CMD_HCRESET  BIT(1)
#define UHCI_CMD_RS       BIT(0)


/* ******* */
/* USBSTS  */
/* ******* */

#define UHCI_STS_HCHALTED            BIT(5)
#define UHCI_STS_HC_PROCESS_ERROR    BIT(4)
#define UHCI_STS_HOST_SYSTEM_ERROR   BIT(3)
#define UHCI_STS_RESUME_DETECT       BIT(2)
#define UHCI_STS_USB_ERROR           BIT(1)
#define UHCI_STS_USBINT              BIT(0)

/* ******* */
/* USBINTR */
/* ******* */

#define UHCI_INT_SHORT_PACKET_ENABLE BIT(3)
#define UHCI_INT_IOC_ENABLE          BIT(2)
#define UHCI_INT_RESUME_ENABLE       BIT(1)
#define UHCI_INT_TIMEOUT_ENABLE      BIT(0)

/* ******* */
/* PORTSC  */
/* ******* */

#define UHCI_SUSPEND                 BIT(12)
#define UHCI_PORT_RESET              BIT(9)
#define UHCI_PORT_LOWSPEED_DEVICE    BIT(8)
#define UHCI_PORT_RESUME_DETECT      BIT(6)
#define UHCI_PORT_ENABLE_CHANGE      BIT(3)
#define UHCI_PORT_ENABLE             BIT(2)
#define UHCI_PORT_CS_CHANGE          BIT(1)
#define UHCI_PORT_CS                 BIT(0)

/* *************** */
/* LEGACY SUPPORT  */
/* *************** */

// Register in PCI (uint16_t)
#define UHCI_PCI_LEGACY_SUPPORT          0xC0

// Interrupt carried out as a PCI interrupt
#define UHCI_PCI_LEGACY_SUPPORT_PIRQ     0x2000

// RO Bits
#define UHCI_PCI_LEGACY_SUPPORT_NO_CHG   0x5040

// Status bits that are cleared by setting to 1
#define UHCI_PCI_LEGACY_SUPPORT_STATUS   0x8F00


// Transfer Descriptors (TD) are always aligned on 16-byte boundaries.
// All transfer descriptors have the same basic, 32-byte structure.
// The last 4 DWORDs are for software use.

typedef struct uhci_td
{
    // pointer to another TD or QH
    // inclusive control bits  (DWORD 0)
    uint32_t next;

    // TD CONTROL AND STATUS  (DWORD 1)
    uint32_t actualLength      : 11; //  0-10
    uint32_t reserved1         :  5; // 11-15
    uint32_t reserved2         :  1; //    16
    uint32_t bitstuffError     :  1; //    17
    uint32_t crc_timeoutError  :  1; //    18
    uint32_t nakReceived       :  1; //    19
    uint32_t babbleDetected    :  1; //    20
    uint32_t dataBufferError   :  1; //    21
    uint32_t stall             :  1; //    22
    uint32_t active            :  1; //    23
    uint32_t intOnComplete     :  1; //    24
    uint32_t isochrSelect      :  1; //    25
    uint32_t lowSpeedDevice    :  1; //    26
    uint32_t errorCounter      :  2; // 27-28
    uint32_t shortPacketDetect :  1; //    29
    uint32_t reserved3         :  2; // 30-31

    // TD TOKEN  (DWORD 2)
    uint32_t PacketID          :  8; //  0-7
    uint32_t deviceAddress     :  7; //  8-14
    uint32_t endpoint          :  4; // 15-18
    uint32_t dataToggle        :  1; //    19
    uint32_t reserved4         :  1; //    20
    uint32_t maxLength         : 11; // 21-31

    // TD BUFFER POINTER (DWORD 3)
    uint32_t         buffer;

    // RESERVED FOR SOFTWARE (DWORDS 4-7)
    struct uhci_td*  q_next;
    uint32_t         dWord5; // ?
    uint32_t         dWord6; // ?
    uint32_t         dWord7; // ?
  } __attribute__((packed)) uhci_TD_t;

// Queue Heads support the requirements of Control, Bulk, and Interrupt transfers
// and must be aligned on a 16-byte boundary
typedef struct uhci_qh
{
    // QUEUE HEAD LINK POINTER
    // inclusive control bits (DWORD 0)
    uint32_t   next;

    // QUEUE ELEMENT LINK POINTER
    // inclusive control bits (DWORD 1)
    uint32_t   transfer;

    // TDs
    uhci_TD_t* q_first;
    uhci_TD_t* q_last;
} __attribute__((packed)) uhci_QH_t;

// UHCI device
typedef struct
{
    pciDev_t*  PCIdevice;          // PCI device
    uint16_t   bar;                // start of I/O space (base address register
    uintptr_t  framelistAddrPhys;  // physical adress of frame list
    uintptr_t  framelistAddrVirt;  // virtual adress of frame list
    uintptr_t  qhPointerPhys;      // physical address of QH
    uhci_QH_t* qhPointerVirt;      // virtual adress of QH
    uint8_t    rootPorts;          // number of rootports
    size_t     memSize;            // memory size of IO space
    mutex_t*   framelistLock;      // mutex for access on the frame list
    mutex_t*   qhLock;             // mutex for access on the QH
    bool       enabledPorts;       // root ports enabled
    port_t     port[UHCIPORTMAX];  // root ports  
} uhci_t;


// functions
void uhci_install(pciDev_t* PCIdev, uintptr_t bar_phys, size_t memorySize);
void uhci_init(void* data, size_t size);
void startUHCI();
int32_t initUHCIHostController(uhci_t* u);
void uhci_resetHostController(uhci_t* u);
void uhci_enablePorts(uhci_t* u);
void uhci_resetPort(uhci_t* u, uint8_t j);
void uhci_handler(registers_t* r, pciDev_t* device);


#endif