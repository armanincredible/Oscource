#ifndef JOS_INC_APIC_H
#define JOS_INC_APIC_H

#include <inc/types.h>

// Local APIC registers.
#define LAPIC_ID       0x0020U // ID
#define LAPIC_VER 0x0030U // Version
// #define LAPIC_TPR      0x0080U // Task Priority
// #define LAPIC_EOI      0x00B0U // EOI
// #define LAPIC_SVR      0x00F0U // Spurious Interrupt Vector
#define LAPIC_ENABLE   0x00000100U // Unit Enable
#define LAPIC_ESR      0x0280U // Error Status
#define LAPIC_ICRLO    0x0300U // Interrupt Command
#define LAPIC_INIT     0x00000500U // INIT/RESET
#define LAPIC_STARTUP  0x00000600U // Startup IPI
#define LAPIC_DELIVS   0x00001000U // Delivery status
#define LAPIC_ASSERT   0x00004000U // Assert interrupt (vs deassert)
#define LAPIC_DEASSERT 0x00000000U
#define LAPIC_LEVEL    0x00008000U // Level triggered
#define LAPIC_BCAST    0x00080000U // Send to all APICs, including self.
#define LAPIC_OTHERS   0x000C0000U // Send to all APICs, excluding self.
#define LAPIC_BUSY     0x00001000U
#define LAPIC_FIXED    0x00000000U
#define LAPIC_ICRHI    0x0310U // Interrupt Command [63:32]
#define LAPIC_TIMER    0x0320U // Local Vector Table 0 (TIMER)
#define LAPIC_X1       0x0000000BU // divide counts by 1
#define LAPIC_PERIODIC 0x00020000U // Periodic
#define LAPIC_PCINT    0x0340U // Performance Counter LVT
#define LAPIC_LINT0    0x0350U // Local Vector Table 1 (LINT0)
#define LAPIC_LINT1    0x0360U // Local Vector Table 2 (LINT1)
#define LAPIC_ERROR    0x0370U // Local Vector Table 3 (ERROR)
#define LAPIC_MASKED   0x00010000U // Interrupt masked
#define LAPIC_TICR     0x0380U // Timer Initial Count
#define LAPIC_TCCR     0x0390U // Timer Current Count
#define LAPIC_TDCR     0x03E0U // Timer Divide Configuration

#define MMIO_LAPIC_SIZE_REGISTERS LAPIC_TDCR

/*
uint32_t LocalApicGetId();
void LocalApicInit();
void LocalApicSendInit(uint32_t apic_id);
void LocalApicSendStartup(uint32_t apic_id, uint32_t vector);*/

uint32_t LocalApicGetId();
void lapic_start_ap(uint8_t apicid, uint32_t addr);
void lapic_init(void);

#endif