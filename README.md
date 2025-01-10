# nes

NES emulator written in three weeks using C, Raylib and ImGui. Prebuilt static libraries for both Raylib (using the SDL2 backend for improved controller support) and ImGui (in combination with the sources, which was not my intention, but we can't all be perfect). 

## Requirements for building
- gcc, g++ and ar (unsurprisingly)
- make
- SDL2

Is this list too short? Really a shame how good Raylib is at everything. 
For building on an inferior operating system one must rebuild the C++ bridge (see nested Makefile) and Raylib + SDL2, which will probably require rewriting the Makefiles anyway
