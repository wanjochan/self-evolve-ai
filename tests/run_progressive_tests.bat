@echo off
echo ========================================
echo Progressive C2ASTC Test Suite
echo ========================================

set PASSED=0
set FAILED=0

echo.
echo Test 1: Basic Return Value
bin\tool_c2astc.exe tests\test_01_basic_return.c tests\test_01.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 1 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 1 FAILED
    set /a FAILED+=1
)

echo.
echo Test 2: Variable Declaration
bin\tool_c2astc.exe tests\test_02_variable_declaration.c tests\test_02.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 2 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 2 FAILED
    set /a FAILED+=1
)

echo.
echo Test 3: Arithmetic Operations
bin\tool_c2astc.exe tests\test_03_arithmetic.c tests\test_03.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 3 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 3 FAILED
    set /a FAILED+=1
)

echo.
echo Test 4: If Statement
bin\tool_c2astc.exe tests\test_04_if_statement.c tests\test_04.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 4 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 4 FAILED
    set /a FAILED+=1
)

echo.
echo Test 5: While Loop
bin\tool_c2astc.exe tests\test_05_while_loop.c tests\test_05.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 5 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 5 FAILED
    set /a FAILED+=1
)

echo.
echo Test 6: Array Access
bin\tool_c2astc.exe tests\test_06_array_access.c tests\test_06.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 6 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 6 FAILED
    set /a FAILED+=1
)

echo.
echo Test 7: Basic Struct
bin\tool_c2astc.exe tests\test_07_struct_basic.c tests\test_07.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 7 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 7 FAILED
    set /a FAILED+=1
)

echo.
echo Test 8: Basic Pointer
bin\tool_c2astc.exe tests\test_08_pointer_basic.c tests\test_08.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 8 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 8 FAILED
    set /a FAILED+=1
)

echo.
echo Test 9: Function Call
bin\tool_c2astc.exe tests\test_09_function_call.c tests\test_09.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 9 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 9 FAILED
    set /a FAILED+=1
)

echo.
echo Test 10: Printf Simple
bin\tool_c2astc.exe tests\test_10_printf_simple.c tests\test_10.astc
if %ERRORLEVEL% EQU 0 (
    echo ✓ Test 10 PASSED
    set /a PASSED+=1
) else (
    echo ✗ Test 10 FAILED
    set /a FAILED+=1
)

echo.
echo ========================================
echo Test Results: %PASSED% passed, %FAILED% failed
echo ========================================
