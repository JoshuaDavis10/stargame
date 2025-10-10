#!/usr/bin/bash

gcc -std=c89 -fsanitize=address -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function -c game.c -fpic -o game.o -lm
gcc -shared game.o -o temp.so
mv temp.so libgame.so
