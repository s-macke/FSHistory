#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/string.h"
#include "../wasm_libc_wrapper/stdbool.h"
#include "../wasm_libc_wrapper/ctype.h"
#include "../wasm_libc_wrapper/stddef.h"
#include "../utils/exit_strategy.h"

#include "../cpu/cpu.h"
#include "../devices/ram.h"
#include "../fs/fs.h"
#include "../fs4.h"
#include "alloc.h"

// I know, this is technically no device.

// https://www.i8086.de/dos-int-21h/dos-int-21h.html
// http://stanislavs.org/helppc/
// http://www.oldlinux.org/Linux.old/docs/interrupts/int-html/int.htm
// http://www2.ift.ulaval.ca/~marchand/ift17583/dosints.pdf
// https://de.wikibooks.org/wiki/Interrupts_80x86

/*
character device
14    device driver can process IOCTL requests (see AX=4402h"DOS 2+")
13    output until busy supported
11    driver supports OPEN/CLOSE calls
8    ??? (set by MS-DOS 6.2x KEYB)
7    set (indicates device)
6    EOF on input
5    raw (binary) mode
4    device is special (uses INT 29)
3    clock device
2    NUL device
1    standard output
0    standard input
disk file
15    file is remote (DOS 3.0+)
14    don't set file date/time on closing (DOS 3.0+)
11    media not removable
8    (DOS 4 only) generate INT 24 if no disk space on write or read past
end of file
7    clear (indicates file)
6    file has not been written
5-0   drive number (0 = A:)
*/

static int dibseg = 0x80; // dos info block segment

typedef struct {
    bool isactive;
    bool isfile;
    FILEFS *file;
    uint32_t offset;
    char filename[64];
    uint16_t device_information;
} HANDLE;
static HANDLE handle[256];

static uint16_t dtaseg = 0x1000 - 8; // should be the last 128 byte of the psp structure
static uint16_t dtaofs = 0x0;

// ------------------------------------

static uint16_t pspseg = 0x1000-16; // program segment prefix

void HandleIOCTL() {
    uint16_t al = regs.byteregs[regal];
    uint32_t h = regs.wordregs[regbx];
    printf("DOS: IOCTL handle: 0x%04x\n", h);

    switch (al) {

        case 0: // get device information
            // http://stanislavs.org/helppc/int_21-44-0.html
            if (handle[h].isactive) {
                regs.wordregs[regax] = handle[h].device_information; // error code, but no error. dosbox fill this register
                regs.wordregs[regdx] = handle[h].device_information;
                ClearCFInInterrupt();
            } else {
                SetCFInInterrupt();
                regs.wordregs[regax] = 0x6;
                printf("Unknown IOCTL %i\n", h);
                //exit_or_restart(1);
            }
            break;

        case 7: // get output status, currently only used for emm device
            // https://stanislavs.org/helppc/int_21-44-7.html
            ClearCFInInterrupt();
            regs.byteregs[regal] = 0xff; // ready
            break;

        default:
            printf("DOS: Unknown DOS IOCTL function 0x%02x\n", al);
            exit_or_restart(1);
            break;
    }
}


// ------------------------------------

void ReadFilename(char *filename) {
    filename[12] = 0;
    printf("'");
    for (int i = 0; i < 30; i++) {
        char c = Read8Long(segregs[regds], regs.wordregs[regdx] + i);
        filename[i] = c;
        if (c == 0) break;
        printf("%c", c);
    }
    printf("'");
    printf("\n");
}

void ReadParameter(char *parameter) {
    printf("'");
    for (int i = 0; i < 50; i++) {
        char c = Read8Long(segregs[reges], regs.wordregs[regbx] + i);
        parameter[i] = c;
        if (c == 0) break;
        printf("%c", c);
    }
    printf("'");
    printf("\n");
}


void HandleDosInterrupt() {
    char filename[30];

    uint16_t ah = regs.byteregs[regah];
    uint16_t al = regs.byteregs[regal];
    printf("DOS: ah: 0x%02x, al: 0x%02x\n", ah, al);

    switch (ah) {

        case 0x00: // terminate program
            printf("DOS: terminate\n");
            exit_or_restart(0);
            break;

        case 0x02: // character output
            printf("DOS: write text: %c\n", regs.byteregs[regdl]);
            break;

        case 0x06: // character output
        {
            uint8_t c = regs.byteregs[regdl];
            if (c == 0xFF)
            {
                printf("DOS: Not supported input request\n");
                exit_or_restart(1);
            }
            printf("DOS: write direct console I/O: %c\n", c);
            ClearCFInInterrupt();
            //exit_or_restart(1);
            break;
        }

        case 0x09: // string output
        {
            printf("DOS: write string: ");
            for(int i=0; i<1024; i++) {
                char c = Read8((segregs[regds] << 4) + regs.wordregs[regdx]+i);
                if (c == '$') break;
                printf("%c", c);
            }
            printf("\n");
            //exit(1);
            break;
        }

        case 0x19: //  Get Current Default Drive
            printf("DOS: get default drive\n");
            regs.byteregs[regal] = 0; // A: drive
            break;

        case 0x2a: // get date
            regs.wordregs[regax] = 0; // Sunday
            regs.wordregs[regcx] = 2020; // year
            regs.byteregs[regdh] = 6; // summer
            regs.byteregs[regdl] = 0; // day
            break;

        case 0x2c: // get time
            // http://stanislavs.org/helppc/int_21-2c.html
            regs.wordregs[regcx] = 0;
            regs.wordregs[regdx] = 0;
            printf("DOS: get time\n");
            break;

        case 0x25: // set interrupt vector
        {
            uint32_t intno = al & 0xFF;
            printf("DOS: set interrupt vector : 0x%02x to 0x%04x:0x%04x\n", intno, segregs[regds], regs.wordregs[regdx]);
            Write16((intno << 2) + 2, segregs[regds]);
            Write16((intno << 2) + 0, regs.wordregs[regdx]);
            break;
        }

        case 0x1a: // set disk transfer access
            // http://stanislavs.org/helppc/int_21-2a.html
            printf("DOS: set disk transfer access structure 0x%04x:0x%04x\n", segregs[regds], regs.wordregs[regdx]);
            dtaseg = segregs[regds];
            dtaofs = regs.wordregs[regdx];
            break;

        case 0x2d: // set time
            regs.byteregs[regal] = 0x00;
            break;

        case 0x2b: // set date
            regs.byteregs[regal] = 0x00;
            break;


        case 0x2f: // get disk transfer access
            // http://stanislavs.org/helppc/int_21-2f.html
            printf("DOS: get disk transfer access structure\n");
            regs.wordregs[regbx] = dtaofs;
            segregs[reges] = dtaseg;
            break;

        case 0x30: // DOS version number TODO
            // https://www.i8086.de/dos-int-21h/funktion-30.html
            //regs.wordregs[regax] = 0; // this is actually version 1, // version 1, to prevent the call of function 52h http://stanislavs.org/helppc/int_21-52.html
            regs.wordregs[regax] = 5; // dosbox
            //exit_or_restart(1);
            //regs.wordregs[regax] = 1; // version 1, to prevent the call of function 52h http://stanislavs.org/helppc/int_21-52.html
            regs.wordregs[regbx] = 0xFF00;
            regs.wordregs[regcx] = 0;
            break;

        case 0x33: // get set system values, ctrl-break checking http://stanislavs.org/helppc/int_21-33.html
        {
            printf("DOS: get set Ctrl-Break checking al=%i dl=%i\n", regs.byteregs[regal], regs.byteregs[regdl]);
            regs.byteregs[regdl] = 0; // always off
            break;
        }

        case 0x35: // get interrupt vector
        {
            uint32_t intno = al & 0xFF;
            printf("DOS: get interrupt vector : 0x%02x\n", intno);
            segregs[reges] = Read16((intno << 2) + 2);
            regs.wordregs[regbx] = Read16((intno << 2) + 0);
            break;
        }

        case 0x3c: // Create File Using Handle
        {
            //http://stanislavs.org/helppc/file_attributes.html
            uint16_t attributes = regs.wordregs[regcx];
            printf("DOS: create file mode : 0x%04x ", attributes);
            int nextfreehandle;
            for(nextfreehandle=5; nextfreehandle<255; nextfreehandle++) if (!handle[nextfreehandle].isactive) break;
            ReadFilename(handle[nextfreehandle].filename);
            handle[nextfreehandle].file = CreateFile(handle[nextfreehandle].filename);
            handle[nextfreehandle].isactive = true;
            handle[nextfreehandle].isfile = true;
            handle[nextfreehandle].offset = 0;
            handle[nextfreehandle].device_information = 0; // just a file

            ClearCFInInterrupt();
            regs.wordregs[regax] = nextfreehandle;
            break;
        }

        case 0x3d: // Open File Using Handle
        {
            uint32_t accessmode = al;
            printf("DOS: open file mode : 0x%02x ", accessmode);
            ReadFilename(filename);
            int nextfreehandle;
            for(nextfreehandle=5; nextfreehandle<255; nextfreehandle++) if (!handle[nextfreehandle].isactive) break;

            if (strcmp(filename, "EMMXXXX0") == 0) {
                handle[nextfreehandle].isfile = false;
                handle[nextfreehandle].isactive = true;
                handle[nextfreehandle].device_information = 0xc080; // character device, supports IOCTL, reserved
                strcpy(handle[nextfreehandle].filename, filename);
                ClearCFInInterrupt();
                regs.wordregs[regax] = nextfreehandle;
                break;
            }

            uint16_t index = 0;
            FILEFS *file = FindFile(filename, &index);
            if (file == NULL) {
                printf("DOS: Warn: file not found\n");
                SetCFInInterrupt();
                regs.wordregs[regax] = 0x2; // File not found
                //PrintStatus();
                //exit_or_restart(1);
            } else {
                printf("DOS: handle 0x%02x\n", nextfreehandle);
                handle[nextfreehandle].file = file;
                handle[nextfreehandle].isfile = true;
                handle[nextfreehandle].isactive = true;
                handle[nextfreehandle].offset = 0;
                handle[nextfreehandle].device_information = 0x0000; // a file
                strcpy(handle[nextfreehandle].filename, file->filename);
                ClearCFInInterrupt();
                regs.wordregs[regax] = nextfreehandle;
            }
            break;
        }

        case 0x3e: // close file
        {
            uint16_t h = regs.wordregs[regbx];
            printf("DOS: close file : 0x%02x '%s'\n", h, handle[h].filename);
            ClearCFInInterrupt();
            handle[h].isactive = false;
            handle[h].file = NULL;
            break;
        }

        case 0x3f: // read from file or device using handle
        {
            uint16_t h = regs.wordregs[regbx];
            uint32_t size = regs.wordregs[regcx];
            printf("DOS: read from handle : 0x%02x, '%s' offset: %5i, size: %5i to 0x%04x:0x%04x ",
                    h, handle[h].filename, handle[h].offset, size, segregs[regds], regs.wordregs[regdx]);
            if (handle[h].isactive == false) {
                printf("DOS: Error: handle not used\n");
                exit_or_restart(1);
            }
            if (handle[h].isfile == false) {
                printf("DOS: Error: Try to read from none-file\n");
                exit_or_restart(1);
            }
            //printf("DOS: offset:%i size:%i\n", handle[h].offset, size);
            if (size + handle[h].offset >= handle[h].file->size) {
                size = handle[h].file->size - handle[h].offset;
            }
            if (size < 0) size = 0;
            printf(" read size:%i\n", size);
            //exit_or_restart(1);
            for (int i = 0; i < size; i++) {
                Write8((((uint32_t) segregs[regds] << 4)) + ((uint32_t) regs.wordregs[regdx]) + i,
                       handle[h].file->data[handle[h].offset++]);
            }
            if (size == 0) {
                regs.wordregs[regax] = size;
                ClearCFInInterrupt();
            } else {
                regs.wordregs[regax] = size;
                ClearCFInInterrupt();
                FS4AlterFiles(handle[h].file, (((uint32_t) segregs[regds] << 4)) + ((uint32_t) regs.wordregs[regdx]), size);
            }
            break;
        }

        case 0x40: // write to file or device
        {
            uint16_t h = regs.wordregs[regbx];
            uint32_t size = regs.wordregs[regcx];
            printf("DOS: write file handle: 0x%04x, %s, size:0x%04x\n", h, handle[h].filename, size);
            regs.wordregs[regax] = size;
            ClearCFInInterrupt();
            if (handle[h].isactive == false) {
                printf("DOS: Error: handle not used\n");
                exit_or_restart(1);
            }
            if (h == 1 || h == 2) { // stdout or stderr
                for (int i = 0; i < size; i++) {
                    printf("%c", Read8((((uint32_t) segregs[regds] << 4)) + ((uint32_t) regs.wordregs[regdx]) + i));
                }
                break;
            }
            if (handle[h].isfile == true) {
                printf("DOS: Write to file\n");
                uint8_t* data = &ram[(((uint32_t)segregs[regds] << 4)) + ((uint32_t)regs.wordregs[regdx])];
                WriteFile(handle[h].file, data, size, handle[h].offset);
                handle[h].offset += size;
                break;
            }
            printf("DOS: Error: Try to write to unknown file\n");
            exit_or_restart(1);
            break;
        }

        case 0x41: // Delete file
        {
            printf("DOS: delete file\n");
            ReadFilename(filename);
            ClearCFInInterrupt();
            break;
        }

        case 0x42: // Move File Pointer Using Handle
        {
            uint16_t h = regs.wordregs[regbx];
            uint32_t offset = (((uint32_t) regs.wordregs[regcx]) << 16u) | regs.wordregs[regdx];
            printf("DOS: file seek handle=0x%02x, mode=0x%02x offset=%i %s\n", h, al, offset, handle[h].filename);
            if (handle[h].file == NULL) {
                printf("DOS: Error: Invalid handle\n");
                exit_or_restart(1);
            }
            switch(al) {
                case 0:
                    handle[h].offset = offset;
                    break;
                case 1:
                    handle[h].offset += offset;
                    break;
                case 2:
                    handle[h].offset = handle[h].file->size+offset; // TODO, is this correct?
                    break;
                default:
                    printf("DOS: Error: Unknown seek mode %i\n", al);
                    exit_or_restart(1);
                    break;
            }
            if (handle[h].offset > handle[h].file->size) {
                printf("DOS: Error: seek too high\n");
                exit_or_restart(1);
            }

            ClearCFInInterrupt();
            regs.wordregs[regdx] = handle[h].offset>>16;
            regs.wordregs[regax] = handle[h].offset&0xFFFF;
            //exit_or_restart(1);
            break;
        }
        case 0x43: // get/set file attributes
            printf("DOS: get/set file attributes : mode=%i  ", al);
            ReadFilename(filename);
            if (al != 0) {
                printf("Error: set file attribute not supported\n");
                exit_or_restart(1);
            }
            uint16_t index = 0;
            FILEFS *file = FindFile(filename, &index);
            if (file == NULL) {
                printf("DOS: Warn: file not found\n");
                SetCFInInterrupt();
                regs.wordregs[regax] = 0x2; // File not found
            } else {
                regs.wordregs[regcx] = 0x20; // compatibility with dosbox
                regs.wordregs[regax] = 0x20;
                ClearCFInInterrupt();
            }
            break;

        case 0x44: // IOCTL
            HandleIOCTL();
            break;

        case 0x48: // allocate memory
        {
            printf("DOS: allocate memory size: %5i segments at 0x%04x\n", regs.wordregs[regbx], nextfreeseg);
            ClearCFInInterrupt();
            regs.wordregs[regax] = Allocate(regs.wordregs[regbx]);
            //exit_or_restart(1);
            break;
        }

        case 0x49: { // free allocated memory
            printf("DOS: free allocated memory: seg:0x%04x\n", segregs[reges]);
            //printf("lastallocseg=0x%04x\n", lastallocseg);
            Free(segregs[reges]);
            ClearCFInInterrupt();
            //exit_or_restart(1);
            break;
        }

        case 0x4a: // modify allocated memory block
        {
            // TODO handle memory segments correctly, see MCB.
            printf("DOS: modify allocated memory block size: %i segments ", regs.wordregs[regbx]);
            printf("at segment 0x%04x\n", segregs[reges] + 0x10);
            bool ret = Modify(segregs[reges] + 0x10, regs.wordregs[regbx]);
            if (ret) {
                ClearCFInInterrupt();
                regs.wordregs[regax] = 0x0;
                regs.wordregs[regbx] = 0; // maximum blocksize possible
            } else {
                SetCFInInterrupt();
            }
            regs.wordregs[regax] = segregs[reges]; // Put in after comparison with dosbox. Probably not necessary
            //exit_or_restart(1);
            break;
        }

        case 0x4b: // exec load and execute program
        {
            char parameter[50];
            ReadFilename(filename);
            ReadParameter(parameter);
            printf("DOS: execute program '%s' mode: %i\n", filename, regs.byteregs[regal]);
            exit_or_restart(1);
            break;
        }

        case 0x4c: // end program
            printf("DOS: program exit with code %i\n", al);
            exit_or_restart(al);
            break;

        case 0x4e: // Find First Matching File
        {
            uint16_t attributes = regs.wordregs[regcx];
            printf("DOS: find first matching file attribute : 0x%04x ", attributes);
            ReadFilename(filename);
            uint16_t index = 0;
            FILEFS *file = FindFile(filename, &index);
            if (file == NULL) {
                SetCFInInterrupt();
                regs.wordregs[regax] = 0x2; // File not found
                printf("DOS: Warn: file not found\n");
            } else {
                ClearCFInInterrupt();
                regs.wordregs[regax] = 0;
                // http://stanislavs.org/helppc/int_21-4e.html
                printf("DOS: found file %s\n", file->filename);
                Write16Long(dtaseg, dtaofs + 0x6, index + 1); // store index for next matching file
                Write8Long(dtaseg, dtaofs + 0x15, attributes); // attribute
                Write16Long(dtaseg, dtaofs + 0x16, 0x0); // file time
                Write16Long(dtaseg, dtaofs + 0x18, 0x0); // file date
                Write16Long(dtaseg, dtaofs + 0x1A, file->size & 0xFFFF); // file size low
                Write16Long(dtaseg, dtaofs + 0x1C, file->size >> 16); // file size high
                for (int i = 0; i < 13; i++) Write8Long(dtaseg, dtaofs + 0x1E + i, 0);
                for (int i = 0; i < strlen(file->filename); i++)
                    Write8Long(dtaseg, dtaofs + 0x1E + i, toupper(file->filename[i]));
            }
            break;
        }

        case 0x4f: // Find Next Matching File
        {
            printf("DOS: find next matching file ");
            ReadFilename(filename);
            uint16_t index = Read16Long(dtaseg, dtaofs + 0x6);
            FILEFS *file = FindFile(filename, &index);
            if (file == NULL) {
                SetCFInInterrupt();
                regs.wordregs[regax] = 0x12; // no more files
                //regs.wordregs[regax] = 0x2; // no more files
                printf("DOS: Error: no more files\n");
                //exit_or_restart(1);
            } else {
                ClearCFInInterrupt();
                // http://stanislavs.org/helppc/int_21-4e.html
                printf("DOS: found file %s\n", file->filename);
                Write16Long(dtaseg, dtaofs + 0x6, index + 1); // store index for next matching file
                Write16Long(dtaseg, dtaofs + 0x16, 0x0); // file time
                Write16Long(dtaseg, dtaofs + 0x18, 0x0); // file date
                Write16Long(dtaseg, dtaofs + 0x1A, file->size & 0xFFFF); // file size low
                Write16Long(dtaseg, dtaofs + 0x1C, file->size >> 16); // file size high
                for (int i = 0; i < 13; i++) Write8Long(dtaseg, dtaofs + 0x1E + i, 0);
                for (int i = 0; i < strlen(file->filename); i++)
                    Write8Long(dtaseg, dtaofs + 0x1E + i, toupper(file->filename[i]));
            }
            break;
        }

        case 0x50: // Set process ID
        {
            printf("DOS: set process id %i\n", regs.wordregs[regbx]);
            pspseg = regs.wordregs[regbx];
            Write16((dibseg<<4) + 0x330, pspseg); // flight simulator 4 needs this
            break;
        }

        case 0x51: // Get process ID
        {
            printf("DOS: get process id\n");
            regs.wordregs[regbx] = pspseg; // this is always the segment of the psp
            break;
        }

        case 0x52: // Get Pointer to DOS "INVARS"
        {
            printf("DOS: get dos info block\n");
            segregs[reges] = dibseg;
            regs.wordregs[regbx] = 0x0026;
            break;
        }

        case 0x58: // get/set memory allocation strategy
        {
            printf("DOS: get/set memory allocation strategy \n");
            PrintStatus();
            if (al == 0) {
                //regs.wordregs[regax]
                regs.wordregs[regax] = 0; // get strategy
            }
            ClearCFInInterrupt();
            break;
        }

        default:
            printf("DOS: Unknown DOS function ah=%02x\n", regs.byteregs[regah]);
            PrintStatus();
            exit_or_restart(1);
            break;
    }
}

void DOSInit() {
    AllocInit();
    dibseg = 0x80; // the same dosbox uses

    // the default dta is in the last 128 byte of the psp structure
    dtaseg = nextfreeseg - 16;
    dtaofs = 0x80;

    pspseg = nextfreeseg-16; // program segment prefix

    memset(handle, 0, sizeof(HANDLE)*256);
    for(int i=0; i<256; i++) handle[i].isactive = false;

    handle[0].isfile = false;
    handle[0].isactive = true;
    handle[0].device_information = 0x80d3; // std input output, binary mode,
    strcpy(handle[0].filename, "stdin");

    handle[1].isfile = false;
    handle[1].isactive = true;
    handle[1].device_information = 0x80d3;
    strcpy(handle[1].filename, "stdout");

    handle[2].isfile = false;
    handle[2].isactive = true;
    handle[2].device_information = 0x80d3;
    strcpy(handle[2].filename, "stderr");

    // dos info block, just after the bios data area
    memset(&ram[dibseg<<4], 0x0, 0x400);
    Write16((dibseg<<4) + 4, 1);

    // dos environment variables
    memset(&ram[0x600], 0x0, 0x100);
    //strcpy(&ram[0x600], "COMSPEC=C:\\COMMAND.COM\0");
}
