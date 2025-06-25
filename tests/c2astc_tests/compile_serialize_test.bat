@echo off
echo 编译serialize_test.c...

REM 检查是否安装了Visual Studio
where cl.exe >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo 未找到Visual Studio编译器(cl.exe)！
    echo 请确保已安装Visual Studio并设置了环境变量。
    exit /b 1
)

REM 使用Visual Studio的cl.exe编译
cl.exe /Fe:serialize_test.exe tests\c2astc_tests\serialize_test.c c2astc.c /I. /W4

if %ERRORLEVEL% NEQ 0 (
    echo 编译失败！
    exit /b %ERRORLEVEL%
)

echo 编译成功！
echo 运行serialize_test.exe...
serialize_test.exe

echo 完成！ 