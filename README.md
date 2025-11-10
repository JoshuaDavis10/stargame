# stargame

## software rendered game engine

## to build with build scripts, you need gcc.
## otherwise all you need is a C compiler and some way to link against xcb (for platform layer)
## - compile game.c to a shared object (.so) file
## - compile linux_window.c, linked against xcb and make sure the game .so file is in the path (see build*.bash files for examples)
## - pass flag -DPROFILE=x, where x is 0 if you don't want profiling, and x is 1 if you do want profiling info

## run game from root directory (./bin/game) since dlopen() assumes this (it attempts to open ./build/libgame.so)
