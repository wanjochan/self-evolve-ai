@echo off
REM 构建和测试c2astc模块

REM 编译test_c2astc.c
echo 编译test_c2astc.c...
tcc -I../../ -o test_c2astc.exe test_c2astc.c ../../c2astc.c

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

echo 测试完成！ 