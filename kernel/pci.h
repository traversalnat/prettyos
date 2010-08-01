#ifndef PCI_H
#define PCI_H

#include "os.h"

#define PCI_CONFIGURATION_ADDRESS 0xCF8   // Address I/O Port
#define PCI_CONFIGURATION_DATA    0xCFC   // Data    I/O Port

// upper byte: length in bytes
// lower byte: register number plus offset

#define PCI_VENDOR_ID   0x0200 // length: 0x02 reg: 0x00 offset: 0x00
#define PCI_DEVICE_ID   0x0202 // length: 0x02 reg: 0x00 offset: 0x02
#define PCI_COMMAND     0x0204
#define PCI_STATUS      0x0206
#define PCI_REVISION    0x0108
#define PCI_CLASS       0x010B
#define PCI_SUBCLASS    0x010A
#define PCI_INTERFACE   0x0109
#define PCI_HEADERTYPE  0x010E
#define PCI_BAR0        0x0410
#define PCI_BAR1        0x0414
#define PCI_BAR2        0x0418
#define PCI_BAR3        0x041C
#define PCI_BAR4        0x0420
#define PCI_BAR5        0x0424
#define PCI_IRQLINE     0x013C

#define PCIBUSES      4     // we access only four busses of 256 possibles
#define PCIDEVICES    32
#define PCIFUNCS      8
#define PCIARRAYSIZE  (PCIBUSES*PCIDEVICES*PCIFUNCS)

typedef struct
{
    uint32_t baseAddress;
    size_t   memorySize;
    uint8_t  memoryType;
}__attribute__((packed)) pciBar_t;

typedef struct
{
   uint8_t   number;
   uint8_t   bus;
   uint8_t   device;
   uint8_t   func;
   uint16_t  vendorID;
   uint16_t  deviceID;
   uint8_t   classID;
   uint8_t   subclassID;
   uint8_t   interfaceID;
   uint8_t   revID;
   uint8_t   irq;
   pciBar_t  bar[6];
}__attribute__((packed)) pciDev_t;


void analyzeHostSystemError(pciDev_t* pciDev);
uint32_t pci_config_read   (uint8_t bus, uint8_t device, uint8_t func, uint16_t content);
void pci_config_write_byte (uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint8_t  val);
void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val);
void listPCI();
void pciScan();

#endif