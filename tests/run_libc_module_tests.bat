@echo off
REM run_libc_module_tests.bat - Build and run LibC module tests
REM Usage: run_libc_module_tests.bat [clean]

echo === LibC Module Tests ===
echo.

REM Check if clean argument is provided
if "%1"=="clean" (
    echo Cleaning previous test artifacts...
    if exist test_libc_module.exe del test_libc_module.exe
    if exist test_libc_module_results.log del test_libc_module_results.log
    echo Clean complete.
    echo.
)

REM Build the test executable
echo Building LibC module tests...
..\external\tcc-win\tcc\tcc.exe -o test_libc_module.exe test_libc_module.c ..\src\core\utils.c ..\src\ext\modules\libc_module.c

REM Check if compilation was successful
if errorlevel 1 (
    echo ERROR: Compilation failed!
    echo.
    echo This might be due to:
    echo 1. Missing dependencies
    echo 2. Include path issues
    echo 3. Function signature mismatches
    echo.
    echo Try compiling manually to see detailed errors:
    echo ..\external\tcc-win\tcc\tcc.exe -v test_libc_module.c ..\src\core\utils.c ..\src\ext\modules\libc_module.c
    pause
    exit /b 1
)

echo Compilation successful.
echo.

REM Run the tests
echo Running LibC module tests...
echo =====================================
test_libc_module.exe > test_libc_module_results.log 2>&1
type test_libc_module_results.log
echo =====================================
echo.

REM Check test results
findstr /C:"All tests passed!" test_libc_module_results.log >nul
if errorlevel 1 (
    echo Some tests failed! Check test_libc_module_results.log for details.
    echo.
    echo Common issues:
    echo 1. Function signature mismatches
    echo 2. Module initialization problems
    echo 3. Memory allocation issues
    echo.
    pause
    exit /b 1
) else (
    echo All tests passed successfully!
)

echo.
echo Test results saved to: test_libc_module_results.log
echo.
pause
