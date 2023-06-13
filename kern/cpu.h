
#ifndef JOS_INC_CPU_H
#define JOS_INC_CPU_H

#include <inc/types.h>
#include <inc/memlayout.h>
#include <inc/mmu.h>
#include <inc/env.h>
#include <kern/spinlock.h>
#include <kern/apic.h>

#define MAX_CPU (4) //This number uses because memory for stacks is limited (see memlayout.h)
#define NCPU MAX_CPU

#define thiscpu (&cpus[LocalApicGetId()])

/* Used by x86 to find stack for interrupt */
extern struct Taskstate cpu_ts;

extern char in_intr;
extern bool in_clk_intr;

static inline bool
in_interrupt(void) {
    return !!in_intr;
}

static inline bool
in_clock_interrupt(void) {
    return in_intr && in_clk_intr;
}

void detect_cores(void);
void smp_init(void);

// Values of status in struct Cpu
enum {
	CPU_UNUSED = 0,
	CPU_STARTED,
	CPU_HALTED,
};

// Per-CPU state
struct CpuInfo {
	//struct AddressSpace space;
	uint8_t cpu_id;                 // Local APIC ID; index into cpus[] below
	volatile unsigned cpu_status;   // The status of the CPU
	struct Env *cpu_env;            // The currently-running environment.
	struct Env *env_list;
	volatile unsigned nenv;
	struct spinlock env_lock;
	struct Taskstate cpu_ts;        // Used by x86 to find stack for interrupt
};

// Initialized in mpconfig.c
extern struct CpuInfo cpus[MAX_CPU];
extern uint8_t numcore;
//extern struct CpuInfo *bootcpu;     // The boot-strap processor (BSP)
//extern physaddr_t lapicaddr;        // Physical MMIO address of the local APIC

#endif
