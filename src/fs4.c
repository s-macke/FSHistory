#include "wasm_libc_wrapper/stdint.h"
#include "wasm_libc_wrapper/stdlib.h"
#include "wasm_libc_wrapper/stdio.h"
#include "wasm_libc_wrapper/string.h"
#include"fs/fs.h"
#include"cpu/cpu.h"
#include"devices/ram.h"

//static uint16_t gra_base_cs = 0x3960; // base of ega.gra
//static uint16_t fs4_base_cs = 0x1000; // base of fs4.exe
static uint16_t fs4_base_cs = 0x01A2; // base of fs4.exe

void FS4AlterFiles(const FILEFS *file, uint32_t addr, uint32_t size) {

    if (strcmp(file->filename, "ega1.gra") != 0) return;
    if (size != 20192) return;

    printf("fs4 alter file '%s' size %i to 0x%08x\n", file->filename, size, addr);
    Write8(addr + 0x4476, 0xC3); // disable mouse
    Write16Long(fs4_base_cs, 0x6DA2, 0xD231); // disable checksum check

    //Write8Long(gra_base_cs, 0x1296, 0xCb); // far return
    /*
    Write8Long(gra_base_cs, 0x1d52, 0xCb); // far return
    Write8Long(gra_base_cs, 0x1753, 0xCb); // far return
    Write8Long(gra_base_cs, 0x15eb, 0xCb); // far return
    Write8Long(gra_base_cs, 0x13ff, 0xCb); // far return
    Write8Long(gra_base_cs, 0x12be, 0xCb); // far return
    Write8Long(gra_base_cs, 0x12c1, 0xCb); // far return
    Write8Long(gra_base_cs, 0x12a7, 0xCb); // far return
    Write8Long(gra_base_cs, 0x1296, 0xCb); // far return
    Write8Long(gra_base_cs, 0x1043, 0xCb); // far return
    Write8Long(gra_base_cs, 0x37e5, 0xCb); // far return
    */
    //Write16Long(gra_base_cs, 0x0f42, 0xCb); // far return
    //Write16Long(gra_base_cs, 0x1753, 0xC3); // near return
}

#ifndef __wasm__

void ExtractImage() {
    uint16_t index = 0;
    //FILEFS *file = FindFile("F6", &index);
    //FILEFS *file = FindFile("F6C", &index);
    //FILEFS *file = FindFile("F6CSENS", &index);
    FILEFS *file = FindFile("F6H", &index);
    //FILEFS *file = FindFile("F6HSENS", &index);
    //FILEFS *file = FindFile("F6MSENS", &index);
    //FILEFS *file = FindFile("F6SENS", &index);

    FILE *fp = fopen("test.pbm", "w");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file\n");
        exit(1);
    }
    fprintf(fp, "P1\n320 457\n");
    for (int j = 0; j < 457; j++) {
        for (int i = 0; i < 320 / 8; i++) {
            uint8_t c = file->data[0 + j * (320 / 8) + i];
            for (int ii = 0; ii < 8; ii++) {
                fprintf(fp, "%i ", (c >> (7 - ii)) & 1);
            }
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

#pragma pack(1)
typedef struct {
    uint16_t signature; /* == 0x5a4D */
    uint16_t bytes_in_last_block;
    uint16_t blocks_in_file;
    uint16_t num_relocs;
    uint16_t header_paragraphs;
    uint16_t min_extra_paragraphs;
    uint16_t max_extra_paragraphs;
    uint16_t ss;
    uint16_t sp;
    uint16_t checksum;
    uint16_t ip;
    uint16_t cs;
    uint16_t reloc_table_offset;
    uint16_t overlay_number;
} EXE;


void FS4Expandexe() {
    //exe already stored at 0x1000:0x0000
    //disasmout(0x239c, 0xF6, 1); // original code
    //disasmout(0x29cf, 0xF6, 1); // copied code
    printf("expand fs4 and store to exe\n");
    for (int i = 0; i < 1000000; i++) {
        if ((segregs[regcs] == 0x29cf) && (ip == 0xF6)) {
            exec86(3);
            PrintStatus();
            printf("program entry at cs:ip = 0x%04x:0x%04x\n", Read16Long(0x29cf, 0x2), Read16Long(0x29cf, 0x0));
            disasmout(0x1000, 0x0, 1);
            break;
        }
        exec86(1);
    }

    FILE *fp = fopen("fs4_expanded.exe", "wb");
    if (fp == NULL) {
        printf("Error: Cannot write to file\n");
    }
    //int size = 0x19cf0;
    int size = 0x19d00;
    int size_exe = size + 16 * 16;

    EXE header;
    header.signature = 0x5a4D;
    header.bytes_in_last_block = 0x0;
    header.num_relocs = 0;
    header.header_paragraphs = 16;
    header.min_extra_paragraphs = 0;
    header.max_extra_paragraphs = 65535;
    header.ss = 0x19cf;
    header.sp = 0x0190;
    header.cs = 0x0000;
    header.ip = 0x0000;
    header.checksum = 0x0;
    header.reloc_table_offset = 30;
    header.overlay_number = 0x0;
    header.blocks_in_file = size_exe / 512;
    fwrite(&header, sizeof(header), 1, fp);

    fseek(fp, header.header_paragraphs * 16, SEEK_SET);
    fwrite(&ram[0x10000], size, 1, fp);

    fclose(fp);

    exit(1);
}

#endif
