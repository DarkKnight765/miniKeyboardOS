global isr_keyboard
global isr_timer

extern keyboard_isr
extern timer_isr

isr_keyboard:
    pusha
    call keyboard_isr
    popa
    iret

isr_timer:
    pusha
    call timer_isr
    popa
    iret
