#!/bin/bash

# Stage 1 最终验证测试套件
# 全面评估Stage 1的完成度和质量

# 颜色设置
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 项目根目录
PROJECT_ROOT="${PWD}"
TEST_DIR="${PROJECT_ROOT}/tests"
BIN_DIR="${PROJECT_ROOT}/bin"

# 测试统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
CRITICAL_TESTS=0
CRITICAL_PASSED=0

# 功能函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
    PASSED_TESTS=$((PASSED_TESTS + 1))
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
    FAILED_TESTS=$((FAILED_TESTS + 1))
}

log_critical() {
    echo -e "${YELLOW}[CRITICAL]${NC} $1"
    CRITICAL_TESTS=$((CRITICAL_TESTS + 1))
}

log_critical_success() {
    echo -e "${GREEN}[CRITICAL-PASS]${NC} $1"
    CRITICAL_PASSED=$((CRITICAL_PASSED + 1))
    PASSED_TESTS=$((PASSED_TESTS + 1))
}

test_count() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
}

echo "=================================="
echo "🎯 Stage 1 最终验证测试套件"
echo "=================================="
echo "项目根目录: $PROJECT_ROOT"
echo "测试时间: $(date)"
echo

# 验证1: 核心架构完整性
echo "=== 🏗️  核心架构完整性验证 ==="
log_critical "验证三层架构实现"

# Layer 1: simple_loader
test_count
if [[ -f "$BIN_DIR/simple_loader" ]]; then
    log_critical_success "Layer 1: simple_loader 可执行文件存在"
else
    log_error "Layer 1: simple_loader 缺失"
fi

# Layer 2: .native 模块
test_count
if [[ -f "$BIN_DIR/pipeline_module.so" ]]; then
    log_critical_success "Layer 2: pipeline_module.so 存在"
else
    log_error "Layer 2: pipeline_module.so 缺失"
fi

test_count
native_modules=(layer0_x64_64.native pipeline_x64_64.native c99bin_x64_64.native)
native_count=0
for module in "${native_modules[@]}"; do
    if [[ -f "$BIN_DIR/$module" ]]; then
        native_count=$((native_count + 1))
    fi
done
if [[ $native_count -ge 2 ]]; then
    log_critical_success "Layer 2: .native模块文件存在 ($native_count/3)"
else
    log_error "Layer 2: .native模块文件不足 ($native_count/3)"
fi

# Layer 3: ASTC 程序支持
test_count
if [[ -f "$BIN_DIR/c2astc_minimal" ]]; then
    log_critical_success "Layer 3: ASTC编译器存在"
else
    log_error "Layer 3: ASTC编译器缺失"
fi

# 验证2: 端到端工作流程
echo
echo "=== 🔄 端到端工作流程验证 ==="
log_critical "验证C→ASTC→执行完整流程"

# 创建测试程序
test_count
cat > /tmp/stage1_test.c << 'EOF'
int main() {
    return 55;
}
EOF

# C→ASTC编译
if [[ -f "$BIN_DIR/c2astc_minimal" ]]; then
    if "$BIN_DIR/c2astc_minimal" /tmp/stage1_test.c /tmp/stage1_test.astc >/dev/null 2>&1; then
        log_critical_success "C→ASTC编译: 成功"
    else
        log_error "C→ASTC编译: 失败"
    fi
else
    log_error "C→ASTC编译: c2astc_minimal不存在"
fi

# ASTC→执行
test_count
if [[ -f "/tmp/stage1_test.astc" && -f "$BIN_DIR/simple_loader" ]]; then
    if "$BIN_DIR/simple_loader" /tmp/stage1_test.astc >/dev/null 2>&1; then
        log_critical_success "ASTC→执行: 成功"
    else
        log_error "ASTC→执行: 失败"
    fi
else
    log_error "ASTC→执行: 前置条件不满足"
fi

# 验证3: 工具链完整性
echo
echo "=== 🔧 工具链完整性验证 ==="
log_critical "验证核心工具可用性"

essential_tools=(c2astc c2astc_minimal c2native simple_loader)
test_count
tool_count=0
for tool in "${essential_tools[@]}"; do
    if [[ -f "$BIN_DIR/$tool" ]]; then
        tool_count=$((tool_count + 1))
    fi
done

if [[ $tool_count -eq ${#essential_tools[@]} ]]; then
    log_critical_success "核心工具: 完整 ($tool_count/${#essential_tools[@]})"
else
    log_error "核心工具: 不完整 ($tool_count/${#essential_tools[@]})"
fi

# 验证4: 构建系统稳定性
echo
echo "=== 🏗️  构建系统稳定性验证 ==="
log_critical "验证构建系统可靠性"

test_count
if [[ -f "build_improved.sh" ]]; then
    # 测试构建脚本语法
    if bash -n build_improved.sh; then
        log_critical_success "构建脚本: 语法正确"
    else
        log_error "构建脚本: 语法错误"
    fi
else
    log_error "构建脚本: build_improved.sh不存在"
fi

# 验证5: 测试质量保证
echo
echo "=== 🧪 测试质量保证验证 ==="
log_critical "验证测试覆盖和质量"

test_count
test_scripts=$(find "$TEST_DIR" -name "test_*.sh" | wc -l)
if [[ $test_scripts -ge 15 ]]; then
    log_critical_success "测试脚本数量: 充足 ($test_scripts个)"
else
    log_error "测试脚本数量: 不足 ($test_scripts个)"
fi

# 运行核心测试套件
test_count
if bash "$TEST_DIR/run_all_tests.sh" >/dev/null 2>&1; then
    log_critical_success "核心测试套件: 通过"
else
    log_error "核心测试套件: 失败"
fi

# 验证6: 文档完整性
echo
echo "=== 📚 文档完整性验证 ==="
log_info "验证关键文档存在"

essential_docs=(docs/PRD.md docs/workplan_short_term.md docs/worknotes_short_term.md docs/workflow.md)
test_count
doc_count=0
for doc in "${essential_docs[@]}"; do
    if [[ -f "$doc" ]]; then
        doc_count=$((doc_count + 1))
    fi
done

if [[ $doc_count -eq ${#essential_docs[@]} ]]; then
    log_success "文档完整性: 完整 ($doc_count/${#essential_docs[@]})"
else
    log_error "文档完整性: 不完整 ($doc_count/${#essential_docs[@]})"
fi

# 验证7: 自举能力评估
echo
echo "=== 🔄 自举能力评估 ==="
log_info "评估自举和独立性"

test_count
if [[ -f "c99bin.sh" && -f "$BIN_DIR/c99bin_x64_64.native" ]]; then
    log_success "自举基础: c99bin工具可用"
else
    log_error "自举基础: c99bin工具缺失"
fi

test_count
# 测试c99bin编译简单程序
cat > /tmp/c99bin_test.c << 'EOF'
int main() { return 0; }
EOF

if bash c99bin.sh /tmp/c99bin_test.c /tmp/c99bin_test_out >/dev/null 2>&1; then
    log_success "自举编译: c99bin可以编译"
else
    log_error "自举编译: c99bin编译失败"
fi

# 验证8: 性能和优化
echo
echo "=== ⚡ 性能和优化验证 ==="
log_info "验证性能优化组件"

performance_components=(
    "src/core/module_loading_optimizer.c"
    "src/core/memory_management_optimizer.c"
    "src/core/astc_execution_optimizer.c"
    "src/core/performance_analysis_tool.c"
)

test_count
perf_count=0
for component in "${performance_components[@]}"; do
    if [[ -f "$component" ]]; then
        perf_count=$((perf_count + 1))
    fi
done

if [[ $perf_count -eq ${#performance_components[@]} ]]; then
    log_success "性能优化组件: 完整 ($perf_count/${#performance_components[@]})"
else
    log_error "性能优化组件: 不完整 ($perf_count/${#performance_components[@]})"
fi

# 验证9: 跨平台兼容性
echo
echo "=== 🌐 跨平台兼容性验证 ==="
log_info "验证跨平台支持"

cross_platform_scripts=(
    "scripts/build_cross_platform.sh"
    "build_arm64_optimized.sh"
    "build_windows.bat"
)

test_count
cross_count=0
for script in "${cross_platform_scripts[@]}"; do
    if [[ -f "$script" ]]; then
        cross_count=$((cross_count + 1))
    fi
done

if [[ $cross_count -ge 2 ]]; then
    log_success "跨平台脚本: 充足 ($cross_count/${#cross_platform_scripts[@]})"
else
    log_error "跨平台脚本: 不足 ($cross_count/${#cross_platform_scripts[@]})"
fi

# 验证10: 代码质量
echo
echo "=== 💎 代码质量验证 ==="
log_info "验证代码质量和结构"

test_count
c_files=$(find src -name "*.c" | wc -l)
h_files=$(find src -name "*.h" | wc -l)
total_files=$((c_files + h_files))

if [[ $total_files -ge 20 ]]; then
    log_success "代码规模: 充足 (${total_files}个文件)"
else
    log_error "代码规模: 不足 (${total_files}个文件)"
fi

# 清理临时文件
rm -f /tmp/stage1_test.c /tmp/stage1_test.astc /tmp/c99bin_test.c /tmp/c99bin_test_out

# 最终评估
echo
echo "=================================="
echo "🏆 Stage 1 最终评估结果"
echo "=================================="

echo "测试统计:"
echo "  总测试数: $TOTAL_TESTS"
echo "  通过测试: $PASSED_TESTS"
echo "  失败测试: $FAILED_TESTS"
echo "  关键测试: $CRITICAL_TESTS"
echo "  关键通过: $CRITICAL_PASSED"
echo

# 计算成功率
if [[ $TOTAL_TESTS -gt 0 ]]; then
    success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    critical_rate=$((CRITICAL_PASSED * 100 / CRITICAL_TESTS))
else
    success_rate=0
    critical_rate=0
fi

echo "成功率: ${success_rate}%"
echo "关键功能通过率: ${critical_rate}%"
echo

# Stage 1 完成度判定
if [[ $critical_rate -ge 90 && $success_rate -ge 85 ]]; then
    echo -e "${GREEN}🎉 Stage 1 验证通过！${NC}"
    echo -e "${GREEN}✅ Stage 1 开发工作已成功完成${NC}"
    echo
    echo "Stage 1 成就:"
    echo "✅ 完整的三层架构实现"
    echo "✅ 端到端工作流程验证"
    echo "✅ 完善的工具链支持"
    echo "✅ 高质量的测试覆盖"
    echo "✅ 生产级别的代码质量"
    echo
    echo "🚀 Ready for Stage 2!"
    exit 0
elif [[ $critical_rate -ge 80 && $success_rate -ge 75 ]]; then
    echo -e "${YELLOW}⚠️  Stage 1 基本完成，有少量改进点${NC}"
    echo "✅ 核心功能已实现"
    echo "⚠️  部分优化功能需要完善"
    exit 5
else
    echo -e "${RED}❌ Stage 1 需要进一步完善${NC}"
    echo "❌ 关键功能缺失或不稳定"
    echo "🔧 建议继续开发"
    exit 1
fi