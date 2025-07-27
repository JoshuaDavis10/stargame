#!/usr/bin/bash

gcc -std=c89 -Wall -Wextra -Wno-unused-variable -Wno-unused-but-set-variable linux_x_window.c -o prog -lxcb
gcc -std=c89 -Wall -Wextra -Wno-unused-variable -Wno-unused-but-set-variable test.c -o test -lxcb
