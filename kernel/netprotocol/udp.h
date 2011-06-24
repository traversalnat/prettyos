#ifndef UDP_H
#define UDP_H

#include "network/network.h"

typedef struct
{
    uint16_t sourcePort;
    uint16_t destPort;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udpPacket_t;


void UDPSend (network_adapter_t* adapter, void* data, uint32_t length, uint16_t srcPort, uint8_t srcIP[4], uint16_t destPort, uint8_t destIP[4]);
void UDPRecv (network_adapter_t* adapter, udpPacket_t* packet, uint32_t length);
void UDPDebug(udpPacket_t* udp);


#endif
