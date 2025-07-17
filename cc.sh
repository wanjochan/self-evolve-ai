#!/bin/bash
#
# cc.sh - C99Bin Compiler Wrapper Script
#
# This script serves as the primary compiler wrapper for the project,
# providing a consistent interface for compilation across the project.
#
# Priority: c99bin (self-developed) > external compiler (if available)
# This ensures complete independence from external dependencies while
# providing graceful fallback when absolutely necessary.
#

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Function to compile with c99bin
compile_with_c99bin() {
    if [ -x "$SCRIPT_DIR/c99bin.sh" ]; then
        "$SCRIPT_DIR/c99bin.sh" "$@"
        return $?
    elif [ -x "$SCRIPT_DIR/tools/c99bin" ]; then
        "$SCRIPT_DIR/tools/c99bin" "$@"
        return $?
    else
        return 1
    fi
}

# Function to compile with external fallback (if enabled)
compile_with_fallback() {
    if [ "$ALLOW_EXTERNAL_COMPILER" = "yes" ] && command -v gcc >/dev/null 2>&1; then
        echo "Warning: Using external GCC as fallback compiler" >&2
        echo "Note: Set ALLOW_EXTERNAL_COMPILER=no to disable external dependencies" >&2
        gcc "$@"
        return $?
    else
        return 1
    fi
}

# Main compilation logic
main() {
    # Try c99bin first (our self-developed compiler)
    if compile_with_c99bin "$@"; then
        exit 0
    fi

    # If c99bin fails, try external fallback (only if explicitly allowed)
    if compile_with_fallback "$@"; then
        exit 0
    fi

    # If all compilation methods fail
    echo "Error: Compilation failed" >&2
    echo "- C99Bin compiler not available or compilation failed" >&2
    echo "- External compiler fallback disabled or not available" >&2
    echo "" >&2
    echo "To enable external compiler fallback:" >&2
    echo "  export ALLOW_EXTERNAL_COMPILER=yes" >&2
    echo "" >&2
    echo "To build c99bin compiler:" >&2
    echo "  ./c99bin_tools_build.sh" >&2
    exit 1
}

# Execute main function
main "$@"