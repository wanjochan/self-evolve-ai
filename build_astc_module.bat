@echo off
chcp 65001 >nul

echo ========================================
echo Building ASTC Module (Layer 2)
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
echo Building ASTC Module...
echo =======================

REM Check if ASTC module source exists
if not exist "src\ext\modules\astc_module.c" (
    echo Warning: Original astc_module.c not found, checking for simplified version...
    if not exist "src\ext\modules\astc_module_simple.c" (
        echo Error: No ASTC module source found
        exit /b 1
    )
    set ASTC_SOURCE=src\ext\modules\astc_module_simple.c
    echo Using simplified ASTC module
) else (
    set ASTC_SOURCE=src\ext\modules\astc_module.c
    echo Using full ASTC module
)

REM Build ASTC module
echo Building astc_x64_64.native...

REM Compile ASTC module as executable first
%TCC% -o "bin\layer2\astc_x64_64_temp.exe" ^
      -DNDEBUG ^
      -DVERSION_STRING="2.0" ^
      -DBUILD_DATE="%DATE%" ^
      -O2 ^
      "%ASTC_SOURCE%" ^
      "src\core\utils.c" ^
      "src\core\astc.c" ^
      "src\core\native.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile ASTC module
    exit /b 1
)

REM Convert to .native format (temporary solution)
copy "bin\layer2\astc_x64_64_temp.exe" "bin\layer2\astc_x64_64.native" >nul
del "bin\layer2\astc_x64_64_temp.exe" >nul

echo.
echo ASTC Module Build Summary:
echo =========================
if exist "bin\layer2\astc_x64_64.native" (
    for %%f in ("bin\layer2\astc_x64_64.native") do echo   - astc_x64_64.native (%%~zf bytes^)
    echo Success: ASTC module built successfully
) else (
    echo Error: ASTC module was not created
    exit /b 1
)

echo.
echo ASTC Module Functions:
echo   - astc_module_init: Module initialization
echo   - astc_module_cleanup: Module cleanup
echo   - astc_module_c2astc: Compile C source to ASTC bytecode
echo   - astc_module_astc2native: Compile ASTC bytecode to native module
echo   - astc_module_get_error: Get last compilation error
echo.
echo ASTC Compilation Pipeline:
echo   C Source (.c) -> ASTC Bytecode (.astc) -> Native Module (.native)
echo.
echo Supported Features:
echo   - C99 source code parsing
echo   - ASTC bytecode generation
echo   - Native module creation
echo   - Optimization levels: 0 (none), 1 (basic), 2 (aggressive)
echo   - Debug information support
echo   - Cross-architecture compilation
echo.
echo Usage:
echo   Used internally by c99.astc compiler for code generation

exit /b 0
