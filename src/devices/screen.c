#include "../wasm_libc_wrapper/stdint.h"
#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/float.h"

#include "screen.h"

uint32_t *pixels = 0;
float *zbuffer = 0;

void ScreenInit() {
    if (pixels == 0) {
        pixels = malloc(sizeof(int32_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
        zbuffer = malloc(sizeof(float) * SCREEN_WIDTH * SCREEN_HEIGHT);
    }
    ScreenClear();
}

void ScreenClear() {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        pixels[i] = 0xFF000000;
        zbuffer[i] = FLT_MAX;
    }
}

uint32_t *ScreenGet() {
    return pixels;
}

