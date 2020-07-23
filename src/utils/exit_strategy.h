#ifndef EXIT_STRATEGY_H
#define EXIT_STRATEGY_H

#include "../wasm_libc_wrapper/stdbool.h"

void exit_or_restart(int status);

bool isRestart();

#endif
