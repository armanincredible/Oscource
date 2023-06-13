#include <inc/mmu.h>
#include <inc/memlayout.h>


.set PROT_MODE_CSEG, 0x8         # kernel code segment selector
.set PROT_MODE_DSEG, 0x10        # kernel data segment selector

.text


.code16
.globl loader_ap
# Application Processor Loader
loader_ap:
        # Each CPU needs a dedicated stack, so delay setting that up until we are in 64-bit mode
        cli
        cld

        xorw    %ax, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %ss

    # Enable A20:
    #   For backwards compatibility with the earliest PCs, physical
    #   address line 20 is tied low, so that addresses higher than
    #   1MB wrap around to zero by default.  This code undoes this.
        seta20.1:
        inb     $0x64,%al               # Wait for not busy
        testb   $0x2,%al
        jnz     seta20.1

        movb    $0xd1,%al               # 0xd1 -> port 0x64
        outb    %al,$0x64

        seta20.2:
        inb     $0x64,%al               # Wait for not busy
        testb   $0x2,%al
        jnz     seta20.2

        movb    $0xdf,%al               # 0xdf -> port 0x60
        outb    %al,$0x60

    # Activate Protected Mode
        movw $(VIRT_TO_PHYS_AP(gdt32_desc, loader_ap, START_AP_PTR)), %di

        # Prefix for 32-bit lgdt instruction in 16-bit mode
        .byte 0x66
        .byte 0x2E
        lgdt (%di)

        movl %cr0, %eax
        orl $CR0_PE, %eax
        movl %eax, %cr0
    # Jump to next instruction, but in 32-bit code segment.
    # Switches processor into 32-bit mode.
        ljmp  $(PROT_MODE_CSEG), $(VIRT_TO_PHYS_AP(ap32, loader_ap, START_AP_PTR))


# -------------------------------------------------------------------------------------------------
.code32
ap32: 
        movw    $(PROT_MODE_DSEG), %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %ss
        movw    %ax, %fs
        movw    %ax, %gs  

        movl $(VIRT_TO_PHYS_AP(aptoentrystacktop, loader_ap, START_AP_PTR)), %esp

        movl $(VIRT_TO_PHYS_AP(gdt64_desc, loader_ap, START_AP_PTR)), %edi 
        lgdt (%edi)

        movl $0x000000a0, %eax         # Set PAE and PGE
        movl %eax, %cr4

        movl $pml4, %eax            # Assign PML4
        movl %eax, %cr3

        movl $0xc0000080, %ecx         # Read from EFER MSR
        rdmsr

        orl $0x00000100, %eax          # Set LME
        wrmsr

        movl %cr0, %eax                # Activate paging
        orl $0x80000000, %eax
        movl %eax, %cr0

        #jmp gdt64_code:ap64  - Jump to 64-bit code
        pushl $PROT_MODE_CSEG
        pushl $apboottoc
        retf
 
# -------------------------------------------------------------------------------------------------

.p2align 2                                # force 4 byte alignment
gdt32:
    SEG_NULL                            # null seg
    SEG(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
    SEG(STA_W, 0x0, 0xffffffff)         # data seg

gdt32_desc:
    .word   0x17                            # sizeof(gdt) - 1
    .long   VIRT_TO_PHYS_AP(gdt32, loader_ap, START_AP_PTR)                             # address gdt


.globl aptoentrystack
aptoentrystack:
.space 32
.globl aptoentrystacktop
aptoentrystacktop:


.code64
# -------------------------------------------------------------------------------------------------
# 64-bit GDT
.p2align 3                              # force 8 byte alignment
gdt64:
    SEG_NULL                            # null seg
    SEG64(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
    SEG64(STA_W, 0x0, 0xffffffff)         # data seg

gdt64_desc:
        .word   0x17
        .quad VIRT_TO_PHYS_AP(gdt64, loader_ap, START_AP_PTR)                    # 64-bit Base Address

