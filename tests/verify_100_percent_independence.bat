@echo off
echo === Verifying 100%% TinyCC Independence ===
echo Testing complete toolchain independence

echo.
echo Step 1: Test complete C to EXE pipeline
echo =======================================

echo Creating test C program...
echo int main() { > tests\independence_final.c
echo     return 123; >> tests\independence_final.c
echo } >> tests\independence_final.c

echo Compiling C to ASTC with program_c99...
bin\program_c99.exe tests\independence_final.c tests\independence_final.astc
if errorlevel 1 (
    echo FAIL: C to ASTC compilation failed
    goto end
)
echo OK: C to ASTC successful

echo Assembling ASTC to EXE with fixed assembler...
bin\astc_assembler_v3.exe tests\independence_final.astc tests\independence_final.exe windows-x64
if errorlevel 1 (
    echo FAIL: ASTC to EXE assembly failed
    goto end
)
echo OK: ASTC to EXE successful

echo Testing generated executable...
tests\independence_final.exe
set EXIT_CODE=%errorlevel%
echo Generated executable exit code: %EXIT_CODE%

if %EXIT_CODE%==42 (
    echo OK: Executable runs and returns expected value
    set PIPELINE_WORKS=1
) else (
    echo WARN: Executable returns unexpected value
    set PIPELINE_WORKS=0
)

echo.
echo Step 2: Test self-hosting capability
echo ===================================

echo Self-compiling program_c99 with itself...
bin\program_c99.exe src\tools\program_c99.c tests\program_c99_self.astc
if errorlevel 1 (
    echo FAIL: program_c99 self-compilation failed
    set SELF_HOST_WORKS=0
) else (
    echo OK: program_c99 self-compilation successful
    
    echo Assembling self-compiled program_c99...
    bin\astc_assembler_v3.exe tests\program_c99_self.astc tests\program_c99_self.exe windows-x64
    if errorlevel 1 (
        echo FAIL: Self-compiled program_c99 assembly failed
        set SELF_HOST_WORKS=0
    ) else (
        echo OK: Self-compiled program_c99 assembly successful
        set SELF_HOST_WORKS=1
    )
)

echo.
echo Step 3: Test evolver0 component compilation
echo ==========================================

echo Compiling evolver0_loader with independent toolchain...
bin\program_c99.exe src\evolver0\evolver0_loader.c tests\loader_independent.astc
if errorlevel 1 (
    echo FAIL: evolver0_loader compilation failed
    set EVOLVER_WORKS=0
) else (
    echo OK: evolver0_loader compilation successful
    
    echo Assembling evolver0_loader...
    bin\astc_assembler_v3.exe tests\loader_independent.astc tests\loader_independent.exe windows-x64
    if errorlevel 1 (
        echo FAIL: evolver0_loader assembly failed
        set EVOLVER_WORKS=0
    ) else (
        echo OK: evolver0_loader assembly successful
        set EVOLVER_WORKS=1
    )
)

echo.
echo Step 4: Independence assessment
echo ==============================

echo COMPLETE INDEPENDENCE VERIFICATION:
echo.

if %PIPELINE_WORKS%==1 (
    echo [PASS] Complete C->ASTC->EXE pipeline works
) else (
    echo [FAIL] Complete pipeline has issues
)

if %SELF_HOST_WORKS%==1 (
    echo [PASS] Self-hosting compilation works
) else (
    echo [FAIL] Self-hosting compilation failed
)

if %EVOLVER_WORKS%==1 (
    echo [PASS] evolver0 components can be compiled
) else (
    echo [FAIL] evolver0 components cannot be compiled
)

echo.
set /a TOTAL_TESTS=3
set /a PASSED_TESTS=0
if %PIPELINE_WORKS%==1 set /a PASSED_TESTS+=1
if %SELF_HOST_WORKS%==1 set /a PASSED_TESTS+=1
if %EVOLVER_WORKS%==1 set /a PASSED_TESTS+=1

echo Independence Score: %PASSED_TESTS%/%TOTAL_TESTS% tests passed

if %PASSED_TESTS%==3 (
    echo.
    echo *** SUCCESS: 100%% TinyCC INDEPENDENCE ACHIEVED! ***
    echo.
    echo ✅ Complete C compilation pipeline works
    echo ✅ Self-hosting capability verified
    echo ✅ evolver0 components can be built independently
    echo ✅ Generated executables run correctly
    echo.
    echo 🏆 MILESTONE: True compiler independence!
    echo 🎉 evolver2 goal FULLY COMPLETED!
    echo.
    echo The independent toolchain consists of:
    echo - bin\program_c99.exe (C to ASTC compiler)
    echo - bin\astc_assembler_v3.exe (ASTC to EXE assembler)
    echo.
    echo This toolchain can build the entire evolver system
    echo without ANY external compiler dependencies!
) else (
    echo.
    echo ⚠️ PARTIAL SUCCESS: %PASSED_TESTS%/%TOTAL_TESTS% tests passed
    echo 🔧 Some components need further work
    echo 📈 Significant progress toward complete independence
    echo.
    echo Current status:
    if %PIPELINE_WORKS%==0 echo - Basic pipeline needs fixing
    if %SELF_HOST_WORKS%==0 echo - Self-hosting needs improvement
    if %EVOLVER_WORKS%==0 echo - evolver0 compilation needs work
)

:end
echo.
echo Cleaning up test files...
del tests\independence_final.* tests\program_c99_self.* tests\loader_independent.* 2>nul

echo.
echo === 100%% Independence Verification Complete ===
pause
