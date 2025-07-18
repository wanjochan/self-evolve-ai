#!/bin/bash
#
# 平台配置文件 - 由 platform_detect.sh 自动生成
# 生成时间: Fri Jul 18 03:44:32 UTC 2025
#

# 平台信息
export PLATFORM_OS="linux"
export PLATFORM_ARCH="x64"
export PLATFORM_BITS="64"
export PLATFORM_ENDIAN="little"
export PLATFORM_TARGET="x64_64"

# 编译器信息
export COMPILER_CC="gcc"
export COMPILER_CXX="g++"

# 文件扩展名
export SHARED_LIB_EXT=".so"
export EXECUTABLE_EXT=""
export PATH_SEPARATOR=":"

# 编译器标志
export CFLAGS="-std=c99 -Wall -Wextra -m64 -fPIC"
export LDFLAGS="-ldl -lpthread"

# 平台特定函数
is_linux() { [ "$PLATFORM_OS" = "linux" ]; }
is_macos() { [ "$PLATFORM_OS" = "macos" ]; }
is_windows() { [ "$PLATFORM_OS" = "windows" ]; }
is_bsd() { [[ "$PLATFORM_OS" =~ bsd$ ]]; }

is_x64() { [ "$PLATFORM_ARCH" = "x64" ]; }
is_x86() { [ "$PLATFORM_ARCH" = "x86" ]; }
is_arm64() { [ "$PLATFORM_ARCH" = "arm64" ]; }
is_arm() { [ "$PLATFORM_ARCH" = "arm" ]; }

is_64bit() { [ "$PLATFORM_BITS" = "64" ]; }
is_32bit() { [ "$PLATFORM_BITS" = "32" ]; }

# 获取共享库文件名
get_shared_lib_name() {
    local base_name="$1"
    echo "${base_name}${SHARED_LIB_EXT}"
}

# 获取可执行文件名
get_executable_name() {
    local base_name="$1"
    echo "${base_name}${EXECUTABLE_EXT}"
}
