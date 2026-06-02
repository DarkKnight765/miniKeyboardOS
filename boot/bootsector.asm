; 512-byte boot sector: loads stage2 loader to 0x9000:0000 and jumps
; Floppy geometry assumed: 18 sectors/track, 2 heads
; Build: nasm -f bin -o bootsector.bin boot/bootsector.asm

BITS 16
ORG 0x7C00

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Load stage2 (loader.bin) starting at LBA sector 2
    mov bx, 0x0000         ; offset within ES
    mov ax, 0x9000
    mov es, ax             ; ES:BX = 0x9000:0000

    mov si, 2              ; sector number (CHS)
    mov di, 0              ; cylinder
    mov bp, 0              ; head
    mov cx, LOADER_SECTORS ; sectors to read

load_loop:
    push cx
    push si
    push di
    push bp

    mov ah, 0x02           ; INT 13h read sectors
    mov al, 1              ; read 1 sector
    mov ch, byte [cyl]     ; cylinder
    mov cl, byte [sec]     ; sector (1..18)
    mov dh, byte [hd]      ; head (0..1)
    mov dl, 0x00           ; floppy A:
    int 0x13
    jc disk_error

    ; Advance ES:BX by 512 bytes
    add bx, 512
    jnc no_es_inc
    mov ax, es
    add ax, 0x20           ; advance ES by 512 bytes (32 paragraphs)
    mov es, ax
no_es_inc:

    ; Advance CHS
    inc byte [sec]
    cmp byte [sec], 19     ; sectors 1..18
    jl cont
    mov byte [sec], 1
    inc byte [hd]
    cmp byte [hd], 2
    jl cont
    mov byte [hd], 0
    inc byte [cyl]
cont:
    pop bp
    pop di
    pop si
    pop cx
    loop load_loop

    ; Jump to stage2 at 0x9000:0000
    jmp 0x9000:0x0000

disk_error:
    ; Hang on error
    cli
    hlt
    jmp disk_error

; CHS state
sec: db 2
hd:  db 0
cyl: db 0

; Placeholder, patched at build time
LOADER_SECTORS equ 16

times 510-($-$$) db 0
dw 0xAA55
