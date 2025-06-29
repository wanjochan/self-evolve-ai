@echo off
REM C99 Compiler - PRD.md Three-Layer Architecture Implementation
REM Usage: c99_prd [options] input.c
REM 
REM This implements the correct PRD.md design:
REM   Loader: bin\c99_loader.exe
REM   Runtime: bin\c99_runtime_x64_64.rt  
REM   Program: bin\c99_program.astc

setlocal enabledelayedexpansion

REM Check if three-layer architecture components exist
if not exist "bin\c99_loader.exe" (
    echo ERROR: c99_loader.exe not found
    echo Please run build0.bat to create the three-layer architecture
    exit /b 1
)

if not exist "bin\c99_runtime_new_x64_64.rt" (
    echo ERROR: c99_runtime_new_x64_64.rt not found
    echo Please rebuild the C99 runtime
    exit /b 1
)

if not exist "bin\c99_program.astc" (
    echo ERROR: c99_program.astc not found
    echo Please run build0.bat to create the program layer
    exit /b 1
)

REM Parse arguments - pass all arguments directly to the C99 compiler program
set ARGS=%*

if "%ARGS%"=="" (
    echo C99 Compiler - Three-Layer Architecture Implementation
    echo Usage: %0 [options] input.c
    echo.
    echo This uses PRD.md three-layer architecture:
    echo   Loader: bin\c99_loader.exe
    echo   Runtime: bin\c99_runtime_x64_64.rt
    echo   Program: bin\c99_program.astc
    echo.
    echo For help: %0 --help
    exit /b 0
)

REM Execute C99 compiler using three-layer architecture with environment variables
echo C99 Compiler - Three-Layer Architecture (Self-Built Runtime)
echo Loader: bin\c99_loader.exe
echo Runtime: bin\c99_runtime_new_x64_64.rt (3193 bytes + header - Self-built with libc forwarding)
echo Program: bin\c99_program.astc
echo Arguments: %ARGS%
echo.

REM Set environment variable for C99 program arguments
set C99_ARGS=%ARGS%
echo Setting C99_ARGS=%C99_ARGS%

bin\c99_loader.exe -r bin\c99_runtime_new_x64_64.rt bin\c99_program.astc

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: C99 compilation failed (exit code %ERRORLEVEL%)
    exit /b %ERRORLEVEL%
)

echo.
echo SUCCESS: C99 compilation completed using three-layer architecture
exit /b 0
