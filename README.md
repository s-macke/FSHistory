FSHistory
=========

Play and Enjoy the History of Microsoft Flight Simulator

# **[Click to play][project demo]**

![web demonstration](images/slides.gif)

Do you know how many Flight Simulators Microsoft developed? It has been more than [ten](https://en.wikipedia.org/wiki/Microsoft_Flight_Simulator) with [Microsoft Flight Simulator (2020)](https://de.wikipedia.org/wiki/Microsoft_Flight_Simulator_(2020)) being the latest version that has been released. 

See how it all began. Play the first four flight simulator games Microsoft developed from 1982 to 1989. It has full mouse and keyboard control and can be even enjoyed on your mobile. However a full IBM style keyboard is recommended.

# Technical aspects

Always when I start a new project I wonder what programming language I should use. Most of the time the requirements are the same. It must be fast, typed and the result must presentable on a website. Especially I would like to keep it as simple as possible. C is usually my language of choice when the logic doesn't get too complicated. And this is exactly the case for such emulators. Low level hardware emulation on a low level language.

C is also the natural choice for WebAssembly as the language features fit perfect. It is like you write an ordinary C-library.

clang of the [LLVM project](https://llvm.org/) offers the ideal compiler infrastructure to create customized WebAssembly files without any additional runtime code. As such my code implements parts of the libc such as printf, malloc and memcpy. Just enough to run the emulator. The result is a project that compiles naturally as a native executable and runs in the browser with just is a few hundred lines of glue code.

In short, this is the technical feature list

 * Developed in C.
 * Emulates an 8086 and features from a 286 and a 386 CPU.
 * Emulates keyboard, mouse, graphics, interrupt controller, timer controller.
 * Implements DOS and BIOS functions similar to DOSBOX.
 * Compiles to an ordinary executable by using the Simple DirectMedia Layer (SDL).
 * Compiles to WebAssembly via the LLVM compiler by using my own trivial libc implementation.

## How To Build

To build the native executable make sure gcc and SDL2 is installed and run

```
m̀ake
```

To compile the webassembly file install clang and run
```
./scripts/compile_wasm.sh
```

## License

The software part of the repository is under the MIT license. Please read the license file for more information. The game data in the data directory is not part of this license

[project demo]: https://s-macke.github.io/FSHistory/

