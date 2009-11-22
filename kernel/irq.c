#include "os.h"
#include "task.h"


typedef void(*interrupt_handler_t)(struct regs*);


/* Array of function pointers handling custom ir handlers for a given ir */
interrupt_handler_t irq_routines[256-32];

/* Implement a custom ir handler for the given ir */
void irq_install_handler(int32_t ir, interrupt_handler_t handler) {irq_routines[ir] = handler;}

/* Clear the custom ir handler */
void irq_uninstall_handler(int32_t ir) {irq_routines[ir] = 0;}



/* Message string corresponding to the exception number 0-31: exception_messages[interrupt_number] */
char* exception_messages[] =
{
    "Division By Zero",        "Debug",                         "Non Maskable Interrupt",    "Breakpoint",
    "Into Detected Overflow",  "Out of Bounds",                 "Invalid Opcode",            "No Coprocessor",
    "Double Fault",            "Coprocessor Segment Overrun",   "Bad TSS",                   "Segment Not Present",
    "Stack Fault",             "General Protection Fault",      "Page Fault",                "Unknown Interrupt",
    "Coprocessor Fault",       "Alignment Check",               "Machine Check",             "Reserved",
    "Reserved",                "Reserved",                      "Reserved",                  "Reserved",
    "Reserved",                "Reserved",                      "Reserved",                  "Reserved",
    "Reserved",                "Reserved",                      "Reserved",                  "Reserved"
};


uint32_t irq_handler( uint32_t esp )
{
    struct regs* r = (struct regs*)esp;

    if (r->int_no < 32) //exception
    {
        settextcolor(4,0);

        if (r->int_no == 6 || r->int_no == 1) //Invalid Opcode
        {
            printformat("err_code: %X address(eip): %X\n", r->err_code, r->eip);
            printformat("edi: %X esi: %X ebp: %X eax: %X ebx: %X ecx: %X edx: %X\n", r->edi, r->esi, r->ebp, r->eax, r->ebx, r->ecx, r->edx);
            printformat("cs: %X ds: %X es: %X fs: %X gs %X ss %X\n", r->cs, r->ds, r->es, r->fs, r->gs, r->ss);
            printformat("int_no %d eflags %X useresp %X\n", r->int_no, r->eflags, r->useresp);
        }

        if (r->int_no == 14) //Page Fault
        {
            uint32_t faulting_address;
            __asm__ volatile("mov %%cr2, %0" : "=r" (faulting_address)); // faulting address <== CR2 register

            // The error code gives us details of what happened.
            int32_t present   = !(r->err_code & 0x1); // Page not present
            int32_t rw        =   r->err_code & 0x2;  // Write operation?
            int32_t us        =   r->err_code & 0x4;  // Processor was in user-mode?
            int32_t reserved  =   r->err_code & 0x8;  // Overwritten CPU-reserved bits of page entry?
            int32_t id        =   r->err_code & 0x10; // Caused by an instruction fetch?

            // Output an error message.
                          printformat("\nPage Fault (");
            if (present)  printformat("page not present");
            if (rw)       printformat(" read-only - write operation");
            if (us)       printformat(" user-mode");
            if (reserved) printformat(" overwritten CPU-reserved bits of page entry");
            if (id)       printformat(" caused by an instruction fetch");
                          printformat(") at %X - EIP: %X\n", faulting_address, r->eip);
        }

        printformat("err_code: %X address(eip): %X\n", r->err_code, r->eip);
        printformat("edi: %X esi: %X ebp: %X eax: %X ebx: %X ecx: %X edx: %X\n", r->edi, r->esi, r->ebp, r->eax, r->ebx, r->ecx, r->edx);
        printformat("cs: %X ds: %X es: %X fs: %X gs %X ss %X\n", r->cs, r->ds, r->es, r->fs, r->gs, r->ss);
        printformat("int_no %d eflags %X useresp %X\n", r->int_no, r->eflags, r->useresp);

        printformat("\n");
        printformat("%s >>> Exception. System Halted! <<<", exception_messages[r->int_no]);
        for (;;);
    }

    if ( pODA->ts_flag && r->int_no==32 )
        esp = task_switch(esp); //new task's esp

    interrupt_handler_t handler = irq_routines[r->int_no];
    if(handler) { handler(r); }

    if ( r->int_no >= 40 )
        outportb(0xA0, 0x20);
    outportb(0x20, 0x20);

    return esp;
}
