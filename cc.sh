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

# Check if TCC exists and is executable
if [ ! -x "$TCC_PATH" ]; then
    echo "Error: TCC not found at $TCC_PATH or not executable"
    exit 1
fi

# Check if the library path exists
if [ ! -d "$TCC_LIB_PATH" ]; then
    echo "Error: TCC library directory not found at $TCC_LIB_PATH"
    exit 1
fi

# Check if libtcc1.a exists
if [ ! -f "$TCC_LIB_PATH/libtcc1.a" ]; then
    echo "Error: libtcc1.a not found at $TCC_LIB_PATH/libtcc1.a"
    exit 1
fi

# Forward all arguments to tcc with proper library path
"$TCC_PATH" -B "$TCC_LIB_PATH" "$@"

# Check the exit status of TCC
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "TCC compilation failed with exit code $exit_code"
    exit $exit_code
fi 