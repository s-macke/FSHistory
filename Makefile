CC = gcc
CFLAGS = -O2 -DFS4 -DUNSAFE_RAM -Werror
LDFLAGS = -lSDL2 -lm

OBJS = main.o mz.o ram.o debugger.o cpu.o vga.o ports.o dos.o alloc.o keyb.o   \
	sdl.o screen.o rom.o fs.o bios.o fonts.o pic.o pit.o mouse.o      \
	clock.o fs4.o exit_strategy.o disk.o multiplex.o compare.o ems.o

all: fshistory data/fs1.fs data/fs2.fs data/fs3.fs data/fs4.fs

fshistory: ${OBJS}
	${CC} -o fshistory ${OBJS} ${LDFLAGS}

fshistory.wasm: ${OBJS} libc.o
	wasm-ld --import-memory --no-entry --strip-all --gc-sections --allow-undefined	\
	--export=SetFSVersion                                   \
	--export=GetMountStorage                                \
	--export=FinishMountStorage                             \
	--export=Init                                           \
	--export=Run                                            \
	--export=ScreenGet                                      \
	--export=UpdateScreen                                   \
	--export=VGA_GetVideoMode                               \
	--export=KeyDown                                        \
	--export=KeyUp                                          \
	--export=MouseButtonDown                                \
	--export=MouseButtonUp                                  \
	--export=MouseMotion                                    \
	-o fshistory.wasm ${OBJS} libc.o

data/fs1.fs: data/fs1.fs.bz2
	bzip2 -dk data/fs1.fs.bz2

data/fs2.fs: data/fs2.fs.bz2
	bzip2 -dk data/fs2.fs.bz2

data/fs3.fs: data/fs3.fs.bz2
	bzip2 -dk data/fs3.fs.bz2

data/fs4.fs: data/fs4.fs.bz2
	bzip2 -dk data/fs4.fs.bz2

libc.o: src/wasm_libc_wrapper
	${CC} ${CFLAGS} -c src/wasm_libc_wrapper/libc.c

main.o: src/main.c
	${CC} ${CFLAGS} -c src/main.c

compare.o: src/debug/compare.c
	${CC} ${CFLAGS} -c src/debug/compare.c

fs4.o: src/fs4.c
	${CC} ${CFLAGS} -c src/fs4.c

screen.o: src/devices/screen.c src/devices/screen.h
	${CC} ${CFLAGS} -c src/devices/screen.c

sdl.o: src/sdl.c src/sdl.h
	${CC} ${CFLAGS} -c src/sdl.c

ram.o: src/devices/ram.c src/devices/ram.h
	${CC} ${CFLAGS} -c src/devices/ram.c

cpu.o: src/cpu/cpu.c src/cpu/cpu.h src/cpu/arith.c src/cpu/repops.c src/cpu/flags.c src/cpu/helper.c src/cpu/opcode66.c
	${CC} ${CFLAGS} -c src/cpu/cpu.c

debugger.o: src/disasm/debugger.c src/disasm/debugger.h
	${CC} ${CFLAGS} -c src/disasm/debugger.c

vga.o: src/devices/vga.c src/devices/vga.h
	${CC} ${CFLAGS} -c src/devices/vga.c

ports.o: src/devices/ports.c src/devices/ports.h
	${CC} ${CFLAGS} -c src/devices/ports.c

pic.o: src/devices/pic.c src/devices/pic.h
	${CC} ${CFLAGS} -c src/devices/pic.c

pit.o: src/devices/pit.c src/devices/pit.h
	${CC} ${CFLAGS} -c src/devices/pit.c

dos.o: src/dos/dos.c src/dos/dos.h
	${CC} ${CFLAGS} -c src/dos/dos.c

alloc.o: src/dos/alloc.c src/dos/alloc.h
	${CC} ${CFLAGS} -c src/dos/alloc.c

mz.o: src/dos/mz.c src/dos/mz.h
	${CC} ${CFLAGS} -c src/dos/mz.c

keyb.o: src/devices/keyb.c src/devices/keyb.h
	${CC} ${CFLAGS} -c src/devices/keyb.c

mouse.o: src/devices/mouse.c src/devices/mouse.h
	${CC} ${CFLAGS} -c src/devices/mouse.c

clock.o: src/devices/clock.c src/devices/clock.h
	${CC} ${CFLAGS} -c src/devices/clock.c

rom.o: src/devices/rom.c src/devices/rom.h
	${CC} ${CFLAGS} -c src/devices/rom.c

bios.o: src/devices/bios.c src/devices/bios.h
	${CC} ${CFLAGS} -c src/devices/bios.c

fs.o: src/fs/fs.c src/fs/fs.h
	${CC} ${CFLAGS} -c src/fs/fs.c

fonts.o: src/devices/fonts.c src/devices/fonts.h
	${CC} ${CFLAGS} -c src/devices/fonts.c

exit_strategy.o: src/utils/exit_strategy.c src/utils/exit_strategy.h
	${CC} ${CFLAGS} -c src/utils/exit_strategy.c

disk.o: src/devices/disk.c src/devices/disk.h
	${CC} ${CFLAGS} -c src/devices/disk.c

multiplex.o: src/dos/multiplex.c src/dos/multiplex.h
	${CC} ${CFLAGS} -c src/dos/multiplex.c

ems.o: src/devices/ems.c src/devices/ems.h
	${CC} ${CFLAGS} -c src/devices/ems.c

.PHONY: clean
clean:
	rm -f *.o fshistory
