#include"rom.h"

#include"../dos/dos.h"
#include"../dos/multiplex.h"
#include"vga.h"
#include"keyb.h"
#include"mouse.h"
#include"bios.h"
#include"clock.h"
#include"ems.h"
#include"ram.h"
#include"rom.h"
#include"disk.h"
#include"../cpu/cpu.h"

#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/string.h"
#include "../utils/exit_strategy.h"

// execute rom and dos functions
// http://stanislavs.org/helppc/idx_interrupt.html

void ROMExec(uint8_t intno, uint32_t addr) {

    //if ((intno != 0x8) && (intno != 0x1C)) printf("intno: 0x%02x, ah: 0x%02x, al: 0x%02x\n", intno, regs.byteregs[regah], regs.byteregs[regal]);

    switch(intno) {
        //case 0x0: // division by 0
        // do nothing
        //	break;

        case 0x4: // COM1
            break;

        case 0x8: // ROM timer interrupt handler
            // write to BIOS data area
            Write16Long(0x40, 0x6c, Read16Long(0x40, 0x6c) + 1);
            //exit_or_restart(1);
            break;

        case 0x9: // ROM keyboard interrupt handler
            break;

        case 0x10:
            VGA_Bios();
            break;

        case 0x11: // BIOS Equipment Determination
            regs.wordregs[regax] = 0x0;
            break;

        case 0x13:
            Disk_Bios();
            break;

        case 0x15:
            HandleBios();
            break;

        case 0x16:
            Keyboard_Bios();
            break;

        case 0x1A:
            Clock_Bios();
            break;

        case 0x1C:
            // User timer interrupt. Ignore
            break;

        case 0x20:
            printf("program termination\n");
            exit_or_restart(0);
            break;

        case 0x21:
            HandleDosInterrupt();
            break;

        case 0x2f:
            DOS_Multiplex_Int();
            break;

        case 0x33:
            MouseInterrupt();
            break;

        case 0x67:
            EMSInterrupt();
            break;

        case 0x81:
            // call to vesa function to change the window. Used only by this emulator
            VGA_Vesa_Window_Function();
            break;

        default:
            printf("unknown interrupt 0x%02x\n", intno);
            PrintStatus();
            exit_or_restart(1);
            break;
    }
}

void ROMInit() {
    // use RAM as ROM, this is easier

    // pointer to rom for every interrupt
    for (int i = 0; i < 256; i++) {
        ram[(i << 2) + 0] = (i*4) & 0xFF;
        ram[(i << 2) + 1] = (i*4) >> 8;
        ram[(i << 2) + 2] = 0x00;
        ram[(i << 2) + 3] = 0xF0;

        ram[0xF0000 + 4*i+0] = 0x63; // callback
        ram[0xF0000 + 4*i+1] = i; // callback
        ram[0xF0000 + 4*i+2] = 0xCF; // iret
        ram[0xF0000 + 4*i+3] = 0x00;
    }

    // special case for timer interrupt
    ram[(0x8 << 2) + 0] = 0x00;
    ram[(0x8 << 2) + 1] = 0x20;
    ram[0xF0000 + 0x2000] = 0x63; // callback
    ram[0xF0000 + 0x2001] = 0x08; //
    ram[0xF0000 + 0x2002] = 0x50; // push ax
    ram[0xF0000 + 0x2003] = 0x52; // push dx
    ram[0xF0000 + 0x2004] = 0x1e; // push ds
    ram[0xF0000 + 0x2005] = 0xcd; // int
    ram[0xF0000 + 0x2006] = 0x1c; // user timer interrupt empty routine executed 18.2 times per second
    ram[0xF0000 + 0x2007] = 0xfa; // cli
    ram[0xF0000 + 0x2008] = 0x1f; // pop ds
    ram[0xF0000 + 0x2009] = 0x5a; // pop dx
    ram[0xF0000 + 0x200a] = 0x58; // pop ax // todo before pop ax, why? mov al,20; out 20,al;
    ram[0xF0000 + 0x200b] = 0xcf; // iret

    // bios data area
    // http://stanislavs.org/helppc/bios_data_area.html
    memset(&ram[0x400], 0x00, 256);
    ram[0x400 + 0x10] = 0x26; // equipment list
    ram[0x400 + 0x11] = 0xd4;

    ram[0x400 + 0x65] = 0x2e; //  6845 CRT mode control register value (port 3x8h)
}
