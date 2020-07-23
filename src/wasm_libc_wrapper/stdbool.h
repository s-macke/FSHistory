#ifndef STDBOOL_H
#define STDBOOL_H

#ifndef __wasm__
	#include<stdbool.h>
#else

typedef int bool;
#define true 1
#define false 0

#endif

#endif


