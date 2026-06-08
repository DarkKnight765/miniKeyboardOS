// Minimal timer ISR to verify interrupts are alive
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

extern "C" void timer_isr()
{
    static volatile uint32_t ticks = 0;
    ticks++;

    // Write a visible indicator at top-left every 16 ticks
    if ((ticks & 0x0F) == 0)
    {
        uint16_t *vga = (uint16_t *)0xB8000;
        char c = '0' + ((ticks >> 4) & 0x0F);
        vga[0] = (0x0A << 8) | c; // green digit at [0,0]
    }

    // Send EOI to PIC master
    outb(0x20, 0x20);
}
