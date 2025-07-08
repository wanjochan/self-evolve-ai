#!/bin/bash

# build_simple_loader.sh - Multi-Architecture Simple Loader Build Script
#
# This script builds the simple_loader for multiple architectures:
# - arm64_64: ARM 64-bit (Apple Silicon, ARM64 servers)
# - x64_64: Intel/AMD 64-bit (x86_64)
# - x86_32: Intel/AMD 32-bit (i386/i686)
#
# Usage: ./build_simple_loader.sh [architecture]
#   architecture: arm64, x64, x86, or 'all' (default: current)

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SRC_FILE="src/layer1/simple_loader.c"
BIN_DIR="bin"
CC_SCRIPT="./cc.sh"

# Create bin directory if it doesn't exist
mkdir -p "$BIN_DIR"

echo -e "${BLUE}=== Multi-Architecture Simple Loader Build ===${NC}"

# Function to print status
print_status() {
    local status=$1
    local message=$2
    if [ "$status" = "OK" ]; then
        echo -e "${GREEN}✓${NC} $message"
    elif [ "$status" = "WARN" ]; then
        echo -e "${YELLOW}⚠${NC} $message"
    elif [ "$status" = "ERROR" ]; then
        echo -e "${RED}✗${NC} $message"
    else
        echo -e "${BLUE}•${NC} $message"
    fi
}

# Check if cc.sh exists and is executable
if [ ! -x "$CC_SCRIPT" ]; then
    print_status "ERROR" "cc.sh 不存在或不可执行"
    echo "请确保 cc.sh 在当前目录且可执行"
    exit 1
fi

# Detect current architecture
detect_current_arch() {
    case "$(uname -m)" in
        "arm64"|"aarch64")
            echo "arm64"
            ;;
        "x86_64"|"amd64")
            echo "x64"
            ;;
        "i386"|"i486"|"i586"|"i686")
            echo "x86"
            ;;
        *)
            print_status "WARN" "未知架构 $(uname -m)，默认使用 x64"
            echo "x64"
            ;;
    esac
}

# Function to build for specific architecture
build_for_arch() {
    local arch=$1
    local bits=$2
    local gcc_arch=$3
    local gcc_flags=$4

    local output_file="$BIN_DIR/simple_loader_${arch}_${bits}"

    print_status "INFO" "构建 simple_loader for ${arch}_${bits}..."

    # Architecture-specific compiler flags
    local arch_flags=""
    case "$arch" in
        "arm64")
            arch_flags="-march=armv8-a -DARCH_ARM64"
            ;;
        "x64")
            arch_flags="-march=x86-64 -DARCH_X64"
            ;;
        "x86")
            arch_flags="-march=i686 -m32 -DARCH_X86"
            ;;
    esac

    # Build command
    if $CC_SCRIPT -o "$output_file" \
        "$SRC_FILE" \
        -std=c99 \
        -O2 \
        -Wall \
        -DTARGET_ARCH=\"$arch\" \
        -DTARGET_BITS=$bits \
        $arch_flags; then

        print_status "OK" "成功构建 $(basename $output_file)"

        # Verify the binary
        if [ -f "$output_file" ] && [ -x "$output_file" ]; then
            local size=$(stat -f%z "$output_file" 2>/dev/null || stat -c%s "$output_file" 2>/dev/null || echo "unknown")
            print_status "OK" "二进制文件大小: $size bytes"
            return 0
        else
            print_status "ERROR" "二进制文件验证失败"
            return 1
        fi
    else
        print_status "ERROR" "构建失败: $(basename $output_file)"
        return 1
    fi
}

# Parse command line arguments
TARGET_ARCH="${1:-current}"

if [ "$TARGET_ARCH" = "current" ]; then
    TARGET_ARCH=$(detect_current_arch)
    print_status "INFO" "检测到当前架构: $TARGET_ARCH"
fi

# Build for specified architecture(s)
SUCCESS_COUNT=0
TOTAL_COUNT=0

case "$TARGET_ARCH" in
    "arm64")
        ((TOTAL_COUNT++))
        if build_for_arch "arm64" "64" "aarch64" "-march=armv8-a"; then
            ((SUCCESS_COUNT++))
        fi
        ;;
    "x64")
        ((TOTAL_COUNT++))
        if build_for_arch "x64" "64" "x86_64" "-march=x86-64"; then
            ((SUCCESS_COUNT++))
        fi
        ;;
    "x86")
        ((TOTAL_COUNT++))
        if build_for_arch "x86" "32" "i686" "-march=i686 -m32"; then
            ((SUCCESS_COUNT++))
        fi
        ;;
    "all")
        print_status "INFO" "构建所有支持的架构..."

        # ARM64
        ((TOTAL_COUNT++))
        if build_for_arch "arm64" "64" "aarch64" "-march=armv8-a"; then
            ((SUCCESS_COUNT++))
        fi

        # x64
        ((TOTAL_COUNT++))
        if build_for_arch "x64" "64" "x86_64" "-march=x86-64"; then
            ((SUCCESS_COUNT++))
        fi

        # x86 (may fail on some systems without 32-bit support)
        ((TOTAL_COUNT++))
        if build_for_arch "x86" "32" "i686" "-march=i686 -m32"; then
            ((SUCCESS_COUNT++))
        else
            print_status "WARN" "x86_32 构建失败 (可能缺少32位支持库)"
        fi
        ;;
    *)
        print_status "ERROR" "不支持的架构: $TARGET_ARCH"
        echo "支持的架构: arm64, x64, x86, all, current"
        exit 1
        ;;
esac

echo ""

# Create symbolic link to current architecture binary
CURRENT_ARCH=$(detect_current_arch)
CURRENT_BINARY="$BIN_DIR/simple_loader_${CURRENT_ARCH}_64"
SYMLINK_TARGET="$BIN_DIR/simple_loader"

if [ -f "$CURRENT_BINARY" ]; then
    print_status "INFO" "创建符号链接: simple_loader -> simple_loader_${CURRENT_ARCH}_64"
    rm -f "$SYMLINK_TARGET"
    ln -s "simple_loader_${CURRENT_ARCH}_64" "$SYMLINK_TARGET"
    print_status "OK" "符号链接创建成功"
else
    print_status "WARN" "当前架构二进制文件不存在，跳过符号链接创建"
fi

echo ""

# Summary
echo -e "${BLUE}=== 构建总结 ===${NC}"
echo "成功构建: $SUCCESS_COUNT/$TOTAL_COUNT"
echo "输出目录: $BIN_DIR"

if [ $SUCCESS_COUNT -gt 0 ]; then
    echo ""
    echo "构建的二进制文件:"
    ls -la "$BIN_DIR"/simple_loader* 2>/dev/null | while read line; do
        echo "  $line"
    done

    echo ""
    echo "使用方法:"
    echo "  ./bin/simple_loader program.astc [args...]"
    echo "  ./bin/simple_loader_arm64_64 program.astc [args...]  # ARM64 specific"
    echo "  ./bin/simple_loader_x64_64 program.astc [args...]    # x64 specific"
    echo "  ./bin/simple_loader_x86_32 program.astc [args...]    # x86 specific"

    if [ $SUCCESS_COUNT -eq $TOTAL_COUNT ]; then
        print_status "OK" "所有目标架构构建成功！"
        exit 0
    else
        print_status "WARN" "部分架构构建失败"
        exit 1
    fi
else
    print_status "ERROR" "所有架构构建失败"
    exit 1
fi
