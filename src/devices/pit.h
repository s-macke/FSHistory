#ifndef PIT_H
#define PIT_H

// Intel 8253 prgrammable interval timer

#include "../wasm_libc_wrapper/stdint.h"

void PIT_Init();

void PIT_count(int32_t instructions);
uint8_t PIT_in(uint16_t portnum);
void PIT_out(uint16_t portnum, uint8_t value);

#endif
