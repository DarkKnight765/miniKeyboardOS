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
    vga_print("[BUILD] 7F2C-KEYBOARD-POLL\n\n");

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

    // Interrupts handle keyboard, just idle with spinner
    const char spinner[4] = {'|', '/', '-', '\\'};
    uint32_t t = 0;
    while (1)
    {
        // Heartbeat: update spinner every cycle chunk
        t++;
        if ((t & 0xFFFF) == 0)
        {
            uint16_t *vga = (uint16_t *)0xB8000;
            char c = spinner[(t >> 16) & 3];
            vga[0] = (0x0A << 8) | c; // green spinner
        }
        keyboard_poll(); // Poll keyboard for visible output + spinner
    }
}
