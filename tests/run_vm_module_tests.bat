@echo off
REM run_vm_module_tests.bat - Build and run VM module tests
REM Usage: run_vm_module_tests.bat [clean]

echo === VM Module Tests ===
echo.

REM Check if clean argument is provided
if "%1"=="clean" (
    echo Cleaning previous test artifacts...
    if exist test_vm_module.exe del test_vm_module.exe
    if exist test_vm_module_results.log del test_vm_module_results.log
    echo Clean complete.
    echo.
)

REM Build the test executable
echo Building VM module tests...
..\external\tcc-win\tcc\tcc.exe -o test_vm_module.exe test_vm_module.c ..\src\core\utils.c ..\src\ext\modules\vm_module.c

REM Check if compilation was successful
if errorlevel 1 (
    echo ERROR: Compilation failed!
    echo.
    echo This might be due to:
    echo 1. Missing dependencies
    echo 2. Include path issues
    echo 3. Function signature mismatches
    echo 4. Missing header files
    echo.
    echo Try compiling manually to see detailed errors:
    echo ..\external\tcc-win\tcc\tcc.exe -v test_vm_module.c ..\src\core\utils.c ..\src\ext\modules\vm_module.c
    pause
    exit /b 1
)

echo Compilation successful.
echo.

REM Run the tests
echo Running VM module tests...
echo =====================================
test_vm_module.exe > test_vm_module_results.log 2>&1
type test_vm_module_results.log
echo =====================================
echo.

REM Check test results
findstr /C:"All tests passed!" test_vm_module_results.log >nul
if errorlevel 1 (
    echo Some tests failed! Check test_vm_module_results.log for details.
    echo.
    echo Common issues:
    echo 1. Function implementation incomplete
    echo 2. Memory management problems
    echo 3. Module integration issues
    echo 4. Architecture-specific problems
    echo.
    pause
    exit /b 1
) else (
    echo All tests passed successfully!
)

echo.
echo Test results saved to: test_vm_module_results.log
echo.
pause
