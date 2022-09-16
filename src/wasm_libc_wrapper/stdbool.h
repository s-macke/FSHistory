#ifndef STDBOOL_H
#define STDBOOL_H

#ifndef __wasm__
    #error "Wasm target only"
#endif

typedef int bool;
#define true 1
#define false 0

#endif


