@echo off
chcp 65001 >nul
echo ========================================
echo Building ASTC2Native Tool
echo ========================================

REM Check if source exists
if not exist "src\tools\tool_astc2native.c" (
    echo Error: ASTC2Native tool source not found at src\tools\tool_astc2native.c
    exit /b 1
)

REM Create tools directory if it doesn't exist
if not exist "tools" mkdir tools

echo Building astc2native.exe...
echo ========================

REM Compile astc2native tool using TCC (simplified version)
echo Compiling src\tools\tool_astc2native.c to tools\astc2native.exe...
external\tcc-win\tcc\tcc.exe -o "tools\astc2native.exe" "src\tools\tool_astc2native.c" "src\core\native.c" "src\core\utils.c" "src\core\astc.c" -Isrc/core -Isrc/ext -DNDEBUG -O2 -DSIMPLIFIED_BUILD

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile astc2native tool
    exit /b 1
)

echo Checking if astc2native.exe was created...
if exist "tools\astc2native.exe" (
    for %%f in ("tools\astc2native.exe") do echo   - astc2native.exe (%%~zf bytes^)
    echo Success: ASTC2Native tool built successfully
) else (
    echo Error: ASTC2Native tool was not created
    exit /b 1
)

echo.
echo ========================================
echo Testing ASTC2Native Tool...
echo ========================================

REM Test the tool
echo Testing astc2native.exe --help...
tools\astc2native.exe --help

echo.
echo ASTC2Native Tool Build Summary:
echo ==============================
echo   - astc2native.exe: ASTC to native module converter
echo   - Location: tools\astc2native.exe
echo   - Usage: tools\astc2native.exe input.astc output.native
echo.
echo Build completed successfully!
