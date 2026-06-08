#include "kernel/idt.hpp"
#include "drivers/vga.hpp"

// PIC + port I/O helpers (local to this file)
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait()
{
    asm volatile("jmp 1f\n1:");
}

static void pic_remap(uint8_t offset1, uint8_t offset2)
{
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    // Start initialization sequence (cascade mode)
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    // Set vector offsets
    outb(0x21, offset1);
    io_wait();
    outb(0xA1, offset2);
    io_wait();

    // Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(0x21, 0x04);
    io_wait();
    // Tell Slave PIC its cascade identity (0000 0010)
    outb(0xA1, 0x02);
    io_wait();

    // Set PICs to 8086/88 mode
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    // Restore saved masks
    outb(0x21, a1);
    outb(0xA1, a2);
}

static IDTDescriptor idt[256];
static IDTPointer idt_ptr;

extern "C" void isr_keyboard();
extern "C" void isr_timer();

void idt_set_descriptor(uint8_t vector, void (*isr)(), uint8_t flags)
{
    uint32_t addr = (uint32_t)isr;
    idt[vector].offset_low = addr & 0xFFFF;
    idt[vector].offset_high = (addr >> 16) & 0xFFFF;
    idt[vector].selector = 0x08;
    idt[vector].type_attr = flags;
    idt[vector].zero = 0;
}

void idt_init()
{
    idt_ptr.base = (uint32_t)&idt[0];
    idt_ptr.limit = (sizeof(IDTDescriptor) * 256) - 1;

    for (int i = 0; i < 256; i++)
    {
        idt_set_descriptor(i, (void (*)())0, 0);
    }

    // Remap hardware IRQs to 0x20–0x2F to avoid CPU exceptions (0x00–0x1F)
    pic_remap(0x20, 0x28);

    // IRQ0 = Timer (after PIC remap: 0x20 + 0 = 32)
    idt_set_descriptor(32, isr_timer, 0x8E);
    // IRQ1 = Keyboard (after PIC remap: 0x20 + 1 = 33)
    idt_set_descriptor(33, isr_keyboard, 0x8E);

    idt_load(&idt_ptr);
    vga_print("IDT initialized\n");
}
