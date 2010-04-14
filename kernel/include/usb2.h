#ifndef USB2_H
#define USB2_H

#include "os.h"

// structs, ...
struct usb2_deviceDescriptor
{
   uint8_t  length;            // 18
   uint8_t  descriptorType;    // 1
   uint16_t bcdUSB;            // e.g. 0x0210 means 2.10
   uint8_t  deviceClass;
   uint8_t  deviceSubclass;
   uint8_t  deviceProtocol;
   uint8_t  maxPacketSize;     // MPS0, must be 8,16,32,64
   uint16_t idVendor;
   uint16_t idProduct;
   uint16_t bcdDevice;         // release of the device
   uint8_t  manufacturer;
   uint8_t  product;
   uint8_t  serialNumber;
   uint8_t  numConfigurations; // number of possible configurations
}__attribute__((packed));

struct usb2_configurationDescriptor
{
   uint8_t  length;            // 9
   uint8_t  descriptorType;    // 2
   uint16_t totalLength;       
   uint8_t  NumInterfaces;
   uint8_t  ConfigurationValue;
   uint8_t  Configuration;
   uint8_t  Attributes;     
   uint8_t  MaxPower;
}__attribute__((packed));

// functions, ...
void testTransfer1(uint32_t device, uint32_t endpoint);
void testTransfer2(uint32_t device, uint32_t endpoint);
void showDeviceDesriptor(struct usb2_deviceDescriptor*);
void showConfigurationDesriptor(struct usb2_configurationDescriptor*);

#endif
