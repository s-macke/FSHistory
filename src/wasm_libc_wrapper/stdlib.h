#ifndef STDLIB_H
#define STDLIB_H

#ifndef __wasm__
    #error "Wasm target only"
#endif

#include<stddef.h>

typedef long size_t;

void *malloc(unsigned long size);
void *memcpy(void *dest, const void *src, unsigned long n);
void *calloc(unsigned long nmemb, unsigned long size);
void free(void *ptr);

void exit(int status);

#endif
