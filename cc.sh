#!/bin/bash
#
# cc.sh - GCC Compiler Wrapper Script
#
# This script serves as the GCC compiler wrapper for the project,
# providing a consistent interface for compilation across the project.
#
# Note: This script replaced TinyCC as part of the replace_tcc project completion.
# TinyCC has been fully replaced by the c99bin compiler for simple programs,
# with GCC serving as the fallback compiler for complex programs.
#

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Use GCC as the primary fallback compiler
# (c99bin handles simple programs, GCC handles complex programs)
gcc "$@"
exit_code=$?

if [ $exit_code -ne 0 ]; then
    echo "Compilation failed with exit code $exit_code"
    exit $exit_code
fi

exit 0