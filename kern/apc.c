#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/elf.h>
#include <inc/vsyscall.h>

#include <kern/env.h>
#include <kern/cpu.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/monitor.h>
#include <kern/sched.h>
#include <kern/kdebug.h>
#include <kern/macro.h>
#include <kern/pmap.h>
#include <kern/traceopt.h>
#include <kern/spinlock.h>
#include <kern/apic.h>

extern uint64_t active_cpu_count;
extern struct spinlock cpus_variable;

static uintptr_t cur_rsp = KERN_STACK_TOP - (KERN_STACK_SIZE + KERN_STACK_GAP);

void ap64c()
{
    cprintf ("my stack %p\n", read_rsp());

    //lcr3(kspace.cr3);

    //extern char aprealbootstacktop[], aprealbootstack[];
    //*aprealbootstack = 0;
    //*aprealbootstacktop = 0;
    //cprintf ("value is worked %p\n", aprealbootstack);
    //cprintf ("value is worked %p\n", aprealbootstacktop);
    lcr0(CR0_PE | CR0_PG | CR0_AM | CR0_WP | CR0_NE | CR0_MP);
    lcr4(CR4_PSE | CR4_PAE | CR4_PCE);

    volatile uint64_t efer = rdmsr(EFER_MSR);
    efer |= EFER_NXE;
    wrmsr(EFER_MSR, efer);

    lcr3((&kspace)->cr3);
    //lcr3(cpus[0].space.cr3);
    cprintf("here\n");
    cprintf("SMP: CPU %d starting\n", LocalApicGetId());
    lapic_init();

    trap_init_percpu();
    //spin_lock(&cpus_variable);
    cprintf ("im in kernel\n");
    //lapic_init();
    //volatile int a = 0;
    //a = a / 0;
    //spin_unlock(&cpus_variable);
    active_cpu_count++;
    while(1){;}
}