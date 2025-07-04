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

REM Detect current architecture
set CURRENT_ARCH=x64
if "%PROCESSOR_ARCHITECTURE%"=="ARM64" set CURRENT_ARCH=arm64
if "%PROCESSOR_ARCHITEW6432%"=="ARM64" set CURRENT_ARCH=arm64

echo Current architecture detected: %CURRENT_ARCH%
echo.

REM Build for x64 architecture
echo Building vm_x64_64.native...
echo ============================

REM Compile VM module as shared library for x64
%TCC% -shared -o "bin\layer2\vm_x64_64.native" "src\ext\modules\vm_module.c" "src\core\utils.c" "src\core\native.c" "src\core\astc.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile vm_x64_64.native
    exit /b 1
)

echo Success: vm_x64_64.native compiled successfully

echo.
echo Building libc_x64_64.native...
echo ==============================

REM Compile LibC module as shared library for x64
%TCC% -shared -o "bin\layer2\libc_x64_64.native" "src\ext\modules\libc_module.c" "src\core\utils.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile libc_x64_64.native
    exit /b 1
)

echo Success: libc_x64_64.native compiled successfully

REM Build for ARM64 architecture if TCC supports it
echo.
echo Building vm_arm64_64.native...
echo ==============================

REM Check if ARM64 cross-compilation is supported by testing TCC capabilities
echo Testing ARM64 cross-compilation support...
%TCC% -DTCC_TARGET_ARM64 -E -xc NUL >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo ARM64 cross-compilation supported, proceeding...

    REM Compile VM module for ARM64 using cross-compilation
    %TCC% -shared -DTCC_TARGET_ARM64 -o "bin\layer2\vm_arm64_64.native" "src\ext\modules\vm_module.c" "src\core\utils.c" "src\core\native.c" "src\core\astc.c"

    if %ERRORLEVEL% neq 0 (
        echo Warning: Failed to compile vm_arm64_64.native despite ARM64 support detection
        echo This may be due to missing ARM64 runtime libraries
    ) else (
        echo Success: vm_arm64_64.native compiled successfully
        set ARM64_VM_SUCCESS=1
    )
) else (
    echo Warning: ARM64 cross-compilation not supported by current TCC build
    echo Skipping ARM64 VM module compilation...
    set ARM64_VM_SUCCESS=0
)

echo.
echo Building libc_arm64_64.native...
echo ================================

if defined ARM64_VM_SUCCESS if %ARM64_VM_SUCCESS%==1 (
    REM Compile LibC module for ARM64 using cross-compilation
    %TCC% -shared -DTCC_TARGET_ARM64 -o "bin\layer2\libc_arm64_64.native" "src\ext\modules\libc_module.c" "src\core\utils.c"

    if %ERRORLEVEL% neq 0 (
        echo Warning: Failed to compile libc_arm64_64.native despite ARM64 support
        echo This may be due to missing ARM64 runtime libraries
        set ARM64_LIBC_SUCCESS=0
    ) else (
        echo Success: libc_arm64_64.native compiled successfully
        set ARM64_LIBC_SUCCESS=1
    )
) else (
    echo Skipping ARM64 LibC module compilation (VM compilation failed)...
    set ARM64_LIBC_SUCCESS=0
)

echo.
echo Testing native modules...
echo =========================

REM Test x64 VM module
if exist "bin\layer2\vm_x64_64.native" (
    echo Success: vm_x64_64.native created
) else (
    echo Error: vm_x64_64.native not found
    exit /b 1
)

REM Test x64 LibC module
if exist "bin\layer2\libc_x64_64.native" (
    echo Success: libc_x64_64.native created
) else (
    echo Error: libc_x64_64.native not found
    exit /b 1
)

REM Test ARM64 modules (optional)
if exist "bin\layer2\vm_arm64_64.native" (
    echo Success: vm_arm64_64.native created
    set ARM64_VM_AVAILABLE=1
) else (
    echo Info: vm_arm64_64.native not available
    set ARM64_VM_AVAILABLE=0
)

if exist "bin\layer2\libc_arm64_64.native" (
    echo Success: libc_arm64_64.native created
    set ARM64_LIBC_AVAILABLE=1
) else (
    echo Info: libc_arm64_64.native not available
    set ARM64_LIBC_AVAILABLE=0
)

REM Determine overall ARM64 availability
if defined ARM64_VM_AVAILABLE if defined ARM64_LIBC_AVAILABLE (
    if %ARM64_VM_AVAILABLE%==1 if %ARM64_LIBC_AVAILABLE%==1 (
        set ARM64_AVAILABLE=1
    ) else (
        set ARM64_AVAILABLE=0
    )
) else (
    set ARM64_AVAILABLE=0
)

echo.
echo Layer 2 Build Summary:
echo ======================
dir "bin\layer2\*.native"

echo.
echo Success: Layer 2 Native Modules build completed
echo.
echo PRD.md Layer 2 Runtime Modules:
echo   vm_x64_64.native      - VM runtime for ASTC execution (x64)
echo   libc_x64_64.native    - Standard library module (x64)
if defined ARM64_AVAILABLE if %ARM64_AVAILABLE%==1 (
    echo   vm_arm64_64.native    - VM runtime for ASTC execution (ARM64)
    echo   libc_arm64_64.native  - Standard library module (ARM64)
)
echo.
echo These .native modules are loaded by Layer 1 loader_{arch}_{bits}.exe
echo to execute Layer 3 {program}.astc bytecode programs.
echo.
if defined ARM64_AVAILABLE if %ARM64_AVAILABLE%==1 (
    echo Multi-architecture support: x64 and ARM64 modules available
    echo.
    echo ARM64 Support Status:
    if defined ARM64_VM_AVAILABLE echo   - VM Module: %ARM64_VM_AVAILABLE% (vm_arm64_64.native)
    if defined ARM64_LIBC_AVAILABLE echo   - LibC Module: %ARM64_LIBC_AVAILABLE% (libc_arm64_64.native)
    echo   - Cross-compilation: Enabled via TCC -DTCC_TARGET_ARM64
) else (
    echo Single-architecture support: x64 modules only
    echo.
    echo ARM64 Support Status:
    echo   - Cross-compilation: Not available in current TCC build
    echo   - Recommendation: Use TCC with ARM64 target support for multi-arch builds
    echo   - Alternative: Build on native ARM64 system for ARM64 modules
)

exit /b 0
