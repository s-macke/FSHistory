#ifndef FS_H
#define FS_H

#include "../wasm_libc_wrapper/stdint.h"

typedef struct {
    char *filename;
    unsigned int size;
    char *data;
} FILEFS;

FILEFS *FindFile(const char *filename, uint16_t *index);
FILEFS *CreateFile(const char* filename);
void WriteFile(FILEFS *file, uint8_t* data, int size, int offset);

char* GetMountStorage(uint32_t size);
void FinishMountStorage();

#endif
