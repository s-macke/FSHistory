#include "../wasm_libc_wrapper/stddef.h"
#include "../wasm_libc_wrapper/string.h"
#include "../wasm_libc_wrapper/stdio.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../utils/exit_strategy.h"
#include"fs.h"

static char* fsdata = NULL;
static uint32_t fssize = 0;

#define MAX_FILES 1024

static FILEFS files[MAX_FILES];
static int nfiles;

void toLowerCase(char *pstr) {
    for (char *p = pstr; *p; ++p) {
        *p = *p >= 'A' && *p <= 'Z' ? *p | 0x60 : *p;
    }
}

char* GetMountStorage(uint32_t size) {
    fsdata = malloc(size);
    fssize = size;
    return fsdata;
}

void FillFileFS(FILEFS *filefs, char* ptr) {
    filefs->filename = ptr;
    filefs->size = *(int32_t*)(ptr+256);
    filefs->data = ptr+256+4;
}

void FinishMountStorage() {

    char* ptr = fsdata;
    const char *ptrend = fsdata + fssize;

    memset(files, 0, sizeof(files));
    nfiles = 0;

    while (ptr < ptrend) {
        FillFileFS(&files[nfiles], ptr);
        ptr += 256 + 4 + files[nfiles].size;
        nfiles++;
        if (nfiles >= MAX_FILES) {
            printf("Error: Number of files exceed maximum\n");
            exit_or_restart(1);
        }
    }
    printf("Loaded files: %i\n", nfiles);
}

FILEFS *CreateFile(const char* filename) {
    files[nfiles].size = 0;
    files[nfiles].data = NULL;
    files[nfiles].filename = malloc(256);
    strcpy(files[nfiles].filename, filename);
    toLowerCase(files[nfiles].filename);

    nfiles++;
    return &files[nfiles-1];
}

void WriteFile(FILEFS *file, uint8_t* data, int size, int offset) {
    int newsize = offset+size;
    if (file->size < newsize) {
        char* olddata = file->data;
        unsigned int oldsize = file->size;

        file->data = malloc(newsize);
        file->size = newsize;

        if (olddata != NULL) memcpy(file->data, olddata, oldsize);
    }
    memcpy(&file->data[offset], data, size);
}


int EndsWith(const char *str, const char *suffix) {
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

FILEFS *FindFile(const char *_filename, uint16_t *index) {
    char filename[256];
    memset(filename, 0, sizeof(filename));
    printf("Find file '%s'\n", _filename);
    strcpy(filename, _filename);
    toLowerCase(filename);

    for(; (*index) < nfiles; (*index)++) {
        if (filename[0] == '*') {
            if (EndsWith(files[*index].filename, &filename[1])) return &files[*index];
        } else {
            //printf("compare with '%s' '%s'\n", filename, fs[*index].filename);
            if (strcmp(filename, files[*index].filename) == 0) return &files[*index];
        }
    }
    return NULL;
}
