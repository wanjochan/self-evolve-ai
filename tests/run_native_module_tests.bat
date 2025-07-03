@echo off
REM run_native_module_tests.bat - Build and run native module system tests
REM Usage: run_native_module_tests.bat [clean]

echo === Native Module System Tests ===
echo.

REM Check if clean argument is provided
if "%1"=="clean" (
    echo Cleaning previous test artifacts...
    if exist test_native_module_system.exe del test_native_module_system.exe
    if exist test_native_module_results.log del test_native_module_results.log
    echo Clean complete.
    echo.
)

REM Build the test executable
echo Building native module system tests...
..\external\tcc-win\tcc\tcc.exe -o test_native_module_system.exe test_native_module_system.c ..\src\core\utils.c

REM Check if compilation was successful
if errorlevel 1 (
    echo ERROR: Compilation failed!
    pause
    exit /b 1
)

echo Compilation successful.
echo.

REM Run the tests
echo Running native module system tests...
echo =====================================
test_native_module_system.exe > test_native_module_results.log 2>&1
type test_native_module_results.log
echo =====================================
echo.

REM Check test results
findstr /C:"All tests passed!" test_native_module_results.log >nul
if errorlevel 1 (
    echo Some tests failed! Check test_native_module_results.log for details.
    pause
    exit /b 1
) else (
    echo All tests passed successfully!
)

echo.
echo Test results saved to: test_native_module_results.log
echo.
pause
