#ifndef __wasm__

#include <SDL2/SDL.h>

#endif

#include "devices/screen.h"
#include "devices/keyb.h"
#include "devices/mouse.h"
#include "sdl.h"

#ifdef __wasm__

void SDLInit() {}
void SDLWait(int ms) {}
void SDLWaitInfinite() {}
void SDLQuit() {}
void SDLUpdate() {}
void SDLPoll() {}
void SDLDelay(int ms) {}

#else

SDL_Window *window = NULL;
SDL_Surface *screen = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;

void SDLInit() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("DOS", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
/*
    screen = SDL_GetWindowSurface(window);
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF));
    SDL_UpdateWindowSurface(window);
*/
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH,
                                SCREEN_HEIGHT);
}

uint8_t ConvertScanCode(uint8_t scancode) {
    //printf(" SDL scancode: 0x%x\n", scancode);
    switch (scancode) {
        case 0x1e:
            return 0x02; // 1
        case 0x1f:
            return 0x03; // 2
        case 0x20:
            return 0x04; // 3
        case 0x21:
            return 0x05; // 4
        case 0x22:
            return 0x06; // 5
        case 0x23:
            return 0x07; // 6
        case 0x24:
            return 0x08; // 7
        case 0x25:
            return 0x09; // 8
        case 0x26:
            return 0x0a; // 9
        case 0x27:
            return 0x0b; // 0
        case 0x04:
            return 0x1e; // a
        case 0x05:
            return 0x30; // b
        case 0x06:
            return 0x2e; // c
        case 0x07:
            return 0x20; // d
        case 0x08:
            return 0x12; // e
        case 0x09:
            return 0x21; // f
        case 0x0a:
            return 0x22; // g
        case 0x0b:
            return 0x23; // h
        case 0x0c:
            //SDLImageExtract();
            return 0x17; // i
        case 0x0d:
            return 0x24; // j
        case 0x0e:
            return 0x25; // k
        case 0x0f:
            return 0x26; // l
        case 0x10:
            return 0x32; // m
        case 0x11:
            return 0x31; // n
        case 0x12:
            return 0x18; // o
        case 0x13:
            return 0x19; // p
        case 0x14:
            return 0x10; // q
        case 0x15:
            return 0x13; // r
        case 0x16:
            return 0x1f; // s
        case 0x17:
            return 0x14; // t
        case 0x18:
            return 0x16; // u
        case 0x19:
            return 0x2f; // v
        case 0x1a:
            return 0x11; // w
        case 0x1b:
            return 0x2d; // x
        case 0x1d:
            return 0x2c; // y
        case 0x1c:
            return 0x15; // z
        case 0xe1:
            return 0x2a; // left shift
        case 0xe5:
            return 0x36; // right shift
        case 0x2c:
            return 0x39; // space
        case 0x28:
            return 0x1c; // enter
        case 0x2a:
            return 0xe; // backspace
        case 0xe0:
            return 0x1d; // left ctrl
        case 0xe4:
            return 0x1d; // right ctrl
        case 0x29:
            return 0x01; // esc
        case 0x52:
            return 0x48; // cursor up
        case 0x51:
            return 0x50; // cursor down
        case 0x50:
            return 0x4b; // cursor left
        case 0x4f:
            return 0x4d; // cursor right
        case 0x3a:
            return 0x3b; // F1
        case 0x3b:
            return 0x3c; // F2
        case 0x3c:
            return 0x3d; // F3
        case 0x3d:
            return 0x3e; // F4
        case 0x3e:
            return 0x3f; // F5
        case 0x3f:
            return 0x40; // F6
        case 0x40:
            return 0x41; // F7
        case 0x41:
            return 0x42; // F8
        case 0x42:
            return 0x43; // F9
        case 0x43:
            return 0x44; // F10
        case 0x44:
            return 0x57; // F11
        case 0x45:
            return 0x58; // F12

        case 0x49:
            return 0x52; // ins
        case 0x4a:
            return 0x47; // home
        case 0x4b:
            return 0x49; // pg up
        case 0x4c:
            return 0x53; // del
        case 0x4d:
            return 0x4f; // end
        case 0x4e:
            return 0x51; // pg down

        case 0x53:
            return 0x45; // num lock
        case 0x54:
            return 0x35; // /
        case 0x55:
            return 0x37; // *
        case 0x56:
            return 0x4a; // -
        case 0x57:
            return 0x4e; // +
        case 0x58:
            return 0x58; // enter

        case 0x2d:
            return 0x0c; // -
        case 0x2e:
            return 0x0d; // =

        case 0x2f:
            return 0x1a; // [
        case 0x30:
            return 0x1b; // ]

        case 0x33:
            return 0x27; // ;
        case 0x34:
            return 0x28; // '
        case 0x31:
            return 0x2b; // \

        case 0x36:
            return 0x33; // ,
        case 0x37:
            return 0x34; // .
        case 0x38:
            return 0x35; // /

        default:
            return 0x0;
    }
}

void SDLPoll() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                KeyDown(ConvertScanCode(event.key.keysym.scancode));
                printf(" Name: %s keycode: 0x%x scancode: 0x%x\n", SDL_GetKeyName(event.key.keysym.sym),
                       event.key.keysym.sym, event.key.keysym.scancode);
                break;

            case SDL_KEYUP:
                KeyUp(ConvertScanCode(event.key.keysym.scancode));
                break;

            case SDL_MOUSEMOTION:
                //printf("mouse motion %i %i\n", event.motion.x, event.motion.y);
                MouseMotion(event.motion.x, event.motion.y);
                break;

            case SDL_MOUSEBUTTONDOWN:
                MouseButtonDown(event.button.button);
                break;

            case SDL_MOUSEBUTTONUP:
                MouseButtonUp(event.button.button);
                break;

            case SDL_QUIT:  // SDL_QUIT  int ein schliessen des windows
                SDLQuit();
                exit(1);
                break;

            default:
                break;
        }
    }
}

void SDLWaitAndQuit(int ms) {
    SDL_Event event;
    while (SDL_WaitEventTimeout(&event, ms)) {
        if (event.type == SDL_QUIT) {
            SDLQuit();
            exit(1);
        }
    }
}

void SDLWaitInfinite() {
    SDL_Event event;
    while (1) {
        while (SDL_PollEvent(&event)) {
            //printf("%i\n", event.type);
            if (event.type == SDL_QUIT) return;
        }
    }
}

void SDLQuit() {
    //SDLWaitInfinite();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDLUpdate() {
    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void SDLImageExtract() {
    uint16_t index = 0;
    printf("Extract image\n");
    FILE *fp = fopen("output.pbm", "w");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file\n");
        exit(1);
    }
    fprintf(fp, "P3\n%i %i\n255\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    for (int j = 0; j < SCREEN_HEIGHT; j++) {
        for (int i = 0; i < SCREEN_WIDTH; i++) {
            uint32_t col = pixels[j*SCREEN_WIDTH+i];
            fprintf(fp, "%i %i %i ", (col >> 0) & 255, (col >> 8) & 255, (col >> 16) & 255);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void SDLDelay(int ms) {
    SDL_Delay(ms);
}

#endif
