#include "../wasm_libc_wrapper/stdlib.h"
#include "../wasm_libc_wrapper/stdio.h"

#include "../cpu/cpu.h"
#include "../sdl.h"
#include "exit_strategy.h"

static bool restart = false;

extern void UpdateScreen(); // hack

bool isRestart() {
    bool temp = restart;
    restart = false;
    return temp;
}

void exit_or_restart(int status) {
    printf("Exit program with status code %i\n", status);

#ifdef __wasm__
    if (status == 0) {
        SetHLTstate(); // enter HLT State of the CPU
        //restart = true;
    } else {
        SDLQuit();
        exit(status);
    }
#else
    UpdateScreen();
    //SDLWaitAndQuit(1000);
    SDLQuit();
    exit(status);
#endif

}
