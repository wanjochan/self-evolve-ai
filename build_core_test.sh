#!/bin/bash
#
# build_core_test.sh - 构建和运行core模块单元测试
#

# 确保脚本在错误时退出
set -e

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 定义路径
SRC_DIR="$SCRIPT_DIR/src/core"
MODULES_DIR="$SRC_DIR/modules"
TESTS_DIR="$SCRIPT_DIR/tests"
BIN_DIR="$SCRIPT_DIR/bin"

echo "=== 构建Core模块单元测试系统 ==="

# 创建输出目录
mkdir -p "$BIN_DIR"

# 检测架构
if [[ "$(uname -m)" == "arm64" ]]; then
    ARCH="arm64"
    BITS="64"
elif [[ "$(uname -m)" == "x86_64" ]]; then
    ARCH="x64"
    BITS="64"
elif [[ "$(uname -m)" == "i386" ]] || [[ "$(uname -m)" == "i686" ]]; then
    ARCH="x86"
    BITS="32"
else
    echo "警告: 未知架构 $(uname -m)，使用默认 x64_64"
    ARCH="x64"
    BITS="64"
fi

echo "检测到架构: ${ARCH}_${BITS}"

# 编译测试框架
echo ""
echo "1. 编译测试框架..."

echo "编译 core_test_framework.c..."
"$SCRIPT_DIR/cc.sh" -c "$TESTS_DIR/core_test_framework.c" -I "$TESTS_DIR" -o "$TESTS_DIR/core_test_framework.o"

# 编译核心模块（如果需要）
echo ""
echo "2. 编译核心模块（用于测试）..."

# 检查核心模块是否已编译
if [[ ! -f "$MODULES_DIR/module_module.o" ]]; then
    echo "编译 module_module.c..."
    "$SCRIPT_DIR/cc.sh" -c "$MODULES_DIR/module_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/module_module.o"
fi

if [[ ! -f "$MODULES_DIR/layer0_module.o" ]]; then
    echo "编译 layer0_module.c..."
    "$SCRIPT_DIR/cc.sh" -c "$MODULES_DIR/layer0_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/layer0_module.o"
fi

# 编译测试用例
echo ""
echo "3. 编译测试用例..."

echo "编译 test_astc_module.c..."
"$SCRIPT_DIR/cc.sh" -c "$TESTS_DIR/test_astc_module.c" -I "$TESTS_DIR" -I "$SRC_DIR" -o "$TESTS_DIR/test_astc_module.o"

echo "编译 test_module_system.c..."
"$SCRIPT_DIR/cc.sh" -c "$TESTS_DIR/test_module_system.c" -I "$TESTS_DIR" -I "$SRC_DIR" -o "$TESTS_DIR/test_module_system.o"

echo "编译 test_specific_modules.c..."
"$SCRIPT_DIR/cc.sh" -c "$TESTS_DIR/test_specific_modules.c" -I "$TESTS_DIR" -I "$SRC_DIR" -o "$TESTS_DIR/test_specific_modules.o"

echo "编译 core_test_main.c..."
"$SCRIPT_DIR/cc.sh" -c "$TESTS_DIR/core_test_main.c" -I "$TESTS_DIR" -I "$SRC_DIR" -o "$TESTS_DIR/core_test_main.o"

# 链接测试可执行文件
echo ""
echo "4. 链接测试可执行文件..."

echo "链接 core_test..."
"$SCRIPT_DIR/cc.sh" \
    "$TESTS_DIR/core_test_main.o" \
    "$TESTS_DIR/core_test_framework.o" \
    "$TESTS_DIR/test_astc_module.o" \
    "$TESTS_DIR/test_module_system.o" \
    "$TESTS_DIR/test_specific_modules.o" \
    "$MODULES_DIR/module_module.o" \
    "$MODULES_DIR/pipeline_module_final.o" \
    -I "$TESTS_DIR" -I "$SRC_DIR" \
    -o "$BIN_DIR/core_test"

echo "测试可执行文件已生成: $BIN_DIR/core_test"

# 运行测试
echo ""
echo "5. 运行测试..."
echo "========================================"

# 检查是否传入了特定的测试参数
if [[ $# -gt 0 ]]; then
    echo "运行测试: $BIN_DIR/core_test $@"
    "$BIN_DIR/core_test" "$@"
else
    echo "运行所有测试: $BIN_DIR/core_test"
    "$BIN_DIR/core_test"
fi

# 获取测试结果
TEST_RESULT=$?

echo "========================================"

if [[ $TEST_RESULT -eq 0 ]]; then
    echo "✅ 所有测试通过!"
else
    echo "❌ 有测试失败!"
fi

# 清理编译产物（可选）
if [[ "$1" == "--clean" ]] || [[ "$2" == "--clean" ]]; then
    echo ""
    echo "6. 清理编译产物..."
    rm -f "$TESTS_DIR"/*.o
    echo "编译产物已清理"
fi

echo ""
echo "=== Core模块测试完成 ==="
echo ""
echo "可用的测试选项:"
echo "  $BIN_DIR/core_test                    # 运行所有测试"
echo "  $BIN_DIR/core_test -v                 # 详细输出模式"
echo "  $BIN_DIR/core_test --astc             # 只运行ASTC模块测试"
echo "  $BIN_DIR/core_test --module           # 只运行模块系统测试"
echo "  $BIN_DIR/core_test --specific         # 只运行具体模块测试"
echo "  $BIN_DIR/core_test --help             # 显示帮助信息"
echo ""
echo "重新构建选项:"
echo "  $0                     # 构建并运行所有测试"
echo "  $0 -v                  # 构建并以详细模式运行测试"
echo "  $0 --clean             # 构建、运行测试并清理编译产物"

# 返回测试结果
exit $TEST_RESULT 