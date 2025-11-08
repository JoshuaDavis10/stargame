#!/usr/bin/bash

export LD_LIBRARY_PATH=/home/josh/dev/stargame:$LD_LIBRARY_PATH
gcc -std=c89 -fsanitize=address -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable linux_x_window.c -o game -lxcb
