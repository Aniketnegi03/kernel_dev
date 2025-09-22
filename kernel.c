#include "kernel.h"
#include "common.h"

extern char __bss[], __bss_end[], __stack_top[];

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}
/*
void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}
*/

void putchar(char ch) {
    volatile char *uart = (volatile char *)0x10000000;
    *uart = ch;
}

extern char __free_ram[], __free_ram_end[];

paddr_t alloc_pages(uint32_t n) {
    static paddr_t next_paddr = (paddr_t) __free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");

    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}

void delay(void) {
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop"); // do nothing
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        switch_context(&proc_a->sp, &proc_b->sp);
        delay();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        switch_context(&proc_b->sp, &proc_a->sp);
        delay();
    }
}


void kernel_main(void) {
    const char *s = "\n\nHello World!\n";
    
    printf("\n\nHello %s\n", "World!");
    printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

    for (int i = 0; s[i] != '\0'; i++) {
        putchar(s[i]);
    }

    paddr_t paddr0 = alloc_pages(2);
    paddr_t paddr1 = alloc_pages(1);
    printf("alloc_pages test: paddr0=%x\n", paddr0);
    printf("alloc_pages test: paddr1=%x\n", paddr1);
    proc_a = create_process((uint32_t) proc_a_entry);
    proc_b = create_process((uint32_t) proc_b_entry);
    proc_a_entry();

    PANIC("booted!");
    printf("unreachable here!\n");

    for (;;) {
        __asm__ __volatile__("wfi");
    }
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // Set the stack pointer
        "j kernel_main\n"       // Jump to the kernel main function
        :
        : [stack_top] "r" (__stack_top) // Pass the stack top address as %[stack_top]
    );
}
