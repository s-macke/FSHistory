#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../utils/exit_strategy.h"

#include "alloc.h"
/*
struct block_info {
    uint32_t magic;
    struct block_info *next; // 32-bit
    uint16_t size; // in blocks of 16 bytes
};
*/
uint16_t nextfreeseg = 0x1000;
uint16_t lastallocseg = 0x0;

uint16_t AvailableRam() {
    return 0x9FFE - nextfreeseg;
}


uint16_t Allocate(uint16_t paragraphs) {
    lastallocseg = nextfreeseg;
    nextfreeseg += paragraphs;
    if (nextfreeseg >= 0xA000) {
        printf("DOS Alloc: Error: Out of memory\n");
        exit_or_restart(1);
    }
    return lastallocseg;
}

void Free(uint16_t seg) {
}

bool Modify(uint16_t seg, uint16_t paragraphs) {

    if ((lastallocseg) != seg) {
        printf("DOS Alloc: Cannot modify allocated memory block\n");
        //exit_or_restart(1);
        return false;
    }
    printf("DOS Alloc: currently alloated: %i segments,  new %i segments\n", (nextfreeseg - lastallocseg), paragraphs);

    nextfreeseg = seg + paragraphs;
    return true;
}

void AllocInit()
{
    //nextfreeseg = 0x1000;
    //nextfreeseg = 0x020E; // dosbox start sector for executable
    nextfreeseg = 0x01A2; // dosbox start sector for executable
    lastallocseg = 0x0;
}
