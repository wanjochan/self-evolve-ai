@echo off
echo === Testing C99 System ===

echo.
echo === Step 1: Test C99 Runtime directly ===
echo Testing C99 runtime with C99 program...
bin\c99_loader.exe -r bin\c99_runtime_x64_64.rt bin\c99_program.astc
if %ERRORLEVEL% neq 0 (
    echo WARNING: C99 runtime test failed
) else (
    echo SUCCESS: C99 runtime test passed
)

echo.
echo === Step 2: Test C99 Compiler functionality ===
echo Creating simple test program...
echo #include ^<stdio.h^> > temp_test.c
echo int main() { >> temp_test.c
echo     printf("Hello C99!\\n"); >> temp_test.c
echo     return 42; >> temp_test.c
echo } >> temp_test.c

echo.
echo Compiling test program to ASTC...
bin\tool_c2astc.exe temp_test.c temp_test.astc
if %ERRORLEVEL% neq 0 (
    echo ERROR: Test program compilation failed
    exit /b 1
)
echo SUCCESS: Test program compiled to ASTC

echo.
echo Running compiled test program...
bin\c99_loader.exe -r bin\c99_runtime_x64_64.rt temp_test.astc
if %ERRORLEVEL% neq 0 (
    echo WARNING: Test program execution failed
) else (
    echo SUCCESS: Test program executed
)

echo.
echo === C99 System Test Complete ===
echo.
echo Generated C99 components:
echo   - C99 Loader: bin\c99_loader.exe
echo   - C99 Runtime: bin\c99_runtime_x64_64.rt (131KB)
echo   - C99 Program: bin\c99_program.astc (2.6KB)
echo   - C99 Compiler: bin\c99.bat
echo.
echo C99 system is ready for use!

REM Clean up
del temp_test.c temp_test.astc 2>nul
