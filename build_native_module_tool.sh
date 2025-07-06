#!/bin/bash

# Build the build_native_module tool
echo "Building build_native_module tool..."

# Use our own cc.sh wrapper for TinyCC
CC="./cc.sh"

# Check if cc.sh exists and is executable
if [ ! -x "$CC" ]; then
    echo "错误: cc.sh 不存在或不可执行"
    echo "请确保 cc.sh 在当前目录且可执行"
    exit 1
fi

# Build the standalone tool using our TinyCC wrapper
echo "使用 TinyCC 编译 build_native_module..."
$CC -o bin/build_native_module \
    src/tools/build_native_module_standalone.c \
    -std=c99 \
    -O2 \
    -Wall

if [ $? -eq 0 ]; then
    echo "✓ build_native_module tool built successfully"
    echo "Location: bin/build_native_module"
    echo ""
    echo "Usage:"
    echo "  ./bin/build_native_module input.o output.native --arch=x86_64 --type=user"
else
    echo "✗ Failed to build build_native_module tool"
    exit 1
fi
