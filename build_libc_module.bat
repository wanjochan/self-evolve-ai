@echo off
chcp 65001 >nul

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
if not exist "src\ext\modules\libc_module.c" (
    echo Warning: Original libc_module.c not found, checking for simplified version...
    if not exist "src\ext\modules\libc_module_simple.c" (
        echo Error: No LibC module source found
        exit /b 1
    )
    set LIBC_SOURCE=src\ext\modules\libc_module_simple.c
    echo Using simplified LibC module
) else (
    set LIBC_SOURCE=src\ext\modules\libc_module.c
    echo Using full LibC module
)

REM Build LibC module
echo Building libc_x64_64.native...

REM Compile LibC module as executable first
%TCC% -o "bin\layer2\libc_x64_64_temp.exe" ^
      -DNDEBUG ^
      -DVERSION_STRING="2.0" ^
      -DBUILD_DATE="%DATE%" ^
      -O2 ^
      "%LIBC_SOURCE%" ^
      "src\core\utils.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile LibC module
    exit /b 1
)

REM Convert to .native format (temporary solution)
copy "bin\layer2\libc_x64_64_temp.exe" "bin\layer2\libc_x64_64.native" >nul
del "bin\layer2\libc_x64_64_temp.exe" >nul

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
echo LibC Module Functions:
echo   - libc_module_init: Module initialization
echo   - libc_module_cleanup: Module cleanup
echo   - libc_module_get_function: Get C standard library function
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

exit /b 0
