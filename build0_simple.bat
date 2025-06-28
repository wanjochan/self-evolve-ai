@echo off
echo === Building evolver0 System ===

set TCC=external\tcc-win\tcc\tcc.exe

echo.
echo === Step 1: Build Tools ===
echo Building tool_c2astc...
%TCC% -o bin\tool_c2astc.exe src\tool_c2astc.c src\runtime\compiler_c2astc.c -Isrc\runtime
if %ERRORLEVEL% neq 0 (
    echo ERROR: tool_c2astc build failed!
    exit /b 1
)
echo SUCCESS: tool_c2astc built

echo Building tool_astc2rt...
%TCC% -o bin\tool_astc2rt.exe src\tool_astc2rt.c src\runtime\compiler_astc2rt.c src\runtime\compiler_c2astc.c src\runtime\compiler_codegen.c src\runtime\compiler_codegen_x64.c -Isrc\runtime
if %ERRORLEVEL% neq 0 (
    echo ERROR: tool_astc2rt build failed!
    exit /b 2
)
echo SUCCESS: tool_astc2rt built

echo.
echo === Step 2: Build Loader Layer ===
echo Building evolver0_loader...
%TCC% -o bin\evolver0_loader.exe src\runtime\core_loader.c src\runtime\platform_minimal.c -Isrc\runtime
if %ERRORLEVEL% neq 0 (
    echo ERROR: evolver0_loader build failed!
    exit /b 3
)
echo SUCCESS: evolver0_loader built

echo.
echo === Step 3: Build Runtime Layer ===
echo Generating evolver0_runtime.astc...
bin\tool_c2astc.exe src\evolver0_runtime.c bin\evolver0_runtime.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: evolver0_runtime.astc generation failed!
    exit /b 4
)
echo SUCCESS: evolver0_runtime.astc generated

echo Generating runtimex64_64.rt...
bin\tool_astc2rt.exe bin\evolver0_runtime.astc bin\runtimex64_64.rt
if %ERRORLEVEL% neq 0 (
    echo ERROR: runtimex64_64.rt generation failed!
    exit /b 5
)
echo SUCCESS: runtimex64_64.rt generated

echo.
echo === Step 4: Build Program Layer ===
echo Generating evolver0_program.astc...
bin\tool_c2astc.exe src\evolver0_program.c bin\evolver0_program.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: evolver0_program.astc generation failed!
    exit /b 6
)
echo SUCCESS: evolver0_program.astc generated

echo.
echo === Step 5: System Test ===
echo Testing evolver0 three-layer architecture...
bin\evolver0_loader.exe bin\evolver0_program.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: evolver0 system test failed!
    exit /b 7
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
