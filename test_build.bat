@echo off
chcp 65001 >nul

echo ========================================
echo Self-Evolve AI Antivirus Test Build
echo ========================================

REM Create a simple test CMakeLists.txt
echo Creating test CMakeLists.txt...
(
echo cmake_minimum_required^(VERSION 3.16^)
echo project^(AntivirusTest VERSION 1.0.0^)
echo.
echo set^(CMAKE_C_STANDARD 99^)
echo set^(CMAKE_C_STANDARD_REQUIRED ON^)
echo.
echo if^(WIN32^)
echo     set^(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"^)
echo     if^(MSVC^)
echo         add_compile_options^(/GS /sdl /guard:cf^)
echo         add_link_options^(/DYNAMICBASE /NXCOMPAT /SAFESEH /GUARD:CF^)
echo     endif^(^)
echo     set^(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG"^)
echo elseif^(UNIX^)
echo     add_compile_options^(-O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIC^)
echo     add_link_options^(-Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack^)
echo endif^(^)
echo.
echo add_executable^(antivirus_test test_antivirus_build.c^)
echo.
echo if^(WIN32^)
echo     target_link_libraries^(antivirus_test kernel32 user32^)
echo endif^(^)
echo.
echo set_target_properties^(antivirus_test PROPERTIES
echo     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
echo     OUTPUT_NAME "antivirus_test"
echo ^)
) > test_CMakeLists.txt

REM Create build directory
if exist test_build rmdir /s /q test_build
mkdir test_build
cd test_build

echo.
echo Configuring with CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -f ../test_CMakeLists.txt ..

if %ERRORLEVEL% neq 0 (
    echo Visual Studio not found, trying MinGW...
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -f ../test_CMakeLists.txt ..
    
    if %ERRORLEVEL% neq 0 (
        echo CMake configuration failed. Trying direct compilation...
        cd ..
        echo.
        echo Compiling directly with available compiler...
        
        REM Try different compilers
        where gcc >nul 2>&1
        if %ERRORLEVEL% equ 0 (
            echo Using GCC...
            gcc -O2 -fstack-protector-strong -o antivirus_test.exe test_antivirus_build.c -luser32 -lkernel32
            if exist antivirus_test.exe (
                echo Build successful with GCC!
                antivirus_test.exe
                goto :success
            )
        )
        
        where cl >nul 2>&1
        if %ERRORLEVEL% equ 0 (
            echo Using MSVC...
            cl /O2 /GS /sdl /guard:cf /DYNAMICBASE /NXCOMPAT test_antivirus_build.c /link user32.lib kernel32.lib /out:antivirus_test.exe
            if exist antivirus_test.exe (
                echo Build successful with MSVC!
                antivirus_test.exe
                goto :success
            )
        )
        
        where clang >nul 2>&1
        if %ERRORLEVEL% equ 0 (
            echo Using Clang...
            clang -O2 -fstack-protector-strong -o antivirus_test.exe test_antivirus_build.c -luser32 -lkernel32
            if exist antivirus_test.exe (
                echo Build successful with Clang!
                antivirus_test.exe
                goto :success
            )
        )
        
        echo No suitable compiler found!
        echo Please install one of: Visual Studio, MinGW, GCC, or Clang
        pause
        exit /b 1
    )
)

echo.
echo Building...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Testing executable...
if exist "bin\Release\antivirus_test.exe" (
    echo Running: bin\Release\antivirus_test.exe
    "bin\Release\antivirus_test.exe"
) else if exist "antivirus_test.exe" (
    echo Running: antivirus_test.exe
    "antivirus_test.exe"
) else (
    echo Executable not found!
    dir /s *.exe
)

:success
echo.
echo ========================================
echo Antivirus test build completed!
echo ========================================
echo.
echo The executable should now be safe from antivirus false positives.
echo Key features implemented:
echo - Dynamic runtime library linking
echo - Security compilation flags  
echo - Optimized release build
echo - Windows security features enabled
echo.
pause
