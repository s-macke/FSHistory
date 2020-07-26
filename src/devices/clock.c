#include"clock.h"
#include"../cpu/cpu.h"

#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../utils/exit_strategy.h"

// System and Real Time Clock BIOS Services
// http://stanislavs.org/helppc/int_1a.html

void Clock_Bios() {
    uint16_t ah = regs.byteregs[regah];
    uint16_t al = regs.byteregs[regal];

    switch (ah) {

        case 0x0: // read system clock counter
            regs.byteregs[regal] = 0;
            regs.wordregs[regcx] = 0; // tick counter TODO
            regs.wordregs[regdx] = 0;
            break;

        case 0x2: // read real time clock time
            regs.byteregs[regch] = 1;   // hours in BCD
            regs.byteregs[regcl] = 1;   // minutes in BCD
            regs.byteregs[regdh] = 1;   // seconds in BCD
            regs.byteregs[regdl] = 1;   // daylight savings
            ClearZFInInterrupt();
            break;

        case 0x04: // read real time clock date
            regs.byteregs[regch] = (2<<4) | 0;  // century in BCD
            regs.byteregs[regcl] = (2<<4) | 0;  // year in BCD
            regs.byteregs[regdh] = 1;   // month in BCD
            regs.byteregs[regdl] = 1;   // day in BCD
            ClearZFInInterrupt();
            break;

        default:
            printf("Error: Unknown clock function ah=0x%02x\n", ah);
            exit_or_restart(1);
            break;

    }
}