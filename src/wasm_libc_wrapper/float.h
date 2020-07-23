#ifndef FLOAT_H
#define FLOAT_H

#ifndef __wasm__
	#include<float.h>
#else
	#define FLT_MAX 3.402823e+38
#endif

#endif
