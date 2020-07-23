#ifndef STDLIB_H
#define STDLIB_H

#ifndef __wasm__
	#include <stdlib.h>
#else

#define NULL 0

typedef long size_t;

void *malloc(unsigned long size);
void *memcpy(void *dest, const void *src, unsigned long n);

void exit(int status);

#endif
#endif
