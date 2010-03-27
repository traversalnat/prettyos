#ifndef TASK_H
#define TASK_H

#include "os.h"
#include "paging.h"
#include "descriptor_tables.h"

#define KERNEL_STACK_SIZE 2048        // Use a 2kb kernel stack.


struct task
{
    int32_t id;                       // Process ID.
    uint32_t esp, ebp;                // Stack and base pointers.
    uint32_t eip;                     // Instruction pointer.
    uint32_t ss;
    page_directory_t* page_directory; // Page directory.
    char* heap_top;
    uint32_t kernel_stack;            // Kernel stack location.
    struct task* next;                // The next task in a linked list.
} __attribute__((packed));

typedef struct task task_t;



extern int32_t userTaskCounter;
extern volatile task_t* displayed_task;

int32_t getUserTaskNumber();
void settaskflag(int32_t i);

void tasking_install();
uint32_t task_switch(uint32_t esp);
int32_t getpid();
task_t* create_task( page_directory_t* directory, void* entry, uint8_t privilege );
void switch_context();
void exit();

void* task_grow_userheap( uint32_t increase );

void task_log(task_t* t);
void TSS_log(tss_entry_t* tss);
void log_task_list();

#endif
