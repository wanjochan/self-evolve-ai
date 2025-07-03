@echo off
REM run_astc_jit_integration_tests.bat - Build and run ASTC+JIT integration tests
REM Usage: run_astc_jit_integration_tests.bat [clean]

echo === ASTC+JIT Integration Tests ===
echo.

REM Check if clean argument is provided
if "%1"=="clean" (
    echo Cleaning previous test artifacts...
    if exist test_astc_jit_integration.exe del test_astc_jit_integration.exe
    if exist test_astc_jit_results.log del test_astc_jit_results.log
    echo Clean complete.
    echo.
)

REM Build the test executable
echo Building ASTC+JIT integration tests...
..\external\tcc-win\tcc\tcc.exe -o test_astc_jit_integration.exe test_astc_jit_integration.c ..\src\core\utils.c

REM Check if compilation was successful
if errorlevel 1 (
    echo ERROR: Compilation failed!
    echo.
    echo This might be due to:
    echo 1. Missing ASTC+JIT integration implementation
    echo 2. Include path issues
    echo 3. Function signature mismatches
    echo 4. Missing core dependencies
    echo.
    echo Try compiling manually to see detailed errors:
    echo ..\external\tcc-win\tcc\tcc.exe -v test_astc_jit_integration.c ..\src\core\utils.c
    pause
    exit /b 1
)

echo Compilation successful.
echo.

REM Run the tests
echo Running ASTC+JIT integration tests...
echo =====================================
test_astc_jit_integration.exe > test_astc_jit_results.log 2>&1
type test_astc_jit_results.log
echo =====================================
echo.

REM Check test results
findstr /C:"All ASTC+JIT integration tests passed!" test_astc_jit_results.log >nul
if errorlevel 1 (
    echo Some ASTC+JIT integration tests failed! Check test_astc_jit_results.log for details.
    echo.
    echo Common issues:
    echo 1. ASTC+JIT integration incomplete
    echo 2. Architecture compatibility problems
    echo 3. Memory management issues
    echo 4. Performance regression
    echo.
    pause
    exit /b 1
) else (
    echo All ASTC+JIT integration tests passed successfully!
    echo.
    echo The new ASTC+JIT flow is ready to replace TCC system calls!
)

echo.
echo Test results saved to: test_astc_jit_results.log
echo.
pause
