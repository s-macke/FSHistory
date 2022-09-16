#ifndef CTYPE_H
#define CTYPE_H

#ifndef __wasm__
	#include <ctype.h>
#else

int toupper(int c); // returns the upper case version of the character c

#endif

#endif
