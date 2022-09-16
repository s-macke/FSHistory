#include "stdarg.h"
#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"

#ifndef __wasm__
    #error "Wasm target only"
#endif


extern char __heap_base; // start of dynamic memory allocation (heap). Exported by clang

char *allocPointer = NULL;
//char *allocPointer = (char *) 0x500000;

extern void outputstr(const char *str);

void *malloc(wasm_size_t size) {
    if (allocPointer == NULL) { // First call: initialize
        allocPointer = &__heap_base;
    }
    void *p = allocPointer;
    allocPointer += size;
    return p;
}

// malloc, but sets the memory to 0
void *calloc(wasm_size_t nmemb, wasm_size_t size) {
    void *ptr = malloc(nmemb * size);
    memset(ptr, 0, nmemb * size);
    return ptr;
}

void free(void *ptr) {
    // Do nothing
}

void *memcpy(void *dest, const void *src, unsigned long n) {
    // Typecast src and dest addresses to (char *)
    char *csrc = (char *) src;
    char *cdest = (char *) dest;

    // Copy contents of src[] to dest[]
    for (int i = 0; i < n; i++)
        cdest[i] = csrc[i];

    return dest;
}

void *memset(void *s, int c, wasm_size_t n) {
    uint8_t *d = (uint8_t *) s;
    for (wasm_size_t i = 0; i < n; i++) d[i] = c;
    return s;
}

int memcmp(const void *buf1, const void *buf2, unsigned long count) {
    if(!count)
        return(0);

    while(--count && *(char*)buf1 == *(char*)buf2 ) {
        buf1 = (char*)buf1 + 1;
        buf2 = (char*)buf2 + 1;
    }

    return (*((unsigned char*)buf1) - *((unsigned char*)buf2));
}

int toupper(int c) {
    return (c >= 'a' && c <= 'z') ? c - 0x20 : c;
}

float fminf(float arg, float arg2) {
    return arg < arg2 ? arg : arg2;
}

float fmaxf(float arg, float arg2) {
    return arg > arg2 ? arg : arg2;
}

float modff(float input, float *arg2) {
    *arg2 = (int) input;
    return input - *arg2;
}

int sprintf(char *str, const char *format, ...) {
    str[0] = 0;
    return 0;
}

// Very basic printf implementation.
int printf(const char *format, ...) {
    /*
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    outputstr(buffer);
    return 0;
     */
    outputstr(format);
    return 0;
}
/*
int vsprintf (char *buffer, const char *format, __gnuc_va_list arg) {
    int i = 0;

    return 0;
}
*/
int puts(const char *s) {
    outputstr(s);
    outputstr("\n");
    return 0;
}


int putchar(int c) {
    char buffer[2];
    buffer[0] = c;
    buffer[1] = 0;
    outputstr(buffer);
    return 0;
}

wasm_size_t strlen(const char *str) {
    const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

char *strcpy(char *dest, const char *src) {
    char *temp = dest;
    while ((*dest++ = *src++));
    return temp;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *) s1 - *(const unsigned char *) s2;
}

int strncmp(const char *s1, const char *s2, wasm_size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) {
        return 0;
    } else {
        return (*(unsigned char *) s1 - *(unsigned char *) s2);
    }
}

char *strcat(char *dest, const char *src) {
    char *ptr = dest + strlen(dest);

    while (*src != 0)
        *ptr++ = *src++;

    *ptr = 0;
    return dest;
}
