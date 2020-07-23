#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/string.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../utils/exit_strategy.h"

#include "../cpu/cpu.h"
#include "../devices/ram.h"

#include"ems.h"

// https://stanislavs.org/helppc/int_67.html

#define PAGE_SIZE 0x4000
const static uint32_t base = 0xe0000;

const static int ntotalpages = 0x0400; // total number of pages, 16 MB
static int nfreepages = 0x3b4;

typedef struct {
    uint32_t size; // in pages
    uint8_t* data;
} EMSMap;

static EMSMap map[256];
static int nexthandle;

static int currenthandle[4];
static int currentpage[4];

void EMSInterrupt() {
    uint16_t ah = regs.byteregs[regah];
    //printf("EMS: ah: 0x%02x\n", ah);

    switch (ah) {

        case 0x40: // get emm status
            printf("EMS: get status\n");
            regs.byteregs[regah] = 0x00;
            break;

        case 0x41: // get page frame base address
            printf("EMS: get base address\n");
            regs.byteregs[regah] = 0x00;
            regs.wordregs[regbx] = base >> 4;
            break;

        case 0x42: // get page counts
            printf("EMS: get page counts\n");
            regs.byteregs[regah] = 0x00;
            regs.wordregs[regbx] = nfreepages;
            regs.wordregs[regdx] = ntotalpages; // total number of pages, 16 MB
            break;

        case 0x43: // Get Handle and Allocate Pages
            printf("EMS: get handle and allocate pages=%i\n", regs.wordregs[regbx]);
            regs.byteregs[regah] = 0x00;
            regs.wordregs[regdx] = nexthandle; // handle
            map[nexthandle].data = malloc(PAGE_SIZE*regs.wordregs[regbx]);
            map[nexthandle].size = regs.wordregs[regbx];
            nfreepages -= regs.wordregs[regbx];
            nexthandle++;
            break;

        case 0x44: // map logical page into physical page window
        {
            int physical_page = regs.byteregs[regal];
            printf("EMS: map logical page into physical page window handle=%i, physical=%i logical=%i\n", regs.wordregs[regdx], physical_page, regs.wordregs[regbx]);
            uint8_t *physical_page_ptr = ram + base + physical_page*PAGE_SIZE;

            if (currenthandle[physical_page] != -1) {
                memcpy(map[currenthandle[physical_page]].data + PAGE_SIZE*currentpage[physical_page], physical_page_ptr, PAGE_SIZE);
            }
            currenthandle[physical_page] = regs.wordregs[regdx];
            currentpage[physical_page] = regs.wordregs[regbx];
            memcpy(physical_page_ptr, map[currenthandle[physical_page]].data + PAGE_SIZE*currentpage[physical_page], PAGE_SIZE);
            regs.byteregs[regah] = 0x00;
            break;
        }

        case 0x4f: // get set partial page map
            printf("EMS: get/set partial map al=%i\n", regs.byteregs[regal]);
            //regs.byteregs[regah] = 0x84; // undefined function 0x91 feature not supported
            //break;

            switch(regs.byteregs[regal]) {

                case 2: // get size of partial page map
                    regs.byteregs[regah] = 0x00;
                    regs.byteregs[regal] = 0x1a;// size of partial page map
                    regs.wordregs[regbx] = 4; // number of mappable segments in the partial map to be saved
                    break;

                default:
                    printf("EMS: Unknown sub function al=0x%02x\n", regs.byteregs[regal]);
                    exit_or_restart(1);
                    break;
            }
            break;


        default:
            printf("EMS: Unknown function ah=%02x\n", ah);
            PrintStatus();
            exit_or_restart(1);
            break;
    }
}

void EMSInit() {
    nexthandle = 1;
    currenthandle[0] = -1;
    currenthandle[1] = -1;
    currenthandle[2] = -1;
    currenthandle[3] = -1;
    currentpage[0] = -1;
    currentpage[1] = -1;
    currentpage[2] = -1;
    currentpage[3] = -1;
    memset(map, 0, sizeof(map));
}