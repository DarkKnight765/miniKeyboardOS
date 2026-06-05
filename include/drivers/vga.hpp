#ifndef VGA_HPP
#define VGA_HPP

#include <stdint.h>

void vga_init();
void vga_clear();
void vga_putchar(char c);
void vga_print(const char *str);

#endif
