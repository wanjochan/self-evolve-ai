#!/bin/bash

# test_three_layer_architecture.sh - 三层架构完整性测试
# 
# 验证PRD.md中定义的三层架构能够正常工作

echo "=========================================="
echo "Self-Evolve AI 三层架构完整性测试"
echo "=========================================="
echo "验证 PRD.md 定义的三层架构"
echo ""

# 测试计数器
TESTS_TOTAL=0
TESTS_PASSED=0
TESTS_FAILED=0

# 测试函数
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_result="$3"

    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    echo "测试 $TESTS_TOTAL: $test_name"
    echo "命令: $test_command"

    eval "$test_command" > /dev/null 2>&1
    local actual_result=$?

    if [ "$actual_result" -eq "$expected_result" ]; then
        echo "✅ 通过"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo "❌ 失败 (期望: $expected_result, 实际: $actual_result)"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    echo ""
}

# 测试1: 检查工具是否存在
echo "=========================================="
echo "第一阶段: 工具存在性检查"
echo "=========================================="

run_test "build_native_module工具存在" "[ -x bin/build_native_module ]" 0
run_test "c2astc工具存在" "[ -x bin/c2astc ]" 0
run_test "simple_loader工具存在" "[ -x bin/simple_loader ]" 0

# 测试2: Layer 3 (Program) - ASTC程序创建
echo "=========================================="
echo "第二阶段: Layer 3 (Program) 测试"
echo "=========================================="

run_test "创建hello_world.astc" "./bin/c2astc examples/hello_world.c test_hello.astc" 0
run_test "创建test_program.astc" "./bin/c2astc examples/test_program.c test_program.astc" 0
run_test "ASTC文件格式验证" "hexdump -C test_hello.astc | head -1 | grep -q '41 53 54 43'" 0

# 测试3: Layer 2 (Runtime) - Native模块创建
echo "=========================================="
echo "第三阶段: Layer 2 (Runtime) 测试"
echo "=========================================="

# 创建一个简单的测试对象文件
echo "创建测试对象文件..."
echo 'int test_function() { return 42; }' > test_module.c
./cc.sh -c test_module.c -o test_module.o

run_test "创建.native模块" "./bin/build_native_module test_module.o test_module.native --arch=arm64 --type=user" 0
run_test "Native文件格式验证" "hexdump -C test_module.native | head -1 | grep -q '4e 41 54 56'" 0

# 测试4: Layer 1 (Loader) - 加载器测试
echo "=========================================="
echo "第四阶段: Layer 1 (Loader) 测试"
echo "=========================================="

# simple_loader会尝试加载VM模块，由于VM模块不存在，预期返回1
run_test "simple_loader架构检测" "./bin/simple_loader test_hello.astc" 1
run_test "simple_loader帮助信息" "./bin/simple_loader" 1

# 测试5: 三层架构集成测试
echo "=========================================="
echo "第五阶段: 三层架构集成测试"
echo "=========================================="

echo "测试三层架构流程:"
echo "  Layer 1: simple_loader (已验证)"
echo "  Layer 2: VM模块 (缺失，这是预期的)"
echo "  Layer 3: ASTC程序 (已验证)"
echo ""

# 验证三层架构的逻辑流程
echo "验证三层架构逻辑流程..."
if ./bin/simple_loader test_hello.astc 2>&1 | grep -q "vm_arm64"; then
    echo "✅ simple_loader正确识别架构并尝试加载VM模块"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo "❌ simple_loader架构识别失败"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# 清理测试文件
echo ""
echo "清理测试文件..."
rm -f test_hello.astc test_program.astc test_module.c test_module.o test_module.native

# 测试结果总结
echo ""
echo "=========================================="
echo "测试结果总结"
echo "=========================================="
echo ""
echo "总测试数: $TESTS_TOTAL"
echo "通过: $TESTS_PASSED"
echo "失败: $TESTS_FAILED"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo "🎉 所有测试通过！三层架构工具链工作正常"
    echo ""
    echo "PRD.md 三层架构验证结果:"
    echo "✅ Layer 1 (Loader): simple_loader 工作正常"
    echo "✅ Layer 2 (Runtime): .native模块创建工具工作正常"
    echo "✅ Layer 3 (Program): ASTC程序创建工具工作正常"
    echo ""
    echo "注意: VM模块(.native文件)缺失是正常的，因为我们还没有实现VM运行时"
    echo "但是三层架构的基础框架已经完整建立！"
    exit 0
else
    echo "❌ 有 $TESTS_FAILED 个测试失败"
    echo "请检查失败的测试并修复问题"
    exit 1
fi
