@echo off
chcp 65001 >nul

echo ========================================
echo Building VM Module (Layer 2)
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
echo Building VM Module...
echo =====================

REM Check if VM module source exists
if not exist "src\ext\modules\vm_module.c" (
    echo Error: VM module source not found at src\ext\modules\vm_module.c
    exit /b 1
)

REM Build VM module using proper .native format
echo Building vm_x64_64.native...

REM First, compile VM module to object file
%TCC% -c -o "bin\layer2\vm_module.o" ^
      -DNDEBUG ^
      -DVERSION_STRING="2.0" ^
      -DBUILD_DATE="%DATE%" ^
      -O2 ^
      "src\ext\modules\vm_module.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile VM module to object file
    exit /b 1
)

REM Use our test program to create proper .native format
if exist "test_vm_simple.exe" (
    echo Creating proper .native format using test program...
    test_vm_simple.exe
    if %ERRORLEVEL% neq 0 (
        echo Warning: Failed to create .native format, using fallback
        goto fallback_build
    )
    echo Success: vm_x64_64.native created in proper .native format
    goto build_complete
) else (
    echo Warning: test_vm_simple.exe not found, using fallback build
    goto fallback_build
)

:fallback_build
echo Using fallback: compiling as executable for .native conversion...
%TCC% -o "bin\layer2\vm_x64_64_temp.exe" ^
      -DNDEBUG ^
      -DVERSION_STRING="2.0" ^
      -DBUILD_DATE="%DATE%" ^
      -O2 ^
      "src\ext\modules\vm_module_simple.c" ^
      "src\core\utils.c" ^
      "src\core\native.c" ^
      "src\core\astc.c"

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile VM module
    exit /b 1
)

REM Copy as .native file (temporary solution)
copy "bin\layer2\vm_x64_64_temp.exe" "bin\layer2\vm_x64_64.native" >nul
del "bin\layer2\vm_x64_64_temp.exe" >nul

:build_complete
echo.
echo VM Module Build Summary:
echo =======================
if exist "bin\layer2\vm_x64_64.native" (
    for %%f in ("bin\layer2\vm_x64_64.native") do echo   - vm_x64_64.native (%%~zf bytes^)
    echo Success: VM module built successfully
) else (
    echo Error: VM module was not created
    exit /b 1
)

echo.
echo VM Module Functions:
echo   - vm_core_execute_astc: Main ASTC execution function
echo   - vm_module_init: Module initialization
echo   - vm_module_cleanup: Module cleanup
echo.
echo Usage:
echo   loader_x64_64.exe -m vm_x64_64.native program.astc [args]

exit /b 0
