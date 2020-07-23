#include "wasm_libc_wrapper/stdio.h"
#include "wasm_libc_wrapper/stdlib.h"

#include"devices/ram.h"
#include"devices/vga.h"
#include"devices/pic.h"
#include"devices/pit.h"
#include"devices/rom.h"
#include"devices/ems.h"
#include"devices/screen.h"
#include"devices/disk.h"
#include"fs/fs.h"
#include"dos/dos.h"
#include"cpu/cpu.h"
#include"utils/exit_strategy.h"

#include"dos/mz.h"
#include"sdl.h"
#include"fs4.h"
#include"debug/compare.h"

//#define DEBUG
//#define SINGLESTEP

void disasmcsip(int count) {
    uint16_t seg, ofs;
    getcsip(&seg, &ofs);
    disasmout(seg, ofs, count);
}

void LoadMz(char *filename) {
    uint16_t index = 0;
    FILEFS *file = FindFile(filename, &index);
    if (file == NULL) {
        printf("Error: Cannot open file '%s'\n", filename);
        exit(1);
    }
    printf("open %s\n", file->filename);
    LoadMzExe(file->data, file->size);
}

void UpdateScreen() {
    VGA_Draw();
    SDLUpdate();
    SDLPoll();
}

#ifndef __wasm__

int fsize(FILE *fp) {
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    return sz;
}

void MountFs(char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        exit(1);
    }
    int size = fsize(fp);
    printf("Load filesystem '%s' with size %i\n", filename, size);
    char *data = GetMountStorage(size);
    size_t ret = fread(data, size, 1, fp);
    fclose(fp);
    FinishMountStorage();
}

#else
    void MountFs(char *filename) {}
#endif

void SetFSVersion(int _version) {
    int fsversion = _version;

    printf("Loading FS version\n");
    if (fsversion == 1) {
        MountFs("data/fs1.fs");
        RunImage("fs1.img");
    }
    if (fsversion == 2) {
        MountFs("data/fs2.fs");
        RunImage("fs2.img");
    }
    if (fsversion == 3) {
        MountFs("data/fs3.fs");
        LoadMz("fs3.exe");
    }
    if (fsversion == 4) {
        MountFs("data/fs4.fs");
        LoadMz("fs4.exe");
    }
    if (fsversion == 5) {
        MountFs("data/fs5.fs");
        LoadMz("fs5.ovl");
        //LoadMz("fs5.exe");
    }
}

void Init() {
    printf("Init system\n");
    printf(" - Init screen\n");
    ScreenInit();
    printf(" - Init SDL\n");
    SDLInit();
    printf(" - Init RAM\n");
    RAMInit();
    printf(" - Init ROM\n");
    ROMInit();
    printf(" - Init DOS\n");
    DOSInit();
    printf(" - Init EMS\n");
    EMSInit();
    printf(" - Init PIC\n");
    PIC_Init();
    printf(" - Init PIT\n");
    PIT_Init();
    printf(" - Init VGA\n");
    VGA_Init();
    printf(" - Init x86\n");
    reset86();
    printf("Init system finished\n");

    //LoadMzExeFromFile("fs/plasma/PLASMA.EXE");
    //LoadMzExeFromFile("fs/fc/FCSLIDE1.EXE");
    //LoadMzExeFromFile("fs/scr/cga.exe");
    //LoadMzExeFromFile("fs/nebega/Nebega.exe");
    //LoadMzExeFromFile("fs/tower/TOWER.EXE");
    //LoadMzExeFromFile("fs/scr/ega.exe");
    //LoadCOMFromFile("fs/bcctests/float.com");
    //LoadMzExeFromFile("fs/tcc/TC/TCDEF.EXE");
    //LoadMzExeFromFile("fs/tcc/tc2/tc2/NONAME.EXE");
    //LoadMzExeFromFile("fs/tcc/TC/BIN/TEST.EXE");
    //LoadMzExeFromFile("fs/tcc/TC/TEST.EXE");
}

static uint32_t count = 0;

void Run(int steps) {
    if (isRestart()) {
        printf("Restart\n");
        Init();
    }

#ifdef DEBUG
    //if (count > 8890000) disasmcsip(1);
    //exec86(0x1);
    compare(count);
    if ((count & 0xFFFF) == 0) UpdateScreen();
#elif defined SINGLESTEP
    disasmcsip(1);
    exec86(0x1);
    if ((count & 0xFFFF) == 0) UpdateScreen();
#else
    //disasmcsip(1);
    exec86(steps);
    //#ifndef __wasm__
        if ((count&0xF) == 0) UpdateScreen();
    //#endif
#endif
    count++;
    /*
    int ticks = SDL_GetTicks();
    if (c64->cpu.count / 985 - ((ticks - starttick)) > 0) {
            SDL_Delay(c64->cpu.count / 985 - ((ticks - starttick)));
    }
    */
}

int main() {
    Init();
    SetFSVersion(4);
    //FS4Expandexe();

    while (1) {
        Run(0x5FFFF);
        //Run(0x001FF);
        //SDLDelay(50);
    }
}
