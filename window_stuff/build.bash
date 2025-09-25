#!/usr/bin/bash

gcc -std=c89 -fsanitize=address -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable game.c -c -fpic game.o -lm
gcc -shared game.o -o libgame.so
gcc -std=c89 -fsanitize=address -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable linux_x_window.c -o game -lxcb
