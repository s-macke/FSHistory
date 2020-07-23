#ifndef PIC_H
#define PIC_H

// Intel 8259 interrupt controller

#include "../wasm_libc_wrapper/stdint.h"

extern uint8_t PIC_triggered;
uint8_t PIC_nextinterrupt();
void PIC_Init();

uint8_t PIC_in(uint16_t portnum); // not used yet
void PIC_out(uint16_t portnum, uint8_t value); // not used yet

void PIC_trigger(uint8_t irqnum);

#endif
