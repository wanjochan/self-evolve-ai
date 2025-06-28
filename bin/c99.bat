@echo off
REM C99 Compiler - TinyCC Compatible Interface
REM Usage: c99 [options] input.c

setlocal enabledelayedexpansion

REM Default values
set INPUT_FILE=
set OUTPUT_FILE=a.exe
set COMPILE_ONLY=0
set VERBOSE=0
set OPTIMIZATION=0

REM Parse command line arguments
:parse_args
if "%1"=="" goto check_input
if "%1"=="-h" goto show_help
if "%1"=="--help" goto show_help
if "%1"=="-v" (
    set VERBOSE=1
    shift
    goto parse_args
)
if "%1"=="-c" (
    set COMPILE_ONLY=1
    shift
    goto parse_args
)
if "%1"=="-o" (
    if "%2"=="" (
        echo Error: -o requires output filename
        exit /b 1
    )
    set OUTPUT_FILE=%2
    shift
    shift
    goto parse_args
)
if "%1"=="-O1" (
    set OPTIMIZATION=1
    shift
    goto parse_args
)
if "%1"=="-O2" (
    set OPTIMIZATION=2
    shift
    goto parse_args
)
if "%1"=="-O3" (
    set OPTIMIZATION=3
    shift
    goto parse_args
)
REM Input file (no dash prefix)
if not "%1"=="-*" (
    set INPUT_FILE=%1
    shift
    goto parse_args
)
shift
goto parse_args

:check_input
if "%INPUT_FILE%"=="" goto show_help

REM Main compilation process
if %VERBOSE%==1 (
    echo C99 Compiler v1.0
    echo Input: %INPUT_FILE%
    echo Output: %OUTPUT_FILE%
    echo Optimization: O%OPTIMIZATION%
    echo.
)

REM Step 1: Compile C to ASTC
if %VERBOSE%==1 echo Step 1: Compiling C to ASTC...
bin\tool_c2astc.exe %INPUT_FILE% %INPUT_FILE%.astc
if %ERRORLEVEL% neq 0 (
    echo Error: C to ASTC compilation failed
    exit /b 1
)
if %VERBOSE%==1 echo Success: ASTC generated

REM If compile-only mode, stop here
if %COMPILE_ONLY%==1 (
    echo Compilation completed: %INPUT_FILE%.astc
    exit /b 0
)

REM Step 2: Generate executable using loader + runtime
if %VERBOSE%==1 echo Step 2: Creating executable...

REM Create a simple launcher script for the executable
echo @echo off > %OUTPUT_FILE%.bat
echo bin\c99_loader.exe -r bin\c99_runtime_x64_64.rt %INPUT_FILE%.astc %%* >> %OUTPUT_FILE%.bat

if %VERBOSE%==1 (
    echo Success: Executable created as %OUTPUT_FILE%.bat
    echo Usage: %OUTPUT_FILE%.bat [args]
) else (
    echo Compilation successful: %OUTPUT_FILE%.bat
)

exit /b 0

:show_help
echo C99 Compiler v1.0 - TinyCC Compatible
echo.
echo Usage: c99 [options] input.c
echo.
echo Options:
echo   -o file       Output executable name (default: a.exe)
echo   -c            Compile only, don't create executable
echo   -v            Verbose output
echo   -O1/-O2/-O3   Optimization level
echo   -h, --help    Show this help
echo.
echo Examples:
echo   c99 hello.c                 # Compile to a.exe.bat
echo   c99 -o hello hello.c        # Compile to hello.bat
echo   c99 -c hello.c              # Compile only to hello.c.astc
echo   c99 -v -O2 program.c        # Verbose, optimized compilation
echo.
exit /b 0
