#stop on error
set -e

#generate Wasm file
make clean
rm -f fshistory.wasm

# more information about the clang WebAssembly target: https://lld.llvm.org/WebAssembly.html#exports

#-flto
#FEATURES="-mbulk-memory -msign-ext"
#FEATURES="-msign-ext"
make fshistory.wasm CC=clang LDFLAGS="--target=wasm32 $FEATURES" CFLAGS="-Os --target=wasm32 -DFS4 -Isrc/wasm_libc_wrapper -Werror -DUNSAFE_RAM -Wimplicit-fallthrough $FEATURES"

# from wabt
wasm-strip fshistory.wasm

# from binaryen
wasm-opt -Os fshistory.wasm -o fshistory.wasm

wasm2wat fshistory.wasm | grep 'import\|export\|(global'
