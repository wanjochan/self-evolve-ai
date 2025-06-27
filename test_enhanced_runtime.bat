@echo off
echo === Testing Enhanced Runtime with libc Forward System ===
echo.

set TCC=external\tcc-win\tcc\tcc.exe

echo Step 1: Building enhanced runtime components...
echo Building libc_forward library...
%TCC% -c src\runtime\libc_forward.c -o bin\libc_forward.o -Isrc\runtime

echo Building enhanced_astc_vm library...
%TCC% -c src\runtime\enhanced_astc_vm.c -o bin\enhanced_astc_vm.o -Isrc\runtime

echo Building enhanced runtime...
%TCC% -c src\evolver0\evolver0_runtime_enhanced.c -o bin\evolver0_runtime_enhanced.o -Isrc\runtime -Isrc\evolver0 -DSTANDALONE_TEST

echo Linking enhanced runtime test executable...
%TCC% -o bin\test_enhanced_runtime.exe bin\evolver0_runtime_enhanced.o bin\enhanced_astc_vm.o bin\libc_forward.o -Isrc\runtime

echo.
echo Step 2: Testing enhanced runtime...
echo Running standalone test...
bin\test_enhanced_runtime.exe test

echo.
echo Step 3: Building enhanced runtime as ASTC...
echo Converting enhanced runtime to ASTC format...
bin\tool_c2astc.exe src\evolver0\evolver0_runtime_enhanced.c bin\evolver0_runtime_enhanced.astc

echo Converting enhanced runtime ASTC to .rt format...
bin\tool_astc2rt.exe bin\evolver0_runtime_enhanced.astc bin\runtimex64_64_enhanced.rt

echo.
echo Step 4: Testing with evolver0_loader...
echo Testing enhanced runtime with loader...
bin\evolver0_loader.exe -v bin\runtimex64_64_enhanced.rt bin\evolver0_program.astc

echo.
echo === Enhanced Runtime Test Complete ===
echo.
echo Files generated:
echo   - bin\test_enhanced_runtime.exe (standalone test)
echo   - bin\evolver0_runtime_enhanced.astc (ASTC format)
echo   - bin\runtimex64_64_enhanced.rt (enhanced runtime)
echo.
echo Next steps:
echo   1. Verify libc forwarding works correctly
echo   2. Test with more complex ASTC programs
echo   3. Implement c99 compiler to replace TinyCC
echo.
pause
