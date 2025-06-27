@echo off
echo === Building C99 Compiler for TinyCC Independence ===
echo.

set TCC=external\tcc-win\tcc\tcc.exe

echo Step 1: Building C99 compiler components...
echo Building x64 code generator...
%TCC% -c src\tools\x64_codegen.c -o bin\x64_codegen.o -Isrc\runtime -Isrc\tools

echo Building program_c99 code generator...
%TCC% -c src\tools\program_c99_codegen.c -o bin\program_c99_codegen.o -Isrc\runtime -Isrc\tools

echo Building standalone C compiler...
%TCC% -c src\tools\standalone_c_compiler.c -o bin\standalone_c_compiler.o -Isrc\runtime -Isrc\tools

echo Building main C99 compiler...
%TCC% -c src\tools\program_c99.c -o bin\program_c99.o -Isrc\runtime -Isrc\tools

echo.
echo Step 2: Linking C99 compiler executable...
%TCC% -o bin\c99_compiler.exe bin\program_c99.o bin\program_c99_codegen.o bin\x64_codegen.o bin\standalone_c_compiler.o src\tools\c2astc.c -Isrc\runtime -Isrc\tools

echo.
echo Step 3: Testing C99 compiler...
echo Testing basic compilation...
bin\c99_compiler.exe

echo.
echo Testing self-bootstrap capability...
bin\c99_compiler.exe --self-bootstrap

echo.
echo Step 4: Converting C99 compiler to ASTC format...
echo Converting C99 compiler to ASTC...
bin\tool_c2astc.exe src\tools\program_c99.c bin\program_c99.astc

echo Converting to runtime format...
bin\tool_astc2rt.exe bin\program_c99.astc bin\program_c99.rt

echo.
echo Step 5: Testing C99 compiler in ASTC environment...
echo Testing with evolver0 loader...
bin\evolver0_loader.exe -v bin\runtimex64_64_enhanced.rt bin\program_c99.astc

echo.
echo === C99 Compiler Build Complete ===
echo.
echo Generated files:
echo   - bin\c99_compiler.exe (standalone C99 compiler)
echo   - bin\program_c99.astc (ASTC format C99 compiler)
echo   - bin\program_c99.rt (runtime format)
echo.
echo Next steps:
echo   1. Test compiling evolver0 components with C99 compiler
echo   2. Verify TinyCC independence
echo   3. Implement complete self-bootstrap
echo.
pause
