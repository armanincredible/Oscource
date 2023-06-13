// ------------------------------------------------------------------------------------------------
// intr/local_apic.c
// ------------------------------------------------------------------------------------------------
#ifndef X86_LAPIC_PRIVATE_H
#define X86_LAPIC_PRIVATE_H

#include <inc/types.h>
#include "apic.h"
#include "tsc.h"
#include <inc/x86.h>
#include <kern/pmap.h>


#define IRQ_IPI      1U
#define IRQ_IDE      14U

#define EXCEPTION_PIC_IRQ_SHIFT IRQ_OFFSET

#define DIVIDE_CONFIG             0x3U // divide by 16
#define CPU_MODEL_PENRYN          0x17U
#define CPU_MODEL_NEHALEM         0x1AU
#define MSR_IA32_PERF_STATUS      0x00000198U
#define MSR_NEHALEM_PLATFORM_INFO 0x000000CEU

#define LAPIC_BASE_MSR                     0x1BU
#define LAPIC_BASE_MSR_BOOTSTRAP_PROCESSOR (1U << 8U)
#define LAPIC_BASE_MSR_X2APIC_MODE         (1U << 10U)
#define LAPIC_BASE_MSR_ENABLE              (1U << 11U)
#define LAPIC_BASE_X2APIC_ENABLED          (LAPIC_BASE_MSR_X2APIC_MODE | LAPIC_BASE_MSR_ENABLE)
#define LAPIC_BASE_MSR_ADDR_MASK           0xFFFFF000U

#define LAPIC_DEFAULT_BASE 0xfee00000U

#define LAPIC_START 0xFEE00000U
#define LAPIC_SIZE  0x00000400U

//#define LAPIC_ID       0x00000020U
#define LAPIC_ID_SHIFT 24
#define LAPIC_ID_MASK  0xFFU

#define LAPIC_VERSION      0x00000030U
#define LAPIC_VERSION_MASK 0xFFU

#define LAPIC_TPR      0x00000080U
#define LAPIC_TPR_MASK 0xFFU

#define LAPIC_APR      0x00000090U
#define LAPIC_APR_MASK 0xFFU

#define LAPIC_PPR      0x000000A0U
#define LAPIC_PPR_MASK 0xFFU

#define LAPIC_EOI         0x000000B0U
#define LAPIC_REMOTE_READ 0x000000C0U
#define LAPIC_LDR         0x000000D0U
#define LAPIC_LDR_SHIFT   24

#define LAPIC_DFR         0x000000E0U
#define LAPIC_DFR_FLAT    0xFFFFFFFFU
#define LAPIC_DFR_CLUSTER 0x0FFFFFFFU
#define LAPIC_DFR_SHIFT   28

#define LAPIC_SVR           0x000000F0U
#define LAPIC_SVR_MASK      0x0FFU
#define LAPIC_SVR_ENABLE    0x100U
#define LAPIC_SVR_FOCUS_OFF 0x200U

#define LAPIC_ISR_BASE          0x00000100U
#define LAPIC_TMR_BASE          0x00000180U
#define LAPIC_IRR_BASE          0x00000200U
#define LAPIC_ERROR_STATUS      0x00000280U
#define LAPIC_LVT_CMCI          0x000002F0U
#define LAPIC_ICR               0x00000300U
#define LAPIC_ICR_VECTOR_MASK   0x000FFU
#define LAPIC_ICR_DM_MASK       0x00700U
#define LAPIC_ICR_DM_FIXED      0x00000U
#define LAPIC_ICR_DM_LOWEST     0x00100U
#define LAPIC_ICR_DM_SMI        0x00200U
#define LAPIC_ICR_DM_REMOTE     0x00300U
#define LAPIC_ICR_DM_NMI        0x00400U
#define LAPIC_ICR_DM_INIT       0x00500U
#define LAPIC_ICR_DM_STARTUP    0x00600U
#define LAPIC_ICR_DM_LOGICAL    0x00800U
#define LAPIC_ICR_DS_PENDING    0x01000U
#define LAPIC_ICR_LEVEL_ASSERT  0x04000U
#define LAPIC_ICR_TRIGGER_LEVEL 0x08000U
#define LAPIC_ICR_RR_MASK       0x30000U
#define LAPIC_ICR_RR_INVALID    0x00000U
#define LAPIC_ICR_RR_INPROGRESS 0x10000U
#define LAPIC_ICR_RR_VALID      0x20000U
#define LAPIC_ICR_DSS_MASK      0xC0000U
#define LAPIC_ICR_DSS_DEST      0x00000U
#define LAPIC_ICR_DSS_SELF      0x40000U
#define LAPIC_ICR_DSS_ALL       0x80000U
#define LAPIC_ICR_DSS_OTHERS    0xC0000U

#define LAPIC_ICRD              0x00000310U
#define LAPIC_ICRD_DEST_SHIFT   24
#define GET_LAPIC_DEST_FIELD(x) (((x) >> 24U) & 0xFFU)
#define SET_LAPIC_DEST_FIELD(x) ((x) << 24U)

#define LAPIC_LVT_TIMER               0x00000320U
#define LAPIC_LVT_THERMAL             0x00000330U
#define LAPIC_LVT_PERFCNT             0x00000340U
#define LAPIC_LVT_LINT0               0x00000350U
#define LAPIC_LVT_TIMER_BASE_MASK     (0x3U << 18)
#define GET_LAPIC_TIMER_BASE(x)       (((x) >> 18) & 0x3U)
#define SET_LAPIC_TIMER_BASE(x)       (((x) << 18))
#define LAPIC_TIMER_BASE_CLKIN        0x0U
#define LAPIC_TIMER_BASE_TMBASE       0x1U
#define LAPIC_TIMER_BASE_DIV          0x2U
#define LAPIC_LVT_TIMER_PERIODIC      (1 << 17)
#define LAPIC_LVT_MASKED              (1 << 16)
#define LAPIC_LVT_LEVEL_TRIGGER       (1 << 15)
#define LAPIC_LVT_REMOTE_IRR          (1 << 14)
#define LAPIC_INPUT_POLARITY          (1 << 13)
#define LAPIC_SEND_PENDING            (1 << 12)
#define LAPIC_LVT_RESERVED_1          (1 << 11)
#define LAPIC_DELIVERY_MODE_MASK      (7 << 8)
#define LAPIC_DELIVERY_MODE_FIXED     (0 << 8)
#define LAPIC_DELIVERY_MODE_NMI       (4 << 8)
#define LAPIC_DELIVERY_MODE_EXTINT    (7 << 8)
#define GET_LAPIC_DELIVERY_MODE(x)    (((x) >> 8) & 0x7U)
#define SET_LAPIC_DELIVERY_MODE(x, y) (((x) & ~0x700U) | ((y) << 8))
#define LAPIC_MODE_FIXED              0x0U
#define LAPIC_MODE_NMI                0x4U
#define LAPIC_MODE_EXINT              0x7U
#define LAPIC_LVT_LINT1               0x00000360U
#define LAPIC_LVT_ERROR               0x00000370U
#define LAPIC_LVT_VECTOR_MASK         0x000FFU
#define LAPIC_LVT_DM_SHIFT            8
#define LAPIC_LVT_DM_MASK             0x00007U
#define LAPIC_LVT_DM_FIXED            0x00000U
#define LAPIC_LVT_DM_NMI              0x00400U
#define LAPIC_LVT_DM_EXTINT           0x00700U
#define LAPIC_LVT_DS_PENDING          0x01000U
#define LAPIC_LVT_IP_PLRITY_LOW       0x02000U
//#define         LAPIC_LVT_REMOTE_IRR    0x04000U
#define LAPIC_LVT_TM_LEVEL 0x08000U
//#define         LAPIC_LVT_MASKED        0x10000U
#define LAPIC_LVT_PERIODIC     0x20000U
#define LAPIC_LVT_TSC_DEADLINE 0x40000U
#define LAPIC_LVT_TMR_SHIFT    17
#define LAPIC_LVT_TMR_MASK     3

#define LAPIC_TIMER_INITIAL_COUNT 0x00000380U
#define LAPIC_TIMER_CURRENT_COUNT 0x00000390U
#define LAPIC_TIMER_DIVIDE_CONFIG 0x000003E0U
/* divisor encoded by bits 0,1,3 with bit 2 always 0: */
#define LAPIC_TIMER_DIVIDE_TMBASE (1 << 2)
#define LAPIC_TIMER_DIVIDE_MASK   0x0000000FU
#define LAPIC_TIMER_DIVIDE_2      0x00000000U
#define LAPIC_TIMER_DIVIDE_4      0x00000001U
#define LAPIC_TIMER_DIVIDE_8      0x00000002U
#define LAPIC_TIMER_DIVIDE_16     0x00000003U
#define LAPIC_TIMER_DIVIDE_32     0x00000008U
#define LAPIC_TIMER_DIVIDE_64     0x00000009U
#define LAPIC_TIMER_DIVIDE_128    0x0000000AU
#define LAPIC_TIMER_DIVIDE_1      0x0000000BU

#define LAPIC_MMIO_PBASE 0xFEE00000U /* Default physical MMIO addr */
//#define LAPIC_MMIO_VBASE        lapic_vbase     /* Actual virtual mapped addr */
#define LAPIC_MSR_BASE 0x800cU


#define LAPIC_MMIO_OFFSET(reg) ((reg) << 4U)
#define LAPIC_MSR_OFFSET(reg)  (reg)

#define LAPIC_MMIO(reg) ((volatile uint32_t*) (LAPIC_MMIO_VBASE + LAPIC_MMIO_OFFSET(reg)))
#define LAPIC_MSR(reg)  (LAPIC_MSR_BASE + LAPIC_MSR_OFFSET(reg))

/*
 * By default, use high vectors to leave vector space for systems
 * with multiple I/O APIC's. However some systems that boot with
 * local APIC disabled will hang in SMM when vectors greater than
 * 0x5FU are used. Those systems are not expected to have I/O APIC
 * so 16 (0x50U - 0x40U) vectors for legacy PIC support is perfect.
 */
#define LAPIC_DEFAULT_INTERRUPT_BASE 0xD0U
#define LAPIC_REDUCED_INTERRUPT_BASE 0x50U
/*
 * Specific lapic interrupts are relative to this base
 * in priority order from high to low:
 */

#define LAPIC_PERFCNT_INTERRUPT        0xFU
#define LAPIC_INTERPROCESSOR_INTERRUPT 0xEU
#define LAPIC_TIMER_INTERRUPT          0xDU
#define LAPIC_THERMAL_INTERRUPT        0xCU
#define LAPIC_ERROR_INTERRUPT          0xBU
#define LAPIC_SPURIOUS_INTERRUPT       0xAU
#define LAPIC_CMCI_INTERRUPT           0x9U
#define LAPIC_PMC_SW_INTERRUPT         0x8U
#define LAPIC_PM_INTERRUPT             0x7U
#define LAPIC_KICK_INTERRUPT           0x6U

#define LAPIC_PMC_SWI_VECTOR (LAPIC_DEFAULT_INTERRUPT_BASE + LAPIC_PMC_SW_INTERRUPT)
#define LAPIC_TIMER_VECTOR   (LAPIC_DEFAULT_INTERRUPT_BASE + LAPIC_TIMER_INTERRUPT)

/* The vector field is ignored for NMI interrupts via the LAPIC
 * or otherwise, so this is not an offset from the interrupt
 * base.
 */

#define LAPIC_NMI_INTERRUPT 0x2U

extern uint8_t* lapic_addr;


/* Write 32-bit value into LAPIC register, specified by given offset. */
static inline void lapicw(int index, uint32_t value)
{
  *(volatile uint32_t*) (lapic_addr + index) = value;
  (void) *(volatile uint32_t*) (lapic_addr + LAPIC_ID); // wait for write to finish, by reading
}

/* Read 32-bit value from LAPIC register, specified by given offset. */
static inline uint32_t lapicr(int index)
{
  return *(volatile uint32_t*) (lapic_addr + index);
}


void lapic_init(void) // init local apic on cpu, use addr of corresponding cpu
{
  // Enable local APIC; set spurious interrupt vector.
  lapicw(LAPIC_SVR, LAPIC_ENABLE | (IRQ_OFFSET + IRQ_SPURIOUS));

  // Disable TIMER (LINT0) on all CPU
  lapicw(LAPIC_LVT_LINT0, IRQ_OFFSET + 1);

  // Disable NMI (LINT1) on all CPUs
  lapicw(LAPIC_LINT1, LAPIC_MASKED);

  // Disable performance counter overflow interrupts
  // on machines that provide that interrupt entry.
  if (((lapicr(LAPIC_VER) >> 16U) & 0xFFU) >= 4)
  {
    lapicw(LAPIC_PCINT, LAPIC_MASKED);
  }

  // Map error interrupt to IRQ_ERROR.
  lapicw(LAPIC_ERROR, IRQ_OFFSET + IRQ_ERROR);

  // Clear error status register (requires back-to-back writes).
  lapicw(LAPIC_ESR, 0);
  lapicw(LAPIC_ESR, 0);

  // Ack any outstanding interrupts.
  lapicw(LAPIC_EOI, 0);

  // Send an Init + Level + De-Assert sequence to synchronize arbitration ID's.
  lapicw(LAPIC_ICRHI, 0);
  lapicw(LAPIC_ICRLO, LAPIC_BCAST | LAPIC_INIT | LAPIC_LEVEL);
  // Wait until sequence is delivered
  // (we read from Lapic reg until DELIVS flag pops up)
  while (lapicr(LAPIC_ICRLO) & LAPIC_DELIVS)
  {
    pit_wait(1);
  }

  // Enable interrupts on the APIC (but not on the processor).
  lapicw(LAPIC_TPR, 0);
}

uint32_t LocalApicGetId()
{
    return lapicr(LAPIC_ID) >> 24;
}

//extern char warm[];

void lapic_start_ap(uint8_t apicid, uint32_t addr)
{
  int i;

	/*uint16_t *wrv;

	// "The BSP must initialize CMOS shutdown code to 0AH
	// and the warm reset vector (DWORD based at 40:67) to point at
	// the AP startup code prior to the [universal startup algorithm]."
	outb(CMOS_CMD, 0xF);  // offset 0xF is shutdown code
	outb(CMOS_DATA, 0x0A);
	wrv = (uint16_t *)warm;  // Warm reset vector
	wrv[0] = 0;
	wrv[1] = addr >> 4;*/

  // "Universal startup algorithm."
  // Send INIT (level-triggered) interrupt to reset other CPU.
  lapicw(LAPIC_ICRHI, apicid << 24U);
  lapicw(LAPIC_ICRLO, LAPIC_INIT | LAPIC_LEVEL | LAPIC_ASSERT);
  pit_wait(200);
  lapicw(LAPIC_ICRLO, LAPIC_INIT | LAPIC_LEVEL);
  pit_wait(100); // should be 10ms, but too slow in Bochs!

  // Send startup IPI (twice!) to enter code.
  // Regular hardware is supposed to only accept a STARTUP
  // when it is in the halted state due to an INIT.  So the second
  // should be ignored, but it is part of the official Intel algorithm.
  // Bochs complains about the second one.  Too bad for Bochs.
  for (i = 0; i < 2; i++)
  {
    lapicw(LAPIC_ICRHI, SET_LAPIC_DEST_FIELD(apicid));
    lapicw(LAPIC_ICRLO, LAPIC_STARTUP | (addr >> 12U));
    pit_wait(200);
  }
}

#endif /* X86_LAPIC_PRIVATE_H */