#include "kernel/keyboard.hpp"
#include "drivers/vga.hpp"
#include <stdint.h>

#define KBD_DATA_PORT 0x60
#define KBD_CTRL_PORT 0x64

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

// --- PS/2 controller helpers with timeouts ---
static bool wait_input_buffer_clear()
{
    // Wait until controller input buffer is clear (bit 1 == 0)
    for (int i = 0; i < 100000; i++)
    {
        if ((inb(KBD_CTRL_PORT) & 0x02) == 0)
            return true;
    }
    return false;
}

static bool wait_output_buffer_full()
{
    // Wait until controller output buffer has data (bit 0 == 1)
    for (int i = 0; i < 100000; i++)
    {
        if ((inb(KBD_CTRL_PORT) & 0x01) != 0)
            return true;
    }
    return false;
}

static void flush_output_buffer()
{
    // Drain any pending data
    int guard = 1024;
    while (guard-- > 0 && (inb(KBD_CTRL_PORT) & 0x01) != 0)
    {
        (void)inb(KBD_DATA_PORT);
    }
}

static const char SCANCODE_MAP[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0};

static volatile uint32_t kbd_irq_count = 0;
static volatile uint8_t kbd_e0_prefix = 0;
static volatile uint8_t kbd_mods = 0; // bit0: LShift, bit1: RShift, bit2: CapsLock

static inline char apply_shift_caps(char ch)
{
    if (ch >= 'a' && ch <= 'z')
    {
        bool uppercase = ((kbd_mods & 0x03) != 0) ^ ((kbd_mods & 0x04) != 0);
        if (uppercase)
            ch = (char)(ch - 'a' + 'A');
    }
    return ch;
}

static void print_hex_byte(uint8_t v)
{
    const char *hex = "0123456789ABCDEF";
    vga_putchar(hex[(v >> 4) & 0xF]);
    vga_putchar(hex[v & 0xF]);
}

extern "C" void keyboard_isr()
{
    kbd_irq_count++;

    if ((inb(KBD_CTRL_PORT) & 1) != 0)
    {
        uint8_t scancode = inb(KBD_DATA_PORT);

        // Handle extended scancode prefix
        if (scancode == 0xE0)
        {
            kbd_e0_prefix = 1;
            outb(0x20, 0x20);
            return;
        }

        // Handle key release: clear modifiers and ignore
        if (scancode & 0x80)
        {
            uint8_t base = scancode & 0x7F;
            if (base == 0x2A)
                kbd_mods &= ~0x01; // LShift up
            else if (base == 0x36)
                kbd_mods &= ~0x02; // RShift up
            outb(0x20, 0x20);
            kbd_e0_prefix = 0;
            return;
        }

        // Modifier presses
        if (scancode == 0x2A)
        {
            kbd_mods |= 0x01;
        } // LShift down
        else if (scancode == 0x36)
        {
            kbd_mods |= 0x02;
        } // RShift down
        else if (scancode == 0x3A)
        {
            kbd_mods ^= 0x04;
        } // CapsLock toggle

        char ch = 0;
        if (scancode < sizeof(SCANCODE_MAP))
            ch = SCANCODE_MAP[scancode];

        if (ch)
        {
            ch = apply_shift_caps(ch);
            if (ch == '\n')
            {
                vga_putchar('\n');
            }
            else if (ch == '\b')
            {
                vga_print("\b \b");
            }
            else if (ch == '\t')
            {
                vga_print("    ");
            }
            else
            {
                vga_putchar(ch);
            }
        }
        else
        {
            vga_print(" [SC:");
            if (kbd_e0_prefix)
            {
                vga_print("E0-");
            }
            print_hex_byte(scancode);
            vga_print("] ");
        }
        kbd_e0_prefix = 0;

        // ISR indicator at top-right to show activity
        uint16_t *vga = (uint16_t *)0xB8000;
        vga[79] = (0x0A << 8) | ('0' + (kbd_irq_count % 10));
    }

    outb(0x20, 0x20);
}

void keyboard_poll()
{
    // Fallback only: if interrupts are working, skip polling to avoid duplicates
    if (kbd_irq_count != 0)
        return;

    if ((inb(KBD_CTRL_PORT) & 1) != 0)
    {
        uint8_t scancode = inb(KBD_DATA_PORT);
        static uint8_t last_scancode = 0;

        // Key release: reset last so same key can be pressed again
        if (scancode & 0x80)
        {
            last_scancode = 0;
            return;
        }

        // Dedup: skip if this key is still held (typematic repeat)
        if (scancode == last_scancode)
            return;
        last_scancode = scancode;

        if (scancode < sizeof(SCANCODE_MAP))
        {
            char ch = SCANCODE_MAP[scancode];
            ch = apply_shift_caps(ch);
            if (ch != 0)
            {
                if (ch == '\n')
                    vga_putchar('\n');
                else if (ch == '\b')
                    vga_print("\b \b");
                else if (ch == '\t')
                    vga_print("    ");
                else
                    vga_putchar(ch);
            }
        }
    }
}

void keyboard_init()
{
    vga_print("Keyboard: init... ");

    // Install basic controller configuration
    // Disable keyboard while configuring
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");
    outb(KBD_CTRL_PORT, 0xAD); // Disable keyboard
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");

    // Read controller config byte
    flush_output_buffer();
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");
    outb(KBD_CTRL_PORT, 0x20); // Read config
    if (!wait_output_buffer_full())
        vga_print("[WARN obuf] ");
    uint8_t config = inb(KBD_DATA_PORT);

    // Enable IRQ1 (bit 0) and clear keyboard disable (bit 5)
    config |= 0x01;
    config &= ~0x20;

    // Write controller config byte
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");
    outb(KBD_CTRL_PORT, 0x60);
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");
    outb(KBD_DATA_PORT, config);
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");

    // Re-enable keyboard
    outb(KBD_CTRL_PORT, 0xAE);
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");

    // Clear any pending scancode
    flush_output_buffer();

    // Enable scanning on the keyboard via controller write-to-device path
    // Send 0xD4 to command port, then 0xF4 to data port; expect ACK (0xFA)
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");
    outb(KBD_CTRL_PORT, 0xD4); // Next byte goes to keyboard device
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");
    outb(KBD_DATA_PORT, 0xF4); // Enable scanning
    if (wait_output_buffer_full())
    {
        (void)inb(KBD_DATA_PORT); // Consume ACK
    }

    // Explicitly unmask IRQ1 on PIC (and keep slave masked)
    uint8_t master_mask = inb(0x21);
    master_mask &= ~(1 << 0); // Enable IRQ0 (timer) for diagnostics
    master_mask &= ~(1 << 1); // Enable IRQ1 (keyboard)
    outb(0x21, master_mask);
    outb(0xA1, 0xFF);

    // Optional: reset keyboard device and verify ACK/SELF-TEST
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");
    outb(KBD_CTRL_PORT, 0xD4); // Next byte goes to device
    if (!wait_input_buffer_clear())
        vga_print("[WARN ibuf] ");
    outb(KBD_DATA_PORT, 0xFF); // Reset
    if (wait_output_buffer_full())
    {
        uint8_t ack = inb(KBD_DATA_PORT);
        vga_print(" ACK:");
        print_hex_byte(ack);
        vga_print(" ");
        if (wait_output_buffer_full())
        {
            uint8_t self = inb(KBD_DATA_PORT);
            vga_print(" SELF:");
            print_hex_byte(self);
            vga_print(" ");
        }
    }

    // Read status register for visibility
    uint8_t status = inb(KBD_CTRL_PORT);
    vga_print(" STATUS:");
    print_hex_byte(status);
    vga_print(" \n");

    vga_print("Ready\n");
}

uint32_t keyboard_get_int_count()
{
    return kbd_irq_count;
}
