@echo off
REM run_jit_tests.bat - Build and run JIT core tests
REM Usage: run_jit_tests.bat [clean]

echo === JIT Core Tests ===
echo.

REM Check if clean argument is provided
if "%1"=="clean" (
    echo Cleaning previous test artifacts...
    if exist test_jit_core.exe del test_jit_core.exe
    if exist test_jit_results.log del test_jit_results.log
    echo Clean complete.
    echo.
)

REM Build the test executable
echo Building JIT extension tests...
..\external\tcc-win\tcc\tcc.exe -o test_jit_core.exe test_jit_core.c ..\src\ext\jit\jit.c ..\src\core\utils.c

REM Check if compilation was successful
if errorlevel 1 (
    echo ERROR: Compilation failed!
    echo.
    echo This might be due to:
    echo 1. Missing JIT core implementation
    echo 2. Include path issues
    echo 3. Function signature mismatches
    echo 4. Missing dependencies in utils.c
    echo.
    echo Try compiling manually to see detailed errors:
    echo ..\external\tcc-win\tcc\tcc.exe -v test_jit_core.c ..\src\core\jit.c ..\src\core\utils.c
    pause
    exit /b 1
)

echo Compilation successful.
echo.

REM Run the tests
echo Running JIT core tests...
echo =====================================
test_jit_core.exe > test_jit_results.log 2>&1
type test_jit_results.log
echo =====================================
echo.

REM Check test results
findstr /C:"All JIT tests passed!" test_jit_results.log >nul
if errorlevel 1 (
    echo Some JIT tests failed! Check test_jit_results.log for details.
    echo.
    echo Common issues:
    echo 1. JIT implementation incomplete
    echo 2. Architecture detection problems
    echo 3. Memory management issues
    echo 4. Code generation errors
    echo.
    pause
    exit /b 1
) else (
    echo All JIT tests passed successfully!
)

echo.
echo Test results saved to: test_jit_results.log
echo.
pause
