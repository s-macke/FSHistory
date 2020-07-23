#ifndef SDL_H
#define SDL_H

#include "sdl.h"

void SDLInit();
void SDLQuit();
void SDLUpdate();
void SDLDelay(int ms);
void SDLWaitAndQuit(int ms);
void SDLPoll();
void SDLImageExtract();

#endif
