#include "ports.h"
#include "keyb.h"

#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../utils/exit_strategy.h"

#include "pic.h"
#include "pit.h"
#include "vga.h"
#include "../cpu/cpu.h"

//define DEBUG

// https://wiki.osdev.org/I/O_Ports
// http://bochs.sourceforge.net/techspec/PORTS.LST

void portout(uint16_t portnum, uint8_t value) {
#ifdef DEBUG
    printf("portout8 0x%04x: 0x%02x\n", portnum, value);
#endif
    switch (portnum) {
        case 0x08: // DMA
        case 0x0b: // DMA
            break;

        case 0x20: // PIC
        case 0x21: // PIC
            PIC_out(portnum - 0x20, value);
            break;

        case 0x40: // PIT
        case 0x42: // PIT
        case 0x43: // PIT
            PIT_out(portnum - 0x40, value);
            break;

        case 0x60:
        case 0x61:
        case 0x62: // The "8042" PS/2 Controller or its predecessors, dealing with keyboards and mice.
            Keyb_out(portnum - 0x60, value);
            break;

        case 0x3b4: // vga
        case 0x3b5:
        case 0x3b8:
        case 0x3c0:
        case 0x3c4:
        case 0x3c5:
        case 0x3c8:
        case 0x3c9:
        case 0x3ce:
        case 0x3cf:
        case 0x3d0:
        case 0x3d4:
        case 0x3d5:
        case 0x3d8:
        case 0x3d9:
            VGA_out(portnum, value);
            break;

        case 0x3bd: // Primary Parallel Printer Adapter
        case 0x3bf:
            break;

        case 0x3f2: // Floppy disk controller
            break;

        default:
            printf("Error: portout8 unknown port 0x%04x\n", portnum);
            PrintStatus();
            exit_or_restart(1);
            break;
    }
}

void portout16(uint16_t portnum, uint16_t value) {
#ifdef DEBUG
    printf("portout16 0x%04x: 0x%04x\n", portnum, value);
#endif
    switch (portnum) {

        case 0x3c4:
        case 0x3ce:
        case 0x3cf:
        case 0x3d4:
            VGA_out16(portnum, value);
            break;
/*
        case 0x6:
        case 0x4cf:
            printf("Warning: portout16 unknown port 0x%04x\n", portnum);
            break;
*/
        default:
            printf("Error: portout16 unknown port 0x%04x\n", portnum);
            PrintStatus();
            exit_or_restart(1);
            break;
    }
}

uint8_t portin(uint16_t portnum) {
#ifdef DEBUG
    printf("portin 0x%04x\n", portnum);
#endif
    switch (portnum) {
        case 0x40: // PIT
            return PIT_in(portnum - 0x40);

        case 0x60:
        case 0x61:
        case 0x62: // The "8042" PS/2 Controller or its predecessors, dealing with keyboards and mice.
            return Keyb_in(portnum - 0x60);

        case 0x8a: // 8255 I/O programmable peripheral interface
            return 0;

        case 0x3ba: // monochrome status register
        case 0x3da: // color status register
        case 0x3c7:
            return VGA_in(portnum);

        default:
            printf("Error: portin unknown port 0x%04x\n", portnum);
            PrintStatus();
            exit_or_restart(1);
            return 0;
    }
}

uint16_t portin16(uint16_t portnum) {
#ifdef DEBUG
    printf("portin16: 0x%04x\n", portnum);
#endif
    exit_or_restart(1);
    return 0;
}
