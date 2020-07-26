#include"bios.h"
#include"ram.h"
#include"../cpu/cpu.h"

#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../utils/exit_strategy.h"

void HandleBios() {
    uint16_t ah = regs.byteregs[regah];
    uint16_t al = regs.byteregs[regal];
    //printf("bios ah: 0x%02x, al: 0x%02x\n", ah, al);

    switch (ah) {
        case 0xc0: // return system configuration parameters (PS/2 only)
            printf("bios return system configuration parameters\n");
            ClearCFInInterrupt();
            segregs[reges] = 0xc8c2; // compatibility with dosbox
            regs.wordregs[regbx] = 0x0;
            regs.byteregs[regah] = 0x0;

            //http://stanislavs.org/helppc/int_15-c0.html
            Write8Long(segregs[reges], 0, 0x08); // length
            Write8Long(segregs[reges], 1, 0x00);
            Write8Long(segregs[reges], 2, 0xfc); // model byte
            Write8Long(segregs[reges], 3, 0x00); // secondary model byte
            Write8Long(segregs[reges], 4, 0x01); // BIOS revision level
            Write8Long(segregs[reges], 5, 0x70); // feature information
            Write8Long(segregs[reges], 6, 0x40); // reserved
            Write8Long(segregs[reges], 7, 0x00); // reserved
            Write8Long(segregs[reges], 8, 0x00); // reserved
            Write8Long(segregs[reges], 9, 0x00); // reserved
            ClearCFInInterrupt();
            break;

        default:
            printf("Error: Unknown bios function\n");
            exit_or_restart(1);
            break;
    }

}