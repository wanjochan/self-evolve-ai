#!/bin/bash
#
# build_cross_platform.sh - 跨平台构建脚本
#
# 统一的跨平台构建系统，支持Linux、macOS、Windows等多个平台
# 支持架构：x86_64、x86_32、arm64、arm32等
#

set -euo pipefail

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# 日志函数
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_step() { echo -e "${CYAN}[STEP]${NC} $1"; }

# 构建统计
BUILD_START_TIME=""
MODULES_BUILT=0
MODULES_FAILED=0
TOOLS_BUILT=0
TOOLS_FAILED=0

# 初始化构建环境
init_build_environment() {
    log_step "初始化构建环境"
    
    BUILD_START_TIME=$(date +%s)
    
    # 检测平台
    if [ ! -f "$SCRIPT_DIR/platform_config.sh" ]; then
        log_info "运行平台检测..."
        "$SCRIPT_DIR/platform_detect.sh" "$SCRIPT_DIR/platform_config.sh"
    fi
    
    # 加载平台配置
    source "$SCRIPT_DIR/platform_config.sh"
    
    log_success "平台: $PLATFORM_OS ($PLATFORM_TARGET)"
    log_success "编译器: $COMPILER_CC"

    # 显示架构特定信息
    case "$PLATFORM_ARCH" in
        "arm64")
            log_info "ARM64架构支持: Apple Silicon、ARM64 Linux"
            ;;
        "x64")
            log_info "x64架构支持: Intel/AMD 64位处理器"
            ;;
        "x86")
            log_info "x86架构支持: Intel/AMD 32位处理器"
            ;;
    esac
    
    # 创建必要目录
    mkdir -p "$PROJECT_ROOT/bin/layer2"
    mkdir -p "$PROJECT_ROOT/bin/tools"
    mkdir -p "$PROJECT_ROOT/build/obj"
    mkdir -p "$PROJECT_ROOT/build/temp"
}

# 编译单个源文件
compile_source() {
    local source_file="$1"
    local output_file="$2"
    local include_dirs="${3:-}"
    local extra_flags="${4:-}"
    
    local compile_cmd="$COMPILER_CC $CFLAGS $extra_flags"
    
    # 添加包含目录
    if [ -n "$include_dirs" ]; then
        for include_dir in $include_dirs; do
            compile_cmd="$compile_cmd -I$include_dir"
        done
    fi
    
    compile_cmd="$compile_cmd -c $source_file -o $output_file"
    
    log_info "编译: $(basename "$source_file")"
    if eval "$compile_cmd"; then
        return 0
    else
        log_error "编译失败: $source_file"
        return 1
    fi
}

# 创建共享库
create_shared_library() {
    local lib_name="$1"
    local object_files="$2"
    local output_dir="$3"
    local extra_libs="${4:-}"
    
    local output_file="$output_dir/$(get_shared_lib_name "$lib_name")"
    local link_cmd="$COMPILER_CC -shared $object_files -o $output_file $LDFLAGS $extra_libs"
    
    log_info "创建共享库: $(basename "$output_file")"
    if eval "$link_cmd"; then
        log_success "共享库创建成功: $output_file"
        return 0
    else
        log_error "共享库创建失败: $lib_name"
        return 1
    fi
}

# 构建核心模块
build_core_modules() {
    log_step "构建核心模块"
    
    local src_dir="$PROJECT_ROOT/src/core"
    local modules_dir="$src_dir/modules"
    local obj_dir="$PROJECT_ROOT/build/obj"
    local output_dir="$PROJECT_ROOT/bin/layer2"
    
    # 核心模块列表
    local modules=(
        "layer0_module"
        "pipeline_module"
        "compiler_module"
        "module_module"
        "libc_module"
    )
    
    # 编译每个模块
    for module in "${modules[@]}"; do
        local source_file="$modules_dir/${module}.c"
        local object_file="$obj_dir/${module}.o"
        
        if [ -f "$source_file" ]; then
            if compile_source "$source_file" "$object_file" "$src_dir" "-D_GNU_SOURCE"; then
                # 创建共享库
                local lib_name="${module%_module}"  # 移除_module后缀
                if create_shared_library "$lib_name" "$object_file" "$output_dir"; then
                    # 创建兼容性符号链接
                    local lib_file="$(get_shared_lib_name "$lib_name")"
                    ln -sf "$lib_file" "$output_dir/${lib_name}.native"
                    MODULES_BUILT=$((MODULES_BUILT + 1))
                else
                    MODULES_FAILED=$((MODULES_FAILED + 1))
                fi
            else
                MODULES_FAILED=$((MODULES_FAILED + 1))
            fi
        else
            log_warn "模块源文件不存在: $source_file"
            MODULES_FAILED=$((MODULES_FAILED + 1))
        fi
    done
}

# 构建工具
build_tools() {
    log_step "构建工具"
    
    local tools_dir="$PROJECT_ROOT/tools"
    local obj_dir="$PROJECT_ROOT/build/obj"
    local output_dir="$PROJECT_ROOT/bin/tools"
    local src_core_dir="$PROJECT_ROOT/src/core"
    
    # 工具列表
    local tools=(
        "c2astc"
        "c2native"
        "simple_loader"
    )
    
    # 首先编译依赖的核心文件
    local core_deps=()
    if [ -f "$src_core_dir/astc.c" ]; then
        local astc_obj="$obj_dir/astc.o"
        if compile_source "$src_core_dir/astc.c" "$astc_obj" "$src_core_dir"; then
            core_deps+=("$astc_obj")
        fi
    fi
    
    # 编译每个工具
    for tool in "${tools[@]}"; do
        local source_file="$tools_dir/${tool}.c"
        local object_file="$obj_dir/${tool}.o"
        local executable_file="$output_dir/$(get_executable_name "${tool}_${PLATFORM_TARGET}")"
        
        if [ -f "$source_file" ]; then
            if compile_source "$source_file" "$object_file" "$src_core_dir $tools_dir"; then
                # 链接可执行文件
                local link_cmd="$COMPILER_CC $object_file ${core_deps[*]} -o $executable_file $LDFLAGS"
                
                log_info "链接工具: $(basename "$executable_file")"
                if eval "$link_cmd"; then
                    log_success "工具构建成功: $executable_file"
                    TOOLS_BUILT=$((TOOLS_BUILT + 1))
                else
                    log_error "工具链接失败: $tool"
                    TOOLS_FAILED=$((TOOLS_FAILED + 1))
                fi
            else
                TOOLS_FAILED=$((TOOLS_FAILED + 1))
            fi
        else
            log_warn "工具源文件不存在: $source_file"
            TOOLS_FAILED=$((TOOLS_FAILED + 1))
        fi
    done
}

# 构建C99编译器
build_c99_compiler() {
    log_step "构建C99编译器"
    
    local c99_dir="$PROJECT_ROOT/src/c99"
    local obj_dir="$PROJECT_ROOT/build/obj"
    local output_dir="$PROJECT_ROOT/bin"
    
    # C99编译器源文件
    local c99_sources=(
        "$c99_dir/tools/c99_main.c"
        "$c99_dir/frontend/c99_lexer.c"
        "$c99_dir/frontend/c99_parser.c"
        "$c99_dir/frontend/c99_semantic.c"
        "$c99_dir/frontend/c99_error.c"
        "$c99_dir/backend/c99_codegen.c"
    )
    
    local c99_objects=()
    local all_compiled=true
    
    # 编译所有C99源文件
    for source_file in "${c99_sources[@]}"; do
        if [ -f "$source_file" ]; then
            local obj_name="$(basename "${source_file%.c}").o"
            local object_file="$obj_dir/$obj_name"
            
            if compile_source "$source_file" "$object_file" "$c99_dir $PROJECT_ROOT/src/core"; then
                c99_objects+=("$object_file")
            else
                all_compiled=false
                break
            fi
        else
            log_warn "C99源文件不存在: $source_file"
            all_compiled=false
            break
        fi
    done
    
    # 链接C99编译器
    if $all_compiled && [ ${#c99_objects[@]} -gt 0 ]; then
        local c99_executable="$output_dir/$(get_executable_name "c99_${PLATFORM_TARGET}")"
        local link_cmd="$COMPILER_CC ${c99_objects[*]} -o $c99_executable $LDFLAGS"
        
        log_info "链接C99编译器: $(basename "$c99_executable")"
        if eval "$link_cmd"; then
            log_success "C99编译器构建成功: $c99_executable"
            
            # 创建便捷符号链接
            ln -sf "$(basename "$c99_executable")" "$output_dir/c99"
            TOOLS_BUILT=$((TOOLS_BUILT + 1))
        else
            log_error "C99编译器链接失败"
            TOOLS_FAILED=$((TOOLS_FAILED + 1))
        fi
    else
        log_error "C99编译器源文件编译失败"
        TOOLS_FAILED=$((TOOLS_FAILED + 1))
    fi
}

# 运行构建测试
run_build_tests() {
    log_step "运行构建测试"
    
    local test_passed=0
    local test_failed=0
    
    # 测试模块加载
    log_info "测试模块加载..."
    local test_program="$PROJECT_ROOT/build/temp/test_modules.c"
    
    cat > "$test_program" << 'EOF'
#include <stdio.h>
#include <dlfcn.h>

int main() {
    const char* modules[] = {"layer0", "pipeline", "compiler", "module", "libc"};
    int module_count = sizeof(modules) / sizeof(modules[0]);
    int success_count = 0;
    
    for (int i = 0; i < module_count; i++) {
        char module_path[256];
        snprintf(module_path, sizeof(module_path), "./bin/layer2/%s.native", modules[i]);
        
        void* handle = dlopen(module_path, RTLD_LAZY);
        if (handle) {
            printf("✅ 模块 %s 加载成功\n", modules[i]);
            dlclose(handle);
            success_count++;
        } else {
            printf("❌ 模块 %s 加载失败: %s\n", modules[i], dlerror());
        }
    }
    
    printf("模块测试结果: %d/%d 成功\n", success_count, module_count);
    return (success_count == module_count) ? 0 : 1;
}
EOF
    
    local test_executable="$PROJECT_ROOT/build/temp/test_modules"
    if $COMPILER_CC "$test_program" -o "$test_executable" $LDFLAGS; then
        cd "$PROJECT_ROOT"
        if "$test_executable"; then
            log_success "模块加载测试通过"
            test_passed=$((test_passed + 1))
        else
            log_error "模块加载测试失败"
            test_failed=$((test_failed + 1))
        fi
    else
        log_error "模块测试程序编译失败"
        test_failed=$((test_failed + 1))
    fi
    
    # 清理测试文件
    rm -f "$test_program" "$test_executable"
    
    return $test_failed
}

# 清理构建文件
clean_build() {
    log_step "清理构建文件"
    
    rm -rf "$PROJECT_ROOT/build/obj"
    rm -rf "$PROJECT_ROOT/build/temp"
    rm -f "$PROJECT_ROOT/bin/layer2"/*.o
    rm -f "$PROJECT_ROOT/src/core/modules"/*.o
    rm -f "$PROJECT_ROOT/src/core"/*.o
    
    log_success "构建文件已清理"
}

# 显示构建摘要
show_build_summary() {
    local build_end_time=$(date +%s)
    local build_duration=$((build_end_time - BUILD_START_TIME))
    
    echo
    log_step "构建摘要"
    echo "构建时间: ${build_duration}秒"
    echo "模块构建: $MODULES_BUILT 成功, $MODULES_FAILED 失败"
    echo "工具构建: $TOOLS_BUILT 成功, $TOOLS_FAILED 失败"
    
    local total_success=$((MODULES_BUILT + TOOLS_BUILT))
    local total_failed=$((MODULES_FAILED + TOOLS_FAILED))
    
    if [ $total_failed -eq 0 ]; then
        log_success "🎉 所有构建任务成功完成！"
        return 0
    else
        log_error "⚠️  有 $total_failed 个构建任务失败"
        return 1
    fi
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  --clean          清理构建文件"
    echo "  --modules-only   只构建模块"
    echo "  --tools-only     只构建工具"
    echo "  --no-tests       跳过构建测试"
    echo "  --help           显示此帮助信息"
    echo
    echo "示例:"
    echo "  $0               # 完整构建"
    echo "  $0 --clean       # 清理构建文件"
    echo "  $0 --modules-only # 只构建模块"
}

# 主函数
main() {
    local clean_only=false
    local modules_only=false
    local tools_only=false
    local run_tests=true
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            --clean)
                clean_only=true
                shift
                ;;
            --modules-only)
                modules_only=true
                shift
                ;;
            --tools-only)
                tools_only=true
                shift
                ;;
            --no-tests)
                run_tests=false
                shift
                ;;
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
    
    # 执行清理
    if $clean_only; then
        clean_build
        exit 0
    fi
    
    # 初始化构建环境
    init_build_environment
    
    # 执行构建
    if ! $tools_only; then
        build_core_modules
    fi
    
    if ! $modules_only; then
        build_tools
        build_c99_compiler
    fi
    
    # 运行测试
    if $run_tests && ! $modules_only && ! $tools_only; then
        run_build_tests
    fi
    
    # 显示构建摘要
    show_build_summary
}

# 运行主函数
main "$@"
