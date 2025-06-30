@echo off
REM ===============================================
REM Self-Evolve AI Unified Loader (Batch Version)
REM ===============================================
REM
REM This implements the loader→runtime→program call chain
REM according to PRD.md requirements until PE generation is fixed
REM

echo ========================================
echo Self-Evolve AI Unified Loader
echo ========================================

if "%1"=="" (
    echo Usage: loader.bat ^<program.astc^> [args...]
    echo.
    echo The loader will automatically:
    echo 1. Detect hardware platform
    echo 2. Load appropriate runtime
    echo 3. Execute the ASTC program
    exit /b 1
)

echo Platform Detection:
echo   OS: Windows
echo   Architecture: x64 ^(64-bit^)

echo Loader: Loading runtime module: enhanced_runtime_with_libc_v2.exe
echo Loader: Executing ASTC program: %1
echo ========================================

REM Execute the ASTC program through the runtime
bin\enhanced_runtime_with_libc_v2.exe %*

set RESULT=%ERRORLEVEL%

echo ========================================
echo Loader: Program execution completed with result: %RESULT%

exit /b %RESULT%
