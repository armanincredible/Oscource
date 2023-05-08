#include <inc/types.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/x86.h>
#include <inc/uefi.h>
#include <kern/timer.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/trap.h>
#include <kern/pmap.h>
#include "acpi.h"

bool 
check_sum(ACPISDTHeader *tableHeader)
{
    unsigned char sum = 0;
 
    for (int i = 0; i < tableHeader->Length; i++)
    {
        sum += ((char *) tableHeader)[i];
    }

    return sum == 0;
}
void
acpi_enable(void) {
    FADT *fadt = get_fadt();
    outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
    while ((inw(fadt->PM1aControlBlock) & 1) == 0) /* nothing */
        ;
}

static RSDT *rsdt;
static RSDP *rsdp;
//ACPISDTHeader headers[]
ACPISDTHeader** headers;


void *
acpi_find_table(const char *sign) {
    /*
     * This function performs lookup of ACPI table by its signature
     * and returns valid pointer to the table mapped somewhere.
     *
     * It is a good idea to checksum tables before using them.
     *
     * HINT: Use mmio_map_region/mmio_remap_last_region
     * before accessing table addresses
     * (Why mmio_remap_last_region is requrired?)
     * HINT: RSDP address is stored in uefi_lp->ACPIRoot
     * HINT: You may want to distunguish RSDT/XSDT
     */

    
    if (!uefi_lp->ACPIRoot)
    {
        panic("No rsdp\n");
    }
    if (!rsdp)
        rsdp = mmio_map_region(uefi_lp->ACPIRoot, sizeof(RSDP));
    if (!rsdt)
    {
        rsdt = mmio_map_region(rsdp->RsdtAddress, sizeof (RSDT));
        rsdt = mmio_remap_last_region(rsdp->RsdtAddress, rsdt, sizeof (RSDT), rsdt->h.Length);
    }

    int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
    if (!headers)
    {
        headers = (ACPISDTHeader**)kzalloc_region(ROUNDUP(entries * sizeof(ACPISDTHeader*), PAGE_SIZE));
    }
 
    for (int i = 0; i < entries; i++)
    {
        uint32_t pheader = rsdt->PointerToOtherSDT[i];
        ACPISDTHeader *header = headers[i];

        if (!header)
        {
            header = mmio_map_region(rsdt->PointerToOtherSDT[i], sizeof (ACPISDTHeader));
            header = mmio_remap_last_region(rsdt->PointerToOtherSDT[i], header, sizeof (ACPISDTHeader), header->Length);
        }

        if (header->Signature && !strncmp(header->Signature, sign, 4) && check_sum (header))
        {
            //cprintf ("header = %p\n", header);
            return (void *) header;
        }
    }



    return NULL;
}

/* Obtain and map FADT ACPI table address. */
FADT *
get_fadt(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)
    // HINT: ACPI table signatures are
    //       not always as their names

    static FADT *kfadt;
    if (!kfadt)
    {
        kfadt = acpi_find_table("FACP");
        if (!kfadt)
        {
            panic("get_fadt:didnt find facp");
        }
    }

    return kfadt;
}

MADT *
get_madt(void) {
    // LAB 5: Your code here
    // (use acpi_find_table)
    // HINT: ACPI table signatures are
    //       not always as their names

    static MADT *madt;
    if (!madt)
    {
        madt = acpi_find_table("APIC");
        if (!madt)
        {
            panic("get_madt:didnt find APIC");
        }
    }

    return madt;
}