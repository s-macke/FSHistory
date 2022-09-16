#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

// Screen dimension constants
#define SCREEN_WIDTH  720
#define SCREEN_HEIGHT 400

extern uint32_t *pixels;

void ScreenInit();
void ScreenClear();

#endif
