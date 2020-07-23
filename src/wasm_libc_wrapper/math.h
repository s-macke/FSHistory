#ifndef MATH_H
#define MATH_H

#ifndef __wasm__
	#include <math.h>
#else
    #define M_PI 3.14159265358979323846

    float sqrtf(float arg);
    float cosf(float arg);
    float sinf(float arg);
    float tanf(float arg);
    float atanf(float arg);
    float fabsf(float arg);
    float floorf(float arg);
    float modff(float arg, float* arg2);
    float fminf(float arg, float arg2);
    float fmaxf(float arg, float arg2);
#endif

#endif
