#include "defs.h"
#include "string.h"
#include "mm.h"

#include "printk.h"

extern char _ekernel[];

struct {
    struct run *freelist;
} kmem;

uint64 kalloc() {
    struct run *r;

    r = kmem.freelist;
    kmem.freelist = r->next;
    
    memset((void *)r, 0x0, PGSIZE);
    return (uint64) r;
}

void kfree(uint64 addr) {
    struct run *r;

    // PGSIZE align 
    addr = addr & ~(PGSIZE - 1);

    memset((void *)addr, 0x0, (uint64)PGSIZE);

    r = (struct run *)addr;
    r->next = kmem.freelist;
    kmem.freelist = r;

    return ;
}

void kfreerange(char *start, char *end) {
    char *addr = (char *)PGROUNDUP((uint64)start);
    // printk("kfreerange\n");
    for (; (uint64)(addr) + PGSIZE <= (char *)VM_START + 128*1024*1024; addr += PGSIZE) {
        // printk("addr = %ld\n",addr);
        kfree((uint64)addr);
    }
}

void mm_init(void) {
    // printk("_ekernel = %ld\n",_ekernel);
    kfreerange(_ekernel , (char *)VM_START + 128*1024*1024);
    printk("...mm_init done!\n");
}
