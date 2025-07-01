@echo off
echo === Building evolver0 System (TinyCC-Free Version) ===

REM ============================================================
REM NOTICE: This script has been updated to eliminate TinyCC dependencies
REM It now uses the independent build system
REM ============================================================

echo.
echo === Step 1: Verify Independent Tools ===
echo Checking for independent toolchain...

if not exist "bin\tool_c2astc.exe" (
    echo ERROR: tool_c2astc.exe not found
    echo Please run build0_independent.bat first to create independent tools
    exit /b 1
)

if not exist "bin\tool_astc2native.exe" (
    echo ERROR: tool_astc2native.exe not found
    echo Please run build0_independent.bat first to create independent tools
    exit /b 1
)

echo SUCCESS: Independent tools verified
echo   - tool_c2astc.exe (C to ASTC compiler)
echo   - tool_astc2native.exe (ASTC to native converter)

echo.
echo === Step 2: Build Loader Layer (Independent) ===
echo Building evolver0_loader using independent toolchain...
echo Compiling core_loader.c to ASTC...
bin\tool_c2astc.exe src\core\loader\core_loader.c bin\evolver0_loader.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: evolver0_loader compilation failed!
    exit /b 3
)

echo Converting ASTC to native executable...
bin\tool_astc2native.exe bin\evolver0_loader.astc bin\evolver0_loader.exe
if %ERRORLEVEL% neq 0 (
    echo ERROR: evolver0_loader native conversion failed!
    exit /b 4
)
echo SUCCESS: evolver0_loader built independently

echo.
echo === Step 3: Build Runtime Layer ===
echo Generating evolver0_runtime.astc...
bin\tool_c2astc.exe src\legacy\evolver0_runtime.c bin\evolver0_runtime.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: evolver0_runtime.astc generation failed!
    exit /b 5
)
echo SUCCESS: evolver0_runtime.astc generated

echo Generating runtimex64_64.rt...
bin\tool_astc2native.exe bin\evolver0_runtime.astc bin\runtimex64_64.rt
if %ERRORLEVEL% neq 0 (
    echo ERROR: runtimex64_64.rt generation failed!
    exit /b 6
)
echo SUCCESS: runtimex64_64.rt generated

echo.
echo === Step 4: Build Program Layer ===
echo Generating evolver0_program.astc...
bin\tool_c2astc.exe src\legacy\evolver0_program.c bin\evolver0_program.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: evolver0_program.astc generation failed!
    exit /b 7
)
echo SUCCESS: evolver0_program.astc generated

echo.
echo === Step 5: System Test ===
echo Testing evolver0 three-layer architecture...
bin\evolver0_loader.exe bin\evolver0_program.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: evolver0 system test failed!
    exit /b 8
)
echo SUCCESS: evolver0 system test passed

echo.
echo === Build Complete ===
echo SUCCESS: evolver0 three-layer architecture built successfully!
echo.
echo Generated components:
echo   - Loader Layer: bin\evolver0_loader.exe
echo   - Runtime Layer: bin\runtimex64_64.rt
echo   - Program Layer: bin\evolver0_program.astc
echo.
echo evolver0 system is ready for self-bootstrap compilation!
