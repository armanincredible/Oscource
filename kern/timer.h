/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TIMER_H
#define JOS_KERN_TIMER_H

#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>
#include "acpi.h"

struct Timer {
    const char *timer_name;          /* Timer name */
    void (*timer_init)(void);        /* Timer init */
    uint64_t (*get_cpu_freq)(void);  /* Get CPU frequency */
    void (*enable_interrupts)(void); /* Init timer interrupts */
    void (*handle_interrupts)(void);
};

#define MAX_TIMERS 5

extern struct Timer timertab[MAX_TIMERS];

extern struct Timer timer_pit;
extern struct Timer timer_rtc;
extern struct Timer timer_hpet0;
extern struct Timer timer_hpet1;
extern struct Timer timer_acpipm;
extern struct Timer *timer_for_schedule;

//void acpi_enable(void);
RSDP *get_rsdp(void);
//FADT *get_fadt(void);
HPET *get_hpet(void);

void hpet_print_struct(void);
void hpet_init(void);
void hpet_print_reg(void);
void hpet_enable_interrupts_tim0(void);
void hpet_enable_interrupts_tim1(void);
uint64_t hpet_cpu_frequency(void);
void hpet_handle_interrupts_tim0(void);
void hpet_handle_interrupts_tim1(void);

uint32_t pmtimer_get_timeval(void);
uint64_t pmtimer_cpu_frequency(void);

#define PM_FREQ 3579545

#endif
