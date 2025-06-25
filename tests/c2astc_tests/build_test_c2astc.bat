@echo off
REM 构建和测试c2astc模块

REM 设置TCC路径
set TCC_PATH=..\..\tcc-win\tcc

REM 检查TCC是否存在
if not exist "%TCC_PATH%\tcc.exe" (
    echo 错误: 找不到TCC编译器 - %TCC_PATH%\tcc.exe
    exit /b 1
)

REM 编译test_c2astc.c
echo 编译test_c2astc.c...
%TCC_PATH%\tcc.exe -I..\..\ -o test_c2astc.exe test_c2astc.c ..\..\c2astc.c

REM 检查编译结果
if %ERRORLEVEL% neq 0 (
    echo 编译失败!
    exit /b 1
)

REM 编译serialize_test.c
echo 编译serialize_test.c...
%TCC_PATH%\tcc.exe -I..\..\ -o serialize_test.exe serialize_test.c ..\..\c2astc.c

REM 检查编译结果
if %ERRORLEVEL% neq 0 (
    echo 编译失败!
    exit /b 1
)

REM 测试简单用例
echo 测试简单用例...
test_c2astc.exe simple_test.c

REM 测试复杂控制流用例
echo 测试复杂控制流用例...
test_c2astc.exe complex_test.c

REM 测试复杂类型用例
echo 测试复杂类型用例...
test_c2astc.exe complex_types_test.c

REM 测试指针类型用例
echo 测试指针类型用例...
test_c2astc.exe pointer_types_test.c

REM 测试数组类型用例
echo 测试数组类型用例...
test_c2astc.exe array_types_test.c

REM 测试函数指针类型用例
echo 测试函数指针类型用例...
test_c2astc.exe function_pointer_test.c

REM 测试数组访问和成员访问用例
echo 测试数组访问和成员访问用例...
test_c2astc.exe array_member_access_test.c

REM 测试序列化和反序列化
echo 测试序列化和反序列化...
serialize_test.exe 