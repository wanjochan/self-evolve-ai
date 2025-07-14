#!/bin/bash
#
# c99.sh - C99 Compiler wrapper script
#
# This script uses the native C99 compiler exclusively.
# If C99 compiler is not available, it will build it automatically.
# No fallback to other compilers - C99 only.
#

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Define paths relative to the script location
C99_COMPILER="$SCRIPT_DIR/bin/c99_compiler"
C99_MAIN="$SCRIPT_DIR/src/c99/tools/c99_main.c"

# Build C99 compiler if it doesn't exist
build_c99_compiler() {
    echo "Building C99 compiler..."

    # Create bin directory if it doesn't exist
    mkdir -p "$SCRIPT_DIR/bin"

    # Check if we have the C99 source files
    if [ ! -f "$C99_MAIN" ]; then
        echo "Error: C99 compiler source not found at $C99_MAIN"
        echo "Please ensure the C99 source code is available in src/c99/"
        exit 1
    fi

    # Build the C99 compiler using cc.sh (bootstrap)
    echo "Bootstrapping C99 compiler using cc.sh..."
    "$SCRIPT_DIR/cc.sh" -o "$C99_COMPILER" \
        "$C99_MAIN" \
        "$SCRIPT_DIR/src/c99/frontend/c99_lexer.c" \
        "$SCRIPT_DIR/src/c99/frontend/c99_parser.c" \
        "$SCRIPT_DIR/src/c99/frontend/c99_semantic.c" \
        "$SCRIPT_DIR/src/c99/frontend/c99_error.c" \
        "$SCRIPT_DIR/src/c99/backend/c99_codegen.c" \
        -I "$SCRIPT_DIR/src/c99" \
        -I "$SCRIPT_DIR/src/core" \
        -std=c99 -O2 -Wall -ldl

    if [ $? -eq 0 ]; then
        echo "C99 compiler built successfully at $C99_COMPILER"
        return 0
    else
        echo "Error: Failed to build C99 compiler"
        echo "Please check the build dependencies and source code"
        exit 1
    fi
}

# Configuration flags
VERBOSE_MODE=false          # Verbose output

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    if [ "$VERBOSE_MODE" = true ]; then
        case "$status" in
            "INFO")  echo -e "${BLUE}[INFO]${NC} $message" >&2 ;;
            "WARN")  echo -e "${YELLOW}[WARN]${NC} $message" >&2 ;;
            "ERROR") echo -e "${RED}[ERROR]${NC} $message" >&2 ;;
            "OK")    echo -e "${GREEN}[OK]${NC} $message" >&2 ;;
        esac
    fi
}

# Function to ensure C99 compiler is available
ensure_c99_compiler() {
    if [ ! -x "$C99_COMPILER" ]; then
        print_status "INFO" "C99 compiler not found, building..."
        build_c99_compiler
    fi

    # Verify it's working
    if [ ! -x "$C99_COMPILER" ]; then
        echo "Error: C99 compiler not available at $C99_COMPILER"
        exit 1
    fi

    print_status "INFO" "C99 compiler ready at $C99_COMPILER"
    return 0
}

# Main compilation function
main() {
    # Parse arguments for verbose mode
    local remaining_args=()
    for arg in "$@"; do
        case "$arg" in
            --c99-verbose)
                VERBOSE_MODE=true
                ;;
            *)
                remaining_args+=("$arg")
                ;;
        esac
    done

    print_status "INFO" "C99 compiler starting..."

    # Ensure C99 compiler is available
    ensure_c99_compiler

    # Use C99 compiler directly
    print_status "INFO" "Compiling with C99 compiler..."
    if "$C99_COMPILER" "${remaining_args[@]}"; then
        print_status "OK" "C99 compilation successful"
        return 0
    else
        echo "Error: C99 compilation failed"
        exit 1
    fi
}

# Run main function with all arguments
main "$@"
