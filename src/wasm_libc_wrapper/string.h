#ifndef STRING_H
#define STRING_H

#ifndef __wasm__
	#include <string.h>
#else
	typedef unsigned long wasm_size_t;
	typedef long size_t;

	void *memset(void *s, int c, wasm_size_t n);
	int memcmp(const void *s1, const void *s2, unsigned long n);
	char *strcpy(char *dest, const char *src);
	char *strcat(char *dest, const char *src);
	wasm_size_t strlen(const char *s);
	int strncmp(const char *s1, const char *s2, wasm_size_t n);
	int strcmp(const char *s1, const char *s2);

#endif

#endif
