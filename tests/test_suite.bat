@echo off
REM Self-Evolving AI System Test Suite
REM Tests various compiler features and optimizations

echo ========================================
echo Self-Evolving AI System Test Suite v1.0
echo ========================================
echo.

set TOTAL_TESTS=0
set PASSED_TESTS=0
set FAILED_TESTS=0

REM Test 1: Basic compilation
echo [Test 1] Basic compilation...
set /a TOTAL_TESTS+=1
bin\c99.bat tests\test_hello.c > nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] Basic compilation test
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Basic compilation test
    set /a FAILED_TESTS+=1
)

REM Test 2: Optimization levels
echo [Test 2] Compiler optimization levels...
set /a TOTAL_TESTS+=1
bin\c99.bat -O2 tests\test_constant_folding.c > nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] Optimization level test
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Optimization level test
    set /a FAILED_TESTS+=1
)

REM Test 3: Array support
echo [Test 3] Array initialization lists...
set /a TOTAL_TESTS+=1
bin\c99.bat tests\test_array.c > nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] Array support test
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Array support test
    set /a FAILED_TESTS+=1
)

REM Test 4: Error handling
echo [Test 4] Syntax error detection...
set /a TOTAL_TESTS+=1
bin\c99.bat tests\test_syntax_error.c > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [PASS] Error detection test
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Error detection test
    set /a FAILED_TESTS+=1
)

REM Test 5: Cross-platform Runtime generation
echo [Test 5] Cross-platform Runtime generation...
set /a TOTAL_TESTS+=1
bin\tool_astc2rt.exe bin\evolver0_runtime.astc tests\test_runtime.rt > nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] Cross-platform Runtime test
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Cross-platform Runtime test
    set /a FAILED_TESTS+=1
)

REM Test 6: Self-bootstrap compilation
echo [Test 6] Self-bootstrap compilation...
set /a TOTAL_TESTS+=1
bin\evolver0_loader.exe bin\evolver0_runtime_x64_64.rt bin\evolver0_program.astc --self-compile > nul 2>&1
if %ERRORLEVEL%==0 (
    echo [PASS] Self-bootstrap test
    set /a PASSED_TESTS+=1
) else (
    echo [FAIL] Self-bootstrap test
    set /a FAILED_TESTS+=1
)

echo.
echo ========================================
echo Test Results Summary
echo ========================================
echo Total tests: %TOTAL_TESTS%
echo Passed: %PASSED_TESTS%
echo Failed: %FAILED_TESTS%

if %FAILED_TESTS%==0 (
    echo.
    echo [SUCCESS] All tests passed! System is working correctly!
    exit /b 0
) else (
    echo.
    echo [WARNING] %FAILED_TESTS% test(s) failed, please check system status
    exit /b 1
)
