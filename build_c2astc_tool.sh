#!/bin/bash

# Build the c2astc tool
echo "Building c2astc tool..."

# Use our own cc.sh wrapper for TinyCC
CC="./cc.sh"

# Check if cc.sh exists and is executable
if [ ! -x "$CC" ]; then
    echo "错误: cc.sh 不存在或不可执行"
    echo "请确保 cc.sh 在当前目录且可执行"
    exit 1
fi

# Create bin directory if it doesn't exist
mkdir -p bin

# Build the simple_c2astc tool using our TinyCC wrapper
echo "使用 TinyCC 编译 simple_c2astc..."
$CC -o bin/c2astc \
    src/tools/simple_c2astc.c \
    src/core/module.c \
    -Isrc/core \
    -std=c99 \
    -O2 \
    -Wall

if [ $? -eq 0 ]; then
    echo "✓ c2astc tool built successfully"
    echo "Location: bin/c2astc"
    echo ""
    echo "Usage:"
    echo "  ./bin/c2astc input.c output.astc"
else
    echo "✗ Failed to build c2astc tool"
    exit 1
fi
