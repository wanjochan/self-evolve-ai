#!/bin/bash
#
# c99.sh - C99 Compiler wrapper script with TinyCC fallback
# 
# This script serves as a wrapper that tries to use the native C99 compiler first,
# then falls back to TinyCC (tcc) if C99 fails or is incomplete.
# This enables gradual replacement of TinyCC with the native C99 implementation.
#

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Define paths relative to the script location
C99_COMPILER="$SCRIPT_DIR/bin/c99_compiler"
TCC_PATH="$SCRIPT_DIR/external/tcc/dist/bin/tcc"
TCC_LIB_PATH="$SCRIPT_DIR/external/tcc/dist/lib/host/tcc"

# Configuration flags
USE_C99_FIRST=true          # Try C99 compiler first
ENABLE_FALLBACK=true        # Enable TinyCC fallback
VERBOSE_MODE=false          # Verbose output
PERFORMANCE_TEST=false      # Enable performance comparison
COMPATIBILITY_MODE=false    # Force compatibility mode (TinyCC only)

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
            "INFO")  echo -e "${BLUE}[INFO]${NC} $message" ;;
            "WARN")  echo -e "${YELLOW}[WARN]${NC} $message" ;;
            "ERROR") echo -e "${RED}[ERROR]${NC} $message" ;;
            "OK")    echo -e "${GREEN}[OK]${NC} $message" ;;
        esac
    fi
}

# Function to check if C99 compiler is available and functional
check_c99_availability() {
    if [ ! -x "$C99_COMPILER" ]; then
        print_status "WARN" "C99 compiler not found at $C99_COMPILER"
        return 1
    fi
    
    # Test basic functionality
    if ! "$C99_COMPILER" --help >/dev/null 2>&1; then
        print_status "WARN" "C99 compiler not functional"
        return 1
    fi
    
    return 0
}

# Function to check if TinyCC is available
check_tcc_availability() {
    if [ ! -x "$TCC_PATH" ] || [ ! -d "$TCC_LIB_PATH" ] || [ ! -f "$TCC_LIB_PATH/libtcc1.a" ]; then
        print_status "WARN" "TinyCC not available"
        return 1
    fi
    return 0
}

# Function to determine if we should use C99 for this compilation
should_use_c99() {
    local args="$*"
    
    # Skip C99 for certain operations that are not yet supported
    if [[ "$args" == *"-c"* ]] && [[ "$args" == *".c"* ]]; then
        # Object file compilation - C99 might not support this yet
        print_status "INFO" "Object compilation detected, using TinyCC"
        return 1
    fi
    
    if [[ "$args" == *"-shared"* ]] || [[ "$args" == *"-fPIC"* ]]; then
        # Shared library compilation - C99 might not support this yet
        print_status "INFO" "Shared library compilation detected, using TinyCC"
        return 1
    fi
    
    # For simple compilation, try C99
    return 0
}

# Function to try C99 compilation
try_c99_compilation() {
    local args="$*"
    
    print_status "INFO" "Attempting C99 compilation..."
    
    # Convert arguments for C99 compiler
    local c99_args=""
    local input_file=""
    local output_file=""
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -o)
                output_file="$2"
                c99_args="$c99_args -o $2"
                shift 2
                ;;
            *.c)
                input_file="$1"
                c99_args="$c99_args $1"
                shift
                ;;
            -O*)
                c99_args="$c99_args $1"
                shift
                ;;
            -g)
                c99_args="$c99_args -g"
                shift
                ;;
            -v)
                c99_args="$c99_args -v"
                shift
                ;;
            *)
                # Skip unsupported arguments for now
                print_status "WARN" "Skipping unsupported argument: $1"
                shift
                ;;
        esac
    done
    
    # Try C99 compilation
    if [ -n "$input_file" ]; then
        if "$C99_COMPILER" $c99_args 2>/dev/null; then
            print_status "OK" "C99 compilation successful"
            return 0
        else
            print_status "WARN" "C99 compilation failed"
            return 1
        fi
    else
        print_status "WARN" "No input file detected for C99"
        return 1
    fi
}

# Function to try TinyCC compilation
try_tcc_compilation() {
    local args="$*"
    
    print_status "INFO" "Attempting TinyCC compilation..."
    
    # Try TCC first
    "$TCC_PATH" -B "$TCC_LIB_PATH" "$@" 2>/dev/null
    local tcc_exit_code=$?

    if [ $tcc_exit_code -eq 0 ]; then
        print_status "OK" "TinyCC compilation successful"
        return 0
    else
        print_status "WARN" "TinyCC compilation failed"
        # Show TCC error for debugging
        if [ "$VERBOSE_MODE" = true ]; then
            "$TCC_PATH" -B "$TCC_LIB_PATH" "$@" 2>&1 | head -3
        fi
        return $tcc_exit_code
    fi
}

# Function to try GCC as final fallback
try_gcc_compilation() {
    local args="$*"
    
    print_status "INFO" "Attempting GCC compilation as final fallback..."
    
    gcc "$@"
    local gcc_exit_code=$?
    
    if [ $gcc_exit_code -eq 0 ]; then
        print_status "OK" "GCC compilation successful"
        return 0
    else
        print_status "ERROR" "GCC compilation failed with exit code $gcc_exit_code"
        return $gcc_exit_code
    fi
}

# Parse command line options for the wrapper itself
parse_wrapper_options() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --c99-verbose)
                VERBOSE_MODE=true
                shift
                ;;
            --c99-no-fallback)
                ENABLE_FALLBACK=false
                shift
                ;;
            --c99-tcc-only)
                COMPATIBILITY_MODE=true
                shift
                ;;
            --c99-performance-test)
                PERFORMANCE_TEST=true
                VERBOSE_MODE=true
                shift
                ;;
            *)
                # Not a wrapper option, pass through
                break
                ;;
        esac
    done
    
    # Return remaining arguments
    echo "$@"
}

# Main compilation function
main() {
    # Parse wrapper-specific options
    local remaining_args
    remaining_args=$(parse_wrapper_options "$@")
    
    print_status "INFO" "C99 wrapper script starting..."
    
    # Force TinyCC mode if requested
    if [ "$COMPATIBILITY_MODE" = true ]; then
        print_status "INFO" "Compatibility mode: using TinyCC only"
        if check_tcc_availability; then
            try_tcc_compilation $remaining_args
            return $?
        else
            try_gcc_compilation $remaining_args
            return $?
        fi
    fi
    
    # Performance test mode
    if [ "$PERFORMANCE_TEST" = true ]; then
        print_status "INFO" "Performance test mode enabled"
        
        # Time C99 compilation
        if check_c99_availability && should_use_c99 $remaining_args; then
            local start_time=$(date +%s%N)
            if try_c99_compilation $remaining_args; then
                local end_time=$(date +%s%N)
                local c99_time=$((($end_time - $start_time) / 1000000))
                print_status "INFO" "C99 compilation time: ${c99_time}ms"
                return 0
            fi
        fi
        
        # Time TinyCC compilation
        if check_tcc_availability; then
            local start_time=$(date +%s%N)
            if try_tcc_compilation $remaining_args; then
                local end_time=$(date +%s%N)
                local tcc_time=$((($end_time - $start_time) / 1000000))
                print_status "INFO" "TinyCC compilation time: ${tcc_time}ms"
                return 0
            fi
        fi
        
        # Fallback to GCC
        try_gcc_compilation $remaining_args
        return $?
    fi
    
    # Normal compilation mode
    # Try C99 first if available and suitable
    if [ "$USE_C99_FIRST" = true ] && check_c99_availability && should_use_c99 $remaining_args; then
        if try_c99_compilation $remaining_args; then
            return 0
        fi
    fi
    
    # Fallback to TinyCC if enabled
    if [ "$ENABLE_FALLBACK" = true ] && check_tcc_availability; then
        if try_tcc_compilation $remaining_args; then
            return 0
        fi
    fi
    
    # Final fallback to GCC
    try_gcc_compilation $remaining_args
    return $?
}

# Run main function with all arguments
main "$@"
