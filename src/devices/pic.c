#include"pic.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../utils/exit_strategy.h"

// https://www4.cs.fau.de/Lehre/WS05/V_BS/oostubs/web/aufgaben/aufgabe2/pic_info.shtml
// http://stanislavs.org/helppc/8259.html

/*
 * IRQ0 -> INT 08h : clock
 * IRQ1 -> INT 09h: keyboard
 */

uint8_t PIC_triggered = 0;

static uint8_t irr; // interrupt request register
static uint8_t imr; // interrupt mask register
static uint8_t isr; // interrupt service register

uint8_t PIC_in(uint16_t portnum) {
    // not used yet
    return 0x0;
}

void PIC_out(uint16_t portnum, uint8_t value) {
    //printf("PIC port %i: %i\n", portnum, value);

    switch (portnum) {
        case 0: // TODO
            break;
        case 1: // TODO
            break;

        default:
            printf("unknown PIC port %i\n", portnum);
            exit_or_restart(1);
    }

}

void PIC_trigger(uint8_t irqnum) {
    irr |= (1 << irqnum);
    PIC_triggered = irr & (~imr);
}

uint8_t PIC_nextinterrupt() {
    uint8_t i, tmpirr;
    tmpirr = irr & (~imr);
    for (i = 0; i < 8; i++)
        if ((tmpirr >> i) & 1) {
            irr ^= (1 << i);
            isr |= (1 << i);
            PIC_triggered = irr & (~imr);
            return 8 + i;
        }
    exit_or_restart(1);
    return 0; // never reached
}

void PIC_Init() {
    PIC_triggered = 0;
    imr = 0x0; // enable all interrupts
    irr = 0x0;
    isr = 0x0;
}
