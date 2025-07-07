#!/bin/bash
#
# cc.sh - TinyCC wrapper script
# 
# This script serves as a wrapper for TinyCC (tcc) compiler,
# automatically setting the correct library paths.
#

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Define paths relative to the script location
TCC_PATH="$SCRIPT_DIR/external/tcc/dist/bin/tcc"
TCC_LIB_PATH="$SCRIPT_DIR/external/tcc/dist/lib/host/tcc"

# Try to use TCC first, fallback to GCC if TCC fails
if [ -x "$TCC_PATH" ] && [ -d "$TCC_LIB_PATH" ] && [ -f "$TCC_LIB_PATH/libtcc1.a" ]; then
    # Try TCC first
    "$TCC_PATH" -B "$TCC_LIB_PATH" "$@" 2>/dev/null
    exit_code=$?

    if [ $exit_code -eq 0 ]; then
        # TCC succeeded
        exit 0
    else
        # TCC failed, try with verbose output to see the error
        echo "Note: TCC failed, trying GCC as fallback..."
        "$TCC_PATH" -B "$TCC_LIB_PATH" "$@" 2>&1 | head -1
    fi
fi

# Fallback to GCC
echo "Note: Using GCC as compiler (TCC unavailable or failed)"
gcc "$@"
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "GCC compilation failed with exit code $exit_code"
    exit $exit_code
fi