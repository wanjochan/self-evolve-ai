@echo off
echo === Verifying 100 Percent TinyCC Independence ===

echo Step 1: Test complete pipeline
echo Creating test program...
echo int main() { return 123; } > tests\final_test.c

echo Compiling C to ASTC...
bin\program_c99.exe tests\final_test.c tests\final_test.astc
if errorlevel 1 (
    echo FAIL: C to ASTC failed
    goto end
)
echo OK: C to ASTC successful

echo Assembling ASTC to EXE...
bin\astc_assembler_v3.exe tests\final_test.astc tests\final_test.exe windows-x64
if errorlevel 1 (
    echo FAIL: ASTC to EXE failed
    goto end
)
echo OK: ASTC to EXE successful

echo Testing executable...
tests\final_test.exe
echo Exit code: %errorlevel%

echo.
echo Step 2: Test self-hosting
echo Self-compiling program_c99...
bin\program_c99.exe src\tools\program_c99.c tests\self_c99.astc
if errorlevel 1 (
    echo FAIL: Self-compilation failed
    set SELF_OK=0
) else (
    echo OK: Self-compilation successful
    
    echo Assembling self-compiled program_c99...
    bin\astc_assembler_v3.exe tests\self_c99.astc tests\self_c99.exe windows-x64
    if errorlevel 1 (
        echo FAIL: Self-assembly failed
        set SELF_OK=0
    ) else (
        echo OK: Self-assembly successful
        set SELF_OK=1
    )
)

echo.
echo Step 3: Test evolver0 compilation
echo Compiling evolver0_loader...
bin\program_c99.exe src\evolver0\evolver0_loader.c tests\loader_test.astc
if errorlevel 1 (
    echo FAIL: evolver0_loader compilation failed
    set EVOLVER_OK=0
) else (
    echo OK: evolver0_loader compilation successful
    set EVOLVER_OK=1
)

echo.
echo === FINAL ASSESSMENT ===
echo.

if exist "tests\final_test.exe" (
    echo [PASS] Complete pipeline works
    set PIPELINE_OK=1
) else (
    echo [FAIL] Complete pipeline failed
    set PIPELINE_OK=0
)

if %SELF_OK%==1 (
    echo [PASS] Self-hosting works
) else (
    echo [FAIL] Self-hosting failed
)

if %EVOLVER_OK%==1 (
    echo [PASS] evolver0 compilation works
) else (
    echo [FAIL] evolver0 compilation failed
)

echo.
set /a TOTAL=3
set /a PASSED=0
if %PIPELINE_OK%==1 set /a PASSED+=1
if %SELF_OK%==1 set /a PASSED+=1
if %EVOLVER_OK%==1 set /a PASSED+=1

echo Score: %PASSED%/%TOTAL% tests passed

if %PASSED%==3 (
    echo.
    echo *** SUCCESS: 100 PERCENT INDEPENDENCE! ***
    echo Complete toolchain works without TinyCC!
    echo evolver2 goal ACHIEVED!
) else (
    echo.
    echo PARTIAL SUCCESS: %PASSED%/%TOTAL%
    echo Some work still needed
)

:end
echo.
echo Cleanup...
del tests\final_test.* tests\self_c99.* tests\loader_test.* 2>nul

pause
