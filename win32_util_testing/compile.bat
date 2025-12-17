@echo off
REM clang-cl generates .pdb files... I'mn not sure if they work in visual studio, I will have to try getting this setup in visual studio if I ever get really sick of printf debugging or smn
SET cc=clang-cl
REM had to add C:\Program Files\LLVM\lib\clang\21\lib\windows to path to get ASAN working
SET cflags=/fsanitize=address
echo compiling...
%cc% %cflags% main.c -o win32_util_test.exe 
echo compilation complete!
