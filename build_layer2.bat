@echo off
chcp 65001 >nul

echo ========================================
echo Building Layer 2 Native Modules
echo PRD.md: vm_{arch}_{bits}.native
echo ========================================

REM Check if TCC is available
if not exist "external\tcc-win\tcc\tcc.exe" (
    echo Error: TCC compiler not found
    exit /b 1
)

REM Create output directory
if not exist "bin\layer2" mkdir "bin\layer2"

REM Set TCC path
set TCC=external\tcc-win\tcc\tcc.exe

echo.
echo Building vm_x64_64.native...
echo ============================

REM Compile VM module as shared library
%TCC% -shared -o "bin\layer2\vm_x64_64.native" "src\layer2\vm_module.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile vm_x64_64.native
    exit /b 1
)

echo Success: vm_x64_64.native compiled successfully

echo.
echo Building libc_x64_64.native...
echo ==============================

REM Compile LibC module as shared library
%TCC% -shared -o "bin\layer2\libc_x64_64.native" "src\layer2\libc_module.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile libc_x64_64.native
    exit /b 1
)

echo Success: libc_x64_64.native compiled successfully

echo.
echo Testing native modules...
echo =========================

REM Test VM module
if exist "bin\layer2\vm_x64_64.native" (
    echo Success: vm_x64_64.native created
) else (
    echo Error: vm_x64_64.native not found
    exit /b 1
)

REM Test LibC module
if exist "bin\layer2\libc_x64_64.native" (
    echo Success: libc_x64_64.native created
) else (
    echo Error: libc_x64_64.native not found
    exit /b 1
)

echo.
echo Layer 2 Build Summary:
echo ======================
dir "bin\layer2\*.native"

echo.
echo Success: Layer 2 Native Modules build completed
echo.
echo PRD.md Layer 2 Runtime Modules:
echo   vm_x64_64.native    - VM runtime for ASTC execution
echo   libc_x64_64.native  - Standard library module
echo.
echo These .native modules are loaded by Layer 1 loader_{arch}_{bits}.exe
echo to execute Layer 3 {program}.astc bytecode programs.

exit /b 0
