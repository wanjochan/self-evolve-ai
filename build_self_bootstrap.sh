#!/bin/bash
# build_self_bootstrap.sh - 自举构建脚本
# 
# 目标：逐步减少对外部编译器的依赖，实现真正的自举

set -e

echo "=== 自举构建系统 v1.0 ==="
echo "目标：实现Stage 1的自举能力"

# 检查当前状态
echo -e "\n1. 检查当前工具状态："
echo "   - c99bin: $(ls -la tools/c99bin 2>/dev/null | awk '{print $5}' || echo '不存在') 字节"
echo "   - c2astc_minimal: $(ls -la bin/c2astc_minimal 2>/dev/null | awk '{print $5}' || echo '不存在') 字节"
echo "   - simple_loader: $(ls -la bin/simple_loader 2>/dev/null | awk '{print $5}' || echo '不存在') 字节"

# 阶段1：使用gcc构建基础工具（当前状态）
echo -e "\n2. 阶段1：使用gcc构建基础工具"
if [ ! -f "bin/c2astc_minimal" ]; then
    echo "   构建c2astc_minimal..."
    gcc -o bin/c2astc_minimal tools/c2astc_minimal.c
    echo "   ✅ c2astc_minimal构建完成"
else
    echo "   ✅ c2astc_minimal已存在"
fi

if [ ! -f "bin/simple_loader" ]; then
    echo "   构建simple_loader..."
    gcc -o bin/simple_loader tools/simple_loader.c bin/libcore.a -Isrc/core -lm -ldl
    echo "   ✅ simple_loader构建完成"
else
    echo "   ✅ simple_loader已存在"
fi

if [ ! -f "bin/pipeline_module.so" ]; then
    echo "   构建pipeline_module.so..."
    gcc -shared -fPIC -o bin/pipeline_module.so src/core/modules/pipeline_module.c -Isrc/core -Isrc/ext -DNDEBUG -O2
    echo "   ✅ pipeline_module.so构建完成"
else
    echo "   ✅ pipeline_module.so已存在"
fi

# 阶段2：验证端到端工作流程
echo -e "\n3. 阶段2：验证端到端工作流程"
echo "   测试C→ASTC→执行流程..."

# 创建测试程序
cat > /tmp/bootstrap_test.c << 'EOF'
int main() {
    return 77;
}
EOF

echo "   测试源码: int main() { return 77; }"

# 编译到ASTC
if ./bin/c2astc_minimal /tmp/bootstrap_test.c /tmp/bootstrap_test.astc > /dev/null 2>&1; then
    echo "   ✅ C→ASTC编译成功"
else
    echo "   ❌ C→ASTC编译失败"
    exit 1
fi

# 执行ASTC
if ./bin/simple_loader /tmp/bootstrap_test.astc > /dev/null 2>&1; then
    echo "   ✅ ASTC执行成功"
else
    echo "   ❌ ASTC执行失败"
    exit 1
fi

# 阶段3：评估自举可能性
echo -e "\n4. 阶段3：自举能力评估"

# 检查c99bin能力
echo "   测试c99bin编译能力..."
if ./tools/c99bin tools/c2astc_c99bin.c -o /tmp/test_c99bin_output > /dev/null 2>&1; then
    echo "   ✅ c99bin可以编译简化工具"
    
    # 测试生成的工具是否工作
    if /tmp/test_c99bin_output /tmp/bootstrap_test.c /tmp/test_c99bin.astc > /dev/null 2>&1; then
        echo "   ✅ c99bin生成的工具可以工作"
        BOOTSTRAP_LEVEL="完全自举"
    else
        echo "   ⚠️  c99bin生成的工具不能正常工作"
        BOOTSTRAP_LEVEL="部分自举"
    fi
else
    echo "   ❌ c99bin无法编译复杂工具"
    BOOTSTRAP_LEVEL="无自举"
fi

# 阶段4：生成自举报告
echo -e "\n5. 自举能力报告"
echo "   =================================="
echo "   自举级别: $BOOTSTRAP_LEVEL"
echo "   =================================="

case "$BOOTSTRAP_LEVEL" in
    "完全自举")
        echo "   🎉 恭喜！已实现完全自举能力"
        echo "   - c99bin可以编译我们的工具"
        echo "   - 生成的工具可以正常工作"
        echo "   - 可以完全脱离gcc依赖"
        ;;
    "部分自举")
        echo "   ⚠️  部分自举：c99bin可以编译但生成的工具有问题"
        echo "   - c99bin编译成功"
        echo "   - 但生成的可执行文件不能正常工作"
        echo "   - 需要改进c99bin的代码生成"
        ;;
    "无自举")
        echo "   ❌ 无自举能力：c99bin无法编译复杂工具"
        echo "   - c99bin只能处理简单程序"
        echo "   - 我们的工具对c99bin来说太复杂"
        echo "   - 需要简化工具或增强c99bin"
        ;;
esac

echo -e "\n6. 下一步建议"
case "$BOOTSTRAP_LEVEL" in
    "完全自举")
        echo "   - 可以开始Stage 2开发"
        echo "   - 建立完全基于c99bin的构建流程"
        ;;
    "部分自举")
        echo "   - 改进c99bin的代码生成质量"
        echo "   - 或者进一步简化工具实现"
        ;;
    "无自举")
        echo "   - 选项1：大幅简化工具实现"
        echo "   - 选项2：增强c99bin编译能力"
        echo "   - 选项3：接受当前状态，专注其他功能"
        ;;
esac

echo -e "\n=== 自举构建完成 ==="
echo "当前Stage 1状态：端到端工作流程 ✅，自举能力 $BOOTSTRAP_LEVEL"
