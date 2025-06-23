@echo off
REM 使用TCC编译C2ASTC测试程序

REM 设置TCC路径
set TCC_PATH=..\..\tcc-win\tcc

REM 检查TCC是否存在
if not exist "%TCC_PATH%\tcc.exe" (
    echo 错误: 找不到TCC编译器 - %TCC_PATH%\tcc.exe
    exit /b 1
)

REM 编译c2astc库和测试程序
echo 编译C2ASTC库和测试程序...
%TCC_PATH%\tcc.exe -I..\..\. -o run_tests.exe run_tests.c ..\..\c2astc.c

REM 检查编译结果
if %ERRORLEVEL% neq 0 (
    echo 编译失败!
    exit /b 1
)

echo 编译成功!
echo 运行测试程序...
run_tests.exe

echo 测试完成!
exit /b %ERRORLEVEL% 