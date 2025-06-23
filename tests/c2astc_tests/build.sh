#!/bin/bash
# 使用TCC编译C2ASTC测试程序

# 设置TCC路径
TCC_PATH="../../tcc/src"

# 检查TCC是否存在
if [ ! -f "$TCC_PATH/tcc" ]; then
    echo "错误: 找不到TCC编译器 - $TCC_PATH/tcc"
    exit 1
fi

# 编译c2astc库和测试程序
echo "编译C2ASTC库和测试程序..."
$TCC_PATH/tcc -I../.. -o run_tests run_tests.c ../../c2astc.c

# 检查编译结果
if [ $? -ne 0 ]; then
    echo "编译失败!"
    exit 1
fi

# 添加执行权限
chmod +x run_tests

echo "编译成功!"
echo "运行测试程序..."
./run_tests

echo "测试完成!"
exit $? 