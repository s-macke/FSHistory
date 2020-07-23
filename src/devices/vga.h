#ifndef VGA_H
#define VGA_H

#include "../wasm_libc_wrapper/stdint.h"

//extern uint16_t VGA_SC[0x100], VGA_CRTC[0x100], VGA_ATTR[0x100], VGA_GC[0x100];

void VGA_Bios();
void VGA_Vesa_Window_Function();

uint8_t VGA_in(uint16_t portnum);
void VGA_out(uint16_t portnum, uint8_t value);
void VGA_out16(uint16_t portnum, uint16_t value);

void VGA_write(uint32_t addr32, uint8_t value);
uint8_t VGA_read(uint32_t addr32);

void VGA_Draw();

void VGA_Init();

#endif
