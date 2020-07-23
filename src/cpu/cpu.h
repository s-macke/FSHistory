#ifndef CPU_H
#define CPU_H
/*
 * This emulator is inspired from Fake86: A portable, open-source 8086 PC emulator from Mike Chambers.
 * This emulator Supports partial 32-Bit.
 */

#include "../wasm_libc_wrapper/stdint.h"

#define regeax 0
#define regecx 1
#define regedx 2
#define regebx 3
#define regesp 4
#define regebp 5
#define regesi 6
#define regedi 7

#define regax 0
#define regcx 2
#define regdx 4
#define regbx 6
#define regsp 8
#define regbp 10
#define regsi 12
#define regdi 14

#define reges 0
#define regcs 1
#define regss 2
#define regds 3
#define regfs 4
#define reggs 5

#define regal 0
#define regah 1
#define regcl 4
#define regch 5
#define regdl 8
#define regdh 9
#define regbl 12
#define regbh 13

union regstruct {
    uint32_t dwordregs[8];
    uint16_t wordregs[16];
    uint8_t byteregs[16];
};

extern union regstruct regs;
extern uint16_t segregs[6];
extern uint16_t ip;

void reset86();
void exec86(uint32_t execloops);

void intcall86(uint8_t intnum);

void SetHLTstate();
void ClearHLTstate();
void PrintStatus();

void disasmout(unsigned seg, unsigned ofs, int count);

void setcsip(uint16_t _cs, uint16_t _ip);
void setsssp(uint16_t _ss, uint16_t _sp);
void getcsip(uint16_t *_cs, uint16_t *_ip);
uint32_t getflags();
void setflags(uint32_t x);

void SetCFInInterrupt();
void ClearCFInInterrupt();
void SetZFInInterrupt();
void ClearZFInInterrupt();

#endif
