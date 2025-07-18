#!/bin/bash
#
# test_cross_platform_build.sh - 跨平台构建验证测试
#
# 验证跨平台构建系统的正确性和兼容性
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 测试统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }

print_test_result() {
    local test_name="$1"
    local result="$2"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✅ PASS${NC}: $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ FAIL${NC}: $test_name"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# 测试1: 平台检测功能
test_platform_detection() {
    log_info "测试平台检测功能..."
    
    local platform_script="$PROJECT_ROOT/scripts/platform_detect.sh"
    local config_file="/tmp/test_platform_config.sh"
    
    if [ -x "$platform_script" ]; then
        if "$platform_script" "$config_file" >/dev/null 2>&1; then
            if [ -f "$config_file" ]; then
                # 验证配置文件内容
                if grep -q "PLATFORM_OS=" "$config_file" && \
                   grep -q "PLATFORM_ARCH=" "$config_file" && \
                   grep -q "COMPILER_CC=" "$config_file"; then
                    print_test_result "平台检测功能" "PASS"
                else
                    print_test_result "平台检测功能" "FAIL"
                fi
                rm -f "$config_file"
            else
                print_test_result "平台检测功能" "FAIL"
            fi
        else
            print_test_result "平台检测功能" "FAIL"
        fi
    else
        print_test_result "平台检测功能" "FAIL"
    fi
}

# 测试2: 跨平台构建脚本
test_cross_platform_build_script() {
    log_info "测试跨平台构建脚本..."
    
    local build_script="$PROJECT_ROOT/scripts/build_cross_platform.sh"
    
    if [ -x "$build_script" ]; then
        # 测试帮助信息
        if "$build_script" --help >/dev/null 2>&1; then
            print_test_result "构建脚本帮助信息" "PASS"
        else
            print_test_result "构建脚本帮助信息" "FAIL"
        fi
        
        # 测试清理功能
        if "$build_script" --clean >/dev/null 2>&1; then
            print_test_result "构建脚本清理功能" "PASS"
        else
            print_test_result "构建脚本清理功能" "FAIL"
        fi
    else
        print_test_result "跨平台构建脚本" "FAIL"
    fi
}

# 测试3: 模块构建验证
test_module_build_verification() {
    log_info "测试模块构建验证..."
    
    local modules_dir="$PROJECT_ROOT/bin/layer2"
    local expected_modules=("layer0" "pipeline" "compiler" "module" "libc")
    
    # 加载平台配置
    if [ -f "$PROJECT_ROOT/scripts/platform_config.sh" ]; then
        source "$PROJECT_ROOT/scripts/platform_config.sh"
    else
        log_warn "平台配置文件不存在，使用默认值"
        SHARED_LIB_EXT=".so"
    fi
    
    local all_modules_exist=true
    
    for module in "${expected_modules[@]}"; do
        local module_file="$modules_dir/${module}${SHARED_LIB_EXT}"
        local native_file="$modules_dir/${module}.native"
        
        if [ -f "$module_file" ] && [ -f "$native_file" ]; then
            log_success "模块 $module 存在"
        else
            log_error "模块 $module 缺失"
            all_modules_exist=false
        fi
    done
    
    if $all_modules_exist; then
        print_test_result "模块构建验证" "PASS"
    else
        print_test_result "模块构建验证" "FAIL"
    fi
}

# 测试4: 模块加载测试
test_module_loading() {
    log_info "测试模块加载..."
    
    # 创建模块加载测试程序
    local test_program="$PROJECT_ROOT/build/temp/test_module_loading.c"
    mkdir -p "$(dirname "$test_program")"
    
    cat > "$test_program" << 'EOF'
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>

int main() {
    const char* modules[] = {"layer0", "pipeline", "compiler", "module", "libc"};
    int module_count = sizeof(modules) / sizeof(modules[0]);
    int success_count = 0;
    
    for (int i = 0; i < module_count; i++) {
        char module_path[256];
        snprintf(module_path, sizeof(module_path), "./bin/layer2/%s.native", modules[i]);
        
        void* handle = dlopen(module_path, RTLD_LAZY);
        if (handle) {
            success_count++;
            dlclose(handle);
        } else {
            printf("Failed to load %s: %s\n", modules[i], dlerror());
        }
    }
    
    printf("Module loading test: %d/%d successful\n", success_count, module_count);
    return (success_count == module_count) ? 0 : 1;
}
EOF
    
    # 编译测试程序
    local test_executable="$PROJECT_ROOT/build/temp/test_module_loading"
    local compiler="${COMPILER_CC:-gcc}"
    
    if $compiler "$test_program" -o "$test_executable" -ldl 2>/dev/null; then
        cd "$PROJECT_ROOT"
        if "$test_executable" >/dev/null 2>&1; then
            print_test_result "模块加载测试" "PASS"
        else
            print_test_result "模块加载测试" "FAIL"
        fi
    else
        print_test_result "模块加载测试编译" "FAIL"
    fi
    
    # 清理
    rm -f "$test_program" "$test_executable"
}

# 测试5: 架构兼容性测试
test_architecture_compatibility() {
    log_info "测试架构兼容性..."
    
    # 检查当前架构
    local current_arch=$(uname -m)
    local supported_archs=("x86_64" "arm64" "aarch64" "i386" "i686")
    local arch_supported=false
    
    for arch in "${supported_archs[@]}"; do
        if [ "$current_arch" = "$arch" ]; then
            arch_supported=true
            break
        fi
    done
    
    if $arch_supported; then
        print_test_result "架构兼容性测试" "PASS"
    else
        print_test_result "架构兼容性测试" "FAIL"
    fi
}

# 测试6: 编译器兼容性测试
test_compiler_compatibility() {
    log_info "测试编译器兼容性..."
    
    local compilers=("gcc" "clang")
    local compatible_compilers=0
    
    for compiler in "${compilers[@]}"; do
        if command -v "$compiler" >/dev/null 2>&1; then
            # 测试基本编译功能
            local test_source="/tmp/test_compiler_$compiler.c"
            local test_binary="/tmp/test_compiler_$compiler"
            
            echo 'int main() { return 42; }' > "$test_source"
            
            if $compiler "$test_source" -o "$test_binary" 2>/dev/null; then
                if [ -x "$test_binary" ]; then
                    compatible_compilers=$((compatible_compilers + 1))
                    log_success "编译器 $compiler 兼容"
                fi
            fi
            
            rm -f "$test_source" "$test_binary"
        fi
    done
    
    if [ $compatible_compilers -gt 0 ]; then
        print_test_result "编译器兼容性测试" "PASS"
    else
        print_test_result "编译器兼容性测试" "FAIL"
    fi
}

# 测试7: 构建性能测试
test_build_performance() {
    log_info "测试构建性能..."
    
    local build_script="$PROJECT_ROOT/scripts/build_cross_platform.sh"
    
    if [ -x "$build_script" ]; then
        local start_time=$(date +%s)
        
        # 运行模块构建
        if "$build_script" --modules-only --no-tests >/dev/null 2>&1; then
            local end_time=$(date +%s)
            local build_duration=$((end_time - start_time))
            
            log_success "构建时间: ${build_duration}秒"
            
            # 如果构建时间合理（小于60秒），认为性能测试通过
            if [ $build_duration -lt 60 ]; then
                print_test_result "构建性能测试" "PASS"
            else
                print_test_result "构建性能测试" "FAIL"
            fi
        else
            print_test_result "构建性能测试" "FAIL"
        fi
    else
        print_test_result "构建性能测试" "FAIL"
    fi
}

# 测试8: 文件权限和路径测试
test_file_permissions_and_paths() {
    log_info "测试文件权限和路径..."
    
    local issues=0
    
    # 检查脚本可执行权限
    local scripts=("platform_detect.sh" "build_cross_platform.sh")
    for script in "${scripts[@]}"; do
        local script_path="$PROJECT_ROOT/scripts/$script"
        if [ -f "$script_path" ]; then
            if [ -x "$script_path" ]; then
                log_success "脚本 $script 有执行权限"
            else
                log_error "脚本 $script 缺少执行权限"
                issues=$((issues + 1))
            fi
        else
            log_error "脚本 $script 不存在"
            issues=$((issues + 1))
        fi
    done
    
    # 检查目录结构
    local required_dirs=("scripts" "src/core/modules" "bin/layer2" "tests")
    for dir in "${required_dirs[@]}"; do
        local dir_path="$PROJECT_ROOT/$dir"
        if [ -d "$dir_path" ]; then
            log_success "目录 $dir 存在"
        else
            log_error "目录 $dir 不存在"
            issues=$((issues + 1))
        fi
    done
    
    if [ $issues -eq 0 ]; then
        print_test_result "文件权限和路径测试" "PASS"
    else
        print_test_result "文件权限和路径测试" "FAIL"
    fi
}

# 运行所有测试
run_all_tests() {
    log_info "开始跨平台构建验证测试..."
    echo
    
    test_platform_detection
    test_cross_platform_build_script
    test_module_build_verification
    test_module_loading
    test_architecture_compatibility
    test_compiler_compatibility
    test_build_performance
    test_file_permissions_and_paths
    
    echo
    log_info "测试结果统计:"
    echo "  总测试数: $TOTAL_TESTS"
    echo -e "  通过: ${GREEN}$PASSED_TESTS${NC}"
    echo -e "  失败: ${RED}$FAILED_TESTS${NC}"
    
    local success_rate=0
    if [ $TOTAL_TESTS -gt 0 ]; then
        success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi
    
    echo "  成功率: $success_rate%"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        log_success "🎉 所有跨平台构建测试通过！"
        return 0
    else
        log_error "⚠️  有 $FAILED_TESTS 个测试失败"
        return 1
    fi
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  --help           显示此帮助信息"
    echo
    echo "此脚本验证跨平台构建系统的正确性，包括："
    echo "  • 平台检测功能"
    echo "  • 构建脚本功能"
    echo "  • 模块构建和加载"
    echo "  • 架构和编译器兼容性"
    echo "  • 构建性能"
    echo "  • 文件权限和路径"
}

# 主函数
main() {
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help)
                show_help
                exit 0
                ;;
            *)
                log_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 运行测试
    run_all_tests
}

# 运行主函数
main "$@"
