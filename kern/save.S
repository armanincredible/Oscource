#include <inc/mmu.h>
#include <inc/memlayout.h>


.text

# -------------------------------------------------------------------------------------------------
# Application Processor Loader
loader_ap:
        .code16    # Assemble for 16-bit mode  
        #Each CPU needs a dedicated stack, so delay setting that up until we are in 64-bit mode

        #Activate Protected Mode
        lgdt [gdt32_desc]

        mov %cr0, %eax
        or %al, 0x01
        mov %eax, %cr0

        #jmp gdt32_code:ap32
        push gdt32_code
        push ap32
        retf


# -------------------------------------------------------------------------------------------------
ap32:
        .code32     # Assemble for 32-bit mode 
        mov gdt32_data, %eax
        mov %ax, %ds
        mov %ax, %es
        mov %ax, %fs
        mov %ax, %gs
        mov %ax, %ss
        mov $apbootstack, %esp

        lgdt [gdt64_desc]

        mov 0x000000a0, %eax         # Set PAE and PGE
        mov %eax, %cr4

        mov $pml4, %eax            # Assign PML4
        mov %eax, %cr3

        mov 0xc0000080, %ecx         # Read from EFER MSR
        rdmsr

        or 0x00000100, %eax          # Set LME
        wrmsr

        mov %cr0, %eax                # Activate paging
        or 0x80000000, %eax
        mov %eax, %cr0

        #jmp gdt64_code:ap64  - Jump to 64-bit code
        push gdt64_code
        push ap64
        retf


ap64:
        .code64
        movabs $ap64c, %rax
        jmpq *%rax

.code64
# -------------------------------------------------------------------------------------------------
# 64-bit GDT
gdt64:
        .quad 0x0000000000000000       # Null Descriptor
.set gdt64_code,  . - gdt64                 # Code segment
        .quad 0x0020980000000000
.set gdt64_data, . - gdt64                 # Data segment
        .quad 0x0000920000000000

gdt64_desc:
        .word . - gdt64 - 1            # 16-bit Size (Limit)
        .quad gdt64                    # 64-bit Base Address


.code32
# -------------------------------------------------------------------------------------------------
# 32-bit GDT
gdt32:
        .quad 0x0000000000000000       # Null Descriptor
.set gdt32_code,  . - gdt32                 # Code segment
        .quad 0x00cf9a000000ffff
.set gdt32_data, . - gdt32                 # Data segment
        .quad 0x00cf92000000ffff

gdt32_desc:
        .word . - gdt32 - 1            # 16-bit Size (Limit)
        .long gdt32                    # 32-bit Base Address


.code64
.data

# Allocate ap boot stack forcing page alignment.
.p2align PAGE_SHIFT
.globl apbootstack
apbootstack:
.space KERN_STACK_SIZE
.globl apbootstacktop
apbootstacktop:
