#ifndef IDT_HPP
#define IDT_HPP

#include <stdint.h>

struct IDTDescriptor
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IDTPointer
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern "C" void idt_load(IDTPointer *ptr);

void idt_init();
void idt_set_descriptor(uint8_t vector, void (*isr)(), uint8_t flags);

#endif
