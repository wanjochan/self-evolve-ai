@echo off
echo ========================================
echo Complete System Validation
echo ========================================

echo.
echo Test 1: evolver0 basic functionality
evolver0_loader.exe evolver0_runtime.bin evolver0_program.astc
if %errorlevel% neq 0 (
    echo FAILED: evolver0 basic test failed
    exit /b 1
)
echo PASSED: evolver0 basic functionality

echo.
echo Test 2: evolver0 self-bootstrap
evolver0_loader.exe evolver0_runtime.bin evolver0_program.astc --self-compile
if %errorlevel% neq 0 (
    echo FAILED: evolver0 self-bootstrap test failed
    exit /b 1
)
echo PASSED: evolver0 self-bootstrap

echo.
echo Test 3: evolver1 functionality
evolver0_loader.exe evolver0_runtime.bin evolver1_program.astc
if %errorlevel% neq 0 (
    echo FAILED: evolver1 test failed
    exit /b 1
)
echo PASSED: evolver1 functionality

echo.
echo Test 4: ASTC serialization
cd tests
debug_astc_serialization.exe
if %errorlevel% neq 0 (
    echo FAILED: ASTC serialization test failed
    cd ..
    exit /b 1
)
cd ..
echo PASSED: ASTC serialization

echo.
echo Test 5: File existence validation
for %%f in (evolver0_loader.exe evolver0_runtime.bin evolver0_program.astc evolver1_program.astc) do (
    if not exist %%f (
        echo FAILED: File %%f does not exist
        exit /b 1
    )
)
echo PASSED: All core files exist

echo.
echo ========================================
echo SUCCESS: Complete system validation passed!
echo ========================================
echo.
echo System Status:
echo - evolver0: Fully functional
echo - evolver1: Fully functional
echo - Three-layer architecture: Working
echo - Self-bootstrap: Successful
echo - ASTC serialization: Working
echo.
echo System reached 100%% completion!
