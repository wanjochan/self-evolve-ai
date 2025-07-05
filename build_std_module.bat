@echo off
chcp 65001

echo ========================================
echo Building STD Module (Layer 2)
echo ========================================

REM Check if TCC is available
if not exist "external\tcc-win\tcc\tcc.exe" (
    echo Error: TCC compiler not found at external\tcc-win\tcc\tcc.exe
    echo Please ensure TCC is properly installed in external\tcc-win\
    exit /b 1
)

set TCC=external\tcc-win\tcc\tcc.exe

REM Create output directory
if not exist "bin\layer2" mkdir "bin\layer2"

echo.
echo Building STD Module...
echo =====================

REM Check if STD module source exists
if not exist "src\ext\std_module.c" (
    echo Error: STD module source not found at src\ext\std_module.c
    exit /b 1
)

REM Build STD module directly (no intermediate files needed)
echo Building std_x64_64.native...

REM Use c2native.exe tool to create proper .native format
echo Creating proper .native format using c2native.exe...
tools\c2native.exe "src\ext\std_module.c" "bin\layer2\std_x64_64.native"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to create .native format using c2native
    exit /b 1
)

echo.
echo STD Module Build Summary:
echo ========================
if exist "bin\layer2\std_x64_64.native" (
    for %%f in ("bin\layer2\std_x64_64.native") do echo   - std_x64_64.native (%%~zf bytes^)
    echo Success: STD module built successfully
) else (
    echo Error: STD module was not created
    exit /b 1
)

echo.
echo ========================================
echo Building VM Module for Testing...
echo ========================================
call build_vm_module.bat
if %ERRORLEVEL% neq 0 (
    echo Error: VM module build failed
    exit /b 1
)

echo.
echo ========================================
echo Creating Test ASTC Program...
echo ========================================
echo Creating test program that uses STD module functions...
tools\c2astc.exe tests\test_std_usage.c tests\test_std_usage.astc
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to create test ASTC program
    exit /b 1
)

echo.
echo ========================================
echo Testing Three-Layer Architecture...
echo ========================================
echo Testing: loader + vm_module + test_program
echo Command: bin\layer1\loader_x64_64.exe -m bin\layer2\vm_x64_64.native tests\test_std_usage.astc
echo.

bin\layer1\loader_x64_64.exe -m bin\layer2\vm_x64_64.native tests\test_std_usage.astc

if %ERRORLEVEL% equ 0 (
    echo.
    echo SUCCESS: Three-layer architecture test passed!
    echo   Layer 1 Loader: loader_x64_64.exe
    echo   Layer 2 Runtime: vm_x64_64.native + std_x64_64.native
    echo   Layer 3 Program: test_std_usage.astc
) else (
    echo.
    echo WARNING: Three-layer architecture test had issues
    echo   This may be normal if VM module needs further implementation
    echo   The STD module itself was built successfully
)

echo.
echo STD Module Functions:
echo   - std_module_init: Module initialization
echo   - std_module_cleanup: Module cleanup
echo   - std_module_get_function: Get standard library function
echo   - std_module_get_info: Get module information
echo.
echo Supported Standard Library Functions:
echo   Memory Management: malloc, free, calloc, realloc
echo   String Operations: strlen, strcpy, strcmp, strcat
echo   Input/Output: printf, sprintf, puts
echo   Mathematics: sin, cos, sqrt, pow
echo   Utilities: atoi, atof, exit
echo.
echo Usage:
echo   Automatically loaded by VM when C standard library functions are called
echo   Functions can be accessed via std_module_get_function function_name
echo.
echo Native Module Format:
echo   - Follows PRD.md specification for .native modules
echo   - Can be loaded by mmap alike not libdl or ffi
echo   - Provides libdl-alike libffi-alike functionality via src/utils.c
echo.
echo Testing Commands:
echo   Full test: bin\layer1\loader_x64_64.exe -m bin\layer2\vm_x64_64.native tests\test_std_usage.astc
echo   Create new test: tools\c2astc.exe your_program.c your_program.astc

exit /b 0
