#include"disk.h"
#include"../cpu/cpu.h"
#include"../fs/fs.h"
#include"../devices/ram.h"

#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/string.h"
#include "../utils/exit_strategy.h"

static FILEFS *file = NULL;

// disk bios functions
// https://stanislavs.org/helppc/int_13.html
void Disk_Bios() {
    uint16_t ah = regs.byteregs[regah];
    switch (ah) {
        case 0x0: // reset disk system
            regs.byteregs[regah] = 0x00; // no error
            ClearCFInInterrupt();
            break;


        case 0x2: // read disk sectors
        {
            //PrintStatus();
            uint16_t nsectors = regs.byteregs[regal];
            uint16_t track = regs.byteregs[regch]; // TODO actually a 10 bit value together with cl
            uint16_t sector = regs.byteregs[regcl] & 31;
            uint8_t head = regs.byteregs[regdh];
            uint8_t dl = regs.byteregs[regdl];
            uint16_t es = segregs[reges];
            uint16_t bx = regs.wordregs[regbx];
            //printf("read disk sectors disk=%i c/h/s=%i/%i/%i n_sectors=%i to es:bx=0x%04x:0x%04x\n", dl, track, head, sector, nsectors, es, bx);

            if (file == NULL) {
                printf("No file specified\n");
                exit_or_restart(1);
            }
            if (dl != 0) {
                printf("unknown drive number %i\n", dl);
                exit_or_restart(1);
            }
            if (head != 0) {
                printf("unknown head number %i\n", head);
                exit_or_restart(1);
            }

            int32_t lba = 0;
            if (file->size == 163840) {
                // for 160kB disks, 1 side 40, tracks per side, 8 sectors
                lba = track * 8 + (sector - 1);
            } else { // 1.44 MB
                lba = (track*2 + head) * 18 + (sector - 1);
            }
            for(int i=0; i < nsectors*512; i++) {
                Write8((es<<4) + bx + i, file->data[lba*512+i]);
            }
            regs.byteregs[regah] = 0x00; // no error, https://stanislavs.org/helppc/int_13-1.html
            regs.byteregs[regal] = nsectors;
            ClearCFInInterrupt();
            //exit(1);
            break;
        }

        case 0x3: // write disk sectors
            regs.byteregs[regah] = 0x00; // no error
            ClearCFInInterrupt();
            break;

        case 0x5: // format disk track
            regs.byteregs[regah] = 0x00; // no error
            ClearCFInInterrupt();
            break;

        default:
            printf("Error: Unknown disk bios function ah=0x%02x\n", ah);
            exit_or_restart(1);
            break;

    }
}

void RunImage(char* filename) {
    uint16_t index = 0;
    file = FindFile(filename, &index);
    if (file == NULL) {
        printf("Error: Cannot open file '%s'\n", filename);
        exit(1);
    }
    printf("open '%s' with size: %i\n", file->filename, file->size);

    int32_t seg = 0x7c0;

    regs.byteregs[regdl] = 0x0; // booting from drive 0
    memcpy(&ram[seg<<4], file->data, 512); // load c/h/s = 0/0/1
    setcsip(seg, 0x0);
    setsssp(seg, 512 + 512); //512 bytes beyond the end of the boot sector
    segregs[reges] = seg;
}
