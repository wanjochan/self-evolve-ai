#!/bin/bash
#
# build_macos.sh - macOS特定构建优化脚本
#
# 针对macOS平台的特殊需求进行优化，包括Apple Silicon支持
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

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }

# 检测macOS版本和架构
detect_macos_environment() {
    log_info "检测macOS环境..."
    
    # 检测macOS版本
    local macos_version=$(sw_vers -productVersion)
    local macos_major=$(echo "$macos_version" | cut -d. -f1)
    local macos_minor=$(echo "$macos_version" | cut -d. -f2)
    
    log_success "macOS版本: $macos_version"
    
    # 检测架构
    local arch=$(uname -m)
    log_success "架构: $arch"
    
    # 检测Xcode Command Line Tools
    if ! xcode-select -p >/dev/null 2>&1; then
        log_error "未安装Xcode Command Line Tools"
        log_info "请运行: xcode-select --install"
        exit 1
    fi
    
    local xcode_path=$(xcode-select -p)
    log_success "Xcode Command Line Tools: $xcode_path"
    
    # 检测可用的编译器
    if command -v clang >/dev/null 2>&1; then
        local clang_version=$(clang --version | head -1)
        log_success "Clang: $clang_version"
        export CC=clang
        export CXX=clang++
    elif command -v gcc >/dev/null 2>&1; then
        local gcc_version=$(gcc --version | head -1)
        log_success "GCC: $gcc_version"
        export CC=gcc
        export CXX=g++
    else
        log_error "未找到可用的C编译器"
        exit 1
    fi
    
    # 设置macOS特定的编译标志
    export MACOS_VERSION="$macos_version"
    export MACOS_MAJOR="$macos_major"
    export MACOS_MINOR="$macos_minor"
    export MACOS_ARCH="$arch"
}

# 设置macOS特定的编译器标志
setup_macos_compiler_flags() {
    log_info "设置macOS编译器标志..."
    
    # 基础标志
    local base_flags="-std=c99 -Wall -Wextra -fPIC"
    
    # macOS特定标志
    base_flags="$base_flags -D_DARWIN_C_SOURCE"
    
    # 设置最低支持的macOS版本
    if [ "$MACOS_MAJOR" -ge 11 ]; then
        # macOS 11+ (Big Sur及以后)
        base_flags="$base_flags -mmacosx-version-min=10.15"
    elif [ "$MACOS_MAJOR" -eq 10 ] && [ "$MACOS_MINOR" -ge 15 ]; then
        # macOS 10.15+ (Catalina及以后)
        base_flags="$base_flags -mmacosx-version-min=10.14"
    else
        # 较老的macOS版本
        base_flags="$base_flags -mmacosx-version-min=10.12"
    fi
    
    # 架构特定标志
    if [ "$MACOS_ARCH" = "arm64" ]; then
        # Apple Silicon (M1/M2)
        base_flags="$base_flags -arch arm64"
        log_success "Apple Silicon优化已启用"
    elif [ "$MACOS_ARCH" = "x86_64" ]; then
        # Intel Mac
        base_flags="$base_flags -arch x86_64"
        log_success "Intel Mac优化已启用"
    fi
    
    # 优化标志
    base_flags="$base_flags -O2 -g"
    
    # 链接器标志
    local linker_flags="-ldl"
    
    # 如果需要支持通用二进制文件（Universal Binary）
    if [ "${BUILD_UNIVERSAL:-no}" = "yes" ]; then
        log_info "构建通用二进制文件..."
        base_flags="$base_flags -arch x86_64 -arch arm64"
    fi
    
    export CFLAGS="$base_flags"
    export LDFLAGS="$linker_flags"
    
    log_success "编译器标志: $CFLAGS"
    log_success "链接器标志: $LDFLAGS"
}

# 检查macOS特定的依赖
check_macos_dependencies() {
    log_info "检查macOS依赖..."
    
    # 检查必要的系统库
    local required_frameworks=(
        "/System/Library/Frameworks/CoreFoundation.framework"
        "/System/Library/Frameworks/Security.framework"
    )
    
    for framework in "${required_frameworks[@]}"; do
        if [ -d "$framework" ]; then
            log_success "找到框架: $(basename "$framework")"
        else
            log_warn "缺少框架: $(basename "$framework")"
        fi
    done
    
    # 检查Homebrew（如果安装了）
    if command -v brew >/dev/null 2>&1; then
        local brew_prefix=$(brew --prefix)
        log_success "Homebrew: $brew_prefix"
        
        # 添加Homebrew路径到编译器搜索路径
        export CFLAGS="$CFLAGS -I$brew_prefix/include"
        export LDFLAGS="$LDFLAGS -L$brew_prefix/lib"
    fi
    
    # 检查MacPorts（如果安装了）
    if command -v port >/dev/null 2>&1; then
        log_success "MacPorts已安装"
        export CFLAGS="$CFLAGS -I/opt/local/include"
        export LDFLAGS="$LDFLAGS -L/opt/local/lib"
    fi
}

# 构建macOS特定的模块
build_macos_modules() {
    log_info "构建macOS特定模块..."
    
    local src_dir="$PROJECT_ROOT/src/core/modules"
    local obj_dir="$PROJECT_ROOT/build/obj"
    local output_dir="$PROJECT_ROOT/bin/layer2"
    
    mkdir -p "$obj_dir" "$output_dir"
    
    # macOS模块列表
    local modules=(
        "layer0_module"
        "pipeline_module"
        "compiler_module"
        "module_module"
        "libc_module"
    )
    
    local success_count=0
    local total_count=${#modules[@]}
    
    for module in "${modules[@]}"; do
        local source_file="$src_dir/${module}.c"
        local object_file="$obj_dir/${module}.o"
        local dylib_file="$output_dir/${module%_module}.dylib"
        local native_file="$output_dir/${module%_module}.native"
        
        if [ -f "$source_file" ]; then
            log_info "编译模块: $module"
            
            # 编译目标文件
            if $CC $CFLAGS -I"$PROJECT_ROOT/src/core" -c "$source_file" -o "$object_file"; then
                # 创建动态库
                if $CC -dynamiclib "$object_file" -o "$dylib_file" $LDFLAGS; then
                    # 创建兼容性符号链接
                    ln -sf "$(basename "$dylib_file")" "$native_file"
                    
                    log_success "模块构建成功: $(basename "$dylib_file")"
                    success_count=$((success_count + 1))
                else
                    log_error "动态库创建失败: $module"
                fi
            else
                log_error "编译失败: $module"
            fi
        else
            log_warn "源文件不存在: $source_file"
        fi
    done
    
    log_info "模块构建完成: $success_count/$total_count"
    return $((total_count - success_count))
}

# 运行macOS特定测试
run_macos_tests() {
    log_info "运行macOS特定测试..."
    
    # 测试动态库加载
    local test_program="$PROJECT_ROOT/build/temp/test_macos_modules.c"
    mkdir -p "$(dirname "$test_program")"
    
    cat > "$test_program" << 'EOF'
#include <stdio.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>

int main() {
    printf("=== macOS模块加载测试 ===\n");
    
    // 获取当前可执行文件路径
    char exe_path[1024];
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) == 0) {
        printf("可执行文件路径: %s\n", exe_path);
    }
    
    // 测试模块加载
    const char* modules[] = {"layer0", "pipeline", "compiler", "module", "libc"};
    int module_count = sizeof(modules) / sizeof(modules[0]);
    int success_count = 0;
    
    for (int i = 0; i < module_count; i++) {
        char module_path[256];
        snprintf(module_path, sizeof(module_path), "./bin/layer2/%s.dylib", modules[i]);
        
        void* handle = dlopen(module_path, RTLD_LAZY);
        if (handle) {
            printf("✅ 模块 %s 加载成功\n", modules[i]);
            
            // 获取模块信息
            Dl_info info;
            if (dladdr(handle, &info)) {
                printf("   路径: %s\n", info.dli_fname);
            }
            
            dlclose(handle);
            success_count++;
        } else {
            printf("❌ 模块 %s 加载失败: %s\n", modules[i], dlerror());
        }
    }
    
    printf("\n测试结果: %d/%d 模块加载成功\n", success_count, module_count);
    
    // 测试系统信息
    printf("\n=== 系统信息 ===\n");
    printf("架构: %s\n", 
#ifdef __arm64__
        "ARM64 (Apple Silicon)"
#elif defined(__x86_64__)
        "x86_64 (Intel)"
#else
        "Unknown"
#endif
    );
    
    return (success_count == module_count) ? 0 : 1;
}
EOF
    
    local test_executable="$PROJECT_ROOT/build/temp/test_macos_modules"
    
    if $CC "$test_program" -o "$test_executable" $CFLAGS $LDFLAGS; then
        cd "$PROJECT_ROOT"
        if "$test_executable"; then
            log_success "macOS测试通过"
            return 0
        else
            log_error "macOS测试失败"
            return 1
        fi
    else
        log_error "macOS测试程序编译失败"
        return 1
    fi
}

# 创建macOS应用程序包（可选）
create_macos_app_bundle() {
    if [ "${CREATE_APP_BUNDLE:-no}" = "yes" ]; then
        log_info "创建macOS应用程序包..."
        
        local app_name="SelfEvolvingAI"
        local app_bundle="$PROJECT_ROOT/build/${app_name}.app"
        local contents_dir="$app_bundle/Contents"
        local macos_dir="$contents_dir/MacOS"
        local resources_dir="$contents_dir/Resources"
        
        # 创建应用程序包结构
        mkdir -p "$macos_dir" "$resources_dir"
        
        # 复制可执行文件
        if [ -f "$PROJECT_ROOT/bin/c99" ]; then
            cp "$PROJECT_ROOT/bin/c99" "$macos_dir/$app_name"
        fi
        
        # 创建Info.plist
        cat > "$contents_dir/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>$app_name</string>
    <key>CFBundleIdentifier</key>
    <string>com.selfevolveai.compiler</string>
    <key>CFBundleName</key>
    <string>$app_name</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
</dict>
</plist>
EOF
        
        log_success "应用程序包已创建: $app_bundle"
    fi
}

# 主函数
main() {
    log_info "开始macOS特定构建..."
    
    # 检测macOS环境
    detect_macos_environment
    
    # 设置编译器标志
    setup_macos_compiler_flags
    
    # 检查依赖
    check_macos_dependencies
    
    # 构建模块
    if build_macos_modules; then
        log_success "macOS模块构建成功"
    else
        log_error "macOS模块构建失败"
        exit 1
    fi
    
    # 运行测试
    if run_macos_tests; then
        log_success "macOS测试通过"
    else
        log_warn "macOS测试失败，但继续构建"
    fi
    
    # 创建应用程序包（可选）
    create_macos_app_bundle
    
    log_success "🍎 macOS构建完成！"
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  --universal      构建通用二进制文件（Intel + Apple Silicon）"
    echo "  --app-bundle     创建macOS应用程序包"
    echo "  --help           显示此帮助信息"
    echo
    echo "环境变量:"
    echo "  BUILD_UNIVERSAL=yes    构建通用二进制文件"
    echo "  CREATE_APP_BUNDLE=yes  创建应用程序包"
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --universal)
            export BUILD_UNIVERSAL=yes
            shift
            ;;
        --app-bundle)
            export CREATE_APP_BUNDLE=yes
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

# 运行主函数
main
