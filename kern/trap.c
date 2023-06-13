#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <inc/vsyscall.h>

#include <kern/pmap.h>
#include <kern/cpu.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/timer.h>
#include <kern/vsyscall.h>
#include <kern/traceopt.h>

#ifdef CONFIG_KSPACE

    extern void clock_thdlr(void);
    extern void timer_thdlr(void);

#else 

    extern void divide_thdlr(void);
    extern void debug_thdlr(void);
    extern void nmi_thdlr(void);
    extern void brkpt_thdlr(void);
    extern void oflow_thdlr(void);
    extern void bound_thdlr(void);
    extern void illop_thdlr(void);
    extern void device_thdlr(void);
    extern void dblflt_thdlr(void);
    extern void tss_thdlr(void);
    extern void segnp_thdlr(void);
    extern void stack_thdlr(void);
    extern void gpflt_thdlr(void);
    extern void pgftl_thdlr(void);
    extern void fperr_thdlr(void);
    extern void align_thdlr(void);
    extern void mchk_thdlr(void);
    extern void simderr_thdlr(void);

    extern void syscall_hdlr(void);

    extern void clock_thdlr(void);
    extern void timer_thdlr(void);

    extern void kbd_thdlr(void);
    extern void serial_thdlr(void);

#endif 

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case */
static struct Trapframe *last_tf;

/* Interrupt descriptor table  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records) */
struct Gatedesc idt[256] = {{0}};
struct Pseudodesc idt_pd = {sizeof(idt) - 1, (uint64_t)idt};

/* Global descriptor table.
 *
 * Set up global descriptor table (GDT) with separate segments for
 * kernel mode and user mode.  Segments serve many purposes on the x86.
 * We don't use any of their memory-mapping capabilities, but we need
 * them to switch privilege levels.
 *
 * The kernel and user segments are identical except for the DPL.
 * To load the SS register, the CPL must equal the DPL.  Thus,
 * we must duplicate the segments for the user and the kernel.
 *
 * In particular, the last argument to the SEG macro used in the
 * definition of gdt specifies the Descriptor Privilege Level (DPL)
 * of that descriptor: 0 for kernel and 3 for user. */

#define SEG_TSS_NULL_TWICE SEG_NULL, SEG_NULL
#define SEG_TSS_NULL_N_CPU SEG_TSS_NULL_TWICE, SEG_TSS_NULL_TWICE, SEG_TSS_NULL_TWICE, SEG_TSS_NULL_TWICE, SEG_TSS_NULL_TWICE, SEG_TSS_NULL_TWICE, SEG_TSS_NULL_TWICE
struct Segdesc32 gdt[2 * NCPU + 7] = {
        /* 0x0 - unused (always faults -- for trapping NULL far pointers) */
        SEG_NULL,
        /* 0x8 - kernel code segment */
        [GD_KT >> 3] = SEG64(STA_X | STA_R, 0x0, 0xFFFFFFFF, 0),
        /* 0x10 - kernel data segment */
        [GD_KD >> 3] = SEG64(STA_W, 0x0, 0xFFFFFFFF, 0),
        /* 0x18 - kernel code segment 32bit */
        [GD_KT32 >> 3] = SEG32(STA_X | STA_R, 0x0, 0xFFFFFFFF, 0),
        /* 0x20 - kernel data segment 32bit */
        [GD_KD32 >> 3] = SEG32(STA_W, 0x0, 0xFFFFFFFF, 0),
        /* 0x28 - user code segment */
        [GD_UT >> 3] = SEG64(STA_X | STA_R, 0x0, 0xFFFFFFFF, 3),
        /* 0x30 - user data segment */
        [GD_UD >> 3] = SEG64(STA_W, 0x0, 0xFFFFFFFF, 3),
        /* Per-CPU TSS descriptors (starting from GD_TSS0) are initialized
     * in trap_init_percpu() */
        [GD_TSS0 >> 3] = SEG_NULL,
        [(GD_TSS0 >> 3) + 1] = SEG_NULL, /* last 8 bytes of the tss since tss is 16 bytes long */

        //SEG_TSS_NULL_N_CPU
};
#undef SEG_TSS_NULL_TWICE
#undef SEG_TSS_NULL_N_CPU

struct Pseudodesc gdt_pd = {sizeof(gdt) - 1, (unsigned long)gdt};

static _Noreturn void page_fault_handler(struct Trapframe *tf);

static const char *
trapname(int trapno) {
    static const char *const excnames[] = {
            "Divide error",
            "Debug",
            "Non-Maskable Interrupt",
            "Breakpoint",
            "Overflow",
            "BOUND Range Exceeded",
            "Invalid Opcode",
            "Device Not Available",
            "Double Fault",
            "Coprocessor Segment Overrun",
            "Invalid TSS",
            "Segment Not Present",
            "Stack Fault",
            "General Protection",
            "Page Fault",
            "(unknown trap)",
            "x87 FPU Floating-Point Error",
            "Alignment Check",
            "Machine-Check",
            "SIMD Floating-Point Exception"};

    if (trapno < sizeof(excnames) / sizeof(excnames[0])) return excnames[trapno];
    if (trapno == T_SYSCALL) return "System call";
    if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16) return "Hardware Interrupt";

    return "(unknown trap)";
}

void
trap_init(void) {
    // LAB 4: Your code here

    clock_idt_init();

    // LAB 5: Your code here


    /* Insert trap handlers into IDT */
    // LAB 8: Your code here

    // WARNING TODO AAAAA

    idt[T_DIVIDE]  = GATE(0, GD_KT, (uint64_t) (&divide_thdlr),  0);
    idt[T_DEBUG]   = GATE(0, GD_KT, (uint64_t) (&debug_thdlr),   0);
    idt[T_NMI]     = GATE(0, GD_KT, (uint64_t) (&nmi_thdlr),     0);
    idt[T_BRKPT]   = GATE(0, GD_KT, (uint64_t) (&brkpt_thdlr),   3);
    idt[T_OFLOW]   = GATE(0, GD_KT, (uint64_t) (&oflow_thdlr),   0);
    idt[T_BOUND]   = GATE(0, GD_KT, (uint64_t) (&bound_thdlr),   0);
    idt[T_ILLOP]   = GATE(0, GD_KT, (uint64_t) (&illop_thdlr),   0);
    idt[T_DEVICE]  = GATE(0, GD_KT, (uint64_t) (&device_thdlr),  0);
    idt[T_DBLFLT]  = GATE(0, GD_KT, (uint64_t) (&dblflt_thdlr),  0);
    idt[T_TSS]     = GATE(0, GD_KT, (uint64_t) (&tss_thdlr),     0);
    idt[T_SEGNP]   = GATE(0, GD_KT, (uint64_t) (&segnp_thdlr),   0);
    idt[T_STACK]   = GATE(0, GD_KT, (uint64_t) (&stack_thdlr),   0);
    idt[T_GPFLT]   = GATE(0, GD_KT, (uint64_t) (&gpflt_thdlr),   0);
    idt[T_PGFLT]   = GATE(0, GD_KT, (uint64_t) (&pgftl_thdlr),   0);
    idt[T_FPERR]   = GATE(0, GD_KT, (uint64_t) (&fperr_thdlr),   0);
    idt[T_ALIGN]   = GATE(0, GD_KT, (uint64_t) (&align_thdlr),   0);
    idt[T_MCHK]    = GATE(0, GD_KT, (uint64_t) (&mchk_thdlr),    0);
    idt[T_SIMDERR] = GATE(0, GD_KT, (uint64_t) (&simderr_thdlr), 0);

    idt[T_SYSCALL] = GATE(0, GD_KT, (uint64_t) (&syscall_hdlr), 3);

    idt[IRQ_CLOCK + IRQ_OFFSET] = GATE(0, GD_KT, (&clock_thdlr), 0);
    idt[IRQ_TIMER + IRQ_OFFSET] = GATE(0, GD_KT, (&timer_thdlr), 0);

    idt[IRQ_KBD    + IRQ_OFFSET] = GATE(0, GD_KT, (&kbd_thdlr),    0);
    idt[IRQ_SERIAL + IRQ_OFFSET] = GATE(0, GD_KT, (&serial_thdlr), 0);

    /* Setup #PF handler dedicated stack
     * It should be switched on #PF because
     * #PF is the only kind of exception that
     * can legally happen during normal kernel
     * code execution */
    idt[T_PGFLT].gd_ist = 1;

    // LAB 11: Your code here

    /* Per-CPU setup */
    trap_init_percpu();
}

void clock_idt_init(void)
{
    #ifdef CONFIG_KSPACE

        idt[IRQ_OFFSET + IRQ_CLOCK] = GATE(0, GD_KT, (uint64_t) (&clock_thdlr), 0);
        //^^^^^^^^^^^^^^^^^^^^^^^^           ^^^^^             ^^^^^^^^^^^^^   ^^
        // we get interrupt with offset      kernel text       Trapentry.S     Only from kernel

        idt[IRQ_OFFSET + IRQ_TIMER] = GATE(0, GD_KT, (uint64_t) (&timer_thdlr), 0);

    #endif 
}

/* Initialize and load the per-CPU TSS and IDT */
void
trap_init_percpu(void) {
    /* Load GDT and segment descriptors. */

    lgdt(&gdt_pd);

    cprintf("start trap cpu init\n");

    int i = thiscpu->cpu_id;
    uintptr_t offset = KERN_STACK_SIZE + KERN_STACK_GAP;
    static uintptr_t kern_i_stack_top = KERN_STACK_TOP;

    /* The kernel never uses GS or FS,
     * so we leave those set to the user data segment
     *
     * For good measure, clear the local descriptor table (LDT),
     * since we don't use it */
    asm volatile(
            "movw %%dx,%%gs\n\t"
            "movw %%dx,%%fs\n\t"
            "movw %%ax,%%es\n\t"
            "movw %%ax,%%ds\n\t"
            "movw %%ax,%%ss\n\t"
            "xorl %%eax,%%eax\n\t"
            "lldt %%ax\n\t"
            "pushq %%rcx\n\t"
            "movabs $1f,%%rax\n\t"
            "pushq %%rax\n\t"
            "lretq\n"
            "1:\n" ::"a"(GD_KD),
            "d"(GD_UD | 3), "c"(GD_KT)
            : "cc", "memory");

    /* Setup a TSS so that we get the right stack
     * when we trap to the kernel. */
    thiscpu->cpu_ts.ts_rsp0 = kern_i_stack_top;
    thiscpu->cpu_ts.ts_ist1 = KERN_PF_STACK_TOP;

    /* Initialize the TSS slot of the gdt. */
    *(volatile struct Segdesc64 *)(&gdt[(GD_TSS0 >> 3) + i * 2]) = SEG64_TSS(STS_T64A, ((uint64_t)&thiscpu->cpu_ts), sizeof(struct Taskstate), 0);

    /* Load the TSS selector (like other segment selectors, the
     * bottom three bits are special; we leave them 0) */
    ltr(GD_TSS0 + i * 16);

    /* Load the IDT */
    lidt(&idt_pd);

    i += 1;
    kern_i_stack_top -= offset;
}

void
print_trapframe(struct Trapframe *tf) {
    cprintf("TRAP frame at %p\n", tf);
    print_regs(&tf->tf_regs);
    cprintf("  es   0x----%04x\n", tf->tf_es);
    cprintf("  ds   0x----%04x\n", tf->tf_ds);
    cprintf("  trap 0x%08lx %s\n", (unsigned long)tf->tf_trapno, trapname(tf->tf_trapno));

    /* If this trap was a page fault that just happened
     * (so %cr2 is meaningful), print the faulting linear address */
    if (tf == last_tf && tf->tf_trapno == T_PGFLT)
        cprintf("  cr2  0x%08lx\n", (unsigned long)rcr2());

    cprintf("  err  0x%08lx", (unsigned long)tf->tf_err);

    /* For page faults, print decoded fault error code:
     *     U/K=fault occurred in user/kernel mode
     *     W/R=a write/read caused the fault
     *     PR=a protection violation caused the fault (NP=page not present) */
    if (tf->tf_trapno == T_PGFLT) {
        cprintf(" [%s, %s, %s]\n",
                tf->tf_err & FEC_U ? "user" : "kernel",
                tf->tf_err & FEC_W ? "write" : tf->tf_err & FEC_I ? "execute" :
                                                                    "read",
                tf->tf_err & FEC_P ? "protection" : "not-present");
    } else
        cprintf("\n");

    cprintf("  rip  0x%08lx\n", (unsigned long)tf->tf_rip);
    cprintf("  cs   0x----%04x\n", tf->tf_cs);
    cprintf("  flag 0x%08lx\n", (unsigned long)tf->tf_rflags);
    cprintf("  rsp  0x%08lx\n", (unsigned long)tf->tf_rsp);
    cprintf("  ss   0x----%04x\n", tf->tf_ss);
}

void
print_regs(struct PushRegs *regs) {
    cprintf("  r15  0x%08lx\n", (unsigned long)regs->reg_r15);
    cprintf("  r14  0x%08lx\n", (unsigned long)regs->reg_r14);
    cprintf("  r13  0x%08lx\n", (unsigned long)regs->reg_r13);
    cprintf("  r12  0x%08lx\n", (unsigned long)regs->reg_r12);
    cprintf("  r11  0x%08lx\n", (unsigned long)regs->reg_r11);
    cprintf("  r10  0x%08lx\n", (unsigned long)regs->reg_r10);
    cprintf("  r9   0x%08lx\n", (unsigned long)regs->reg_r9);
    cprintf("  r8   0x%08lx\n", (unsigned long)regs->reg_r8);
    cprintf("  rdi  0x%08lx\n", (unsigned long)regs->reg_rdi);
    cprintf("  rsi  0x%08lx\n", (unsigned long)regs->reg_rsi);
    cprintf("  rbp  0x%08lx\n", (unsigned long)regs->reg_rbp);
    cprintf("  rbx  0x%08lx\n", (unsigned long)regs->reg_rbx);
    cprintf("  rdx  0x%08lx\n", (unsigned long)regs->reg_rdx);
    cprintf("  rcx  0x%08lx\n", (unsigned long)regs->reg_rcx);
    cprintf("  rax  0x%08lx\n", (unsigned long)regs->reg_rax);
}

static void
trap_dispatch(struct Trapframe *tf) {

    // cprintf("trap_dispatch(): trapno %ld\n", tf->tf_trapno - IRQ_OFFSET);

    switch (tf->tf_trapno) {
    case T_SYSCALL:
        tf->tf_regs.reg_rax = syscall(
                tf->tf_regs.reg_rax,
                tf->tf_regs.reg_rdx,
                tf->tf_regs.reg_rcx,
                tf->tf_regs.reg_rbx,
                tf->tf_regs.reg_rdi,
                tf->tf_regs.reg_rsi,
                tf->tf_regs.reg_r8);
        return;
    case T_PGFLT:
        /* Handle processor exceptions. */
        // LAB 9: Your code here.
        page_fault_handler(tf);
        return;
    case T_BRKPT:
        // LAB 8: Your code here
        monitor(tf);
        return;
    case IRQ_OFFSET + IRQ_SPURIOUS:
        /* Handle spurious interrupts
         * The hardware sometimes raises these because of noise on the
         * IRQ line or other reasons, we don't care */
        if (trace_traps) {
            cprintf("Spurious interrupt on irq 7\n");
            print_trapframe(tf);
        }
        return;
    case IRQ_OFFSET + IRQ_TIMER:
    case IRQ_OFFSET + IRQ_CLOCK:
        // LAB 5: Your code here
        // LAB 4: Your code here
        {
            // rtc_timer_pic_handle();

            vsys[VSYS_gettime] = gettime();

            assert(timer_for_schedule);
            timer_for_schedule->handle_interrupts();

            // cprintf("trap_dispath(): timer/clock - calling sched_yield()\n");

            sched_yield();  // passing control to the scheduler since 
                            // we have timer/clocl interrupt
            return;
        }
        /* Handle keyboard and serial interrupts. */
        // LAB 11: Your code here
    case IRQ_OFFSET + IRQ_SERIAL:
        {
            serial_intr();
            return;
        }
    case IRQ_OFFSET + IRQ_KBD:
        {
            kbd_intr();
            return;
        }
    default:
        print_trapframe(tf);
        if (!(tf->tf_cs & 3))
            panic("Unhandled trap in kernel");
        env_destroy(curenv);
    }
}

/* We do not support recursive page faults in-kernel */
bool in_page_fault;

_Noreturn void
trap(struct Trapframe *tf) {
    /* The environment may have set DF and some versions
     * of GCC rely on DF being clear */
    asm volatile("cld" ::
                         : "cc");

    /* Halt the CPU if some other CPU has called panic() */
    extern char *panicstr;
    if (panicstr) asm volatile("hlt");

    /* Check that interrupts are disabled.  If this assertion
     * fails, DO NOT be tempted to fix it by inserting a "cli" in
     * the interrupt path */
    assert(!(read_rflags() & FL_IF));

    if (trace_traps) cprintf("Incoming TRAP[%ld] frame at %p\n", tf->tf_trapno, tf);
    if (trace_traps_more) print_trapframe(tf);

    /* #PF should be handled separately */
    if (tf->tf_trapno == T_PGFLT) {
        assert(current_space);
        assert(!in_page_fault);
        in_page_fault = 1;

        uintptr_t va = rcr2();

#if defined(SANITIZE_USER_SHADOW_BASE) && LAB == 8
        /* NOTE: Hack!
         * This is an early user address sanitizer memory allocation
         * hook until proper memory allocation syscalls
         * and userspace pagefault handlers are implemented */
        if ((tf->tf_err & ~FEC_W) == FEC_U && curenv && SANITIZE_USER_SHADOW_BASE <= va &&
            va < SANITIZE_USER_SHADOW_BASE + SANITIZE_USER_SHADOW_SIZE) {
            int res = map_region(&curenv->address_space, ROUNDDOWN(va, PAGE_SIZE),
                                 NULL, 0, PAGE_SIZE, ALLOC_ONE | PROT_R | PROT_W | PROT_USER_);
            assert(!res);
        }
#endif

        /* If #PF was caused by write it can be lazy copying/allocation (fast path)
         * It is required to be handled here because of in-kernel page faults
         * which can happen with curenv == NULL */

        /* Read processor's CR2 register to find the faulting address */
        int res = force_alloc_page(current_space, va, MAX_ALLOCATION_CLASS);
        if (trace_pagefaults) {
            bool can_redir = tf->tf_err & FEC_U && curenv && curenv->env_pgfault_upcall;
            cprintf("<%p> Page fault ip=%08lX va=%08lX err=%c%c%c%c%c -> %s\n", current_space, tf->tf_rip, va,
                    tf->tf_err & FEC_P ? 'P' : '-',
                    tf->tf_err & FEC_U ? 'U' : '-',
                    tf->tf_err & FEC_W ? 'W' : '-',
                    tf->tf_err & FEC_R ? 'R' : '-',
                    tf->tf_err & FEC_I ? 'I' : '-',
                    res ? can_redir ? "redirected to user" : "fault" : "resolved by kernel");
        }
        if (!res) {
            in_page_fault = 0;
            env_pop_tf(tf);
        }
    }

    assert(curenv);

    /* Copy trap frame (which is currently on the stack)
     * into 'curenv->env_tf', so that running the environment
     * will restart at the trap point */
    curenv->env_tf = *tf;
    /* The trapframe on the stack should be ignored from here on */
    tf = &curenv->env_tf;

    /* Record that tf is the last real trapframe so
     * print_trapframe can print some additional information */
    last_tf = tf;

    /* Dispatch based on what type of trap occurred */
    trap_dispatch(tf);

    /* If we made it to this point, then no other environment was
     * scheduled, so we should return to the current environment
     * if doing so makes sense */
    if (curenv && curenv->env_status == ENV_RUNNING)
    {
        // cprintf("end of trap(), env_run called \n");
        env_run(curenv);
    }
    else
    {
        // cprintf("end of trap(), shed_yield called \n");
        sched_yield();
    }
}

static _Noreturn void
page_fault_handler(struct Trapframe *tf) {
    // LAB 9: Your code here:

    uintptr_t cr2 = rcr2();

    /* Handle kernel-mode page faults. */
    if (!(tf->tf_err & FEC_U)) {
        print_trapframe(tf);
        panic("Kernel pagefault\n");
    }

    /* We've already handled kernel-mode exceptions, so if we get here,
     * the page fault happened in user mode.
     *
     * Call the environment's page fault upcall, if one exists.  Set up a
     * page fault stack frame on the user exception stack (below
     * USER_EXCEPTION_STACK_TOP), then branch to curenv->env_pgfault_upcall.
     *
     * The page fault upcall might cause another page fault, in which case
     * we branch to the page fault upcall recursively, pushing another
     * page fault stack frame on top of the user exception stack.
     *
     * The trap handler needs one word of scratch space at the top of the
     * trap-time stack in order to return.  In the non-recursive case, we
     * don't have to worry about this because the top of the regular user
     * stack is free.  In the recursive case, this means we have to leave
     * an extra word between the current top of the exception stack and
     * the new stack frame because the exception stack _is_ the trap-time
     * stack.
     *
     * If there's no page fault upcall, the environment didn't allocate a
     * page for its exception stack or can't write to it, or the exception
     * stack overflows, then destroy the environment that caused the fault.
     * Note that the grade script assumes you will first check for the page
     * fault upcall and print the "user fault va" message below if there is
     * none.  The remaining three checks can be combined into a single test.
     *
     * Hints:
     *   user_mem_assert() and env_run() are useful here.
     *   To change what the user environment runs, modify 'curenv->env_tf'
     *   (the 'tf' variable points at 'curenv->env_tf'). */


    static_assert(UTRAP_RIP == offsetof(struct UTrapframe, utf_rip), "UTRAP_RIP should be equal to RIP offset");
    static_assert(UTRAP_RSP == offsetof(struct UTrapframe, utf_rsp), "UTRAP_RSP should be equal to RSP offset");
    

    if (!curenv->env_pgfault_upcall)
    {
        cprintf("user fault va\n");
     
        user_mem_assert(curenv, (const void*) (USER_EXCEPTION_STACK_TOP - PAGE_SIZE), PAGE_SIZE, PROT_R | PROT_W);
        env_destroy(curenv);
    }

    /* Force allocation of exception stack page to prevent memcpy from
     * causing pagefault during another pagefault */
    // LAB 9: Your code here:

    force_alloc_page(current_space, USER_EXCEPTION_STACK_TOP - PAGE_SIZE, MAX_ALLOCATION_CLASS);

    /* Assert existance of exception stack using user mem assert */
    // LAB 9: Your code here:

    user_mem_assert(curenv, (const void*) (USER_EXCEPTION_STACK_TOP - PAGE_SIZE), PAGE_SIZE, PROT_R | PROT_W);

    /* Build local copy of UTrapframe */
    // LAB 9: Your code here:

    struct UTrapframe utf = 
    {
        .utf_fault_va = (uint64_t) cr2,
        .utf_err = tf->tf_err,
        .utf_regs = tf->tf_regs,
        .utf_rip = tf->tf_rip,
        .utf_rflags = tf->tf_rflags,
        .utf_rsp = tf->tf_rsp
    };

    /* And then copy it userspace (nosan_memcpy) */
    // LAB 9: Your code here:

    uintptr_t frame_start = 0;

    if (tf->tf_rsp >  USER_EXCEPTION_STACK_TOP - PAGE_SIZE
     && tf->tf_rsp <= USER_EXCEPTION_STACK_TOP - 1)
    {
        frame_start = tf->tf_rsp - sizeof(uint64_t) - sizeof(struct UTrapframe);
        user_mem_assert(curenv, (void*) frame_start, USER_EXCEPTION_STACK_TOP - frame_start, PROT_W | PROT_R);
    }
    else 
    {
        frame_start = USER_EXCEPTION_STACK_TOP - sizeof(struct UTrapframe);
    }

    nosan_memcpy((void*) frame_start, (void*) &utf, sizeof(struct UTrapframe));

    /* Reset in_page_fault flag */
    // LAB 9: Your code here:

    in_page_fault = 0;

    /* Rerun current environment */
    // LAB 9: Your code here:

    tf->tf_rsp = frame_start;
    tf->tf_rip = (uintptr_t) (curenv->env_pgfault_upcall);
    env_run(curenv);

    while (1) {}
}
