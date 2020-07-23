#ifndef STDINT_H
#define STDINT_H

#ifndef __wasm__
	#include <stdint.h>
#else
	typedef unsigned short uint16_t;
	typedef signed short   int16_t;
	typedef unsigned int   uint32_t;
	typedef unsigned long long int   uint64_t;
	typedef signed int     int32_t;
	typedef unsigned char  uint8_t;
	typedef signed char    int8_t;
#endif

#endif
