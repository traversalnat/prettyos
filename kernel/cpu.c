/*
*  license and disclaimer for the use of this source code as per statement below
*  Lizenz und Haftungsausschluss f�r die Verwendung dieses Sourcecodes siehe unten
*/

#include "cpu.h"
#include "video/console.h"
#include "ipc.h"


// http://www.lowlevel.eu/wiki/Cpuid

static bool cpuid_available = false;

int64_t* cpu_frequency;

void cpu_analyze()
{
    textColor(LIGHT_GRAY);
    printf("   => CPU:\n");
    textColor(TEXT);

    ipc_node_t* node;
    ipc_createNode("PrettyOS/CPU/Frequency (kHz)", &node, IPC_INTEGER);
    cpu_frequency = &node->data.integer;

	uint32_t result = 0;
    // Test if the CPU supports the CPUID-Command
    __asm__ volatile ("pushfl\n"
                      "pop %%ecx\n"
                      "mov %%ecx, %%eax\n"
                      "xor %%eax, 0x200000\n"
                      "push %%eax\n"
                      "popfl\n"
                      "pushfl\n"
                      "pop %%eax\n"
                      "sub %%ecx, %%eax\n"
                      "mov %%eax, %0\n" : "=r"(result) : : "%eax", "%ecx");
    cpuid_available = (result == 0);

    if (!cpuid_available)
    {
        textColor(ERROR);
        printf("     => CPU does not support cpuid instruction.\n");
        textColor(TEXT);
        return;
    }

    // Read out VendorID
    char cpu_vendor[13];
    ((uint32_t*)cpu_vendor)[0] = cpu_idGetRegister(0, CR_EBX);
    ((uint32_t*)cpu_vendor)[1] = cpu_idGetRegister(0, CR_EDX);
    ((uint32_t*)cpu_vendor)[2] = cpu_idGetRegister(0, CR_ECX);
    cpu_vendor[12] = 0;

    ipc_setString("PrettyOS/CPU/VendorID", cpu_vendor);

    textColor(LIGHT_GRAY);
    printf("     => VendorID: ");
    textColor(TEXT);
    printf("%s\n", cpu_vendor);
}

void cpu_calculateFrequency()
{
    static uint64_t LastRdtscValue = 0; // rdtsc: read time-stamp counter

    // calculate cpu frequency
    uint64_t Rdtsc = rdtsc();
    uint64_t RdtscKCounts   = (Rdtsc - LastRdtscValue);  // Build difference
    uint32_t RdtscKCountsHi = RdtscKCounts >> 32;        // high dword
    uint32_t RdtscKCountsLo = RdtscKCounts & 0xFFFFFFFF; // low dword
    LastRdtscValue = Rdtsc;

    if (RdtscKCountsHi == 0)
        *cpu_frequency = RdtscKCountsLo/1000;
}

bool cpu_supports(CPU_FEATURE feature)
{
    if (feature == CF_CPUID) return (cpuid_available);
    if (!cpuid_available) return (false);

    CPU_REGISTER r = feature&~31;
    return (cpu_idGetRegister(0x00000001, r) & (BIT(feature-r)));
}

uint32_t cpu_idGetRegister(uint32_t function, CPU_REGISTER reg)
{
    if (!cpuid_available) return (0);

    switch (reg)
    {
        case CR_EAX:
        {
			register uint32_t temp;
			__asm__ ("movl %1, %%eax\n"
					 "cpuid\n"
					 "mov %%eax, %0" : "=r"(temp) : "r"(function) : "%eax");
            return (temp);
        }
        case CR_EBX:
        {
			register uint32_t temp;
			__asm__ ("movl %1, %%eax\n"
					 "cpuid\n"
					 "mov %%ebx, %0" : "=r"(temp) : "r"(function) : "%eax", "%ebx");
            return (temp);
        }
        case CR_ECX:
        {
			register uint32_t temp;
			__asm__ ("movl %1, %%eax\n"
					 "cpuid\n"
					 "mov %%ecx, %0" : "=r"(temp) : "r"(function) : "%eax", "%ecx");
            return (temp);
        }
        case CR_EDX:
        {
			register uint32_t temp;
			__asm__ ("movl %1, %%eax\n"
					 "cpuid\n"
					 "mov %%edx, %0" : "=r"(temp) : "r"(function) : "%eax", "%edx");
            return (temp);
        }
        default:
            return (0);
    }
}

uint64_t cpu_MSRread(uint32_t msr)
{
    uint32_t low, high;

    __asm__ volatile ("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));

    return ((uint64_t)high << 32) | low;
}

void cpu_MSRwrite(uint32_t msr, uint64_t value)
{
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;

    __asm__ volatile ("wrmsr" :: "a"(low), "c"(msr), "d"(high));
}


/*
* Copyright (c) 2010-2011 The PrettyOS Project. All rights reserved.
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
