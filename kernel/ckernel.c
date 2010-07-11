/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "util.h"
#include "timer.h"
#include "cmos.h"
#include "time.h"
#include "kheap.h"
#include "initrd.h"
#include "flpydsk.h"
#include "ehci.h"
#include "mouse.h"
#include "keyboard.h" 
#include "video.h"
#include "task.h"
#include "event_list.h"
#include "syscall.h"
#include "pci.h"
#include "cdi.h"
#include "devicemanager.h"

#define ADDR_MEM_INFO   0x1000 // RAM detection by second stage bootloader
#define FILEBUFFERSIZE 0x10000 // intermediate buffer for user program, e.g. shell

const char* version = "0.0.1.30 - Rev: 591";

// .bss
extern uintptr_t _bss_start;  // linker script
extern uintptr_t _kernel_end; // linker script

// Informations about the system
system_t system;

void fpu_install(); // fpu.c

static void init()
{
    // set .bss to zero
    memset(&_bss_start, 0x0, (uintptr_t)&_kernel_end - (uintptr_t)&_bss_start);

    // descriptors
    gdt_install();
    idt_install(); // cf. interrupts.asm

    // video
    kernel_console_init();
    clear_screen();

    // memory
    system.Memory_Size = paging_install();
    heap_install();

    // internal devices
    timer_install(100); // Sets system frequency to ... Hz
    fpu_install();

    // external devices
    keyboard_install();
    mouse_install();

    // processes/threads, messaging and system calls
    tasking_install();
    events_install();
    syscall_install();

    cdi_init();

    deviceManager_install(); // device management for mass storage devices
    fsmanager_install();

    sti();
}

void showMemorySize()
{
    if (system.Memory_Size >= 1048576)
    {
        printf("\nMemory size: %u MiB / %u MB  (%u Bytes)\n", system.Memory_Size>>20, system.Memory_Size/1000000, system.Memory_Size);
    }
    else
    {
        printf("\nMemory size: %u KiB / %u KB  (%u Bytes)\n", system.Memory_Size>>10, system.Memory_Size/1000, system.Memory_Size);
    }
}

void main()
{
    init();
    task_switching = true;
    create_cthread(&bootscreen, "Booting ...");

    kdebug(0x00, ".bss from %X to %X set to zero.\n", &_bss_start, &_kernel_end);

    showMemorySize();

    flpydsk_install(); // detect FDDs
    pciScan();         // scan of pci bus; results go to: pciDev_t pciDev_Array[PCIARRAYSIZE]; (cf. pci.h)

  #ifdef _DIAGNOSIS_
    listPCI();
  #endif

    void* ramdisk_start = initrd_install(ramdisk_install(), 0, 0x200000);

    // search and load shell
    bool shell_found = false;
    uint8_t* buf = malloc(FILEBUFFERSIZE, 0);
    struct dirent* node = 0;
    for (int i = 0; (node = readdir_fs(fs_root, i)) != 0; ++i)
    {
        fs_node_t* fsnode = finddir_fs(fs_root, node->name);

        if ((fsnode->flags & 0x7) == FS_DIRECTORY)
        {
            printf("<RAM Disk at %X DIR> %s\n", ramdisk_start, node->name);
        }
        else
        {
            uint32_t sz = read_fs(fsnode, 0, fsnode->length, buf);

            char name[40];
            memset(name, 0, 40);
            memcpy(name, node->name, 35); // protection against wrong / too long filename
            printf("%u \t%s\n",sz,name);

            if (strcmp(node->name, "shell") == 0)
            {
                shell_found = true;
                if (!elf_exec(buf, sz, "Shell"))
                {
                    textColor(0x04);
                    printf("\nCannot start shell!\n");
                    textColor(0x0F);
                }
            }
        }
    }
    free(buf);
    if (! shell_found)
    {
        textColor(0x04);
        printf("\nProgram not found.\n");
        textColor(0x0F);
    }

    showPortList(); 
    showDiskList(); 

    const char* progress    = "|/-\\";    // rotating asterisk
    uint64_t LastRdtscValue = 0;          // rdtsc: read time-stamp counter
    uint32_t CurrentSeconds = 0xFFFFFFFF; // Set on a high value to force a refresh of the statusbar at the beginning.
    char     DateAndTime[81];             // String for Date&Time

    //////////////////////////// start of kernel idle loop ////////////////////////////
    while (true) 
    {
        // show rotating asterisk
        *((uint16_t*)(0xB8000 + sizeof(uint16_t)*(49*80 + 79))) = 0x0C00 | *progress;
        if (! *++progress) { progress = "|/-\\"; }

        if (timer_getSeconds() != CurrentSeconds)
        {
            CurrentSeconds = timer_getSeconds();

            // all values 64 bit
            uint64_t RdtscDiffValue = rdtsc() - LastRdtscValue;
            LastRdtscValue = rdtsc();
            uint64_t RdtscKCounts   = RdtscDiffValue >> 10;         // division by 1024
            uint32_t RdtscKCountsHi = RdtscKCounts   >> 32;         // high dword
            uint32_t RdtscKCountsLo = RdtscKCounts   &  0xFFFFFFFF; // low dword

            if (RdtscKCountsHi == 0)
            {
                system.CPU_Frequency_kHz = (RdtscKCountsLo/1000) << 10;
            }

            // draw status bar with date & time and frequency
            getCurrentDateAndTime(DateAndTime);
            kprintf("%s   %i s runtime. CPU: %i MHz    ", 49, 0x0C, DateAndTime, CurrentSeconds, system.CPU_Frequency_kHz/1000); // output in status bar

            deviceManager_checkDrives(); // switch off motors if they are not neccessary
        }

        handleEvents();

        __asm__ volatile ("hlt"); // CPU is stopped until the next interrupt 
    } 
    ///////////////////////////// end of kernel idle loop /////////////////////////////
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
