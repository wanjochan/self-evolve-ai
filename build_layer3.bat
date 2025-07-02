@echo off
chcp 65001 >nul

echo ========================================
echo Building Layer 3 ASTC Programs
echo PRD.md: c99.astc (C99 Compiler)
echo ========================================

REM Check if TCC is available
if not exist "external\tcc-win\tcc\tcc.exe" (
    echo Error: TCC compiler not found
    exit /b 1
)

REM Create output directory
if not exist "bin\layer3" mkdir "bin\layer3"

REM Set TCC path
set TCC=external\tcc-win\tcc\tcc.exe

echo.
echo Building c99.astc program...
echo ============================

REM Compile c99 compiler with optimization
%TCC% -o "bin\layer3\c99.astc" -g -O2 "src\layer3\c99.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile c99.astc
    exit /b 1
)

echo Success: c99.astc compiled successfully

REM TODO: Build other ASTC programs (evolver0.astc, etc.)
echo Note: Only building c99.astc for now

echo.
echo Testing c99.astc program...
echo ===========================

REM Test c99.astc
if exist "bin\layer3\c99.astc" (
    echo Success: c99.astc created

    REM Test basic functionality
    echo Testing c99.astc --help...
    "bin\layer3\c99.astc" --help

    if %ERRORLEVEL% equ 0 (
        echo Success: c99.astc runs successfully
    ) else (
        echo Warning: c99.astc compiled but may have runtime issues
    )
) else (
    echo Error: c99.astc not found
    exit /b 1
)

echo.
echo Layer 3 Build Summary:
echo ======================
dir "bin\layer3\*.astc"

echo.
echo Success: Layer 3 c99.astc build completed
echo.
echo PRD.md Layer 3 Program:
echo   c99.astc - C99 compiler program
echo.
echo Usage in PRD.md three-layer architecture:
echo   loader_x64_64.exe c99.astc source.c -o output
echo   Layer 1 (loader) → Layer 2 (vm) → Layer 3 (c99.astc)

exit /b 0
