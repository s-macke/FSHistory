#ifndef KEYB_H
#define KEYB_H

#include "../wasm_libc_wrapper/stdint.h"

void KeyDown(unsigned int scancode);
void KeyUp(unsigned int scancode);

uint8_t Keyb_in(uint16_t portnum);
void Keyb_out(uint16_t portnum, uint8_t value);

void Keyboard_Bios();

#endif
