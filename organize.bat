@echo off
chcp 65001 >nul
echo === Organizing evolver0 project structure ===

REM Create directories
echo Creating directories...
if not exist "src\evolver0" mkdir "src\evolver0"
if not exist "src\ai" mkdir "src\ai"
if not exist "src\tools" mkdir "src\tools"
if not exist "src\runtime" mkdir "src\runtime"
if not exist "bin" mkdir "bin"
if not exist "docs" mkdir "docs"
if not exist "external" mkdir "external"
if not exist "build" mkdir "build"

REM Move evolver0 core files
echo Moving evolver0 core files...
if exist "evolver0*.c" move "evolver0*.c" "src\evolver0\" >nul 2>&1
if exist "evolver0*.h" move "evolver0*.h" "src\evolver0\" >nul 2>&1

REM Move AI files
echo Moving AI files...
if exist "ai_*.c" move "ai_*.c" "src\ai\" >nul 2>&1
if exist "ai_*.h" move "ai_*.h" "src\ai\" >nul 2>&1

REM Move tool files
echo Moving tool files...
if exist "tool_*.c" move "tool_*.c" "src\tools\" >nul 2>&1
if exist "c2astc.c" move "c2astc.c" "src\tools\" >nul 2>&1
if exist "c2astc.h" move "c2astc.h" "src\tools\" >nul 2>&1
if exist "x64_codegen.c" move "x64_codegen.c" "src\tools\" >nul 2>&1
if exist "x64_codegen.h" move "x64_codegen.h" "src\tools\" >nul 2>&1

REM Move runtime files
echo Moving runtime files...
if exist "runtime.c" move "runtime.c" "src\runtime\" >nul 2>&1
if exist "runtime.h" move "runtime.h" "src\runtime\" >nul 2>&1
if exist "astc.h" move "astc.h" "src\runtime\" >nul 2>&1
if exist "loader.h" move "loader.h" "src\runtime\" >nul 2>&1
if exist "program.h" move "program.h" "src\runtime\" >nul 2>&1

REM Move executables
echo Moving executables...
if exist "*.exe" move "*.exe" "bin\" >nul 2>&1

REM Move binary files
echo Moving binary files...
if exist "*.bin" move "*.bin" "bin\" >nul 2>&1
if exist "*.astc" move "*.astc" "bin\" >nul 2>&1

REM Move object files
echo Moving object files...
if exist "*.o" move "*.o" "build\" >nul 2>&1

REM Move documentation
echo Moving documentation...
if exist "*.md" move "*.md" "docs\" >nul 2>&1

REM Move external dependencies
echo Moving external dependencies...
if exist "tcc" move "tcc" "external\" >nul 2>&1
if exist "tcc-src" move "tcc-src" "external\" >nul 2>&1
if exist "tcc-win" move "tcc-win" "external\" >nul 2>&1

REM Move build scripts
echo Moving build scripts...
if exist "build_*.bat" move "build_*.bat" "build\" >nul 2>&1

echo.
echo Project structure organized successfully!
echo.
echo New directory structure:
echo   src/evolver0/    - evolver0 core source
echo   src/ai/          - AI modules
echo   src/tools/       - compilation tools
echo   src/runtime/     - runtime system
echo   bin/             - executables and binaries
echo   docs/            - documentation
echo   external/        - external dependencies
echo   build/           - build scripts and temp files
echo   tests/           - test files (unchanged)
echo.
