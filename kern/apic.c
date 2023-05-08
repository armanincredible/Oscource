// ------------------------------------------------------------------------------------------------
// intr/local_apic.c
// ------------------------------------------------------------------------------------------------

#include <inc/types.h>
#include "apic.h"

// ------------------------------------------------------------------------------------------------
// Interrupt Command Register

// Delivery Mode
#define ICR_FIXED                       0x00000000
#define ICR_LOWEST                      0x00000100
#define ICR_SMI                         0x00000200
#define ICR_NMI                         0x00000400
#define ICR_INIT                        0x00000500
#define ICR_STARTUP                     0x00000600

// Destination Mode
#define ICR_PHYSICAL                    0x00000000
#define ICR_LOGICAL                     0x00000800

// Delivery Status
#define ICR_IDLE                        0x00000000
#define ICR_SEND_PENDING                0x00001000

// Level
#define ICR_DEASSERT                    0x00000000
#define ICR_ASSERT                      0x00004000

// Trigger Mode
#define ICR_EDGE                        0x00000000
#define ICR_LEVEL                       0x00008000

// Destination Shorthand
#define ICR_NO_SHORTHAND                0x00000000
#define ICR_SELF                        0x00040000
#define ICR_ALL_INCLUDING_SELF          0x00080000
#define ICR_ALL_EXCLUDING_SELF          0x000c0000

// Destination Field
#define ICR_DESTINATION_SHIFT           24


extern uint8_t* lapic_ptr;

// ------------------------------------------------------------------------------------------------
static uint32_t LocalApicIn(uint32_t reg)
{
    return *(uint32_t*)(lapic_ptr + reg);
}

// ------------------------------------------------------------------------------------------------
static void LocalApicOut(uint32_t reg, uint32_t data)
{
    *(uint32_t*)(lapic_ptr + reg) = data;
}

// ------------------------------------------------------------------------------------------------
void LocalApicInit()
{
    // Clear task priority to enable all interrupts
    LocalApicOut(LAPIC_TPR, 0);

    // Logical Destination Mode
    LocalApicOut(LAPIC_DFR, 0xffffffff);   // Flat mode
    LocalApicOut(LAPIC_LDR, 0x01000000);   // All cpus use logical id 1

    // Configure Spurious Interrupt Vector Register
    LocalApicOut(LAPIC_SVR, 0x100 | 0xff);
}

// ------------------------------------------------------------------------------------------------
uint32_t LocalApicGetId()
{
    return LocalApicIn(LAPIC_ID) >> 24;
}

// ------------------------------------------------------------------------------------------------
void LocalApicSendInit(uint32_t apic_id)
{
    LocalApicOut(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
    LocalApicOut(LAPIC_ICRLO, ICR_INIT | ICR_PHYSICAL
        | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);

    while (LocalApicIn(LAPIC_ICRLO) & ICR_SEND_PENDING)
        ;
}

// ------------------------------------------------------------------------------------------------
void LocalApicSendStartup(uint32_t apic_id, uint32_t vector)
{
    LocalApicOut(LAPIC_ICRHI, apic_id << ICR_DESTINATION_SHIFT);
    LocalApicOut(LAPIC_ICRLO, vector | ICR_STARTUP
        | ICR_PHYSICAL | ICR_ASSERT | ICR_EDGE | ICR_NO_SHORTHAND);

    while (LocalApicIn(LAPIC_ICRLO) & ICR_SEND_PENDING)
        ;
}
