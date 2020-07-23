#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/string.h"

#include"mz.h"

#include"../devices/ram.h"
#include"../cpu/cpu.h"
#include"alloc.h"

// EXE MZ Format: http://www.delorie.com/djgpp/doc/exe/

#pragma pack(1)
typedef struct {            // DOS .EXE header
    uint16_t magic;         // Magic number
    uint16_t cblp;          // Bytes on last page of file
    uint16_t cp;            // Pages in file
    uint16_t crlc;          // Relocations
    uint16_t cparhdr;       // Size of header in paragraphs
    uint16_t minalloc;      // Minimum extra paragraphs needed
    uint16_t maxalloc;      // Maximum extra paragraphs needed
    uint16_t ss;            // Initial (relative) SS value
    uint16_t sp;            // Initial SP value
    uint16_t csum;          // Checksum
    uint16_t ip;            // Initial IP value
    uint16_t cs;            // Initial (relative) CS value
    uint16_t lfarlc;        // File address of relocation table
    uint16_t ovno;          // Overlay number
} IMAGE_DOS_HEADER;

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

#pragma pack(1)
typedef struct {
    uint16_t offset;
    uint16_t segment;
} EXE_RELOC;

#ifndef __wasm__

void LoadMzExeFromFile(char *filename) {
    printf("MZ: Filename: %s\n", filename);

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("MZ: Cannot open file '%s'\n", filename);
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    uint8_t *data = malloc(sz);
    int ret = fread(data, sz, 1, fp);
    fclose(fp);

    LoadMzExe(data, sz);
}

void LoadCOMFromFile(char *filename) {
    printf("COM: Filename: %s\n", filename);

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("COM: Cannot open file '%s'\n", filename);
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    uint8_t *data = malloc(sz);
    int ret = fread(data, sz, 1, fp);
    fclose(fp);

    LoadCOM(data, sz);
}

#else
void LoadMzExeFromFile(char* filename)
{
    printf("Not implemented\n");
    exit(1);
}
#endif

void LoadMzExe(char *data, int size) {
    IMAGE_DOS_HEADER head;

    memcpy(&head, data, sizeof(head));
    printf("MZ:  - file size: %i\n", size);
    printf("MZ:  - %c%c\n", head.magic & 0xFF, head.magic >> 8);
    printf("MZ:  - bytes on the last page of file: %i\n", head.cblp);
    printf("MZ:  - pages in file: %i\n", head.cp);
    printf("MZ:  - number of relocations: %i\n", head.crlc);
    printf("MZ:  - size of header in paragraphs: %i\n", head.cparhdr);
    printf("MZ:  - minimum extra paragraphs needed: %i\n", head.minalloc);
    printf("MZ:  - maximum extra paragraphs needed: %i\n", head.maxalloc);
    printf("MZ:  - ss:sp : 0x%04x:0x%04x\n", head.ss, head.sp);
    printf("MZ:  - cs:ip : 0x%04x:0x%04x\n", head.cs, head.ip);
    printf("MZ:  - offset of relocation table: %i\n", head.lfarlc);
    printf("MZ:  - overlay number: %i\n", head.ovno);

    uint16_t additional_header;
    memcpy(&additional_header, &data[sizeof(head)], 2);

    //printf(" - additional header: %c%c\n", additional_header&0xFF, additional_header>>8);

    int size_exe = head.cp * 512 - 512 + head.cblp;
    if (head.cblp == 0) size_exe += 512;
    printf("MZ:  - size executable according to header: %i\n", size_exe);

    size_exe -= head.cparhdr * 16;
    int allocate_size = (size_exe >> 4) + 1;
    allocate_size += head.maxalloc;
    if (allocate_size > AvailableRam()) allocate_size = AvailableRam();

    printf("MZ:  - allocate paragraphs: 0x%04x\n", allocate_size);
    int relocseg = Allocate(allocate_size); // is this correct. the proram prefix is missing

    printf("MZ:  - to seg: 0x%04x\n", relocseg);
    printf("MZ:  - next free seg: 0x%04x\n", nextfreeseg);
    memcpy(&ram[relocseg << 4], &data[head.cparhdr << 4], size_exe);

    for (int i = 0; i < head.crlc; i++) {
        EXE_RELOC reloc;
        memcpy(&reloc, &data[head.lfarlc + 4 * i], 4);
        uint16_t seg = Read16Long(relocseg + reloc.segment, reloc.offset);
        //printf("reloc 0x%04x:0x%04x 0x%04x\n", reloc.segment, reloc.offset, seg);
        //printf("0x%04x:0x%04x\n", seg, ofs);
        //printf("0x%04x\n", seg+ relocseg);
        Write16Long(relocseg + reloc.segment, reloc.offset, seg + relocseg);
    }
    setcsip(head.cs + relocseg, head.ip);
    setsssp(head.ss + relocseg, head.sp);

    segregs[regds] = relocseg - 0x10; // point to Program Segment Prefix
    segregs[reges] = relocseg - 0x10; // point to Program Segment Prefix

    // TODO, fill the program segment prefix coorrectly
    // https://en.wikipedia.org/wiki/Program_Segment_Prefix
    // exe filename

    for (int i = 0; i < 11; i++) {
        ram[(relocseg << 4) - 0x100 + 0x5d + i] = 32;
        ram[(relocseg << 4) - 0x100 + 0x6d + i] = 32;
    }
    // ret command int 20h
    ram[(relocseg << 4) - 0x100 + 0x0] = 0xcd;
    ram[(relocseg << 4) - 0x100 + 0x1] = 0x20;

    // Segmentadresse des ersten vom Programm nicht mehr belegten Speichers
    nextfreeseg++;
    ram[(relocseg << 4) - 0x100 + 0x2] = nextfreeseg & 0xFF;
    ram[(relocseg << 4) - 0x100 + 0x3] = nextfreeseg >> 8;

    ram[(relocseg << 4) - 0x100 + 0x2C] = 0x60; // segment with environment variables
    ram[(relocseg << 4) - 0x100 + 0x2D] = 0x0;

    ram[(relocseg << 4) - 0x100 + 0x80] = 0; // no parameter on the command line
    ram[(relocseg << 4) - 0x100 + 0x81] = 0x0d; // new line
    printf("MZ: Finished Loading Exe\n");
}

void LoadCOM(char *data, int size) {
    printf("COM: - size: %i\n", size);
    int seg = Allocate((size>>4) + 1 + 0x10);
    memcpy(&ram[(seg << 4) + 0x100], data, size);
    setcsip(seg, 0x100);
    setsssp(seg, 0xfffe);
    segregs[regds] = seg - 0x10; // I don't think this is correct
    segregs[reges] = seg - 0x10; // I don't think this is correct

    // fill program segment prefix
    for (int i = 0; i < 11; i++) {
        ram[(seg << 4) + 0x5d + i] = 32;
        ram[(seg << 4) + 0x6d + i] = 32;
    }
    // ret command int 20h
    ram[(seg << 4) + 0x0] = 0xcd;
    ram[(seg << 4) + 0x1] = 0x20;

    // Segmentadresse des ersten vom Programm nicht mehr belegten Speichers
    ram[(seg << 4) + 0x2] = nextfreeseg & 0xFF;
    ram[(seg << 4) + 0x3] = nextfreeseg >> 8;

    ram[(seg << 4) + 0x80] = 0; // no parameter on the command line
    ram[(seg << 4) + 0x81] = 0x0d; // new line
    printf("COM: Finished Loading Com\n");
}
