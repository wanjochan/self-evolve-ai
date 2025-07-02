@echo off
chcp 65001 >nul
REM test_build.bat - Simple test build to verify antivirus solution

echo ========================================
echo Self-Evolve AI Test Build
echo Testing Antivirus False Positive Solutions
echo ========================================

REM Set directories
set BUILD_DIR=test_build
set SOURCE_DIR=%~dp0..

REM Clean old build
if exist %BUILD_DIR% (
    echo Cleaning old build directory...
    rmdir /s /q %BUILD_DIR%
)

REM Create build directory
mkdir %BUILD_DIR%
cd %BUILD_DIR%

echo.
echo Configuring CMake project...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release %SOURCE_DIR%

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed! Trying with different generator...
    echo.
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release %SOURCE_DIR%
    
    if %ERRORLEVEL% neq 0 (
        echo CMake configuration failed with all generators!
        echo Checking available generators...
        cmake --help
        pause
        exit /b 1
    )
)

echo.
echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.

REM Check if executable was created
if exist "bin\Release\loader.exe" (
    echo Generated file: bin\Release\loader.exe
    dir "bin\Release\loader.exe"
    echo.
    echo File properties:
    powershell "Get-ItemProperty 'bin\Release\loader.exe' | Select-Object Name, Length, CreationTime, LastWriteTime"
) else if exist "loader.exe" (
    echo Generated file: loader.exe
    dir "loader.exe"
    echo.
    echo File properties:
    powershell "Get-ItemProperty 'loader.exe' | Select-Object Name, Length, CreationTime, LastWriteTime"
) else (
    echo Warning: No executable found!
    echo Searching for any .exe files...
    dir /s *.exe
)

echo.
echo Testing executable...
if exist "bin\Release\loader.exe" (
    echo Running: bin\Release\loader.exe
    "bin\Release\loader.exe"
) else if exist "loader.exe" (
    echo Running: loader.exe
    "loader.exe"
) else (
    echo No executable to test!
)

echo.
echo ========================================
echo Test completed!
echo ========================================
echo.
echo Next steps:
echo 1. Check if the executable triggers antivirus warnings
echo 2. If still flagged, try adding to antivirus whitelist
echo 3. Consider getting a code signing certificate
echo.
pause
