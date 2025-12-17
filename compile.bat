@echo off
SET build_directory=build
SET binary_directory=bin
SET source_directory=engine_code

mkdir %build_directory%
mkdir %binary_directory%

SET cc=clang-cl
REM TODO more warning flags! plus, whichever ones you switched off on Linux, switch those off as well
SET cflags=/fsanitize=address /O1

SET build="tilegame"

REM TODO nasm the clear_background.asm

REM TODO how to do if statements in batch?

REM TODO compile it to a .dll
%cc% %cflags% /DPROFILER=0 -c %source_directory%/tilegame.c -o blah.exe
