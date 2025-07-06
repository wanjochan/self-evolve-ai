#!/bin/bash

# 使用系统默认编译器
CC=${CC:-gcc}

# 如果使用TinyCC，添加包含路径
if [[ "$CC" == *"tcc"* ]]; then
    TCC_BASE=$(dirname $(dirname "$CC"))
    INCLUDE_PATH="-I$TCC_BASE/include/macos-arm64"
    echo "使用TinyCC，添加包含路径: $INCLUDE_PATH"
else
    INCLUDE_PATH=""
fi

# 编译测试文件
$CC $INCLUDE_PATH -o test_module_load test_module_load.c ../../src/core/module.c -ldl

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "编译成功，可以运行 ./test_module_load"
else
    echo "编译失败"
    exit 1
fi
