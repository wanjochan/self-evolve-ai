@echo off
chcp 65001 >nul
echo === Building evolver0 System ===

REM Set paths
set TCC=external\tcc-win\tcc\tcc.exe
set SRC_EVOLVER0=src\evolver0
set SRC_AI=src\ai
set SRC_TOOLS=src\tools
set SRC_RUNTIME=src\runtime
set BIN=bin
set BUILD=build

echo.
echo Step 1: Building core tools...

REM Build C to ASTC converter
echo Building tool_c2astc...
%TCC% -o %BIN%\tool_c2astc.exe %SRC_TOOLS%\tool_c2astc.c %SRC_TOOLS%\c2astc.c -I%SRC_RUNTIME%
if errorlevel 1 (
    echo ERROR: Failed to build tool_c2astc
    pause
    exit /b 1
)

REM Build ASTC to binary converter
echo Building tool_astc2bin...
%TCC% -o %BIN%\tool_astc2bin.exe %SRC_TOOLS%\tool_astc2bin.c -I%SRC_RUNTIME%
if errorlevel 1 (
    echo ERROR: Failed to build tool_astc2bin
    pause
    exit /b 1
)

echo.
echo Step 2: Building evolver0 loader...
%TCC% -o %BIN%\evolver0_loader.exe %SRC_EVOLVER0%\evolver0_loader.c -I%SRC_RUNTIME%
if errorlevel 1 (
    echo ERROR: Failed to build evolver0_loader
    pause
    exit /b 1
)

echo.
echo Step 3: Generating evolver0 runtime...
echo Compiling runtime C to ASTC...
%BIN%\tool_c2astc.exe %SRC_EVOLVER0%\evolver0_runtime.c %BIN%\evolver0_runtime.astc
if errorlevel 1 (
    echo ERROR: Failed to compile runtime to ASTC
    pause
    exit /b 1
)

echo Converting runtime ASTC to binary...
%BIN%\tool_astc2bin.exe %BIN%\evolver0_runtime.astc %BIN%\evolver0_runtime.bin
if errorlevel 1 (
    echo ERROR: Failed to convert runtime to binary
    pause
    exit /b 1
)

echo.
echo Step 4: Generating evolver0 program...
echo Compiling program C to ASTC...
%BIN%\tool_c2astc.exe %SRC_EVOLVER0%\evolver0_program.c %BIN%\evolver0_program.astc
if errorlevel 1 (
    echo ERROR: Failed to compile program to ASTC
    pause
    exit /b 1
)

echo.
echo Step 5: Building AI modules...

REM Build AI evolution engine
echo Building AI evolution engine...
%TCC% -o %BIN%\test_ai_evolution.exe tests\test_ai_evolution.c %SRC_AI%\ai_evolution.c %SRC_TOOLS%\c2astc.c -I%SRC_AI% -I%SRC_RUNTIME%
if errorlevel 1 (
    echo WARNING: Failed to build AI evolution test
)

REM Build AI learning engine
echo Building AI learning engine...
%TCC% -o %BIN%\test_ai_learning.exe tests\test_ai_learning.c %SRC_AI%\ai_learning.c %SRC_TOOLS%\c2astc.c -I%SRC_AI% -I%SRC_RUNTIME%
if errorlevel 1 (
    echo WARNING: Failed to build AI learning test
)

REM Build AI optimizer
echo Building AI optimizer...
%TCC% -o %BIN%\test_ai_optimizer.exe tests\test_ai_optimizer.c %SRC_AI%\ai_optimizer.c %SRC_AI%\ai_learning.c %SRC_TOOLS%\c2astc.c -I%SRC_AI% -I%SRC_RUNTIME%
if errorlevel 1 (
    echo WARNING: Failed to build AI optimizer test
)

REM Build complete AI adaptive framework
echo Building AI adaptive framework...
%TCC% -o %BIN%\test_ai_adaptive_framework.exe tests\test_ai_adaptive_framework.c %SRC_AI%\ai_adaptive_framework.c %SRC_AI%\ai_optimizer.c %SRC_AI%\ai_learning.c %SRC_AI%\ai_evolution.c %SRC_TOOLS%\c2astc.c -I%SRC_AI% -I%SRC_RUNTIME%
if errorlevel 1 (
    echo WARNING: Failed to build AI adaptive framework test
)

echo.
echo Step 6: Building complete system test...
%TCC% -o %BIN%\test_complete_evolver0_system.exe tests\test_complete_evolver0_system.c %SRC_AI%\ai_adaptive_framework.c %SRC_AI%\ai_optimizer.c %SRC_AI%\ai_learning.c %SRC_AI%\ai_evolution.c %SRC_TOOLS%\c2astc.c -I%SRC_AI% -I%SRC_RUNTIME%
if errorlevel 1 (
    echo WARNING: Failed to build complete system test
)

echo.
echo Step 7: Testing self-bootstrap capability...
echo Testing evolver0 self-compilation...
%BIN%\evolver0_loader.exe %BIN%\evolver0_runtime.bin %BIN%\evolver0_program.astc %SRC_EVOLVER0%\evolver0_program.c %BIN%\evolver1_program.astc
if errorlevel 1 (
    echo WARNING: Self-bootstrap test failed
) else (
    echo SUCCESS: evolver0 can compile itself!
)

echo.
echo === Build Summary ===
echo Core Tools:
if exist %BIN%\tool_c2astc.exe echo   ✅ tool_c2astc.exe
if exist %BIN%\tool_astc2bin.exe echo   ✅ tool_astc2bin.exe

echo.
echo evolver0 System:
if exist %BIN%\evolver0_loader.exe echo   ✅ evolver0_loader.exe
if exist %BIN%\evolver0_runtime.bin echo   ✅ evolver0_runtime.bin
if exist %BIN%\evolver0_program.astc echo   ✅ evolver0_program.astc

echo.
echo AI Modules:
if exist %BIN%\test_ai_evolution.exe echo   ✅ AI Evolution Engine
if exist %BIN%\test_ai_learning.exe echo   ✅ AI Learning Engine
if exist %BIN%\test_ai_optimizer.exe echo   ✅ AI Optimizer Engine
if exist %BIN%\test_ai_adaptive_framework.exe echo   ✅ AI Adaptive Framework

echo.
echo System Tests:
if exist %BIN%\test_complete_evolver0_system.exe echo   ✅ Complete System Test
if exist %BIN%\evolver1_program.astc echo   ✅ Self-Bootstrap Test

echo.
echo 🎉 evolver0 build complete!
echo.
echo To run the complete system test:
echo   %BIN%\test_complete_evolver0_system.exe
echo.
echo To test AI adaptive framework:
echo   %BIN%\test_ai_adaptive_framework.exe
echo.
pause
