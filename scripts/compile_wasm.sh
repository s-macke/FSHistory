#stop on error
set -e

#generate wasm file
make clean
rm -f fshistory.wasm
make fshistory.wasm CC=clang LDFLAGS="--target=wasm32" CFLAGS="-Os --target=wasm32 -DFS4 -Werror -DUNSAFE_RAM -Wimplicit-fallthrough"

wasm-gc fshistory.wasm
wasm-strip fshistory.wasm

wasm2wat fshistory.wasm | grep 'import\|export'
