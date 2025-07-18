#!/bin/bash
#
# build_improved.sh - T4.3改进的构建系统
#
# 目标: 构建过程简化，支持增量构建
#

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# 构建配置
BUILD_DIR="$PROJECT_ROOT/build"
OUTPUT_DIR="$PROJECT_ROOT/bin"
TEMP_DIR="$BUILD_DIR/temp"
CACHE_FILE="$BUILD_DIR/build_cache.txt"

# 编译器配置
CC="${CC:-gcc}"
CXX="${CXX:-g++}"
AR="${AR:-ar}"

# 构建选项
BUILD_TYPE="${BUILD_TYPE:-debug}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc 2>/dev/null || echo 4)}"
ENABLE_INCREMENTAL="${ENABLE_INCREMENTAL:-true}"
ENABLE_VERBOSE="${ENABLE_VERBOSE:-false}"
ENABLE_CCACHE="${ENABLE_CCACHE:-false}"

# 日志函数
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_step() { echo -e "${CYAN}[STEP]${NC} $1"; }

# 显示帮助信息
show_help() {
    cat << EOF
T4.3 改进的构建系统

用法: $0 [选项] [目标]

选项:
  -h, --help              显示此帮助信息
  -c, --clean             清理构建文件
  -v, --verbose           启用详细输出
  -j, --jobs N            并行构建作业数 (默认: $PARALLEL_JOBS)
  -t, --type TYPE         构建类型: debug, release, profile (默认: $BUILD_TYPE)
  --no-incremental        禁用增量构建
  --enable-ccache         启用ccache
  --list-targets          列出所有可用目标

目标:
  all                     构建所有目标 (默认)
  core                    构建核心库
  tools                   构建工具
  tests                   构建测试
  clean                   清理构建文件
  install                 安装构建结果

环境变量:
  CC                      C编译器 (默认: gcc)
  CXX                     C++编译器 (默认: g++)
  AR                      归档工具 (默认: ar)
  BUILD_TYPE              构建类型
  PARALLEL_JOBS           并行作业数
  ENABLE_INCREMENTAL      启用增量构建
  ENABLE_VERBOSE          启用详细输出
  ENABLE_CCACHE           启用ccache

示例:
  $0                      # 构建所有目标 (debug模式)
  $0 -t release all       # 发布模式构建所有目标
  $0 -j 8 core            # 使用8个并行作业构建核心库
  $0 --clean              # 清理构建文件
  $0 --enable-ccache all  # 启用ccache构建所有目标

EOF
}

# 解析命令行参数
parse_arguments() {
    TARGETS=()
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                TARGETS+=("clean")
                shift
                ;;
            -v|--verbose)
                ENABLE_VERBOSE="true"
                shift
                ;;
            -j|--jobs)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            -t|--type)
                BUILD_TYPE="$2"
                shift 2
                ;;
            --no-incremental)
                ENABLE_INCREMENTAL="false"
                shift
                ;;
            --enable-ccache)
                ENABLE_CCACHE="true"
                shift
                ;;
            --list-targets)
                list_targets
                exit 0
                ;;
            -*)
                log_error "未知选项: $1"
                show_help
                exit 1
                ;;
            *)
                TARGETS+=("$1")
                shift
                ;;
        esac
    done
    
    # 如果没有指定目标，默认构建all
    if [ ${#TARGETS[@]} -eq 0 ]; then
        TARGETS=("all")
    fi
}

# 列出所有可用目标
list_targets() {
    echo "可用的构建目标:"
    echo "  all         - 构建所有目标"
    echo "  core        - 构建核心库"
    echo "  tools       - 构建工具"
    echo "  tests       - 构建测试"
    echo "  clean       - 清理构建文件"
    echo "  install     - 安装构建结果"
    echo ""
    echo "核心库目标:"
    echo "  libcore.a   - 核心静态库"
    echo ""
    echo "工具目标:"
    echo "  c2astc      - C到ASTC编译器"
    echo "  c2native    - C到本地代码编译器"
    echo "  simple_loader - 简单加载器"
    echo ""
    echo "测试目标:"
    echo "  test_astc   - ASTC测试"
    echo "  test_modules - 模块测试"
}

# 设置构建环境
setup_build_environment() {
    log_step "设置构建环境"
    
    # 创建构建目录
    mkdir -p "$BUILD_DIR" "$OUTPUT_DIR" "$TEMP_DIR"
    
    # 设置编译器标志
    case "$BUILD_TYPE" in
        debug)
            CFLAGS="-g -O0 -DDEBUG -Wall -Wextra"
            CXXFLAGS="-g -O0 -DDEBUG -Wall -Wextra"
            ;;
        release)
            CFLAGS="-O3 -DNDEBUG -Wall"
            CXXFLAGS="-O3 -DNDEBUG -Wall"
            ;;
        profile)
            CFLAGS="-g -O2 -pg -DPROFILE"
            CXXFLAGS="-g -O2 -pg -DPROFILE"
            ;;
        *)
            log_error "未知的构建类型: $BUILD_TYPE"
            exit 1
            ;;
    esac
    
    # 启用ccache
    if [ "$ENABLE_CCACHE" = "true" ] && command -v ccache >/dev/null 2>&1; then
        CC="ccache $CC"
        CXX="ccache $CXX"
        log_info "启用ccache加速编译"
    fi
    
    # 设置详细输出
    if [ "$ENABLE_VERBOSE" = "true" ]; then
        CFLAGS="$CFLAGS -v"
        CXXFLAGS="$CXXFLAGS -v"
    fi
    
    log_info "构建配置:"
    log_info "  构建类型: $BUILD_TYPE"
    log_info "  并行作业: $PARALLEL_JOBS"
    log_info "  增量构建: $ENABLE_INCREMENTAL"
    log_info "  详细输出: $ENABLE_VERBOSE"
    log_info "  编译器: $CC"
    log_info "  CFLAGS: $CFLAGS"
}

# 检查文件是否需要重新编译
needs_rebuild() {
    local source_file="$1"
    local object_file="$2"
    
    if [ "$ENABLE_INCREMENTAL" != "true" ]; then
        return 0  # 总是重新构建
    fi
    
    if [ ! -f "$object_file" ]; then
        return 0  # 目标文件不存在
    fi
    
    if [ "$source_file" -nt "$object_file" ]; then
        return 0  # 源文件比目标文件新
    fi
    
    return 1  # 不需要重新构建
}

# 编译源文件
compile_source() {
    local source_file="$1"
    local object_file="$2"
    local include_dirs="$3"
    local extra_flags="$4"
    
    # 创建目标文件目录
    mkdir -p "$(dirname "$object_file")"
    
    # 检查是否需要重新编译
    if ! needs_rebuild "$source_file" "$object_file"; then
        if [ "$ENABLE_VERBOSE" = "true" ]; then
            log_info "跳过 $source_file (最新)"
        fi
        return 0
    fi
    
    log_info "编译 $source_file"
    
    local compile_cmd="$CC $CFLAGS $include_dirs $extra_flags -c $source_file -o $object_file"
    
    if [ "$ENABLE_VERBOSE" = "true" ]; then
        echo "$compile_cmd"
    fi
    
    if ! eval "$compile_cmd"; then
        log_error "编译失败: $source_file"
        return 1
    fi
    
    return 0
}

# 链接目标文件
link_objects() {
    local output_file="$1"
    local object_files="$2"
    local libraries="$3"
    local extra_flags="$4"
    
    # 创建输出文件目录
    mkdir -p "$(dirname "$output_file")"
    
    log_info "链接 $output_file"
    
    local link_cmd="$CC $object_files $libraries $extra_flags -o $output_file"
    
    if [ "$ENABLE_VERBOSE" = "true" ]; then
        echo "$link_cmd"
    fi
    
    if ! eval "$link_cmd"; then
        log_error "链接失败: $output_file"
        return 1
    fi
    
    return 0
}

# 创建静态库
create_static_library() {
    local output_file="$1"
    local object_files="$2"
    
    # 创建输出文件目录
    mkdir -p "$(dirname "$output_file")"
    
    log_info "创建静态库 $output_file"
    
    local ar_cmd="$AR rcs $output_file $object_files"
    
    if [ "$ENABLE_VERBOSE" = "true" ]; then
        echo "$ar_cmd"
    fi
    
    if ! eval "$ar_cmd"; then
        log_error "创建静态库失败: $output_file"
        return 1
    fi
    
    return 0
}

# 构建核心库
build_core() {
    log_step "构建核心库"
    
    local core_sources=(
        "src/core/astc.c"
        "src/core/modules/module_module.c"
        "src/core/module_loading_optimizer.c"
        "src/core/enhanced_debug_system.c"
        "src/core/performance_analysis_tool.c"
        "src/core/memory_management_optimizer.c"
        "src/core/astc_execution_optimizer.c"
        "src/core/build_system_manager.c"
    )
    
    local core_objects=()
    local include_dirs="-Isrc/core -Isrc/core/modules"
    
    # 编译源文件
    for source in "${core_sources[@]}"; do
        if [ -f "$source" ]; then
            local object="$TEMP_DIR/$(basename "$source" .c).o"
            if compile_source "$source" "$object" "$include_dirs" ""; then
                core_objects+=("$object")
            else
                return 1
            fi
        fi
    done
    
    # 创建静态库
    if [ ${#core_objects[@]} -gt 0 ]; then
        create_static_library "$OUTPUT_DIR/libcore.a" "${core_objects[*]}"
    else
        log_warning "没有找到核心源文件"
        return 1
    fi
    
    log_success "核心库构建完成"
    return 0
}

# 构建工具
build_tools() {
    log_step "构建工具"
    
    local tools=("c2astc" "c2native" "simple_loader")
    local include_dirs="-Isrc/core -Itools"
    local libraries="$OUTPUT_DIR/libcore.a -lm"
    
    for tool in "${tools[@]}"; do
        local source="tools/$tool.c"
        local output="$OUTPUT_DIR/$tool"
        
        if [ -f "$source" ]; then
            local object="$TEMP_DIR/$tool.o"
            
            if compile_source "$source" "$object" "$include_dirs" ""; then
                if link_objects "$output" "$object" "$libraries" ""; then
                    log_success "工具 $tool 构建完成"
                else
                    log_error "工具 $tool 链接失败"
                    return 1
                fi
            else
                log_error "工具 $tool 编译失败"
                return 1
            fi
        else
            log_warning "工具源文件不存在: $source"
        fi
    done
    
    return 0
}

# 构建测试
build_tests() {
    log_step "构建测试"
    
    # 这里可以添加测试构建逻辑
    log_info "测试构建功能待实现"
    
    return 0
}

# 清理构建文件
clean_build() {
    log_step "清理构建文件"
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        log_info "删除构建目录: $BUILD_DIR"
    fi
    
    if [ -d "$OUTPUT_DIR" ]; then
        rm -rf "$OUTPUT_DIR"
        log_info "删除输出目录: $OUTPUT_DIR"
    fi
    
    log_success "清理完成"
}

# 安装构建结果
install_build() {
    log_step "安装构建结果"
    
    local install_dir="${INSTALL_PREFIX:-/usr/local}"
    local bin_dir="$install_dir/bin"
    local lib_dir="$install_dir/lib"
    local include_dir="$install_dir/include"
    
    log_info "安装到: $install_dir"
    
    # 创建安装目录
    mkdir -p "$bin_dir" "$lib_dir" "$include_dir"
    
    # 安装库文件
    if [ -f "$OUTPUT_DIR/libcore.a" ]; then
        cp "$OUTPUT_DIR/libcore.a" "$lib_dir/"
        log_info "安装库: libcore.a"
    fi
    
    # 安装工具
    for tool in c2astc c2native simple_loader; do
        if [ -f "$OUTPUT_DIR/$tool" ]; then
            cp "$OUTPUT_DIR/$tool" "$bin_dir/"
            chmod +x "$bin_dir/$tool"
            log_info "安装工具: $tool"
        fi
    done
    
    # 安装头文件
    if [ -d "src/core" ]; then
        cp -r src/core/*.h "$include_dir/" 2>/dev/null || true
        log_info "安装头文件"
    fi
    
    log_success "安装完成"
}

# 主构建函数
main() {
    local start_time=$(date +%s)
    
    echo "=== T4.3 改进的构建系统 ==="
    echo "开始时间: $(date)"
    echo
    
    parse_arguments "$@"
    setup_build_environment
    
    # 执行构建目标
    for target in "${TARGETS[@]}"; do
        case "$target" in
            all)
                build_core && build_tools && build_tests
                ;;
            core)
                build_core
                ;;
            tools)
                build_tools
                ;;
            tests)
                build_tests
                ;;
            clean)
                clean_build
                ;;
            install)
                install_build
                ;;
            *)
                log_error "未知目标: $target"
                exit 1
                ;;
        esac
        
        if [ $? -ne 0 ]; then
            log_error "目标 '$target' 构建失败"
            exit 1
        fi
    done
    
    local end_time=$(date +%s)
    local build_time=$((end_time - start_time))
    
    echo
    echo "=== 构建完成 ==="
    echo "构建时间: ${build_time} 秒"
    echo "结束时间: $(date)"
    
    log_success "所有目标构建成功！"
}

# 运行主函数
main "$@"
