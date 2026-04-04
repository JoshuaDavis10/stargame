@echo off
SET build_directory=build
SET binary_directory=bin
SET app_directory=app_code
SET platform_directory=platform_code

mkdir %build_directory%
mkdir %binary_directory%

SET cc=clang-cl
SET cflags=/fsanitize=address /O1 /W3 -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable

REM NOTE if we want .asm to do the clear background like we have working on Linux, we'll need a separate .asm file that uses the windows ABI
REM Additionally, we'll have to link it with the .dll, which I'm not super sure how to do atm

REM TODO: need to do like a compile to a temp.dll then move to the actual name so that engine can potentially load the .dll when it is changed (i.e. hot reloading)

echo compiling %1.c...
%cc% %cflags% /DPROFILER=1 %app_directory%/%1.c /LD -o %build_directory%/temp_%1.dll
move %build_directory%\temp_%1.dll %build_directory%\%1.dll


echo compiling win32_platform.c...
%cc% %cflags% %platform_directory%/win32/win32_platform.c -o %binary_directory%/temp_template_game.exe user32.lib gdi32.lib /link /SUBSYSTEM:CONSOLE 
move %binary_directory%\temp_template_game.exe %binary_directory%\template_game.exe
move %binary_directory%\temp_template_game.pdb %binary_directory%\template_game.pdb
echo compilation complete.
