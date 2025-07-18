#!/bin/bash
#
# platform_detect.sh - 跨平台检测和配置脚本
#
# 检测操作系统、架构、编译器等环境信息，为构建系统提供统一的配置
#

# 设置严格模式
set -euo pipefail

# 全局变量
PLATFORM_OS=""
PLATFORM_ARCH=""
PLATFORM_BITS=""
PLATFORM_ENDIAN=""
COMPILER_CC=""
COMPILER_CXX=""
LINKER_FLAGS=""
SHARED_LIB_EXT=""
EXECUTABLE_EXT=""
PATH_SEPARATOR=""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# 检测操作系统
detect_os() {
    log_info "检测操作系统..."
    
    case "$(uname -s)" in
        Linux*)
            PLATFORM_OS="linux"
            SHARED_LIB_EXT=".so"
            EXECUTABLE_EXT=""
            PATH_SEPARATOR=":"
            ;;
        Darwin*)
            PLATFORM_OS="macos"
            SHARED_LIB_EXT=".dylib"
            EXECUTABLE_EXT=""
            PATH_SEPARATOR=":"
            ;;
        CYGWIN*|MINGW*|MSYS*)
            PLATFORM_OS="windows"
            SHARED_LIB_EXT=".dll"
            EXECUTABLE_EXT=".exe"
            PATH_SEPARATOR=";"
            ;;
        FreeBSD*)
            PLATFORM_OS="freebsd"
            SHARED_LIB_EXT=".so"
            EXECUTABLE_EXT=""
            PATH_SEPARATOR=":"
            ;;
        NetBSD*)
            PLATFORM_OS="netbsd"
            SHARED_LIB_EXT=".so"
            EXECUTABLE_EXT=""
            PATH_SEPARATOR=":"
            ;;
        OpenBSD*)
            PLATFORM_OS="openbsd"
            SHARED_LIB_EXT=".so"
            EXECUTABLE_EXT=""
            PATH_SEPARATOR=":"
            ;;
        *)
            log_warn "未知操作系统: $(uname -s)，假设为类Unix系统"
            PLATFORM_OS="unix"
            SHARED_LIB_EXT=".so"
            EXECUTABLE_EXT=""
            PATH_SEPARATOR=":"
            ;;
    esac
    
    log_success "操作系统: $PLATFORM_OS"
}

# 检测架构
detect_arch() {
    log_info "检测系统架构..."
    
    local machine_arch="$(uname -m)"
    
    case "$machine_arch" in
        x86_64|amd64)
            PLATFORM_ARCH="x64"
            PLATFORM_BITS="64"
            ;;
        i386|i686|x86)
            PLATFORM_ARCH="x86"
            PLATFORM_BITS="32"
            ;;
        arm64|aarch64)
            PLATFORM_ARCH="arm64"
            PLATFORM_BITS="64"
            ;;
        armv7*|armv6*|arm)
            PLATFORM_ARCH="arm"
            PLATFORM_BITS="32"
            ;;
        mips64*)
            PLATFORM_ARCH="mips64"
            PLATFORM_BITS="64"
            ;;
        mips*)
            PLATFORM_ARCH="mips"
            PLATFORM_BITS="32"
            ;;
        ppc64*|powerpc64*)
            PLATFORM_ARCH="ppc64"
            PLATFORM_BITS="64"
            ;;
        ppc*|powerpc*)
            PLATFORM_ARCH="ppc"
            PLATFORM_BITS="32"
            ;;
        s390x)
            PLATFORM_ARCH="s390x"
            PLATFORM_BITS="64"
            ;;
        riscv64)
            PLATFORM_ARCH="riscv64"
            PLATFORM_BITS="64"
            ;;
        *)
            log_warn "未知架构: $machine_arch，假设为x64"
            PLATFORM_ARCH="x64"
            PLATFORM_BITS="64"
            ;;
    esac
    
    log_success "架构: ${PLATFORM_ARCH}_${PLATFORM_BITS}"
}

# 检测字节序
detect_endian() {
    log_info "检测字节序..."
    
    if command -v lscpu >/dev/null 2>&1; then
        local endian_info=$(lscpu | grep "Byte Order" | awk '{print $3}')
        case "$endian_info" in
            "Little")
                PLATFORM_ENDIAN="little"
                ;;
            "Big")
                PLATFORM_ENDIAN="big"
                ;;
            *)
                PLATFORM_ENDIAN="unknown"
                ;;
        esac
    else
        # 使用Python检测字节序
        if command -v python3 >/dev/null 2>&1; then
            PLATFORM_ENDIAN=$(python3 -c "import sys; print('little' if sys.byteorder == 'little' else 'big')")
        elif command -v python >/dev/null 2>&1; then
            PLATFORM_ENDIAN=$(python -c "import sys; print('little' if sys.byteorder == 'little' else 'big')")
        else
            # 默认假设为小端序（现代系统大多数是小端序）
            PLATFORM_ENDIAN="little"
            log_warn "无法检测字节序，假设为小端序"
        fi
    fi
    
    log_success "字节序: $PLATFORM_ENDIAN"
}

# 检测编译器
detect_compiler() {
    log_info "检测可用编译器..."
    
    # 检测C编译器
    if command -v gcc >/dev/null 2>&1; then
        COMPILER_CC="gcc"
        log_success "找到GCC: $(gcc --version | head -1)"
    elif command -v clang >/dev/null 2>&1; then
        COMPILER_CC="clang"
        log_success "找到Clang: $(clang --version | head -1)"
    elif command -v cc >/dev/null 2>&1; then
        COMPILER_CC="cc"
        log_success "找到系统默认C编译器"
    else
        log_error "未找到C编译器"
        return 1
    fi
    
    # 检测C++编译器
    if command -v g++ >/dev/null 2>&1; then
        COMPILER_CXX="g++"
    elif command -v clang++ >/dev/null 2>&1; then
        COMPILER_CXX="clang++"
    elif command -v c++ >/dev/null 2>&1; then
        COMPILER_CXX="c++"
    else
        log_warn "未找到C++编译器"
        COMPILER_CXX=""
    fi
}

# 设置平台特定的编译器标志
setup_compiler_flags() {
    log_info "设置编译器标志..."
    
    # 基础标志
    local base_flags="-std=c99 -Wall -Wextra"
    
    # 平台特定标志
    case "$PLATFORM_OS" in
        linux)
            LINKER_FLAGS="-ldl -lpthread"
            if [ "$PLATFORM_ARCH" = "x64" ]; then
                base_flags="$base_flags -m64"
            elif [ "$PLATFORM_ARCH" = "x86" ]; then
                base_flags="$base_flags -m32"
            fi
            ;;
        macos)
            LINKER_FLAGS="-ldl"
            # macOS特定标志
            base_flags="$base_flags -D_DARWIN_C_SOURCE"
            if [ "$PLATFORM_ARCH" = "arm64" ]; then
                base_flags="$base_flags -arch arm64"
            elif [ "$PLATFORM_ARCH" = "x64" ]; then
                base_flags="$base_flags -arch x86_64"
            fi
            ;;
        windows)
            LINKER_FLAGS="-lws2_32"
            base_flags="$base_flags -D_WIN32_WINNT=0x0600"
            ;;
        freebsd|netbsd|openbsd)
            LINKER_FLAGS="-ldl -lpthread"
            base_flags="$base_flags -D_BSD_SOURCE"
            ;;
        *)
            LINKER_FLAGS="-ldl"
            ;;
    esac
    
    # 添加位置无关代码标志（用于共享库）
    base_flags="$base_flags -fPIC"
    
    # 导出编译器标志
    export CFLAGS="$base_flags"
    export LDFLAGS="$LINKER_FLAGS"
    
    log_success "编译器标志: $CFLAGS"
    log_success "链接器标志: $LDFLAGS"
}

# 检查必要的工具
check_required_tools() {
    log_info "检查必要工具..."
    
    local required_tools=("make" "bash" "awk" "sed")
    local missing_tools=()
    
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            missing_tools+=("$tool")
        fi
    done
    
    if [ ${#missing_tools[@]} -gt 0 ]; then
        log_error "缺少必要工具: ${missing_tools[*]}"
        return 1
    fi
    
    log_success "所有必要工具都已安装"
}

# 生成配置文件
generate_config() {
    local config_file="${1:-platform_config.sh}"
    
    log_info "生成平台配置文件: $config_file"
    
    cat > "$config_file" << EOF
#!/bin/bash
#
# 平台配置文件 - 由 platform_detect.sh 自动生成
# 生成时间: $(date)
#

# 平台信息
export PLATFORM_OS="$PLATFORM_OS"
export PLATFORM_ARCH="$PLATFORM_ARCH"
export PLATFORM_BITS="$PLATFORM_BITS"
export PLATFORM_ENDIAN="$PLATFORM_ENDIAN"
export PLATFORM_TARGET="${PLATFORM_ARCH}_${PLATFORM_BITS}"

# 编译器信息
export COMPILER_CC="$COMPILER_CC"
export COMPILER_CXX="$COMPILER_CXX"

# 文件扩展名
export SHARED_LIB_EXT="$SHARED_LIB_EXT"
export EXECUTABLE_EXT="$EXECUTABLE_EXT"
export PATH_SEPARATOR="$PATH_SEPARATOR"

# 编译器标志
export CFLAGS="$CFLAGS"
export LDFLAGS="$LDFLAGS"

# 平台特定函数
is_linux() { [ "\$PLATFORM_OS" = "linux" ]; }
is_macos() { [ "\$PLATFORM_OS" = "macos" ]; }
is_windows() { [ "\$PLATFORM_OS" = "windows" ]; }
is_bsd() { [[ "\$PLATFORM_OS" =~ bsd$ ]]; }

is_x64() { [ "\$PLATFORM_ARCH" = "x64" ]; }
is_x86() { [ "\$PLATFORM_ARCH" = "x86" ]; }
is_arm64() { [ "\$PLATFORM_ARCH" = "arm64" ]; }
is_arm() { [ "\$PLATFORM_ARCH" = "arm" ]; }

is_64bit() { [ "\$PLATFORM_BITS" = "64" ]; }
is_32bit() { [ "\$PLATFORM_BITS" = "32" ]; }

# 获取共享库文件名
get_shared_lib_name() {
    local base_name="\$1"
    echo "\${base_name}\${SHARED_LIB_EXT}"
}

# 获取可执行文件名
get_executable_name() {
    local base_name="\$1"
    echo "\${base_name}\${EXECUTABLE_EXT}"
}
EOF
    
    chmod +x "$config_file"
    log_success "配置文件已生成: $config_file"
}

# 主函数
main() {
    log_info "开始平台检测..."
    
    detect_os
    detect_arch
    detect_endian
    detect_compiler
    setup_compiler_flags
    check_required_tools
    
    # 生成配置文件
    local config_file="${1:-$(dirname "$0")/platform_config.sh}"
    generate_config "$config_file"
    
    log_success "平台检测完成"
    echo
    echo "平台信息摘要:"
    echo "  操作系统: $PLATFORM_OS"
    echo "  架构: ${PLATFORM_ARCH}_${PLATFORM_BITS}"
    echo "  字节序: $PLATFORM_ENDIAN"
    echo "  C编译器: $COMPILER_CC"
    echo "  共享库扩展名: $SHARED_LIB_EXT"
    echo "  配置文件: $config_file"
}

# 如果直接运行此脚本
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
    main "$@"
fi
