#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <stdint.h>

void keyboard_init();
uint32_t keyboard_get_int_count();
void keyboard_poll();

#endif
