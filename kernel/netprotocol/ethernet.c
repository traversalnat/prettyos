/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/


// http://www.rfc-editor.org/rfc/rfc793.txt <--- TRANSMISSION CONTROL PROTOCOL

#include "network/rtl8139.h"
#include "video/console.h"
#include "ethernet.h"
#include "arp.h"
#include "ipv4.h"
#include "icmp.h"
#include "tcp.h"
#include "udp.h"

extern uint32_t BaseAddressRTL8139_MMIO;
extern uint8_t IP_address[4];
extern uint8_t MAC_address[6];

void EthernetRecv(void* data, uint32_t length)
{
    ethernet_t* eth = (ethernet_t*)data;

    textColor(0x0E);
    if (((eth->type_len[0] << 8) | eth->type_len[1]) > 1500) { printf("Ethernet 2. "); }
    else                                                     { printf("Ethernet 1. "); }

    // now we set our arp/ip pointer to the Ethernet data (payload) behind the Ethernet header
    arp_t* arp = (arp_t*)((uintptr_t)eth + sizeof(ethernet_t));
    ip_t*  ip  = (ip_t*) ((uintptr_t)eth + sizeof(ethernet_t));
	
	// IP protocol is parsed here and distributed in switch/case 
    uint32_t ipHeaderLength = 4 * ip->ipHeaderLength; // is given as number of 32 bit pieces (4 byte) 
    if ((ip->version == 4) || (ip->version == 6))
    {
        printf(" IP version: %u, IP Header Length: %u byte", ip->version, ipHeaderLength);
    
        switch(ip->protocol)
	    {
		    case 1: // icmp
			    ICMPAnswerPing(data, length);
			    icmpDebug(data, length);
			    break;
		    case 4: // ipv4
			    /*
			    tcpheader_t tcp; 
			    tcp.sourcePort = 1025;
			    tcp.destinationPort = 80;
			    tcp.sequence_number = 1;
			    tcp.ACK = 1;
			    tcpDebug(&tcp);
			    */
			    break;
		    case 6: // tcp
			    break;
		    case 17: // udp
			    UDPRecv(  (void*)((uintptr_t)data + sizeof(ethernet_t) + ipHeaderLength), length - ipHeaderLength, *(uint32_t*)ip->sourceIP, *(uint32_t*)ip->destIP);			
			    break;
		    case 41: // ipv6
			    break;
		    default:
			    break;
	    }
    }
    else // check for ARP packet
    {   
        if ((((arp->hardware_addresstype[0] << 8) | arp->hardware_addresstype[1]) ==    1) &&
            (((arp->protocol_addresstype[0] << 8) | arp->protocol_addresstype[1]) == 2048) &&
              (arp->hardware_addresssize == 6) &&
              (arp->protocol_addresssize == 4))
        {
            printf("ARP. ");

            // extract the operation
            switch ((arp->operation[0] << 8) | arp->operation[1])
            {
            case 1: // ARP-Request
                if ((arp->sourceIP[0] == arp->destIP[0])&&
                    (arp->sourceIP[1] == arp->destIP[1])&&
                    (arp->sourceIP[2] == arp->destIP[2])&&
                    (arp->sourceIP[3] == arp->destIP[3])) // IP requ. and searched is identical
                {
                    printf("Operation: Gratuitous Request\n");
                }
                else
                {
                    printf("Operation: Request\n");
                }

                textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
                for (uint8_t i = 0; i < 6; i++) { printf("%y ", arp->source_mac[i]); }

                textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
                for (uint8_t i = 0; i < 4; i++) { printf("%u", arp->sourceIP[i]); if (i<3) printf("."); }

                textColor(0x0D); printf("\nMAC Searched:   "); textColor(0x07);
                for (uint8_t i = 0; i < 6; i++) { printf("%y ", arp->dest_mac[i]);   }

                textColor(0x0D); printf("  IP Searched:   "); textColor(0x03);
                for (uint8_t i = 0; i < 4; i++) { printf("%u", arp->destIP[i]);   if (i<3) printf("."); }

                // requested IP is our own IP?
                if ( arp->destIP[0] == IP_address[0] && arp->destIP[1] == IP_address[1] &&
                     arp->destIP[2] == IP_address[2] && arp->destIP[3] == IP_address[3])
                {
                     printf("\n Tx prepared:");
                     arpPacket_t reply;
                     for (uint32_t i = 0; i < 6; i++)
                     {
                        reply.eth.recv_mac[i]   = arp->source_mac[i];
                        reply.eth.send_mac[i]   = MAC_address[i];

                     }
                     reply.eth.type_len[0] = 0x08; reply.eth.type_len[1] = 0x06;

                     for (uint32_t i = 0; i < 2; i++)
                     {
                         reply.arp.hardware_addresstype[i] = arp->hardware_addresstype[i];
                         reply.arp.protocol_addresstype[i] = arp->protocol_addresstype[i];
                     }
                     reply.arp.operation[0]         = 0;
                     reply.arp.operation[1]         = 2; // reply

                     reply.arp.hardware_addresssize = arp->hardware_addresssize;
                     reply.arp.protocol_addresssize = arp->protocol_addresssize;

                     for (uint32_t i = 0; i < 6; i++)
                     {
                        reply.arp.dest_mac[i]   = arp->source_mac[i];
                        reply.arp.source_mac[i] = *((uint8_t*)(BaseAddressRTL8139_MMIO + RTL8139_IDR0 + i));
                     }

                     for (uint32_t i = 0; i < 4; i++)
                     {
                        reply.arp.destIP[i]   = arp->sourceIP[i];
                        reply.arp.sourceIP[i] = IP_address[i];
                     }

                     EthernetSend((void*)&reply, length);
                }
                break;
            case 2: // ARP-Reply
                printf("Operation: Response\n");

                textColor(0x0D); printf("\nMAC Replying:   "); textColor(0x03);
                for (uint8_t i = 0; i < 6; i++) { printf("%y ", arp->source_mac[i]); }

                textColor(0x0D); printf("  IP Replying:   "); textColor(0x03);
                for (uint8_t i = 0; i < 4; i++) { printf("%u", arp->sourceIP[i]); if (i<3) printf("."); }

                textColor(0x0D); printf("\nMAC Requesting: "); textColor(0x03);
                for (uint8_t i = 0; i < 6; i++) { printf("%y ", arp->dest_mac[i]);   }

                textColor(0x0D); printf("  IP Requesting: "); textColor(0x03);
                for (uint8_t i = 0; i < 4; i++) { printf("%u", arp->destIP[i]);   if (i<3) printf("."); }
                break;
            }
        }
        else
        {
            // TODO
        }
    }
    printf("\n");
    textColor(0x0F);
}


bool EthernetSend(void* data, uint32_t length)
{
    if (length > 0x700)
    {
        printf("\nerror: EthernetSend: length: %u. This is more than (1792) 0x700",length);
        return false;
    }
    else
    {
        printf("\nEthernetSend: length: %u.",length);
    }

    // TODO: check whether Tx buffer is already occupied

    ethernet_t* eth = (ethernet_t*)data;
    textColor(0x0C);

    if (((eth->type_len[0] << 8) | eth->type_len[1]) > 1500)
    {
        printf("\nPacket now sent with Ethernet 2. ");
    }
    else
    {
        printf("\nPacket now sent with Ethernet 1. ");
    }
    textColor(0x0F);

    return transferDataToTxBuffer(data, length);
}

/*
* Copyright (c) 2009-2010 The PrettyOS Project. All rights reserved.
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