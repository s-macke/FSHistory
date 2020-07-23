#ifndef RAM_H
#define RAM_H

#include "../wasm_libc_wrapper/stdint.h"

extern uint8_t *ram;

#define segbase(x)                   (((uint32_t) (x)) << 4u)

#ifdef UNSAFE_RAM

#define Read8LongUnsafe(x, y)     (ram[segbase(x) + (y)])
//#define Read16LongUnsafe(x, y)    ((int16_t)ram[segbase(x) + (y)] | ((int16_t)ram[segbase(x) + (y)+1]<<8))
#define Read16LongUnsafe(x, y)    (*(uint16_t*)&ram[segbase(x) + (y)])
#define Read32LongUnsafe(x, y)    (*(uint32_t*)&ram[segbase(x) + (y)])
#define Write8LongUnsafe(x, y)    Write8Long(x, y)
#define Write16LongUnsafe(x, y)   Write16Long(x, y)
#define Read8Unsafe(x)            (ram[x])
//#define Read16Unsafe(x, y)        Read16(x, y)
#define Read16Unsafe(x)           (*(int16_t*)&ram[x])
#define Write8Unsafe(x)           Write8(x)
#define Write16Unsafe(x)          Write16(x)

#else

#define Read8LongUnsafe(x, y)     Read8Long(x, y)
#define Read16LongUnsafe(x, y)    Read16Long(x, y)
#define Write8LongUnsafe(x, y)    Write8Long(x, y)
#define Write16LongUnsafe(x, y)   Write16Long(x, y)
#define Read8Unsafe(x, y)         Read8(x, y)
#define Read16Unsafe(x, y)        Read16(x, y)
#define Write8Unsafe(x, y)        Write8(x, y)
#define Write16Unsafe(x, y)       Write16(x, y)

#endif


void Write8(uint32_t addr, uint8_t x);
void Write8Long(uint16_t s, uint16_t o, uint8_t x);

void Write16(uint32_t addr, uint16_t x);
void Write16Long(uint16_t s, uint16_t o, uint16_t x);

void Write32(uint32_t addr, uint32_t x);
void Write32Long(uint16_t s, uint16_t o, uint32_t x);

uint8_t Read8(uint32_t addr);
uint8_t Read8Long(uint16_t s, uint16_t o);

uint32_t Read32(uint32_t addr);
uint32_t Read32Long(uint16_t s, uint16_t o);

uint16_t Read16(uint32_t addr);
uint16_t Read16Long(uint16_t s, uint16_t o);
void RAMInit();

#endif
