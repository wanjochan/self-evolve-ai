@echo off
chcp 65001 >nul

echo ========================================
echo Building Layer 2 Native Modules
echo Modular Build System
echo ========================================

echo Using modular build system...
echo Calling individual module build scripts...
echo.

REM Build VM Module
echo ========================================
echo Building VM Module...
echo ========================================
call build_vm_module.bat
if %ERRORLEVEL% neq 0 (
    echo Error: VM module build failed
    exit /b 1
)

echo.
REM Build LibC Module
echo ========================================
echo Building LibC Module...
echo ========================================
call build_libc_module.bat
if %ERRORLEVEL% neq 0 (
    echo Error: LibC module build failed
    exit /b 1
)

echo.
REM Build ASTC Module
echo ========================================
echo Building ASTC Module...
echo ========================================
call build_astc_module.bat
if %ERRORLEVEL% neq 0 (
    echo Error: ASTC module build failed
    exit /b 1
)

echo.
REM Build STD Module
echo ========================================
echo Building STD Module...
echo ========================================
call build_std_module.bat
if %ERRORLEVEL% neq 0 (
    echo Error: STD module build failed
    exit /b 1
)

echo.
echo Layer 2 Build Summary:
echo ======================
dir "bin\layer2\*.native"

echo.
echo Success: All Layer 2 modules built successfully
echo.
echo Available Native Modules:
if exist "bin\layer2\vm_x64_64.native" echo   - vm_x64_64.native (VM Runtime)
if exist "bin\layer2\libc_x64_64.native" echo   - libc_x64_64.native (C Standard Library)
if exist "bin\layer2\astc_x64_64.native" echo   - astc_x64_64.native (ASTC Compiler)
if exist "bin\layer2\std_x64_64.native" echo   - std_x64_64.native (Standard Library)
echo.
echo These .native modules are loaded by Layer 1 loader to execute Layer 3 programs.
echo All modules follow PRD.md specification for native module format.

exit /b 0
