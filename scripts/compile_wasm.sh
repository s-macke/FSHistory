#stop on error
set -e

#generate wasm file
make clean
rm -f fshistory.wasm

#-flto
#FEATURES="-mbulk-memory -msign-ext"
#FEATURES="-msign-ext"
make fshistory.wasm CC=clang LDFLAGS="--target=wasm32 $FEATURES" CFLAGS="-Os --target=wasm32 -DFS4 -Isrc/wasm_libc_wrapper -Werror -DUNSAFE_RAM -Wimplicit-fallthrough $FEATURES"

wasm-strip fshistory.wasm

wasm2wat fshistory.wasm | grep 'import\|export\|(global'

