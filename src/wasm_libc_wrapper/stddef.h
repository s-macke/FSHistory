#ifndef STDDEF_H
#define STDDEF_H

#ifndef __wasm__
	#include <stddef.h>
#else
    #define NULL 0
#endif

#endif
