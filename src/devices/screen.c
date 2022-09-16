#include <stdint.h>
#include <stdlib.h>

#include "screen.h"

uint32_t *pixels = NULL;

void ScreenInit() {
    if (pixels == NULL) {
        pixels = malloc(sizeof(int32_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
    }
    ScreenClear();
}

void ScreenClear() {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        pixels[i] = 0xFF000000;
    }
}

uint32_t *ScreenGet() {
    return pixels;
}

