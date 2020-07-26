#include"keyb.h"
#include"pic.h"
#include"../cpu/cpu.h"

#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../utils/exit_strategy.h"

// The "8042" PS/2 Controller or its predecessors, dealing with keyboards and mice.
/*
Ports 60H-63H: PC 8255 Parallel    I/O Port Chip for Keyboard and Status
----------------------------------------------------------------------------
Ports 60H-64H: PS/2 Intel 8042 Keyboard/Auxiliary Device Controller
*/

static uint8_t lastscancode = 0x80; // last scancode was a keyup
static uint8_t shiftpressed;

void KeyDown(unsigned int scancode) {
    //printf("scancode %i\n", scancode);
    if (scancode == 0) return;
    lastscancode = scancode;
    if ((lastscancode == 0x2a) || (lastscancode == 0x36)) shiftpressed = 1;
    PIC_trigger(0x1);
}

void KeyUp(unsigned int scancode) {
    if (scancode == 0) return;
    if ((lastscancode == 0x2a) || (lastscancode == 0x36)) shiftpressed = 0;
    lastscancode = scancode | 0x80;
    PIC_trigger(0x1);
}

uint8_t Keyb_in(uint16_t portnum) {
    //printf("read keyb port %i: %i\n", portnum, lastscancode);
    switch (portnum) {

        case 0: // port 60h
            return lastscancode;

        case 1: // port 61h
            return 0x00;

        default:
            printf("Error Unknown in Keyboard port\n");
            exit_or_restart(1);
            return 0;
    }
}

void Keyb_out(uint16_t portnum, uint8_t value) {

    switch (portnum) {

        case 1: // port 61h
            //printf("keyb port 61h: 0x%02x\n", value);
            //exit_or_restart(1);
            break;

        default:
            printf("Error Unknown out Keyboard port\n");
            exit_or_restart(1);
            break;
    }
}

uint8_t getASCII(uint8_t scancode) {

    if (shiftpressed) {
        switch (lastscancode & 0x7F) {
            case 0x02:
                return '!'; // 1
            case 0x03:
                return '@'; // 2
            case 0x04:
                return '#'; // 3
            case 0x05:
                return '$'; // 4
            case 0x06:
                return '%'; // 5
            case 0x07:
                return '^'; // 6
            case 0x08:
                return '&'; // 7
            case 0x09:
                return '*'; // 8
            case 0x0a:
                return '('; // 9
            case 0x0b:
                return ')'; // 0
            case 0x1e:
                return 'A'; // a
            case 0x30:
                return 'B'; // b
            case 0x2e:
                return 'C'; // c
            case 0x20:
                return 'D'; // d
            case 0x12:
                return 'E'; // e
            case 0x21:
                return 'F'; // f
            case 0x22:
                return 'G'; // g
            case 0x23:
                return 'H'; // h
            case 0x17:
                return 'I'; // i
            case 0x24:
                return 'J'; // j
            case 0x25:
                return 'K'; // k
            case 0x26:
                return 'L'; // l
            case 0x32:
                return 'M'; // m
            case 0x31:
                return 'N'; // n
            case 0x18:
                return 'O'; // o
            case 0x19:
                return 'P'; // p
            case 0x10:
                return 'Q'; // q
            case 0x13:
                return 'R'; // r
            case 0x1f:
                return 'S'; // s
            case 0x14:
                return 'T'; // t
            case 0x16:
                return 'U'; // u
            case 0x2f:
                return 'V'; // v
            case 0x11:
                return 'W'; // w
            case 0x2d:
                return 'X'; // x
            case 0x2c:
                return 'Y'; // y
            case 0x15:
                return 'Z'; // z

            default:
                //printf("unknown scancode %i\n", scancode);
                //exit_or_restart(1);
                return 0;
        }
    } else {
        switch (lastscancode & 0x7F) {
            case 0x02:
                return '1'; // 1
            case 0x03:
                return '2'; // 2
            case 0x04:
                return '3'; // 3
            case 0x05:
                return '4'; // 4
            case 0x06:
                return '5'; // 5
            case 0x07:
                return '6'; // 6
            case 0x08:
                return '7'; // 7
            case 0x09:
                return '8'; // 8
            case 0x0a:
                return '9'; // 9
            case 0x0b:
                return '0'; // 0
            case 0x1e:
                return 'a'; // a
            case 0x30:
                return 'b'; // b
            case 0x2e:
                return 'c'; // c
            case 0x20:
                return 'd'; // d
            case 0x12:
                return 'e'; // e
            case 0x21:
                return 'f'; // f
            case 0x22:
                return 'g'; // g
            case 0x23:
                return 'h'; // h
            case 0x17:
                return 'i'; // i
            case 0x24:
                return 'j'; // j
            case 0x25:
                return 'k'; // k
            case 0x26:
                return 'l'; // l
            case 0x32:
                return 'm'; // m
            case 0x31:
                return 'n'; // n
            case 0x18:
                return 'o'; // o
            case 0x19:
                return 'p'; // p
            case 0x10:
                return 'q'; // q
            case 0x13:
                return 'r'; // r
            case 0x1f:
                return 's'; // s
            case 0x14:
                return 't'; // t
            case 0x16:
                return 'u'; // u
            case 0x2f:
                return 'v'; // v
            case 0x11:
                return 'w'; // w
            case 0x2d:
                return 'x'; // x
            case 0x2c:
                return 'y'; // y
            case 0x15:
                return 'z'; // z

            default:
                //printf("unknown scancode %i\n", scancode);
                //exit_or_restart(1);
                return 0;
        }
    }
}

void Keyboard_Bios() {
    uint16_t ah = regs.byteregs[regah];
    uint16_t al = regs.byteregs[regal];
    //printf("keyb ah: 0x%02x, al: 0x%02x\n", ah, al);

    switch (ah) {
        case 0:
            if (lastscancode == 0) {
                printf("Wait indefinitely for keyboard\n");
                exit_or_restart(1);
            }
            regs.byteregs[regah] = lastscancode & 0x7F;
            regs.byteregs[regal] = getASCII(lastscancode);
            lastscancode = 0x80;
            break;

        case 1: // get keyboard status
            // http://stanislavs.org/helppc/int_16-1.html
            //printf("get keyboard status\n");
            if ((lastscancode & 0x80) == 0) {
                ClearZFInInterrupt(); // key is pressed
                regs.byteregs[regah] = lastscancode & 0x7F;
                regs.byteregs[regal] = getASCII(lastscancode);
            } else {
                SetZFInInterrupt(); // no key is pressed
                regs.wordregs[regax] = 0;
            }
            break;

        default:
            printf("Error: Unknown keyboard function\n");
            exit_or_restart(1);
            break;

    }
}
