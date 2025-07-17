#!/bin/bash

# Get the project root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Use project's compiler wrapper (c99bin-first, external fallback)
CC="$PROJECT_ROOT/cc.sh"

# Check if project compiler is available
if [ ! -x "$CC" ]; then
    echo "Error: Project compiler wrapper not found at $CC"
    exit 1
fi

echo "Using project compiler: $CC"

# Enable external compiler fallback for complex module compilation
export ALLOW_EXTERNAL_COMPILER=yes

# Compile test file
echo "Compiling module test..."
if "$CC" -o test_module_load test_module_load.c ../../src/core/module.c -ldl; then
    echo "✅ 编译成功，可以运行 ./test_module_load"
    echo "Note: Used project's c99bin-first compilation strategy"
else
    echo "❌ 编译失败"
    echo "Note: This may be expected if the module uses complex C features"
    echo "      that are not yet supported by c99bin"
    exit 1
fi
