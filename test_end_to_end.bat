@echo off
echo ===============================================
echo End-to-End Integration Test
echo ===============================================

echo.
echo Step 1: Compile VM module to ASTC
bin\tool_c2astc.exe tests\vm_module.c tests\vm_module.astc
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile VM module to ASTC
    exit /b 1
)
echo ✓ VM module compiled to ASTC

echo.
echo Step 2: Convert VM ASTC to native module
bin\tool_astc2native.exe -vm tests\vm_module.astc tests\vm_x64_64.native
if %errorlevel% neq 0 (
    echo ERROR: Failed to convert VM ASTC to native
    exit /b 1
)
echo ✓ VM native module created

echo.
echo Step 3: Compile libc module to ASTC
bin\tool_c2astc.exe tests\libc_module.c tests\libc_module.astc
if %errorlevel% neq 0 (
    echo ERROR: Failed to compile libc module to ASTC
    exit /b 1
)
echo ✓ libc module compiled to ASTC

echo.
echo Step 4: Convert libc ASTC to native module
bin\tool_astc2native.exe -libc tests\libc_module.astc tests\libc_x64_64.native
if %errorlevel% neq 0 (
    echo ERROR: Failed to convert libc ASTC to native
    exit /b 1
)
echo ✓ libc native module created

echo.
echo Step 5: Test universal loader with VM module
src\core\loader\universal_loader.exe tests\vm_x64_64.native
if %errorlevel% neq 0 (
    echo ERROR: Failed to load VM module
    exit /b 1
)
echo ✓ VM module loaded and executed successfully

echo.
echo Step 6: Test universal loader with libc module
src\core\loader\universal_loader.exe tests\libc_x64_64.native
if %errorlevel% neq 0 (
    echo ERROR: Failed to load libc module
    exit /b 1
)
echo ✓ libc module loaded and executed successfully

echo.
echo ===============================================
echo All tests passed! End-to-end integration successful.
echo ===============================================

echo.
echo Generated files:
dir /b tests\*.astc tests\*.native 2>nul

echo.
echo File sizes:
for %%f in (tests\*.astc tests\*.native) do (
    echo   %%f: %%~zf bytes
)

echo.
echo Integration test completed successfully!
