#ifndef FLOAT_H
#define FLOAT_H

#ifndef __wasm__
	#include<float.h>
#else
	#define FLT_MAX 3.402823e+38
    #define FLT_MIN 1.175494e-38
#endif

#endif
