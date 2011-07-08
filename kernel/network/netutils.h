#ifndef NETUTILS_H
#define NETUTILS_H

#include "types.h"

// htonl = Host To Network Long
#define htons(v) ((((v) >> 8) & 0xFF) | (((v) & 0xFF) << 8))
#define htonl(v) ((((v) >> 24) & 0xFF) | (((v) >> 8) & 0xFF00) | (((v) & 0xFF00) << 8) | (((v) & 0xFF) << 24))
// ntohl = Network To Host Long
#define ntohs(v) htons(v)
#define ntohl(v) htonl(v)


enum
{
    BL_NET_ARP
};

typedef union
{
    uint8_t IP[4];
    uint32_t iIP;
} __attribute__((packed)) IP_t;


uint16_t internetChecksum(void* addr, size_t count, uint32_t pseudoHeaderChecksum);
uint16_t udptcpCalculateChecksum(void* p, uint16_t length, IP_t srcIP, IP_t destIP, uint16_t protocol);


#endif