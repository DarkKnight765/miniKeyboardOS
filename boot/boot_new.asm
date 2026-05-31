; Multiboot header - MUST be in first 8KB
MBALIGN  equ 1<<0
MEMINFO  equ 1<<1
FLAGS    equ MBALIGN | MEMINFO
MAGIC    equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .text
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; GDT
align 8
gdt:
    dq 0
    dq 0x00cf9a000000ffff
    dq 0x00cf92000000ffff
gdt_ptr:
    dw $ - gdt - 1
    dd gdt

global _start
extern kernel_main

_start:
    mov esp, stack_top
    lgdt [gdt_ptr]
    jmp 0x08:.setcs
.setcs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    push ebx
    push eax
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
    resb 16384
stack_top:
