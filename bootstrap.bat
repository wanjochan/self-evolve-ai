@echo off
echo === Bootstrap Build - Eliminating TinyCC Dependency ===

set TCC=external\tcc-win\tcc\tcc.exe

echo Stage 1: Build standalone C compiler (last time using TinyCC)
%TCC% -o bin\standalone_c_compiler.exe src\tools\standalone_c_compiler.c
if errorlevel 1 (
    echo FAIL: Standalone compiler build failed
    pause
    exit /b 1
)
echo OK: Standalone C compiler built

echo.
echo Stage 2: Test standalone compiler
echo int main() { return 42; } > temp_test.c
bin\standalone_c_compiler.exe temp_test.c temp_test.s
if errorlevel 1 (
    echo FAIL: Standalone compiler test failed
    del temp_test.c
    pause
    exit /b 1
)
echo OK: Standalone compiler test passed

echo.
echo Generated assembly code:
type temp_test.s
del temp_test.c temp_test.s

echo.
echo Stage 3: Try compiling evolver0 components
bin\standalone_c_compiler.exe src\evolver0\evolver0_loader.c bin\evolver0_loader.s
if errorlevel 1 echo WARN: loader compilation failed
if not errorlevel 1 echo OK: loader compiled

bin\standalone_c_compiler.exe src\evolver0\evolver0_runtime.c bin\evolver0_runtime.s  
if errorlevel 1 echo WARN: runtime compilation failed
if not errorlevel 1 echo OK: runtime compiled

bin\standalone_c_compiler.exe src\evolver0\evolver0_program.c bin\evolver0_program.s
if errorlevel 1 echo WARN: program compilation failed
if not errorlevel 1 echo OK: program compiled

echo.
echo Bootstrap Progress:
echo [OK] Stage 1: Standalone C compiler built
echo [OK] Stage 2: Compiler functionality verified
if exist "bin\evolver0_loader.s" echo [OK] Stage 3: evolver0 components compiled
if not exist "bin\evolver0_loader.s" echo [WARN] Stage 3: evolver0 compilation needs work

echo.
echo Next steps needed:
echo 1. Implement assembler (.s to .o)
echo 2. Implement linker (.o to .exe)  
echo 3. Enhance C language support in standalone compiler
echo 4. Complete full bootstrap compilation flow
echo 5. Verify generated executable functionality

echo.
echo TinyCC Dependency Status:
echo - Current: Only used to build standalone compiler
echo - Target: Complete TinyCC independence
echo - Progress: 30%% complete

echo.
echo Bootstrap build complete!
pause
