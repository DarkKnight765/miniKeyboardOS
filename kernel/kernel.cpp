#include "drivers/vga.hpp"
#include "kernel/idt.hpp"
#include "kernel/keyboard.hpp"

extern "C" void kernel_main()
{
    vga_init();
    vga_print("========================================\n");
    vga_print("   miniKeyboardOS\n");
    vga_print("========================================\n\n");

    // Unique build marker to confirm new binary is running
    vga_print("[BUILD] A1B2-IRQ-ONLY\n\n");

    vga_print("Step 1: IDT... ");
    idt_init();
    vga_print("OK\n");

    vga_print("Step 2: Keyboard... ");
    keyboard_init();
    vga_print("OK\n");

    vga_print("Step 3: Enable interrupts... ");
    asm volatile("sti");
    vga_print("OK\n");

    vga_print("\n=== SYSTEM READY ===\n");
    vga_print("Type to see keyboard input:\n\n");

    // Primary input: IRQ1 interrupt → keyboard_isr()
    // Fallback: keyboard_poll() if ISR not firing (QEMU quirk)
    // Dedup logic in poll prevents double prints
    const char spinner[4] = {'|', '/', '-', '\\'};
    uint32_t t = 0;
    while (1)
    {
        t++;
        if ((t & 0x3FFFF) == 0)
        {
            uint16_t *vga = (uint16_t *)0xB8000;
            char c = spinner[(t >> 18) & 3];
            vga[0] = (0x0A << 8) | c; // green spinner
        }
        keyboard_poll(); // fallback: safe to call, dedup prevents doubles
    }
}
