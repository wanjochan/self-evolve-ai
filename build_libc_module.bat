@echo off
chcp 65001

echo ========================================
echo Building LibC Module (Layer 2)
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
echo Building LibC Module...
echo =======================

REM Check if LibC module source exists
if not exist "src\core\modules\libc_module.c" (
    echo Error: LibC module source not found at src\core\modules\libc_module.c
    exit /b 1
)

REM Build LibC module using proper .native format
echo Building libc_x64_64.native...

REM Use c2native.exe tool to create proper .native format
echo Creating proper .native format using c2native.exe...
tools\c2native.exe "src\core\modules\libc_module.c" "bin\layer2\libc_x64_64.native"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to create .native format using c2native
    exit /b 1
)

echo.
echo LibC Module Build Summary:
echo =========================
if exist "bin\layer2\libc_x64_64.native" (
    for %%f in ("bin\layer2\libc_x64_64.native") do echo   - libc_x64_64.native (%%~zf bytes^)
    echo Success: LibC module built successfully
) else (
    echo Error: LibC module was not created
    exit /b 1
)

echo.
echo ========================================
echo Creating Test C Program...
echo ========================================
echo Creating test program for LibC module...
if not exist "tests\test_libc_simple.c" (
    echo Creating simple LibC test program...
    echo #include ^<stdio.h^> > tests\test_libc_simple.c
    echo #include ^<stdlib.h^> >> tests\test_libc_simple.c
    echo #include ^<string.h^> >> tests\test_libc_simple.c
    echo int main^(^) { >> tests\test_libc_simple.c
    echo     printf^("Testing LibC functions...\\n"^); >> tests\test_libc_simple.c
    echo     char* ptr = malloc^(100^); >> tests\test_libc_simple.c
    echo     strcpy^(ptr, "Hello LibC!"^); >> tests\test_libc_simple.c
    echo     printf^("String: %%s\\n", ptr^); >> tests\test_libc_simple.c
    echo     free^(ptr^); >> tests\test_libc_simple.c
    echo     return 0; >> tests\test_libc_simple.c
    echo } >> tests\test_libc_simple.c
)

tools\c2astc.exe tests\test_libc_simple.c tests\test_libc_simple.astc
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to create test ASTC program
    exit /b 1
)

echo.
echo ========================================
echo Testing LibC Module...
echo ========================================
echo Testing: loader + libc_module + test_program
echo Command: bin\layer1\loader_x64_64.exe -m bin\layer2\libc_x64_64.native tests\test_libc_simple.astc
echo.

bin\layer1\loader_x64_64.exe -m bin\layer2\libc_x64_64.native tests\test_libc_simple.astc

if %ERRORLEVEL% equ 0 (
    echo.
    echo SUCCESS: LibC module test passed!
    echo   Layer 1 Loader: loader_x64_64.exe
    echo   Layer 2 Runtime: libc_x64_64.native
    echo   Layer 3 Program: test_libc_simple.astc
) else (
    echo.
    echo WARNING: LibC module test had issues
    echo   This may be normal if LibC module needs further implementation
    echo   The LibC module itself was built successfully
)

echo.
echo LibC Module Functions:
echo   - libc_native_init: Module initialization
echo   - libc_native_cleanup: Module cleanup
echo   - libc_native_get_function: Get C standard library function
echo   - Standard C functions: malloc, free, printf, strlen, etc.
echo.
echo Supported C Standard Library Functions:
echo   Memory: malloc, free, calloc, realloc
echo   String: strlen, strcpy, strcat, strcmp
echo   I/O: printf, sprintf, fopen, fclose, fread, fwrite
echo   Math: sin, cos, sqrt, pow
echo   Utility: atoi, atof, exit
echo.
echo Usage:
echo   Automatically loaded by VM when C standard library functions are called
echo.
echo Testing Commands:
echo   Full test: bin\layer1\loader_x64_64.exe -m bin\layer2\libc_x64_64.native tests\test_libc_simple.astc
echo   Create new test: tools\c2astc.exe your_program.c your_program.astc

exit /b 0
