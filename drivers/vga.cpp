#include "drivers/vga.hpp"

#define VGA_ADDRESS 0xB8000
#define VGA_COLS 80
#define VGA_ROWS 25

static uint8_t col = 0;
static uint8_t row = 0;
static uint16_t *vga_buffer = (uint16_t *)VGA_ADDRESS;

void vga_init()
{
    vga_clear();
}

void vga_clear()
{
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
    {
        vga_buffer[i] = 0x0F00 | ' ';
    }
    col = 0;
    row = 0;
}

void vga_putchar(char c)
{
    if (c == '\n')
    {
        row++;
        col = 0;
        if (row >= VGA_ROWS)
        {
            row = 0;
            vga_clear();
        }
        return;
    }

    uint16_t index = row * VGA_COLS + col;
    vga_buffer[index] = 0x0F00 | c;

    col++;
    if (col >= VGA_COLS)
    {
        col = 0;
        row++;
        if (row >= VGA_ROWS)
        {
            row = 0;
            vga_clear();
        }
    }
}

void vga_print(const char *str)
{
    while (*str)
    {
        vga_putchar(*str++);
    }
}
