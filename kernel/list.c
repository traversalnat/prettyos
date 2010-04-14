/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "list.h"
#include "kheap.h" // malloc and free
#include "console.h"

listHead_t* listCreate()
{
    listHead_t* hd = (listHead_t*)malloc(sizeof(listHead_t),0);
    if (hd)
    {
        hd->head = hd->tail = 0;
    }
    return(hd);
}

bool listAppend(listHead_t* hd, void* data)
{
    element_t* ap = (element_t*)malloc(sizeof(element_t),0);
    if (ap)
    {
        ap->data = data;
        ap->next = 0;

        if (!hd->head) // there exist no list element
        {
            hd->head = ap;
        }
        else   // there exist at least one list element
        {
            hd->tail->next = ap;
        }
        hd->tail = ap;
        return(true);
    }
    return(false);
}

void listDeleteAll(listHead_t* hd)
{
    element_t* cur = hd->head;
    element_t* nex;

    while (cur)
    {
        nex=cur->next;
        free(cur);
        cur=nex;
    }
    free(hd);
}

void listShow(listHead_t* hd)
{
    printf("List elements:\n");
    element_t* cur = hd->head;
    if (!cur)
    {
        printf("The list is empty.");
    }
    else
    {
        while (cur)
        {
            printf("%X\t",cur);
            cur = cur->next;
        }
    }
}

void* listGetElement(listHead_t* hd, uint32_t number)
{
    element_t* cur = hd->head;
    while (cur)
    {
        if (number == 0)
        {
            return(cur->data);
        }
        --number;
        cur = cur->next;
    }
    return(0);
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
