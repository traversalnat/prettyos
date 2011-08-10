/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "task.h"
#include "pe.h"


typedef struct
{
    char     signature[2]; // Should be "MZ"
    char     res1[6];
    uint16_t size;         // Should be 0x0004
    char     res2[50];
    uint16_t offset;       // Points to the begin of the 4 byte PE signature, followed by COFF header
    char     res3[2];
} __attribute__((packed)) pe_msdosStub_t;

typedef struct
{
    uint16_t machine;
    uint16_t numberOfSections;
    uint32_t timeStamp;
    uint32_t symbolTable;
    uint32_t numberOfSymbols;
    uint16_t optionalHeaderSize;
    uint16_t characteristics;
} __attribute__((packed)) pe_coffHeader_t;

typedef struct
{
    uint16_t magic; // 0x10B: PE32;  0x107: ROM image;  0x20B: PE32+
    uint8_t  majorLinkerVersion;
    uint8_t  minorLinkerVersion;
    uint32_t sizeOfCode;
    uint32_t sizeOfInitializedData;
    uint32_t sizeOfUninitializedData;
    uint32_t entryPoint;
    uint32_t baseOfCode;
    uint32_t baseOfData; // PE32 only
    uint32_t imageBase;
} __attribute__((packed)) pe_optionalHeader_t;

typedef struct
{
    char name[8];
    uint32_t virtSize;
    uint32_t virtAddress;
    uint32_t rawDataSize;
    uint32_t rawDataPointer;
    uint32_t relocationsPointer;
    uint32_t linenumberPointer;
    uint16_t relocationNumber;
    uint16_t linenumberNumber;
    uint32_t characteristics;
} __attribute__((packed)) pe_sectionTableEntry_t;


enum MACHINE_TYPE
{
    MT_UNKNOWN = 0,
    MT_AMD64  = 0x8664,
    MT_I386   = 0x14C
};

enum PE_TYPE
{
    PT_PE32  = 0x10B,
    PT_ROM   = 0x107,
    PT_PE32P = 0x20B
};


bool pe_filename(const char* filename)
{
    return(strcmp(filename+strlen(filename)-4, ".exe") == 0);
}

bool pe_header(file_t* file)
{
    pe_msdosStub_t MSDOS_stub;
    fread(&MSDOS_stub, sizeof(pe_msdosStub_t), 1, file);

    bool valid = true;
    valid =          MSDOS_stub.signature[0]  == 'M';
    valid = valid && MSDOS_stub.signature[1]  == 'Z';

    fseek(file, MSDOS_stub.offset, SEEK_SET);
    char PE_sig[4];
    fread(PE_sig, 4, 1, file);
    valid = valid && PE_sig[0]    == 'P';
    valid = valid && PE_sig[1]    == 'E';
    valid = valid && PE_sig[2]    == 0;
    valid = valid && PE_sig[3]    == 0;

    return(valid);
}

void* pe_prepare(const void* file, size_t size, pageDirectory_t* pd)
{
    const pe_msdosStub_t* MSDOS_stub = file;
    const pe_coffHeader_t* coffHeader = file + MSDOS_stub->offset + 4; // Seek to COFF header

    switch (coffHeader->machine)
    {
        case MT_I386:
            break;
        case MT_AMD64:
            printf("x64. File will not be executed.");
            return(0);
        case MT_UNKNOWN: default:
            printf("Unknown. File will not be executed.");
            return(0);
    }
    if (!(coffHeader->characteristics & 0x0002))
    {
        textColor(ERROR);
        printf("\nInvalid PE executable.");
        textColor(TEXT);
        return(0);
    }
    if (!(coffHeader->characteristics & 0x0100))
    {
        textColor(ERROR);
        printf("\nNo 32-bit PE executable.");
        textColor(TEXT);
        return(0);
    }


    const pe_optionalHeader_t* optHeader = file + MSDOS_stub->offset + 4 + sizeof(pe_coffHeader_t);

    switch (optHeader->magic)
    {
        case PT_PE32:
            break;
        case PT_ROM:
            break;
        case PT_PE32P:
            textColor(ERROR);
            printf("\nPE32+ file. Cannot be executed");
            textColor(TEXT);
            return(0);
    }

  #ifdef _DIAGNOSIS_
    printf("\nSections:");
  #endif
    const pe_sectionTableEntry_t* sectionTable = ((void*)optHeader) + coffHeader->optionalHeaderSize;
    for (uint32_t i = 0; i < coffHeader->numberOfSections; ++i)
    {
      #ifdef _DIAGNOSIS_
        char name[9];
        name[8] = 0;
        memcpy(name, sectionTable[i].name, 8);
        printf("\n%s: characteristics: %X", name, sectionTable[i].characteristics);
      #endif

        MEMFLAGS_t memFlags = MEM_USER;
        if (sectionTable[i].characteristics & 0x80000000)
            memFlags |= MEM_WRITE;

        // Allocate code area for the user program
        if (!paging_alloc(pd, (void*)(sectionTable[i].virtAddress + optHeader->imageBase), alignUp(sectionTable[i].virtSize, PAGESIZE), memFlags))
        {
            return(0);
        }

        // Copy the code, using the user's page directory
        cli();
        paging_switch (pd);
        memcpy((void*)(sectionTable[i].virtAddress + optHeader->imageBase), file+sectionTable[i].rawDataPointer, sectionTable[i].rawDataSize);
        paging_switch (currentTask->pageDirectory);
        sti();
    }

    return((void*)(optHeader->entryPoint + optHeader->imageBase));
}


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
