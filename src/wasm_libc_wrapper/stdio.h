#ifndef STDIO_H
#define STDIO_H

#ifndef __wasm__
	#include<stdio.h>
#else
#endif
	int printf(const char *format, ...);
	int sprintf(char *str, const char *format, ...);
	int puts(const char *s);
#endif
