@echo off
SET build_directory=build
SET binary_directory=bin
SET source_directory=engine_code

mkdir %build_directory%
mkdir %binary_directory%

SET cc=clang-cl
REM TODO more warning flags! plus, whichever ones you switched off on Linux, switch those off as well
SET cflags=/fsanitize=address /O1 /W3 -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable

REM TODO nasm the clear_background.asm

REM TODO how to do if statements in batch?

REM TODO compile it to a .dll
echo compiling template_game.c...
%cc% %cflags% /DPROFILER=1 %source_directory%/template_game.c /LD -o %build_directory%/template_game.dll
echo compiling win32_platform.c...
%cc% %cflags% %source_directory%/win32_platform.c -o %binary_directory%/template_game.exe
echo compilation complete.
