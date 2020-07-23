#ifndef PORTS_H
#define PORTS_H

#include "../wasm_libc_wrapper/stdint.h"

void portout (uint16_t portnum, uint8_t value);
void portout16 (uint16_t portnum, uint16_t value);
uint8_t	portin (uint16_t portnum);
uint16_t portin16 (uint16_t portnum);

#endif
