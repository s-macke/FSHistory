#include "../wasm_libc_wrapper/stdint.h"
#include "../utils/exit_strategy.h"
#include "../wasm_libc_wrapper/stdio.h"

#include "../cpu/cpu.h"

void DOS_Multiplex_Int()
{
    uint16_t ah = regs.byteregs[regah];
    uint16_t al = regs.byteregs[regal];
    //printf("DOS: ah: 0x%02x, al: 0x%02x\n", ah, al);

    switch (al) {
        case 0x0: // get installed state
            regs.byteregs[regal] = 0x0; // nothing installed
            break;

        default:
            printf("DOS Multiplex: Unknown function ah=%02x al=%02x\n", ah, al);
            exit_or_restart(1);
            break;
    }
}