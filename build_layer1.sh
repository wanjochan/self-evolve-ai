#!/bin/bash
# build_layer1.sh - Cross-platform build script for Layer 1 Loader
# Supports Linux and macOS

set -e  # Exit on any error

echo "========================================"
echo "Building Layer 1 (Loader) Executables"
echo "Cross-Platform Build (Linux/macOS)"
echo "========================================"

# Detect platform
PLATFORM=$(uname -s)
ARCH=$(uname -m)

echo "Detected platform: $PLATFORM"
echo "Detected architecture: $ARCH"

# Create output directory
mkdir -p bin/layer1

# Determine compiler
if command -v gcc >/dev/null 2>&1; then
    CC=gcc
elif command -v clang >/dev/null 2>&1; then
    CC=clang
else
    echo "Error: No suitable C compiler found (gcc or clang required)"
    exit 1
fi

echo "Using compiler: $CC"

# Set compiler flags
CFLAGS="-std=c99 -O2 -DNDEBUG -Wall -Wextra"
LDFLAGS=""

# Platform-specific adjustments
case "$PLATFORM" in
    "Linux")
        CFLAGS="$CFLAGS -D_GNU_SOURCE"
        LDFLAGS="$LDFLAGS -ldl"
        ;;
    "Darwin")  # macOS
        CFLAGS="$CFLAGS -D_DARWIN_C_SOURCE"
        LDFLAGS="$LDFLAGS -ldl"
        ;;
    *)
        echo "Warning: Unsupported platform $PLATFORM, using default settings"
        ;;
esac

# Architecture-specific settings
case "$ARCH" in
    "x86_64"|"amd64")
        ARCH_NAME="x64"
        BITS="64"
        ;;
    "i386"|"i686")
        ARCH_NAME="x86"
        BITS="32"
        CFLAGS="$CFLAGS -m32"
        ;;
    "aarch64"|"arm64")
        ARCH_NAME="arm64"
        BITS="64"
        ;;
    "armv7l"|"armv6l")
        ARCH_NAME="arm32"
        BITS="32"
        ;;
    *)
        echo "Warning: Unknown architecture $ARCH, defaulting to x64"
        ARCH_NAME="x64"
        BITS="64"
        ;;
esac

echo "Target: ${ARCH_NAME}_${BITS}"

# Build loader
echo ""
echo "Building loader_${ARCH_NAME}_${BITS}..."
echo "================================"

SOURCE_FILES="src/layer1/loader.c src/core/utils.c src/core/native.c"
OUTPUT_FILE="bin/layer1/loader_${ARCH_NAME}_${BITS}"

echo "Compiling with: $CC $CFLAGS $SOURCE_FILES $LDFLAGS -o $OUTPUT_FILE"

$CC $CFLAGS $SOURCE_FILES $LDFLAGS -o "$OUTPUT_FILE"

if [ $? -eq 0 ]; then
    echo "Success: loader_${ARCH_NAME}_${BITS} compiled successfully"
else
    echo "Error: Failed to compile loader_${ARCH_NAME}_${BITS}"
    exit 1
fi

# Test the executable
echo ""
echo "Testing loader_${ARCH_NAME}_${BITS}..."
echo "=============================="

if [ -x "$OUTPUT_FILE" ]; then
    echo "Testing --help option:"
    "$OUTPUT_FILE" --help
    if [ $? -eq 0 ]; then
        echo "Success: loader_${ARCH_NAME}_${BITS} runs successfully"
    else
        echo "Warning: loader_${ARCH_NAME}_${BITS} compiled but may have runtime issues"
    fi
else
    echo "Error: $OUTPUT_FILE is not executable"
    exit 1
fi

# Build summary
echo ""
echo "Layer 1 Build Summary:"
echo "======================"
ls -la bin/layer1/loader_*
echo ""

echo "Success: Layer 1 Loader build completed successfully"
echo ""
echo "Usage:"
echo "  ./bin/layer1/loader_${ARCH_NAME}_${BITS} program.astc"
echo "  ./bin/layer1/loader_${ARCH_NAME}_${BITS} --help"
echo ""
echo "Next steps:"
echo "1. Build Layer 2 (VM modules): run ./build_layer2.sh"
echo "2. Build Layer 3 (ASTC programs): run ./build_layer3.sh"
echo "3. Test complete flow: loader → vm → program"

exit 0
