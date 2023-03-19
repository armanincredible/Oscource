/* Simple command-line kernel monitor useful for
 * controlling the kernel and exploring the system interactively. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/env.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/tsc.h>
#include <kern/timer.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>

#define WHITESPACE "\t\r\n "
#define MAXARGS    16

/* Functions implementing monitor commands */
int mon_help(int argc, char **argv, struct Trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
int mon_backtrace(int argc, char **argv, struct Trapframe *tf);
int mon_dumpcmos(int argc, char **argv, struct Trapframe *tf);
int mon_start(int argc, char **argv, struct Trapframe *tf);
int mon_stop(int argc, char **argv, struct Trapframe *tf);
int mon_frequency(int argc, char **argv, struct Trapframe *tf);
int mon_memory(int argc, char **argv, struct Trapframe *tf);
int mon_pagetable(int argc, char **argv, struct Trapframe *tf);
int mon_virt(int argc, char **argv, struct Trapframe *tf);

struct Command {
    const char *name;
    const char *desc;
    /* return -1 to force monitor to exit */
    int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
        {"help", "Display this list of commands", mon_help},
        {"kerninfo", "Display information about the kernel", mon_kerninfo},
        {"backtrace", "Print stack backtrace", mon_backtrace},
        {"dumpcmos", "Print CMOS contents", mon_dumpcmos},
        {"timer_stop", "Print time since start and stop current timer", mon_stop},
        {"timer_start", "Start timer with name is typed by user", mon_start},
        {"timer_cpu_frequency", "Print cpu frequency what measured by typed timer", mon_frequency},
        {"my_text", "print text of developer", print_my_text},
        {"memory", "show information about memory", mon_memory},
        {"virt_tree", "show vitrual tree", mon_virt},
        {"page_table", "show page table", mon_pagetable}
};
#define NCOMMANDS (sizeof(commands) / sizeof(commands[0]))

int print_my_text (int argc, char **argv, struct Trapframe *tf)
{
    cprintf("Hello Im young JETOSER\n");
    return 0;
}

/* Implementations of basic kernel monitor commands */

int
mon_help(int argc, char **argv, struct Trapframe *tf) {
    for (size_t i = 0; i < NCOMMANDS; i++)
        cprintf("%s - %s\n", commands[i].name, commands[i].desc);
    return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf) {
    extern char _head64[], entry[], etext[], edata[], end[];

    cprintf("Special kernel symbols:\n");
    cprintf("  _head64 %16lx (virt)  %16lx (phys)\n", (unsigned long)_head64, (unsigned long)_head64);
    cprintf("  entry   %16lx (virt)  %16lx (phys)\n", (unsigned long)entry, (unsigned long)entry - KERN_BASE_ADDR);
    cprintf("  etext   %16lx (virt)  %16lx (phys)\n", (unsigned long)etext, (unsigned long)etext - KERN_BASE_ADDR);
    cprintf("  edata   %16lx (virt)  %16lx (phys)\n", (unsigned long)edata, (unsigned long)edata - KERN_BASE_ADDR);
    cprintf("  end     %16lx (virt)  %16lx (phys)\n", (unsigned long)end, (unsigned long)end - KERN_BASE_ADDR);
    cprintf("Kernel executable memory footprint: %luKB\n", (unsigned long)ROUNDUP(end - entry, 1024) / 1024);
    return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf) {
    // LAB 2: Your code here
    uint64_t rbp = 0;
    uint64_t rip = 0;
    struct Ripdebuginfo info = {};
    cprintf("Stack backtrace:\n");

    if (tf)
    {
        rbp = tf->tf_regs.reg_rbp;
        rip = tf->tf_rip;
    }
    else
    {
        rbp = read_rbp();
        rip = *((uint64_t*)rbp + 1);
    }

    //rip = *((uint64_t*)rbp + 1);

    while (rbp)
    {
        rip = *((uint64_t*)rbp + 1);
        debuginfo_rip(rip, &info);
        cprintf("  rbp %016lx  rip %016lx\n", rbp, rip); 
        cprintf("    %s:%d: %s+%ld\n", info.rip_file, info.rip_line, info.rip_fn_name, rip - info.rip_fn_addr);
        rbp = *((uint64_t*)rbp);
        //rip = *((uint64_t*)rbp + 1);
    }

    return 0;
}

int
mon_dumpcmos(int argc, char **argv, struct Trapframe *tf) {

    for (unsigned int i = 0; i < 128; i++)
    {
        if (i % 16 == 0)
        {
            cprintf ("\n%x:", i);
        }
        uint8_t data = cmos_read8 (i);
        cprintf (" %x", data);
    }
    cprintf ("\n");
    // Dump CMOS memory in the following format:
    // 00: 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
    // 10: 00 ..
    // Make sure you understand the values read.
    // Hint: Use cmos_read8()/cmos_write8() functions.
    // LAB 4: Your code here

    return 0;
}

/* Implement timer_start (mon_start), timer_stop (mon_stop), timer_freq (mon_frequency) commands. */
// LAB 5: Your code here:

int mon_start(int argc, char **argv, struct Trapframe *tf)
{
    if (argc == 2)
    {
        timer_start(argv[1]);
        return 0;
    }
    return -1;
}

int mon_stop(int argc, char **argv, struct Trapframe *tf)
{
    timer_stop();
    return 0;
}

int mon_frequency(int argc, char **argv, struct Trapframe *tf)
{
    if (argc == 2)
    {
        timer_cpu_frequency(argv[1]);
        return 0;
    }
    return -1;
}
/* Implement memory (mon_memory) command.
 * This command should call dump_memory_lists()
 */
// LAB 6: Your code here

int mon_memory(int argc, char **argv, struct Trapframe *tf)
{
    dump_memory_lists();
    return 0;
}
/* Implement mon_pagetable() and mon_virt()
 * (using dump_virtual_tree(), dump_page_table())*/
// LAB 7: Your code here

int mon_virt(int argc, char **argv, struct Trapframe *tf)
{
    dump_virtual_tree(current_space->root, current_space->root->class);
    return 0;
}

int mon_pagetable(int argc, char **argv, struct Trapframe *tf)
{
    dump_page_table(current_space->pml4, 4);
    return 0;
}

/* Kernel monitor command interpreter */

static int
runcmd(char *buf, struct Trapframe *tf) {
    int argc = 0;
    char *argv[MAXARGS];

    argv[0] = NULL;

    /* Parse the command buffer into whitespace-separated arguments */
    for (;;) {
        /* gobble whitespace */
        while (*buf && strchr(WHITESPACE, *buf)) *buf++ = 0;
        if (!*buf) break;

        /* save and scan past next arg */
        if (argc == MAXARGS - 1) {
            cprintf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf)) buf++;
    }
    argv[argc] = NULL;

    /* Lookup and invoke the command */
    if (!argc) return 0;
    for (size_t i = 0; i < NCOMMANDS; i++) {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv, tf);
    }

    cprintf("Unknown command '%s'\n", argv[0]);
    return 0;
}

void
monitor(struct Trapframe *tf) {

    cprintf("Welcome to the JOS kernel monitor!\n");
    cprintf("Type 'help' for a list of commands.\n");

    if (tf) print_trapframe(tf);

    char *buf;
    do buf = readline("K> ");
    while (!buf || runcmd(buf, tf) >= 0);
}
