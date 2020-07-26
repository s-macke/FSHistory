#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/string.h"
#include "../utils/exit_strategy.h"

#include "../cpu/cpu.h"
#include "mouse.h"

// http://stanislavs.org/helppc/int_33.html

static uint16_t x = 0, y = 0, buttons = 0;


void MouseButtonDown(uint8_t button) {
//printf("mouse button %i\n", button);
    switch (button) {
        case 1:
            buttons |= 1;
            break;
        case 3:
            buttons |= 2;
            break;
    }

}

void MouseButtonUp(uint8_t button) {
    switch (button) {
        case 1:
            buttons &= ~1;
            break;
        case 3:
            buttons &= ~2;
            break;
    }
}

void MouseMotion(int _x, int _y) {
    x = _x;
    y = _y;
}

void MouseInterrupt() {
    uint8_t ax = regs.wordregs[regax];
    //printf("Mouse: ax: 0x%04x\n", ax);

    switch (ax) {

        case 0x0: // mouse reset/get mouse installed flag
            //regs.wordregs[regax] = 0x0; // not installed
            regs.wordregs[regax] = 0xFFFF; // installed
            regs.wordregs[regbx] = 3; // 2 buttons
            break;

        case 0x3: // get mouse position
            //printf("get mouse position\n");
            regs.wordregs[regcx] = x; // x
            regs.wordregs[regdx] = y; // y
            regs.wordregs[regbx] = buttons; // button status
            break;

        case 0x4: // set mouse position
            break;

        case 0x7: // Set Mouse Horizontal Min/Max Position
            break;

        case 0x8: // Set Mouse Vertical Min/Max Position
            break;

        case 0xc: // Set Mouse user defined subroutine and input mask
            // TODO
            break;

        case 0xf: // Set Mouse Mickey Pixel Ratio
            break;

        case 0x1c: // Set Mouse interrupt rate
            break;


        default:
            printf("Error: Unknown Mouse Bios function: 0x%04x\n", ax);
            exit_or_restart(1);
            break;

    }

}

