#include"ram.h"
#include"vga.h"
#include"../cpu/cpu.h"

#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/string.h"
#include "../utils/exit_strategy.h"

uint8_t *ram = 0;

//#define DEBUG

inline void CheckBoundary(uint32_t addr) {
    if (addr > 0xFFFFF) {
        printf("Access outside of 20 bit\n");
        exit(1);
    }
}

inline uint8_t Read8(uint32_t addr) {
    //CheckBoundary(addr);
    addr &= 0xFFFFF;
    //printf("Read8 at 0x%08x\n", addr);

    /* TODO
    if (addr == 0x3963D)
    {
        //if (segregs[regcs] == 0x3960) // ega1.gra
        printf("read abcd 0x%04x:0x%04x\n", segregs[regcs], ip);
        //file->data[0x3D] = 0x46;
    }
     */

/* TODO
    if (segregs[regcs] == 0x3960) // ega1.gra
    {
        // write to 0x1000
        if ((addr >= 0x10000) && (addr <= 0x20000)) {
        //if (ip == 0x10F4) {
            //printf("0x%04x\n", segregs[regds]);
            printf("abcdefgh 0x%04x read\n", addr);
            //exit_or_restart(1);
        }
    }
*/

#ifdef DEBUG
    if (addr >= 0x00400 && addr < 0x00500 && addr != 0x46C && addr != 0x449 && addr != 0x410 && addr != 0x411)
    {
        printf("BIOS data area 0x%08x\n", addr);
        exit_or_restart(1);
    }
    if (addr >= 0x0FF00 && addr < 0x10000)
    {
        printf("program segment prefix 0x%08x\n", addr);
        exit_or_restart(1);
    }

    if (addr >= 0x00100 && addr < 0x00200)
    {
        printf("dos info block 0x%08x\n", addr);
        exit_or_restart(1);
    }
#endif

    if (addr >= 0xa0000 && addr < 0xc0000) {
        //printf("ega read not implented 0x%06x\n", addr);
        //exit_or_restart(1);
        return VGA_read(addr - 0xa0000);
    }

    return ram[addr];
}

inline void Write8(uint32_t addr, uint8_t x) {
    //CheckBoundary(addr);
    addr &= 0xFFFFFu;
    // TODO
    /*
    if (segregs[regcs] == 0x3960) // ega1.gra
    {
        // write to 0x1000
        if ((addr >= 0x10000) && (addr <= 0x20000)) {
            //if (ip == 0x10F4) {
            //printf("0x%04x\n", segregs[regds]);
            printf("abcdefgh 0x%04x write\n", addr);
            //exit_or_restart(1);
        }
    }
    */

    if (addr >= 0xa0000 && addr < 0xc0000) VGA_write(addr - 0xa0000, x);
    ram[addr] = x;
}

inline void Write8Long(uint16_t s, uint16_t o, uint8_t x) {
    Write8(((uint32_t) s << 4u) + o, x);
}

inline void Write16(uint32_t addr, uint16_t x) {
    Write8(addr + 0, (x >> 0u) & 0xFF);
    Write8(addr + 1, (x >> 8u) & 0xFF);
}

inline void Write16Long(uint16_t s, uint16_t o, uint16_t x) {
    //Write16(((uint32_t) s << 4u) + o, x);
    Write8Long(s, o,   (x >> 0u) & 0xFFu);
    Write8Long(s,o+1, (x >> 8u) & 0xFFu);
}

inline void Write32(uint32_t addr, uint32_t x) {
    Write8(addr + 0, (x >>  0u) & 0xFF);
    Write8(addr + 1, (x >>  8u) & 0xFF);
    Write8(addr + 2, (x >> 16u) & 0xFF);
    Write8(addr + 3, (x >> 24u) & 0xFF);
}

inline void Write32Long(uint16_t s, uint16_t o, uint32_t x) {
    //Write16(((uint32_t) s << 4u) + o, x);
    Write8Long(s,o+0, (x >> 0u ) & 0xFFu);
    Write8Long(s,o+1, (x >> 8u ) & 0xFFu);
    Write8Long(s,o+2, (x >> 16u) & 0xFFu);
    Write8Long(s,o+3, (x >> 24u) & 0xFFu);
}


inline uint8_t Read8Long(uint16_t s, uint16_t o) {
    return Read8(((uint32_t) s << 4u) + o);
}

inline uint16_t Read16(uint32_t addr) {
    return Read8(addr + 0) | (Read8(addr + 1) << 8u);
}

inline uint32_t Read32(uint32_t addr) {
    return Read8(addr + 0) | (Read8(addr + 1) << 8u) | (Read8(addr + 2) << 16u)  | (Read8(addr + 3) << 24u);
}

inline uint16_t Read16Long(uint16_t s, uint16_t o) {
    //uint32_t addr = ((uint32_t) s << 4u) + o;
    //return Read8(addr + 0) | (Read8(addr + 1) << 8u);
    return Read8Long(s, o + 0) | (Read8Long(s, o + 1) << 8u);
}

inline uint32_t Read32Long(uint16_t s, uint16_t o) {
    //uint32_t addr = ((uint32_t) s << 4u) + o;
    //return Read8(addr + 0) | (Read8(addr + 1) << 8u);
    return Read8Long(s, o + 0) | (Read8Long(s, o + 1) << 8u) | (Read8Long(s, o + 2) << 16u) | (Read8Long(s, o + 3) << 24u);
}

void RAMInit() {
    if (ram == 0) {
        ram = malloc(1024 * 1024 + 65536);
    }
    memset(ram, 0, 1024 * 1024 + 65536);
}


