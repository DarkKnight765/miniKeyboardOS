; Stage2 loader: loads kernel.bin from disk, enables A20, switches to PM,
; copies kernel to 0x00100000 and jumps there.
; Built as flat bin and placed right after bootsector in the image.

BITS 16
ORG 0x0000

%define SECTORS_PER_TRACK 18
LOADER_SECTORS equ 16
KERNEL_SECTORS equ 256 ; default 256 sectors (~128KB)

start:
    ; Load kernel starting at sector (2 + LOADER_SECTORS)
    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov byte [sec], 2 + LOADER_SECTORS
    mov byte [hd], 0
    mov byte [cyl], 0

    mov cx, KERNEL_SECTORS
loadk:
    push cx
    mov ah, 0x02
    mov al, 1
    mov ch, byte [cyl]
    mov cl, byte [sec]
    mov dh, byte [hd]
    mov dl, 0x00
    int 0x13
    jc hang

    add bx, 512
    jnc no_inc_es
    mov ax, es
    add ax, 0x20           ; advance ES by 512 bytes
    mov es, ax
no_inc_es:
    ; advance CHS
    inc byte [sec]
    cmp byte [sec], SECTORS_PER_TRACK+1
    jl next_sector
    mov byte [sec], 1
    inc byte [hd]
    cmp byte [hd], 2
    jl next_sector
    mov byte [hd], 0
    inc byte [cyl]
next_sector:
    pop cx
    loop loadk

    ; Enable A20 using keyboard controller
    call enable_a20

    ; Setup minimal GDT
    lgdt [gdt_ptr]

    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pmode

hang:
    cli
    hlt
    jmp hang

enable_a20:
    ; Toggle A20 via KBC
    in al, 0x64
wait_in_clear:
    test al, 2
    jnz wait_in_clear
    mov al, 0xD1
    out 0x64, al
wait_in_clear2:
    in al, 0x64
    test al, 2
    jnz wait_in_clear2
    mov al, 0xDF ; set A20 enable bit
    out 0x60, al
    ret

sec: db 2
hd:  db 0
cyl: db 0

align 8
gdt:
    dq 0
    dq 0x00CF9A000000FFFF ; code
    dq 0x00CF92000000FFFF ; data

gdt_ptr:
    dw gdt_end - gdt - 1
    dd gdt
gdt_end:

BITS 32
pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Copy kernel from 0x00010000 to 0x00100000
    mov esi, 0x00010000
    mov edi, 0x00100000
    mov ecx, KERNEL_SECTORS
    shl ecx, 7 ; *128 dwords (512 bytes per sector)
    rep movsd

    ; Jump to kernel start
    jmp 0x08:0x00100000
