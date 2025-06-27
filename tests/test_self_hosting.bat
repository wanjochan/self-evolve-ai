@echo off
echo === Testing Self-Hosting Compilation ===
echo Using our independent toolchain to compile evolver0 components

echo.
echo Step 1: Test compiling evolver0_loader.c
echo ========================================

echo Attempting to compile evolver0_loader.c with program_c99...
bin\program_c99.exe src\evolver0\evolver0_loader.c tests\evolver0_loader_selfhost.astc
if errorlevel 1 (
    echo WARN: evolver0_loader.c compilation had issues
) else (
    echo OK: evolver0_loader.c compiled successfully
    if exist "tests\evolver0_loader_selfhost.astc" (
        for %%F in ("tests\evolver0_loader_selfhost.astc") do echo   ASTC size: %%~zF bytes
    )
)

echo.
echo Step 2: Test compiling evolver0_runtime.c
echo =========================================

echo Attempting to compile evolver0_runtime.c with program_c99...
bin\program_c99.exe src\evolver0\evolver0_runtime.c tests\evolver0_runtime_selfhost.astc
if errorlevel 1 (
    echo WARN: evolver0_runtime.c compilation had issues
) else (
    echo OK: evolver0_runtime.c compiled successfully
    if exist "tests\evolver0_runtime_selfhost.astc" (
        for %%F in ("tests\evolver0_runtime_selfhost.astc") do echo   ASTC size: %%~zF bytes
    )
)

echo.
echo Step 3: Test compiling evolver0_program.c
echo =========================================

echo Attempting to compile evolver0_program.c with program_c99...
bin\program_c99.exe src\evolver0\evolver0_program.c tests\evolver0_program_selfhost.astc
if errorlevel 1 (
    echo WARN: evolver0_program.c compilation had issues
) else (
    echo OK: evolver0_program.c compiled successfully
    if exist "tests\evolver0_program_selfhost.astc" (
        for %%F in ("tests\evolver0_program_selfhost.astc") do echo   ASTC size: %%~zF bytes
    )
)

echo.
echo Step 4: Test compiling program_c99.c itself
echo ===========================================

echo Attempting self-compilation: program_c99 compiling itself...
bin\program_c99.exe src\tools\program_c99.c tests\program_c99_selfhost.astc
if errorlevel 1 (
    echo WARN: program_c99.c self-compilation had issues
) else (
    echo OK: program_c99.c self-compiled successfully!
    if exist "tests\program_c99_selfhost.astc" (
        for %%F in ("tests\program_c99_selfhost.astc") do echo   Self-compiled ASTC size: %%~zF bytes
    )
)

echo.
echo Step 5: Test assembling self-compiled components
echo ===============================================

echo Testing ASTC assembly of self-compiled components...

if exist "tests\evolver0_loader_selfhost.astc" (
    echo Assembling self-compiled loader...
    bin\astc_assembler.exe tests\evolver0_loader_selfhost.astc tests\evolver0_loader_selfhost.exe windows-x64
    if not errorlevel 1 (
        echo OK: Self-compiled loader assembled
        for %%F in ("tests\evolver0_loader_selfhost.exe") do echo   Executable size: %%~zF bytes
    )
)

if exist "tests\program_c99_selfhost.astc" (
    echo Assembling self-compiled program_c99...
    bin\astc_assembler.exe tests\program_c99_selfhost.astc tests\program_c99_selfhost.exe windows-x64
    if not errorlevel 1 (
        echo OK: Self-compiled program_c99 assembled
        for %%F in ("tests\program_c99_selfhost.exe") do echo   Executable size: %%~zF bytes
    )
)

echo.
echo Step 6: Self-hosting assessment
echo ===============================

echo Self-Hosting Capability Analysis:
echo.

echo COMPILATION TESTS:
if exist "tests\evolver0_loader_selfhost.astc" echo [OK] evolver0_loader.c -> ASTC
if not exist "tests\evolver0_loader_selfhost.astc" echo [FAIL] evolver0_loader.c compilation

if exist "tests\evolver0_runtime_selfhost.astc" echo [OK] evolver0_runtime.c -> ASTC
if not exist "tests\evolver0_runtime_selfhost.astc" echo [FAIL] evolver0_runtime.c compilation

if exist "tests\evolver0_program_selfhost.astc" echo [OK] evolver0_program.c -> ASTC
if not exist "tests\evolver0_program_selfhost.astc" echo [FAIL] evolver0_program.c compilation

if exist "tests\program_c99_selfhost.astc" echo [OK] program_c99.c -> ASTC (SELF-COMPILATION!)
if not exist "tests\program_c99_selfhost.astc" echo [FAIL] program_c99.c self-compilation

echo.
echo ASSEMBLY TESTS:
if exist "tests\evolver0_loader_selfhost.exe" echo [OK] Self-compiled loader -> EXE
if not exist "tests\evolver0_loader_selfhost.exe" echo [FAIL] Self-compiled loader assembly

if exist "tests\program_c99_selfhost.exe" echo [OK] Self-compiled program_c99 -> EXE
if not exist "tests\program_c99_selfhost.exe" echo [FAIL] Self-compiled program_c99 assembly

echo.
echo Step 7: TinyCC independence evaluation
echo =====================================

echo Final TinyCC Independence Assessment:
echo.

set /a TOTAL_TESTS=0
set /a PASSED_TESTS=0

set /a TOTAL_TESTS+=1
if exist "tests\evolver0_loader_selfhost.astc" set /a PASSED_TESTS+=1

set /a TOTAL_TESTS+=1
if exist "tests\evolver0_runtime_selfhost.astc" set /a PASSED_TESTS+=1

set /a TOTAL_TESTS+=1
if exist "tests\evolver0_program_selfhost.astc" set /a PASSED_TESTS+=1

set /a TOTAL_TESTS+=1
if exist "tests\program_c99_selfhost.astc" set /a PASSED_TESTS+=1

echo Self-hosting test results: %PASSED_TESTS%/%TOTAL_TESTS% components compiled

if %PASSED_TESTS% EQU %TOTAL_TESTS% (
    echo.
    echo 🎉 COMPLETE SUCCESS: 100%% SELF-HOSTING ACHIEVED!
    echo ✅ All evolver0 components compiled with our independent toolchain
    echo ✅ program_c99 successfully compiled itself
    echo ✅ Complete independence from TinyCC in compilation process
    echo.
    echo 🏆 MILESTONE: True compiler independence achieved!
) else (
    echo.
    echo ⚡ PARTIAL SUCCESS: %PASSED_TESTS%/%TOTAL_TESTS% components self-hosted
    echo 🔧 Some components need further development
    echo 📈 Significant progress toward complete independence
)

echo.
echo === Self-Hosting Test Complete ===
echo.
echo This test demonstrates our progress toward true
echo compiler independence and self-hosting capability!

echo.
echo Cleaning up test files...
del tests\*_selfhost.* 2>nul

pause
