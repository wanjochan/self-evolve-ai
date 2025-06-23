@echo off
REM 编译和运行C2ASTC测试程序

REM 设置TCC路径
set TCC_PATH=..\..\tcc-win\tcc

REM 检查TCC是否存在
if not exist "%TCC_PATH%\tcc.exe" (
    echo Error: TCC compiler not found - %TCC_PATH%\tcc.exe
    exit /b 1
)

REM 编译所有测试
echo Compiling all tests...

REM 编译run_tests.c
echo Compiling run_tests.c...
"%TCC_PATH%\tcc.exe" -I..\..\. -o run_tests.exe run_tests.c ..\..\c2astc.c
if %ERRORLEVEL% neq 0 (
    echo Compilation of run_tests.c failed!
    exit /b 1
)

REM 编译test1.c
echo Compiling test1.c...
"%TCC_PATH%\tcc.exe" -I..\..\. -o test1.exe test1.c ..\..\c2astc.c
if %ERRORLEVEL% neq 0 (
    echo Compilation of test1.c failed!
    exit /b 1
)

REM 编译test2.c
echo Compiling test2.c...
"%TCC_PATH%\tcc.exe" -I..\..\. -o test2.exe test2.c ..\..\c2astc.c
if %ERRORLEVEL% neq 0 (
    echo Compilation of test2.c failed!
    exit /b 1
)

REM 编译test3.c
echo Compiling test3.c...
"%TCC_PATH%\tcc.exe" -I..\..\. -o test3.exe test3.c ..\..\c2astc.c
if %ERRORLEVEL% neq 0 (
    echo Compilation of test3.c failed!
    exit /b 1
)

REM 编译test4.c
echo Compiling test4.c...
"%TCC_PATH%\tcc.exe" -I..\..\. -o test4.exe test4.c ..\..\c2astc.c
if %ERRORLEVEL% neq 0 (
    echo Compilation of test4.c failed!
    exit /b 1
)

echo All tests compiled successfully!

REM 运行测试
echo.
echo Running tests...
echo.

REM 运行主测试程序
echo Running main test suite...
run_tests.exe test1.c test2.c test3.c test4.c
if %ERRORLEVEL% neq 0 (
    echo Main test suite failed!
    exit /b 1
)

REM 运行单独测试
echo.
echo Running individual tests...
echo.

echo Running test1...
test1.exe
if %ERRORLEVEL% neq 0 (
    echo Test1 failed!
    exit /b 1
)

echo.
echo Running test2...
test2.exe
if %ERRORLEVEL% neq 0 (
    echo Test2 failed!
    exit /b 1
)

echo.
echo Running test3...
test3.exe
if %ERRORLEVEL% neq 0 (
    echo Test3 failed!
    exit /b 1
)

echo.
echo Running test4...
test4.exe
if %ERRORLEVEL% neq 0 (
    echo Test4 failed!
    exit /b 1
)

echo.
echo All tests passed successfully!
exit /b 0 