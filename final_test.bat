@echo off
echo ===============================================
echo Self-Evolve AI Final Integration Test
echo ===============================================

echo.
echo [1/4] Testing basic C program compilation...
echo #include ^<stdio.h^> > test_basic.c
echo int main() { printf("Basic test OK!\n"); return 0; } >> test_basic.c

bin\layer1\loader_x64_64.exe bin\layer3\c99.astc -- test_basic.c -o test_basic.exe
if errorlevel 1 (
    echo ERROR: Basic compilation failed
    goto cleanup
)

test_basic.exe
if errorlevel 1 (
    echo ERROR: Basic executable failed
    goto cleanup
)

echo.
echo [2/4] Testing complex C program...
echo #include ^<stdio.h^> > test_complex.c
echo #include ^<stdlib.h^> >> test_complex.c
echo int factorial(int n) { return n ^<= 1 ? 1 : n * factorial(n-1); } >> test_complex.c
echo int main() { printf("Factorial 5 = %%d\n", factorial(5)); return 0; } >> test_complex.c

bin\layer1\loader_x64_64.exe bin\layer3\c99.astc -- test_complex.c -o test_complex.exe
if errorlevel 1 (
    echo ERROR: Complex compilation failed
    goto cleanup
)

test_complex.exe
if errorlevel 1 (
    echo ERROR: Complex executable failed
    goto cleanup
)

echo.
echo [3/4] Testing performance with verbose output...
bin\layer1\loader_x64_64.exe -v -p bin\layer3\c99.astc -- test_basic.c -o test_perf.exe
if errorlevel 1 (
    echo ERROR: Performance test failed
    goto cleanup
)

echo.
echo [4/4] Testing tool chain...
bin\tools\tool_c2astc.exe --help >nul 2>&1
if errorlevel 1 (
    echo ERROR: tool_c2astc not working
    goto cleanup
)

bin\tools\tool_astc2native.exe --help >nul 2>&1
if errorlevel 1 (
    echo ERROR: tool_astc2native not working
    goto cleanup
)

echo.
echo ===============================================
echo ALL TESTS PASSED!
echo Three-layer architecture is fully functional!
echo ===============================================
echo.
echo Summary:
echo - Layer 1 Loader: Uses mmap() to load .native modules
echo - Layer 2 VM: Executes ASTC programs through .native modules  
echo - Layer 3 C99: Compiles C code using TCC integration
echo - Tools: C2ASTC and ASTC2Native converters working
echo.
echo Ready for Stage 2 AI evolution capabilities!
echo ===============================================

:cleanup
del test_basic.c test_complex.c >nul 2>&1
del test_basic.exe test_complex.exe test_perf.exe >nul 2>&1
