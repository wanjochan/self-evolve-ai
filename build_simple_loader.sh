#!/bin/bash

# Build the simple_loader
echo "Building simple_loader..."

# Use our own cc.sh wrapper for TinyCC
CC="./cc.sh"

# Check if cc.sh exists and is executable
if [ ! -x "$CC" ]; then
    echo "错误: cc.sh 不存在或不可执行"
    echo "请确保 cc.sh 在当前目录且可执行"
    exit 1
fi

# Build the simple_loader using our TinyCC wrapper
echo "使用 TinyCC 编译 simple_loader..."
$CC -o bin/simple_loader \
    src/layer1/simple_loader.c \
    -std=c99 \
    -O2 \
    -Wall

if [ $? -eq 0 ]; then
    echo "✓ simple_loader built successfully"
    echo "Location: bin/simple_loader"
    echo ""
    echo "Usage:"
    echo "  ./bin/simple_loader program.astc [args...]"
else
    echo "✗ Failed to build simple_loader"
    exit 1
fi
