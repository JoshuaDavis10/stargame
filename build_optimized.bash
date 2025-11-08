#!/usr/bin/bash

gcc -std=c89 -O3 -DPROFILER=$1 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -c game.c -fpic -o game.o -lm
gcc -shared game.o -o temp.so -lm
mv temp.so libgame.so

export LD_LIBRARY_PATH=/home/josh/dev/stargame:$LD_LIBRARY_PATH
gcc -std=c89 -O3 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable linux_x_window.c -o game -lxcb
