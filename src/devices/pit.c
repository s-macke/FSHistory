#include"pic.h"
#include"pit.h"

#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../utils/exit_strategy.h"

// https://wiki.osdev.org/Programmable_Interval_Timer
// https://wiki.osdev.org/PIT

/*
runs at 1.193182 MHz

I/O port     Usage
0x40         Channel 0 data port (read/write), timer
0x41         Channel 1 data port (read/write), DRAM refresh
0x42         Channel 2 data port (read/write), PC Speaker
0x43         Mode/Command register (write only, a read is ignored)
*/

uint8_t io_hi_lo = 0;
int32_t counter0;
int32_t timerdata0;

void PIT_count(int32_t instructions) {
    if (io_hi_lo == 1) return;

    //counter0 -= instructions / 3; // approx 6 Million Instructions per second
    counter0 -= instructions / 2;
    //counter0 -= instructions / 10;
    if (counter0 < 0) {
        //printf("trigger PIT\n");
        PIC_trigger(0x0);
        counter0 += timerdata0;
    }
}

uint8_t PIT_in(uint16_t portnum) {
    //printf("PIT in\n");

    switch (portnum) {
        case 0:
            if (io_hi_lo == 0) {
                io_hi_lo = 1;
                return counter0 & 0xFF;
            } else {
                io_hi_lo = 0;
                return counter0 >> 8;
            }
        default:
            printf("unknown PIT port %i\n", portnum);
            exit_or_restart(1);
    }
    exit_or_restart(1);
    return 0x0;
}

void PIT_out(uint16_t portnum, uint8_t value) {

    //printf("PIT %i 0x%02x\n", portnum, value);

    switch (portnum) {
        case 0:
            if (io_hi_lo == 0) {
                timerdata0 = (timerdata0 & 0xFF00) | value;
                io_hi_lo = 1;
            } else {
                timerdata0 = (timerdata0 & 0x00FF) | (((uint16_t) value) << 8);
                counter0 = timerdata0;
                io_hi_lo = 0;
                //printf("Timer %i\n", timerdata0);
            }
            break;

        case 2:
            // PC speaker
            break;

        case 3: // mode register TODO
            //printf("set PIT mode for channel: %i\n", (value>>6) & 3);
            break;


        default:
            printf("unknown PIT port %i\n", portnum);
            exit_or_restart(1);
    }

}

void PIT_Init() {
    io_hi_lo = 0;
    timerdata0 = 100000;
    counter0 = timerdata0;
}