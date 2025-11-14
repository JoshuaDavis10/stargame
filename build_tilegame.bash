#!/usr/bin/bash

build_directory=build
binary_directory=bin
source_directory=engine_code

nasm -f elf64 $source_directory/clear_background.asm -o $build_directory/clear_background.o
gcc -std=c89 -O1 -DPROFILER=$1 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -c $source_directory/tilegame.c -fpic -o $build_directory/tilegame.o -lm
gcc -shared $build_directory/tilegame.o $build_directory/clear_background.o -o $build_directory/temp.so -lm
mv $build_directory/temp.so $build_directory/libtilegame.so

export LD_LIBRARY_PATH=/home/josh/dev/stargame/$build_directory:$LD_LIBRARY_PATH
gcc -std=c89 -O1 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable $source_directory/linux_x_window.c -o $binary_directory/tilegame -lxcb
