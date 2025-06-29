@echo off
REM Self-Hosted Build System - 完全自举的构建系统
REM 根据PRD.md要求，使用自研C99编译器构建整个系统，摆脱TinyCC依赖

echo === Self-Hosted Build System ===
echo Building system using our own C99 compiler (no external CC dependency)
echo.

REM 检查是否存在基础的自研编译器工具
if not exist "bin\c99_loader.exe" (
    echo ERROR: c99_loader.exe not found. Need bootstrap build first.
    echo Please run build0.bat once to create initial tools.
    exit /b 1
)

if not exist "bin\c99_runtime_x64_64.rt" (
    echo ERROR: c99_runtime_x64_64.rt not found. Need bootstrap build first.
    exit /b 1
)

echo === Phase 1: Self-Compile Core Tools ===
echo.

echo Step 1.1: Self-compile tool_c2astc using our C99 compiler...
bin\c99.bat -v -o bin\tool_c2astc_self src\tool_c2astc.c
if %ERRORLEVEL% neq 0 (
    echo ERROR: Self-compilation of tool_c2astc failed!
    exit /b 2
)
echo SUCCESS: tool_c2astc self-compiled as three-layer architecture

echo Step 1.2: Self-compile tool_astc2rt using our C99 compiler...
bin\c99.bat -v -o bin\tool_astc2rt_self src\tool_astc2rt.c
if %ERRORLEVEL% neq 0 (
    echo ERROR: Self-compilation of tool_astc2rt failed!
    exit /b 3
)
echo SUCCESS: tool_astc2rt self-compiled as three-layer architecture

echo.
echo === Phase 2: Self-Build Three-Layer Architecture ===
echo.

echo Step 2.1: Self-compile Loader Layer...
bin\c99.bat -v -o bin\c99_loader_self src\runtime\core_loader.c
if %ERRORLEVEL% neq 0 (
    echo ERROR: Self-compilation of loader failed!
    exit /b 4
)
echo SUCCESS: Loader self-compiled as three-layer architecture

echo Step 2.2: Self-generate Runtime Layer...
bin\tool_c2astc_self.exe.bat src\c99_runtime.c bin\c99_runtime_self.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: Self-generation of runtime ASTC failed!
    exit /b 5
)

bin\tool_astc2rt_self.exe.bat bin\c99_runtime_self.astc bin\c99_runtime_self_x64_64.rt
if %ERRORLEVEL% neq 0 (
    echo ERROR: Self-generation of runtime RT failed!
    exit /b 6
)
echo SUCCESS: Runtime self-generated using three-layer architecture

echo Step 2.3: Self-generate Program Layer...
bin\tool_c2astc_self.exe.bat src\c99_program.c bin\c99_program_self.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: Self-generation of program ASTC failed!
    exit /b 7
)
echo SUCCESS: Program self-generated using three-layer architecture

echo.
echo === Phase 3: Verification ===
echo.

echo Step 3.1: Test self-hosted C99 compiler...
if not exist "tests" mkdir tests
echo int main() { return 42; } > tests\test_self_hosted.c
bin\c99_loader_self.exe.bat -r bin\c99_runtime_self_x64_64.rt bin\c99_program_self.astc tests\test_self_hosted.c
if %ERRORLEVEL% neq 0 (
    echo ERROR: Self-hosted compiler test failed!
    exit /b 8
)
echo SUCCESS: Self-hosted three-layer architecture verified

echo.
echo === SUCCESS: Self-Hosted Build Complete ===
echo.
echo Generated self-hosted components (Three-Layer Architecture):
echo   - Loader: bin\c99_loader_self.exe.bat
echo   - Runtime: bin\c99_runtime_self_x64_64.rt
echo   - Program: bin\c99_program_self.astc
echo   - Tools: bin\tool_c2astc_self.exe.bat, bin\tool_astc2rt_self.exe.bat
echo.
echo SUCCESS: Three-layer architecture is now completely self-hosted!
echo The system follows PRD.md design: Loader + Runtime + Program
echo Next: Verify complete independence from TinyCC
