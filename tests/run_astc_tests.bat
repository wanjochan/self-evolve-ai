@echo off
REM run_astc_tests.bat - Build and run ASTC module tests
REM Usage: run_astc_tests.bat [clean]

echo === ASTC Module Tests ===
echo.

REM Check if clean argument is provided
if "%1"=="clean" (
    echo Cleaning previous test artifacts...
    if exist test_astc_module.exe del test_astc_module.exe
    if exist test_astc_results.log del test_astc_results.log
    echo Clean complete.
    echo.
)

REM Build the test executable
echo Building ASTC module tests...
..\external\tcc-win\tcc\tcc.exe -o test_astc_module.exe test_astc_module.c ..\src\core\utils.c ..\src\ext\modules\astc_module.c

REM Check if compilation was successful
if errorlevel 1 (
    echo ERROR: Compilation failed!
    echo.
    echo This might be due to:
    echo 1. Missing ASTC module implementation
    echo 2. Include path issues
    echo 3. Function signature mismatches
    echo 4. Missing compiler dependencies
    echo.
    echo Try compiling manually to see detailed errors:
    echo ..\external\tcc-win\tcc\tcc.exe -v test_astc_module.c ..\src\core\utils.c ..\src\ext\modules\astc_module.c
    pause
    exit /b 1
)

echo Compilation successful.
echo.

REM Run the tests
echo Running ASTC module tests...
echo =====================================
test_astc_module.exe > test_astc_results.log 2>&1
type test_astc_results.log
echo =====================================
echo.

REM Check test results
findstr /C:"All ASTC module tests passed!" test_astc_results.log >nul
if errorlevel 1 (
    echo Some ASTC module tests failed! Check test_astc_results.log for details.
    echo.
    echo Common issues:
    echo 1. ASTC compilation implementation incomplete
    echo 2. Architecture detection problems
    echo 3. File I/O issues
    echo 4. Memory management problems
    echo.
    pause
    exit /b 1
) else (
    echo All ASTC module tests passed successfully!
)

echo.
echo Test results saved to: test_astc_results.log
echo.
pause
