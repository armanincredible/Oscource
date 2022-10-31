/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <kern/kclock.h>
#include <kern/trap.h>
#include <kern/picirq.h>

/* HINT: Note that selected CMOS
 * register is reset to the first one
 * after first access, i.e. it needs to be selected
 * on every access.
 *
 * Don't forget to disable NMI for the time of
 * operation (look up for the appropriate constant in kern/kclock.h)
 *
 * Why it is necessary?
 */

uint8_t
cmos_read8(uint8_t reg) {
    /* MC146818A controller */
    // LAB 4: Your code here

    uint8_t res = 0;
    outb (CMOS_CMD, reg);
    res = inb (CMOS_DATA);

    return res;
}

void
cmos_write8(uint8_t reg, uint8_t value) {
    // LAB 4: Your code here
    outb (CMOS_CMD, reg);
    outb (CMOS_DATA, value);
}

uint16_t
cmos_read16(uint8_t reg) {
    return cmos_read8(reg) | (cmos_read8(reg + 1) << 8);
}

void
rtc_timer_pic_interrupt(void) {
    pic_irq_unmask (IRQ_CLOCK);
    // LAB 4: Your code here
    // Enable PIC interrupts.
}

void
rtc_timer_pic_handle(void) {
    cprintf ("here\n");
    rtc_check_status();
    pic_send_eoi(IRQ_CLOCK);
}

void
rtc_timer_init(void) {
    nmi_disable();

    uint8_t val = cmos_read8(RTC_BREG);
    val |=  RTC_PIE;
    cmos_write8(RTC_BREG, val);

    //val = cmos_read8(RTC_AREG);
    //uint8_t rate = RTC_NON_RATE_MASK(14);
    //cmos_write8 (RTC_AREG, RTC_SET_NEW_RATE(val, rate));

    /*val = rtc_check_status ();
    if (!(val & RTC_PIE))
    {
        cprintf ("\n\n\ndsds\n");
    }*/
    rtc_check_status ();
    nmi_enable();
    // LAB 4: Your code here
    // (use cmos_read8/cmos_write8)
}

uint8_t
rtc_check_status(void) {
    // LAB 4: Your code here
    // (use cmos_read8)
    uint8_t val = cmos_read8 (RTC_CREG);
    return val;

    //return 0;
}
