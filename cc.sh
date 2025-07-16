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

# TEMPORARY WORKAROUND: Use GCC with TCC-like behavior due to GLIBC compatibility issues
# TODO: Fix TCC GLIBC 2.38 dependency and restore pure TCC usage

echo "Warning: Using GCC as TCC replacement due to GLIBC compatibility issues"
echo "TCC Error: $TCC_PATH requires GLIBC 2.38, system has $(ldd --version | head -1 | grep -o '[0-9]\+\.[0-9]\+')"

# Use GCC with TCC-compatible flags
gcc "$@"
exit_code=$?

if [ $exit_code -ne 0 ]; then
    echo "Compilation failed with exit code $exit_code"
    exit $exit_code
fi

exit 0