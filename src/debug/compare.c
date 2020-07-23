#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/string.h"
#include "../cpu/cpu.h"
#include "../devices/ram.h"

#ifndef __wasm__

static int count = 0;
static FILE *fp = NULL;

#pragma pack(1)
typedef struct {
    uint16_t exec_cs, exec_ip; // executed cs and ip
    uint8_t  op, data; // executed operation
    // results:
    uint16_t cs, ip;
    uint32_t eax, ebx, ecx, edx;
    uint32_t ebp, esp;
    uint32_t edi, esi;
    uint16_t ds, es;
    uint16_t flags;
    uint8_t  new_op, new_data; // new operation
} CMPREGS;

static CMPREGS cmp;

void load_next() {
    int ret;
    ret = fread(&cmp, 2, 25, fp);
    if (ret != 25) {
        //exit_or_restart(1);
        printf("load_next: End of file\n");
        exit(1);
    }
}

void PrintComparison() {
    //load_next();
    //load_next();
    printf("count:%7i 0x%04x:0x%04x 0x%04x:0x%04x ax=0x%08x bx=0x%08x cx=0x%08x dx=0x%08x sp=0x%08x bp=0x%08x flags=0x%04x ds=0x%04x es=0x%04x di=0x%08x si=0x%08x\n",
           count,
           cmp.exec_cs, cmp.exec_ip,
           cmp.cs, cmp.ip,
           cmp.eax, cmp.ebx, cmp.ecx, cmp.edx,
           cmp.esp, cmp.ebp,
           cmp.flags,
           cmp.ds, cmp.es,
           cmp.edi, cmp.esi);
}

static void Init() {
    fp = fopen("utils/trace.dat", "rb");
    for (int i = 0; i < 22; i++) {
        load_next();
        //printf("0x%04x:0x%04x\n", cmp_cs, cmp_ip);
    }
    regs.wordregs[regcx] = 0x00ff;
    regs.wordregs[regdx] = 0x0192;
    regs.wordregs[regbp] = 0x091c;

    // for nebulus
    //regs.wordregs[regdi] = 0x0080;
    //regs.wordregs[regsi] = 0x0009;
    //Write16Long(0, 0x1c * 4 + 0, 0x1260);
    //Write16Long(0, 0x09 * 4 + 0, 0xe987);

    // for fs5
    regs.wordregs[regdi] = 0x0080;
    regs.wordregs[regsi] = 0x000e;
    Write16Long(0, 0x24 * 4 + 0, 0x0110); // critical error handler
    Write16Long(0, 0x24 * 4 + 2, 0x0118); // critical error handler
    Write16Long(0, 0x05 * 4 + 0, 0x1060);
    Write16Long(0, 0x00 * 4 + 0, 0x1060);
    Write16Long(0, 0x33 * 4 + 0, 0x0010); // mouse
    Write16Long(0, 0x33 * 4 + 2, 0xc7ff); // mouse
    Write16Long(0, 0x0a * 4 + 0, 0xff55);
    Write16Long(0, 0x0b * 4 + 0, 0x1060);
    Write16Long(0, 0x0c * 4 + 0, 0x1060);
    Write16Long(0, 0x0d * 4 + 0, 0x1060);
    Write16Long(0, 0x0f * 4 + 0, 0x1060);
    Write16Long(0, 0x08 * 4 + 0, 0xfea5);
    Write16Long(0, 0x01 * 4 + 0, 0x0008);
    Write16Long(0, 0x01 * 4 + 2, 0x0070);
    Write16Long(0, 0x02 * 4 + 0, 0x0008);
    Write16Long(0, 0x02 * 4 + 2, 0x0070);
    Write16Long(0, 0x03 * 4 + 0, 0x0008);
    Write16Long(0, 0x03 * 4 + 2, 0x0070);
    Write16Long(0, 0x1b * 4 + 0, 0x1060);
    Write16Long(0, 0x09 * 4 + 0, 0xe987);


    Write16Long(0xc7ff, 0x0010, 0x63);
    Write16Long(0xc7ff, 0x0011, 0x33); // int 33
    Write16Long(0xc7ff, 0x0012, 0xcf);

    Write16Long(0xf000, 0xfea5, 0x63);
    Write16Long(0xf000, 0xfea5, 0x08); // int 33
    Write16Long(0xf000, 0xfea5, 0xcf);

    //Write16Long(0, 0x08 * 4 + 0, 0xfea5);

    // for fs4
    /*
    regs.wordregs[regdi] = 0x0080;
    regs.wordregs[regsi] = 0x0010;
    Write16Long(0, 0x24 * 4 + 0, 0x0110); // critical error handler
    Write16Long(0, 0x24 * 4 + 2, 0x0118);
    Write16Long(0, 0x05 * 4 + 0, 0x1060);
    Write16Long(0, 0x00 * 4 + 0, 0x1060);
*/
    //regs.wordregs[regdi] = 0x0080;

    // for cga stunt car racer
    //regs.wordregs[regdi] = 0x0100;
    //Write16Long(0, 9 * 4 + 0, 0xe987);
    //Write16Long(0, 8 * 4 + 0, 0xfea5);
}

void CompareState()  {
    if (
            (cmp.cs != segregs[regcs]) ||
            (cmp.ip != ip) ||
            (cmp.eax != regs.dwordregs[regeax]) ||
            (cmp.ebx != regs.dwordregs[regebx]) ||
            (cmp.ecx != regs.dwordregs[regecx]) ||
            (cmp.edx != regs.dwordregs[regedx]) ||
            (cmp.esp != regs.dwordregs[regesp]) ||
            (cmp.ebp != regs.dwordregs[regebp]) ||
            (cmp.edi != regs.dwordregs[regedi]) ||
            (cmp.esi != regs.dwordregs[regesi]) ||
            (cmp.ds != segregs[regds]) ||
            (cmp.es != segregs[reges]) ||
            //((cmp.flags & 0xFFEF & 0xF7FF & 0x0FFF) != (getflags() & 0xFFEF & 0xF7FF & 0x0FFF))
            ((cmp.flags & 0xFFEF & 0xF7FF) != (getflags() & 0xFFEF & 0xF7FF))
            ) {
        PrintComparison();
        PrintStatus();
        exit(1);
    }
}

static int repmode = 0;
static int repip = 0;
static uint8_t opcode, data;

void compare(int _count) {
    count = _count;
    if (fp == NULL) {
        Init();
        load_next();
    }

// -------------------
    // for test.exe
    //if (count == 248) { regs.wordregs[regcx]=0x0014; regs.wordregs[regdx]=0x7957; } // BIOS clock

// ------------------------
    int intinrep = 0;
    do {
        int last_op = cmp.op;
        load_next();
        if (count >= 14300000) printf("0x%04x:0x%04x   0x%02x\n", cmp.exec_cs, cmp.exec_ip, cmp.op);

        // for scr cga
        //if (cmp.cs == 0x01a2 && cmp.ip == 0x066a) { // in case timer interrupt is triggered during rep
        //    intinrep = 1;
        //    break;
        //}
        // fs5
        if ((last_op == 0xF2) || (last_op == 0xF3))
        if (cmp.cs == 0x01a2 && cmp.ip == 0x4d7e) { // in case timer interrupt is triggered during rep
            intinrep = 1;
            printf("int in rep\n");
            //PrintStatus();
            //exit(1);
            break;
        }

    } while ((cmp.op == 0xF2) || (cmp.op == 0xF3) || (cmp.cs >= 0xc000) ||
        ((cmp.exec_cs >= 0xc000) && ((cmp.new_op == 0xF2) || (cmp.new_op == 0xF3)))); // in case a interrupt is triggered during rep and we have continue rep
// ------------------------
    do {
        opcode = Read8LongUnsafe(segregs[regcs], ip);
        data = Read8LongUnsafe(segregs[regcs], ip+1);

        if (count >= 14300000) disasmout(segregs[regcs], ip, 1);
        ClearHLTstate();
        uint8_t prev_ah = regs.byteregs[regah];
        exec86(0x1);

        // for allocation and modify
        if (((prev_ah == 0x48) || (prev_ah == 0x4a) || (prev_ah == 0x2a) || (prev_ah == 0x2c)) && (segregs[regcs] == 0xf000) && (ip == 0x0086)) { // port 21 used for allocate
            regs.wordregs[regax] = cmp.eax;
            regs.wordregs[regcx] = cmp.ecx; // for get date and get time
            regs.wordregs[regdx] = cmp.edx; // for get date and get time

            ClearCFInInterrupt(); // always succeed
        }

        if (intinrep)
        if ((opcode == 0xF2) || (opcode == 0xF3) && (regs.wordregs[regcx] == (cmp.ecx&0xFFFF))) {
            intcall86(8);
            /*
                opcode = Read8LongUnsafe(segregs[regcs], ip);
                data = Read8LongUnsafe(segregs[regcs], ip+1);
                ClearHLTstate();
                exec86(0x1);
                opcode = Read8LongUnsafe(segregs[regcs], ip);
                data = Read8LongUnsafe(segregs[regcs], ip + 1);
                ClearHLTstate();
                exec86(0x1);
           */
        }

    } while ((opcode == 0xF2) || (opcode == 0xF3) || (segregs[regcs] >= 0xf000)); // test for rep

// ------------------------

// for stunt car racer cga.exe, timer interrupt triggered
/*
    if (cmp.cs == 0x01a2 && cmp.ip == 0x066a) {
        intcall86(8);
        printf("trigger int8 reason: cs:ip\n");
        //PIC_trigger(0x0);
    }
    if (cmp.cs == 0x01a2 && cmp.ip == 0x0241) {
        intcall86(8);
        //printf("trigger int8 reason: cs:ip\n");
        //PIC_trigger(0x0);
    }
    if (cmp.cs == 0x01a2 && cmp.ip == 0x5b0) {
        intcall86(9);
        //printf("trigger int8 reason: cs:ip\n");
        //PIC_trigger(0x0);
    }
*/

    if (opcode == 0xF6) { // mul ( and more
        setflags(cmp.flags);
    }
    if (opcode == 0xF7) { // mul ( and more
        setflags(cmp.flags);
    }
    if (opcode == 0xd4) { // aam
        setflags(cmp.flags);
    }

    // in al, 0x40
    if (opcode == 0xE4 && data == 0x40) {
        regs.wordregs[regax] = cmp.eax;
    }
    // in al, 0x61
    if (opcode == 0xE4 && data == 0x61) {
        regs.wordregs[regax] = cmp.eax;
    }
    // in al, 0x60
    if (opcode == 0xE4 && data == 0x60) {
        regs.wordregs[regax] = cmp.eax;
    }
    if (opcode == 0xEC && regs.wordregs[regdx] == 0x03da) {
        regs.wordregs[regax] = cmp.eax;
    }


    // for fs4
    /*
    if (count == 19160)  { // find first matching file
        regs.wordregs[regax] = cmp.ax;
    }
    if (count == 19317) { // alloc
        regs.wordregs[regax] = cmp.ax;
    }
    */
/*
    if (count == 348548)  {
        regs.wordregs[regax] = cmp.eax; // allocate
    }
*/
    /*
    if (count == 348548)  {
        regs.wordregs[regax] = cmp.ax; // allocate
    }
    if (count == 348700)  {
        regs.wordregs[regax] = cmp.ax; // allocate
    }
    if (count == 352698)  {
        regs.wordregs[regax] = cmp.ax; // allocate
    }
    if (count == 354776)  {
        regs.wordregs[regax] = cmp.ax; // allocate
    }
    if (count == 357328)  {
        regs.wordregs[regax] = cmp.ax; // allocate
    }
    if (count == 572734)  {
        regs.wordregs[regax] = cmp.ax; // allocate
    }
    if (count == 963273)  {
        regs.wordregs[regax] = cmp.ax; // allocate
    }
    if (count == 963465)  {
        regs.wordregs[regax] = cmp.ax; // allocate
    }
 */
    // fs5 timer
    if (cmp.cs == 0x01a2 && cmp.ip == 0x4d7d) {
        intcall86(8);
    }
    // mouse int 33
    if ((segregs[regcs] == 0xc7ff) && (ip == 0x0010)) {
        exec86(0x1);
        exec86(0x1);
    }
/*
    if (count == 1480091) { // get date
        regs.wordregs[regax] = cmp.eax;
        regs.wordregs[regcx] = cmp.ecx;
        regs.wordregs[regdx] = cmp.edx;
    }
    */

    // fs5 not sure why
    if (count == 7751565 || count == 8670749) {
        setflags(cmp.flags);
    }

    if (cmp.cs == 0x01a2 && cmp.ip == 0x549a) {
        regs.wordregs[regax] = cmp.eax; // read daily timer country
    }

    if (cmp.cs == 0x01a2 && cmp.ip == 0x549f) {
        setflags(cmp.flags);
    }
/*
    if (regs.wordregs[regsp] == 0xFF6) {
        printf("write sssp 0x%04x\n", Read16Long(segregs[regss], regs.wordregs[regsp]));
        //exit(1);
    }
*/
    /*
    if (count == 348681)  {
        regs.wordregs[regax] = cmp.ax; // emmXXXX0
        setflags(cmp.flags);
    }*/

    if (count == 346737)  {
        regs.dwordregs[regeax] = cmp.eax;
    }

    if (count == 348670)  {
        regs.dwordregs[regeax] = cmp.eax;
    }


    /*
    if (count == 1479738) { // dos get interrupt vector
        segregs[reges] = 0xc7ff;
        regs.wordregs[regbx] = 0x0010;
    }
*/

/*
    if (count == 346698)  {
        regs.wordregs[regbx] = 0x7246;
    }
    if (count == 346712)  {
        regs.wordregs[regax] = 0x7246;
    }
    if (count == 346718)  {
        regs.wordregs[regax] = 0x7206;
    }
*/

    /*
    if (count > 346720) {
        exit(1);
    }
     */

    //if (count >= 1000000) printf("Compare\n");
    CompareState();
}

#endif