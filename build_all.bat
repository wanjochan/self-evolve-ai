@echo off
chcp 65001 >nul

echo ========================================
echo Self-Evolve AI Complete Build System
echo Three-Layer Architecture Build
echo ========================================

echo Starting complete build process...
echo This will build all three layers according to PRD.md specification:
echo   Layer 1: loader_{arch}_{bits}.exe
echo   Layer 2: vm_{arch}_{bits}.native + libc_{arch}_{bits}.native + astc_{arch}_{bits}.native
echo   Layer 3: c99.astc
echo.

REM Build Layer 1 (Loader)
echo ========================================
echo Building Layer 1 (Loader)
echo ========================================
call build_loader.bat
if %ERRORLEVEL% neq 0 (
    echo Error: Layer 1 build failed
    exit /b 1
)

echo.
echo Layer 1 build completed successfully!
echo.

REM Build Layer 2 (Runtime Modules)
echo ========================================
echo Building Layer 2 (Runtime Modules)
echo ========================================
call build_layer2.bat
if %ERRORLEVEL% neq 0 (
    echo Error: Layer 2 build failed
    exit /b 1
)

echo.
echo Layer 2 build completed successfully!
echo.

REM Build Layer 3 (ASTC Programs)
echo ========================================
echo Building Layer 3 (ASTC Programs)
echo ========================================
call build_layer3.bat
if %ERRORLEVEL% neq 0 (
    echo Error: Layer 3 build failed
    exit /b 1
)

echo.
echo Layer 3 build completed successfully!
echo.

REM Build Summary
echo ========================================
echo Complete Build Summary
echo ========================================

echo Layer 1 (Loader):
if exist "bin\layer1\loader_x64_64.exe" (
    for %%f in ("bin\layer1\loader_x64_64.exe") do echo   ✓ loader_x64_64.exe (%%~zf bytes)
) else (
    echo   ✗ loader_x64_64.exe (missing)
)

echo.
echo Layer 2 (Runtime Modules):
if exist "bin\layer2\vm_x64_64.native" (
    for %%f in ("bin\layer2\vm_x64_64.native") do echo   ✓ vm_x64_64.native (%%~zf bytes)
) else (
    echo   ✗ vm_x64_64.native (missing)
)

if exist "bin\layer2\libc_x64_64.native" (
    for %%f in ("bin\layer2\libc_x64_64.native") do echo   ✓ libc_x64_64.native (%%~zf bytes)
) else (
    echo   ✗ libc_x64_64.native (missing)
)

if exist "bin\layer2\astc_x64_64.native" (
    for %%f in ("bin\layer2\astc_x64_64.native") do echo   ✓ astc_x64_64.native (%%~zf bytes)
) else (
    echo   ✗ astc_x64_64.native (missing)
)

echo.
echo Layer 3 (ASTC Programs):
if exist "bin\layer3\c99.astc" (
    for %%f in ("bin\layer3\c99.astc") do echo   ✓ c99.astc (%%~zf bytes)
) else (
    echo   ✗ c99.astc (missing)
)

echo.
echo ========================================
echo Build Process Completed Successfully!
echo ========================================

echo.
echo Three-Layer Architecture Ready:
echo   1. Loader: bin\layer1\loader_x64_64.exe
echo   2. VM Runtime: bin\layer2\vm_x64_64.native
echo   3. C99 Compiler: bin\layer3\c99.astc
echo.
echo Usage Example:
echo   bin\layer1\loader_x64_64.exe -m bin\layer2\vm_x64_64.native bin\layer3\c99.astc -- hello.c -o hello.exe
echo.
echo Next Steps:
echo   1. Test the complete pipeline with a sample C program
echo   2. Verify end-to-end compilation flow
echo   3. Begin Stage 2 development (AI evolution)

exit /b 0
