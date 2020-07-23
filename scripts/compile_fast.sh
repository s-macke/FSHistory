gcc -Ofast -DFS4 -DUNSAFE_RAM -ofshistory src/*.c src/cpu/cpu.c src/devices/*.c src/disasm/*.c src/dos/*.c src/fs/*.c src/utils/*.c  -lSDL2

#cat src/*.c src/cpu/*.h src/cpu/*.c src/devices/*.c src/disasm/*.c src/dos/*.c src/fs/*.c src/utils/*.c > single.c
#gcc -Ofast -DFS4 single.c -lSDL2
