#ifndef VM86_H
#define VM86_H

// code derived on basic proposal at http://osdev.berlios.de/v86.html

#include "os.h"

// eflags
#define VALID_FLAGS  0x3FFFFF   // bit 21 ... 0 // http://futura.disca.upv.es/~eso/en/t2-arquitectura/eflags_register.png
#define EFLAG_IF     0x00000200 // bit  9 in eflags
#define EFLAG_VM     0x00020000 // bit 17 in eflags
#define FP_TO_LINEAR(seg, off) ((void*) ((((uint16_t) (seg)) << 4) + ((uint16_t) (off))))

typedef struct context_v86
{
	uint32_t cs;
	uint32_t eip;
	uint32_t ss;
	uint32_t useresp;
	uint32_t eflags;
	uint32_t ds;
	uint32_t es;
	uint32_t gs;
	uint32_t fs;
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t edi;
	uint32_t esi;
}context_v86_t;

typedef struct current
{
    uint32_t       v86_if;
    uint32_t       v86_in_handler;
    context_v86_t  v86_context;
    uint32_t       kernel_esp;
    uint32_t       user_stack_top;
    uint32_t       v86_handler;
}current_t;

bool vm86sensitiveOpcodehandler(context_v86_t* ctx);

#endif
