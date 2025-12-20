@echo off
SET build_directory=build
SET binary_directory=bin
SET source_directory=engine_code

mkdir %build_directory%
mkdir %binary_directory%

SET cc=clang-cl
SET cflags=/fsanitize=address /O1 /W3 -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable

REM NOTE if we want .asm to do the clear background like we have working on Linux, we'll need a separate .asm file that uses the windows ABI
REM Additionally, we'll have to link it with the .dll, which I'm not super sure how to do atm

echo compiling template_game.c...
%cc% %cflags% /DPROFILER=1 %source_directory%/template_game.c /LD -o %build_directory%/template_game.dll
echo compiling win32_platform.c...
%cc% %cflags% %source_directory%/win32_platform.c -o %binary_directory%/template_game.exe user32.lib /link /SUBSYSTEM:CONSOLE 
echo compilation complete.
