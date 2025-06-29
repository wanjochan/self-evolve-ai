@echo off
echo Testing Self-Evolving AI System...

echo Test 1: Basic compilation
bin\c99.bat tests\test_hello.c
if %ERRORLEVEL%==0 echo [PASS] Basic compilation

echo Test 2: Optimization
bin\c99.bat -O2 tests\test_constant_folding.c
if %ERRORLEVEL%==0 echo [PASS] Optimization

echo Test 3: Array support
bin\c99.bat tests\test_array.c
if %ERRORLEVEL%==0 echo [PASS] Array support

echo Test 4: Cross-platform Runtime
bin\tool_astc2rt.exe bin\evolver0_runtime.astc tests\test_runtime.rt
if %ERRORLEVEL%==0 echo [PASS] Cross-platform Runtime

echo All tests completed!
