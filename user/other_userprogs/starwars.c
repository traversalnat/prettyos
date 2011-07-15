#include "userlib.h"
#include "stdlib.h"
#include "stdio.h"


int main()
{
    setScrollField(7, 46);
    printLine("================================================================================", 0, 0x0B);
    printLine("                        StarWars - Network test program!",                         2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);

    iSetCursor(0, 7);
    IP_t IP = {.IP = {94,142,241,111}};
    uint32_t connection = tcp_connect(IP, 23);
    printf("\nConnected (ID = %u). Wait until connection is established... ", connection);

    event_enable(true);
    char buffer[4096];
    EVENT_t ev = event_poll(buffer, 4096, EVENT_NONE);
    for(;;)
    {
        switch(ev)
        {
            case EVENT_NONE:
                waitForEvent(0);
                break;
            case EVENT_TCP_CONNECTED:
                printf("ESTABLISHED.\n");
                break;
            case EVENT_TCP_RECEIVED:
            {
                tcpReceivedEventHeader_t* header = (void*)buffer;
                char* data = (void*)(header+1);
                data[header->length] = 0;
                
                for (size_t i = 0; data[i] != 0; i++)
                {
                    if (data[i]==27 /*ESC*/ && data[i+1]=='[' && data[i+2]=='H')
                    {
                        clearScreen(0x00); // black 
                        i=i+3;
                    } 
                    else if (data[i]=='R' && data[i+1]=='U')
                    {
                        // do_nothing 
                        i=i+2;
                    }
                    else
                    {
                        putchar(data[i]);
                    }
                }
                break;
            }
            case EVENT_KEY_DOWN:
            {
                KEY_t* key = (void*)buffer;
                if(*key == KEY_ESC)
                {
                    tcp_close(connection);
                    return(0);
                }
            }
            default:
                break;
        }
        ev = event_poll(buffer, 4096, EVENT_NONE);
    }

    tcp_close(connection);
    return(0);
}
