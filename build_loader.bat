@echo off
chcp 65001 >nul

echo ========================================
echo Building Loader Executables (Layer 1)
echo ========================================

REM Check if TCC is available
if not exist "external\tcc-win\tcc\tcc.exe" (
    echo Error: TCC compiler not found at external\tcc-win\tcc\tcc.exe
    echo Please ensure TCC is available for compilation
    exit /b 1
)
REM Create output directory
if not exist "bin\layer1" mkdir "bin\layer1"

REM Set TCC path
set TCC=external\tcc-win\tcc\tcc.exe

echo.
echo Building loader_x64_64.exe...
echo ================================

REM Compile loader for x64 64-bit (using ultra safe version)
REM Include all required source files and use anti-virus friendly flags
%TCC% -o "bin\layer1\loader_x64_64.exe" ^
      -DNDEBUG ^
      -DVERSION_STRING="2.0" ^
      -DBUILD_DATE="%DATE%" ^
      -O2 ^
      "src\layer1\loader.c" ^
      "src\core\utils.c" ^
      "src\core\native.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile loader_x64_64.exe
    echo Check that all source files exist and TCC is working properly
    exit /b 1
)

echo Success: loader_x64_64.exe compiled successfully

REM Test the executable
echo.
echo Testing loader_x64_64.exe...
echo ============================

"bin\layer1\loader_x64_64.exe" --help 
if %ERRORLEVEL% equ 0 (
    echo Success: loader_x64_64.exe runs successfully
) else (
    echo Warning: loader_x64_64.exe compiled but may have runtime issues
)

echo.
echo Building loader_x86_32.exe...
echo ==============================

REM Compile loader for x86 32-bit (if supported)
%TCC% -m32 -o "bin\layer1\loader_x86_32.exe" ^
      -DNDEBUG ^
      -DVERSION_STRING="2.0" ^
      -DBUILD_DATE="%DATE%" ^
      -O2 ^
      "src\layer1\loader.c" ^
      "src\core\utils.c" ^
      "src\core\native.c"

if %ERRORLEVEL% equ 0 (
    echo Success: loader_x86_32.exe compiled successfully
) else (
    echo Warning: loader_x86_32.exe compilation failed (32-bit may not be supported)
)

echo.
echo Layer 1 Build Summary:
echo ======================
dir "bin\layer1\loader_*.exe"
if %ERRORLEVEL% neq 0 (
    echo No loader executables found
    exit /b 1
)

echo.
echo Success: Layer 1 Loader build completed successfully
echo.
echo Usage:
echo   bin\layer1\loader_x64_64.exe program.astc
echo   bin\layer1\loader_x64_64.exe --help
echo.
echo Next steps:
echo 1. Build Layer 2 (VM modules): run build_layer2.bat
echo 2. Build Layer 3 (ASTC programs): run build_layer3.bat
echo 3. Test complete flow: loader → vm → program

exit /b 0

