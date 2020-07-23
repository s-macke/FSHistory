#ifndef ROM_C
#define ROM_C

#include "../wasm_libc_wrapper/stdint.h"

void ROMExec(uint8_t intno, uint32_t addr);
void ROMInit();

#endif
