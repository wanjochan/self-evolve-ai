#!/bin/bash
#
# 模块系统测试脚本
#

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 编译器设置
CC="$PROJECT_ROOT/cc.sh"

# 源文件路径
MODULE_C="$PROJECT_ROOT/src/core/module.c"

# 测试程序
TEST_C="$SCRIPT_DIR/core_module_tests.c"

# 输出文件
OUTPUT="$SCRIPT_DIR/module_test"

# 检查文件是否存在
echo "检查源文件..."
if [ ! -f "$MODULE_C" ]; then
    echo "错误: 模块源文件不存在: $MODULE_C"
    exit 1
fi

# 编译测试程序
echo "编译模块测试程序..."
$CC -I"$PROJECT_ROOT/src/core" -o "$OUTPUT" "$TEST_C" "$MODULE_C"

# 检查编译是否成功
if [ $? -ne 0 ]; then
    echo "编译失败!"
    exit 1
fi

echo "编译成功!"

# 运行测试程序
echo "运行模块测试..."
"$OUTPUT"

# 检查测试是否成功
if [ $? -ne 0 ]; then
    echo "测试失败!"
    exit 1
fi

echo "模块系统测试完成!"
echo "测试通过!"
exit 0 