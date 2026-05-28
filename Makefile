TARGET = i686-elf

CC = $(TARGET)-g++
AS = nasm
LD = $(TARGET)-ld

CFLAGS = -std=c++17 -ffreestanding -fno-exceptions -fno-rtti -Wall -Wextra -Werror -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -T linker.ld -n

BOOT = boot/boot.asm
KERNEL_CPP = kernel/kernel.cpp kernel/idt.cpp kernel/keyboard.cpp kernel/timer.cpp
KERNEL_ASM = kernel/idt_load.asm kernel/isr.asm
DRIVER_SRC = drivers/vga.cpp

OBJS = $(BOOT:.asm=.o) $(KERNEL_CPP:.cpp=.o) $(KERNEL_ASM:.asm=.o) $(DRIVER_SRC:.cpp=.o)

.PHONY: all clean distclean iso run-iso

all: kernel.bin

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.elf $^
	objcopy -O binary kernel.elf kernel.bin

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

# Cross-platform clean
ifeq ($(OS),Windows_NT)
RM = cmd /C del /Q
RMDIR = cmd /C rmdir /S /Q
else
RM = rm -f
RMDIR = rm -rf
endif

clean:
	-$(RM) kernel.elf 2>NUL || true
	-$(RM) kernel.bin 2>NUL || true
	-$(RM) boot\boot.o 2>NUL || true
	-$(RM) drivers\vga.o 2>NUL || true
	-$(RM) kernel\*.o 2>NUL || true

distclean: clean
	-$(RMDIR) iso 2>NUL || true
	-$(RM) miniKeyboardOS.iso 2>NUL || true
	-$(RM) boot.img 2>NUL || true

# --- GRUB ISO (requires grub-mkrescue, typically via WSL/Linux) ---
.PHONY: iso run-iso

ISO_DIR = iso
ISO_BOOT = $(ISO_DIR)/boot
ISO_GRUB = $(ISO_BOOT)/grub
ISO_IMAGE = miniKeyboardOS.iso

$(ISO_GRUB)/grub.cfg:
	mkdir -p $(ISO_GRUB)
	echo 'set timeout=0' > $(ISO_GRUB)/grub.cfg
	echo 'set default=0' >> $(ISO_GRUB)/grub.cfg
	echo 'menuentry "miniKeyboardOS" {' >> $(ISO_GRUB)/grub.cfg
	echo '  multiboot /boot/kernel.elf' >> $(ISO_GRUB)/grub.cfg
	echo '  boot' >> $(ISO_GRUB)/grub.cfg
	echo '}' >> $(ISO_GRUB)/grub.cfg

$(ISO_BOOT)/kernel.elf: kernel.elf
	mkdir -p $(ISO_BOOT)
	cp kernel.elf $(ISO_BOOT)/kernel.elf

iso: kernel.elf $(ISO_GRUB)/grub.cfg $(ISO_BOOT)/kernel.elf
	grub-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

run-iso: iso
	qemu-system-i386 -cdrom $(ISO_IMAGE) -m 256
