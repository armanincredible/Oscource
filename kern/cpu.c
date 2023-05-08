#include "acpi.h"
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/uefi.h>
#include <inc/memlayout.h>
#include "apic.h"
#include "pmap.h"
#include "tsc.h"

uint8_t lapic_ids[256] = {0}; // CPU core Local APIC IDs
uint8_t numcore = 0;          // number of cores detected
uint8_t* lapic_ptr = 0;       // pointer to the Local APIC MMIO registers
uint8_t* ioapic_ptr = 0;      // pointer to the IO APIC MMIO registers
uint64_t active_cpu_count = 0;  //

void detect_cores(void)
{
    MADT* madt = get_madt();
    uint32_t madt_length = madt->h.Length;
    
    lapic_ptr = (uint8_t*)(uintptr_t)(madt->local_apic_address);

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
                    lapic_ids [ numcore ++ ]  = record->apic_id;
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
                lapic_ptr = (uint8_t*)(uintptr_t)(record5->ph_ptr_loc_apic);
                break;
            }
            default:
                break;
        }
    }
    

    cprintf("Found %d cores, IOAPIC %lx, LAPIC %lx, Processor IDs:", numcore, (uintptr_t)ioapic_ptr, (uintptr_t)lapic_ptr);
    for(int i = 0; i < numcore; i++)
    {
        cprintf(" %d", lapic_ids[i]);
    }
    cprintf("\n");

    lapic_ptr = mmio_map_region((uintptr_t)lapic_ptr, MMIO_LAPIC_SIZE_REGISTERS);
    //ioapic_ptr = mmio_map_region((uintptr_t)ioapic_ptr, MMIO_LAPIC_SIZE_REGISTERS);

    return;
}

void smp_init()
{
    cprintf ("Start waking up all CPUs\n");
    active_cpu_count = 1;
    uint32_t localId = LocalApicGetId();

    // Send Init to all cpus except self
    for (uint32_t i = 0; i < numcore; ++i)
    {
        uint32_t apicId = lapic_ids[i];
        if (apicId != localId)
        {
            LocalApicSendInit(apicId);
        }
    }

    pit_wait(10);

    // Send Startup to all cpus except self
    for (uint32_t i = 0; i < numcore; ++i)
    {
        uint32_t apicId = lapic_ids[i];
        if (apicId != localId)
        {
            LocalApicSendStartup(apicId, 0x8);
        }
    }

    // Wait for all cpus to be active
    pit_wait(1);
    while (active_cpu_count != numcore)
    {
        cprintf("Waiting... active %ld cores\n", active_cpu_count);
        pit_wait(1);
    }

    cprintf("All CPUs activated\n");
}