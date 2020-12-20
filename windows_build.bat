rem To build in VS Code, open cmd terminal, run the vcvars32.bat command below, then execute this file.
rem "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"

set lib=%lib%;C:\SDL2\lib\x86
set include=%include%;C:\SDL2\include
bash.exe "./sdl2-config" --cflags
bash.exe "./sdl2-config" --libs
make
"C:\Program Files\LLVM\bin\clang-cl.exe" -LD -IWindows -Iinclude -I./ -Wno-deprecated-declarations --target=i386-pc-windows addins/test_addin1.c build/bin/SDL/sameboy.lib -o addins/

rem pause
