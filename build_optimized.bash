#!/usr/bin/bash

build_directory=build
binary_directory=bin
source_directory=engine_code

gcc -std=c89 -O3 -DPROFILER=$1 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -c $source_directory/game.c -fpic -o $build_directory/game.o -lm
gcc -shared $build_directory/game.o -o $build_directory/temp.so -lm
mv $build_directory/temp.so $build_directory/libgame.so

export LD_LIBRARY_PATH=/home/josh/dev/stargame/$build_directory:$LD_LIBRARY_PATH
gcc -std=c89 -O3 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable $source_directory/linux_x_window.c -o $binary_directory/game -lxcb
