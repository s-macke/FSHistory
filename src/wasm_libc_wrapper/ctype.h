#ifndef CTYPE_H
#define CTYPE_H

#ifndef __wasm__
    #error "Wasm target only"
#endif


int toupper(int c); // returns the upper case version of the character c

#endif
