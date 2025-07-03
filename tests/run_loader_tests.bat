@echo off
REM run_loader_tests.bat - Build and run Loader tests
REM Usage: run_loader_tests.bat [clean]

echo === Enhanced Loader Tests ===
echo.

REM Check if clean argument is provided
if "%1"=="clean" (
    echo Cleaning previous test artifacts...
    if exist test_loader.exe del test_loader.exe
    if exist test_loader_results.log del test_loader_results.log
    echo Clean complete.
    echo.
)

REM Build the test executable
echo Building Loader tests...
..\external\tcc-win\tcc\tcc.exe -o test_loader.exe test_loader.c ..\src\core\utils.c

REM Check if compilation was successful
if errorlevel 1 (
    echo ERROR: Compilation failed!
    echo.
    echo This might be due to:
    echo 1. Missing core utilities implementation
    echo 2. Include path issues
    echo 3. Function signature mismatches
    echo 4. Missing dependencies
    echo.
    echo Try compiling manually to see detailed errors:
    echo ..\external\tcc-win\tcc\tcc.exe -v test_loader.c ..\src\core\utils.c
    pause
    exit /b 1
)

echo Compilation successful.
echo.

REM Run the tests
echo Running Loader tests...
echo =====================================
test_loader.exe > test_loader_results.log 2>&1
type test_loader_results.log
echo =====================================
echo.

REM Check test results
findstr /C:"All loader tests passed!" test_loader_results.log >nul
if errorlevel 1 (
    echo Some Loader tests failed! Check test_loader_results.log for details.
    echo.
    echo Common issues:
    echo 1. Architecture detection problems
    echo 2. Platform compatibility issues
    echo 3. Module system initialization failures
    echo 4. Memory management problems
    echo.
    pause
    exit /b 1
) else (
    echo All Loader tests passed successfully!
)

echo.
echo Test results saved to: test_loader_results.log
echo.
pause
