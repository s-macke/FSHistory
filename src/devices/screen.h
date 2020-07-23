#ifndef SCREEN_H
#define SCREEN_H

#include "../wasm_libc_wrapper/stdint.h"

// Screen dimension constants
#define SCREEN_WIDTH  720
#define SCREEN_HEIGHT 400

extern uint32_t *pixels;
extern float *zbuffer;

void ScreenInit();
void ScreenClear();

#endif
