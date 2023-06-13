#include "acpi.h"
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/uefi.h>
#include <inc/memlayout.h>
#include "apic.h"
#include "pmap.h"
#include "tsc.h"
#include <kern/spinlock.h>
#include <kern/cpu.h>

// Per-CPU kernel stacks
unsigned char percpu_kstacks[NCPU][KERN_STACK_SIZE]
__attribute__ ((aligned(PAGE_SIZE)));

struct CpuInfo cpus[MAX_CPU];
//uint8_t lapic_ids[256] = {0}; // CPU core Local APIC IDs
uint8_t numcore = 0;          // number of cores detected
volatile uint8_t* lapic_addr = 0;       // pointer to the Local APIC MMIO registers
uint8_t* ioapic_ptr = 0;      // pointer to the IO APIC MMIO registers
__attribute__((aligned(8))) uint64_t active_cpu_count = 0;  //

struct spinlock cpus_variable;

void detect_cores(void)
{
    MADT* madt = get_madt();
    uint32_t madt_length = madt->h.Length;
    
    lapic_addr = (uint8_t*)(uintptr_t)(madt->local_apic_address);

    MADT_RECORD_HEADER* ptr = (MADT_RECORD_HEADER*)((uint8_t*)madt + sizeof(MADT));
    MADT_RECORD_HEADER* ptr2 = (MADT_RECORD_HEADER*)((uint8_t*)madt + madt_length);

    cprintf ("MADT: %p, ptr %p, ptr2 %p\n", madt, ptr, ptr2);

    for(;ptr < ptr2; ptr = (MADT_RECORD_HEADER*)((uint8_t*)ptr + ptr->record_length))
    {
        switch (ptr->entry_type)
        {
            case APIC_TYPE_LOCAL_APIC:
            {
                MADT_RECORD_LOCAL_APIC* record = (MADT_RECORD_LOCAL_APIC*)ptr;
                if (record->flags)
                    //lapic_ids [ numcore ++ ]  = record->apic_id;
                    if (numcore < MAX_CPU)
                        cpus[numcore ++].cpu_id = record->apic_id;
                break;
            }
            case APIC_TYPE_IO_APIC:
            {
                MADT_RECORD_IO_APIC* record1 = (MADT_RECORD_IO_APIC*)ptr;
                ioapic_ptr = (uint8_t*)(uintptr_t)(record1->io_apic_ptr);
                break;
            }
            case APIC_TYPE_ADDRESS_OVERRIDE:
            {
                MADT_RECORD_ADDRESS_OVERRIDE* record5 = (MADT_RECORD_ADDRESS_OVERRIDE*)ptr;
                lapic_addr = (uint8_t*)(uintptr_t)(record5->ph_ptr_loc_apic);
                break;
            }
            case TYPE_WAKEUP_STRUCTURE:
            {
                assert(0);
            }
            default:
                break;
        }
    }
    
    lapic_addr = mmio_map_region((uintptr_t)lapic_addr, MMIO_LAPIC_SIZE_REGISTERS);
    cprintf("Found %d cores, IOAPIC %lx, LAPIC %lx, Processor IDs:", numcore, (uintptr_t)ioapic_ptr, (uintptr_t)lapic_addr);
    for(int i = 0; i < numcore; i++)
    {
        cprintf(" %d", cpus[i].cpu_id);
    }
    cprintf("\n");
    
    add_ap_stacks_mapping();
    //ioapic_ptr = mmio_map_region((uintptr_t)ioapic_ptr, MMIO_LAPIC_SIZE_REGISTERS);

    // switch to getting interrupts from the LAPIC.
    // not harmful, probably, even if firmware did the switch already
    outb(0x22, 0x70); // Select IMCR
    outb(0x23, inb(0x23) | 1U); // Mask external interrupts.


    return;
}

void smp_init()
{
    cprintf ("Start waking up all CPUs\n");
    active_cpu_count = 1;
    uint32_t localId = LocalApicGetId();

    lapic_init();
    pit_wait(10);

    extern char load[];
    extern void loader_ap();
    cprintf ("loader_ap: %16x\n", (uint64_t)loader_ap);
    cprintf ("detect_cores: %16x\n", (uint64_t)detect_cores);
    memcpy(load, (void*)loader_ap, PAGE_SIZE);
    //load[20] = 0;

    // Send Startup to all cpus except self
    for (uint32_t i = 0; i < numcore; ++i)
    {
        uint32_t apicId = cpus[i].cpu_id;
        if (apicId != localId)
        {
            //spin_lock(&cpus_variable);

            uint32_t cur_core = active_cpu_count;

            extern char aprealbootstacktop[], aprealbootstack[];
            *aprealbootstack = 0;
            *aprealbootstacktop = 0;
            *((uintptr_t*)0x8041624fc8) = 0; 
            cprintf ("value is worked %p\n", aprealbootstack);
            cprintf ("value is worked %p\n", aprealbootstacktop);

            lapic_start_ap(apicId, START_AP_PTR);

            while (cur_core == active_cpu_count)
            {
                pit_wait(1);
            }

            *aprealbootstack = 0;
            *aprealbootstacktop = 0;
            cprintf ("value is worked second time %p\n", aprealbootstack);
            cprintf ("value is worked second time %p\n", aprealbootstacktop);
            //spin_unlock(&cpus_variable);
        }
    }

    // Wait for all cpus to be active
    pit_wait(1);
    while (1)
    {
        spin_lock(&cpus_variable);
        cprintf("Waiting... active %ld cores\n", active_cpu_count);
        if (numcore == active_cpu_count)
        {
            spin_unlock(&cpus_variable);
            break;
        }
        spin_unlock(&cpus_variable);
        pit_wait(10);
    }

    cprintf("All CPUs activated\n");
}