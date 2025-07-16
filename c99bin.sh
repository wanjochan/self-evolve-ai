#!/bin/bash
#
# c99bin.sh - C99Bin Compiler wrapper script
# 
# This script serves as a wrapper for the C99Bin compiler,
# providing the same interface as cc.sh but using c99bin toolchain.
# NO FALLBACK - exclusively uses c99bin tools.
#

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Define paths relative to the script location
C99BIN_PATH="$SCRIPT_DIR/tools/c99bin"

# Check if c99bin tool exists
if [ ! -x "$C99BIN_PATH" ]; then
    echo "Error: C99Bin compiler not found at $C99BIN_PATH"
    echo "Please ensure c99bin is built by running: ./cc.sh -o tools/c99bin tools/c99bin.c"
    exit 1
fi

# Parse arguments to handle special cases
COMPILE_MODE=0
OUTPUT_FILE=""
SOURCE_FILES=()
OTHER_ARGS=()

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -c)
            # Compile only (object file generation)
            echo "Warning: C99Bin does not support -c flag (object file generation)"
            echo "C99Bin generates executable files directly"
            exit 1
            ;;
        -o)
            # Output file specification
            if [[ -n $2 ]]; then
                OUTPUT_FILE="$2"
                shift 2
            else
                echo "Error: -o option requires an argument"
                exit 1
            fi
            ;;
        -E|-S)
            # Preprocessing or assembly only
            echo "Warning: C99Bin does not support $1 flag (preprocessing/assembly only)"
            echo "C99Bin generates executable files directly"
            exit 1
            ;;
        -I*|-L*|-D*|-U*)
            # Include paths, library paths, defines, undefines
            echo "Warning: C99Bin does not support $1 flag"
            echo "C99Bin uses simplified compilation model"
            OTHER_ARGS+=("$1")
            shift
            ;;
        -l*)
            # Library linking
            echo "Warning: C99Bin does not support $1 flag (library linking)"
            echo "C99Bin uses simplified compilation model"
            OTHER_ARGS+=("$1")
            shift
            ;;
        --version)
            # Version information
            echo "C99Bin Compiler Wrapper v1.0"
            echo "Based on C99Bin toolchain"
            "$C99BIN_PATH" --help | head -1
            exit 0
            ;;
        --help|-h)
            # Help information
            echo "C99Bin Compiler Wrapper - cc.sh compatible interface"
            echo ""
            echo "Usage: $0 [options] source.c [-o output]"
            echo ""
            echo "Supported options:"
            echo "  -o <file>    Output executable file"
            echo "  --version    Show version information"
            echo "  --help, -h   Show this help message"
            echo ""
            echo "Unsupported options (C99Bin limitations):"
            echo "  -c           Compile only (object files)"
            echo "  -E           Preprocessing only"
            echo "  -S           Assembly only"
            echo "  -I<path>     Include paths"
            echo "  -L<path>     Library paths"
            echo "  -l<lib>      Link libraries"
            echo "  -D<macro>    Define macros"
            echo ""
            echo "C99Bin generates executable files directly from C source."
            exit 0
            ;;
        -*)
            # Other flags - warn but continue
            echo "Warning: Flag $1 may not be supported by C99Bin"
            OTHER_ARGS+=("$1")
            shift
            ;;
        *.c)
            # C source file
            SOURCE_FILES+=("$1")
            shift
            ;;
        *)
            # Other arguments
            OTHER_ARGS+=("$1")
            shift
            ;;
    esac
done

# Validate input
if [ ${#SOURCE_FILES[@]} -eq 0 ]; then
    echo "Error: No C source files specified"
    echo "Usage: $0 source.c [-o output]"
    exit 1
fi

if [ ${#SOURCE_FILES[@]} -gt 1 ]; then
    echo "Error: C99Bin does not support multiple source files"
    echo "Found: ${SOURCE_FILES[*]}"
    echo "Please compile one file at a time"
    exit 1
fi

# Set default output file if not specified
if [ -z "$OUTPUT_FILE" ]; then
    OUTPUT_FILE="a.out"
fi

# Build c99bin command
SOURCE_FILE="${SOURCE_FILES[0]}"
C99BIN_CMD=("$C99BIN_PATH" "$SOURCE_FILE" "-o" "$OUTPUT_FILE")

# Show what we're doing
echo "C99Bin: Compiling $SOURCE_FILE -> $OUTPUT_FILE"

# Execute c99bin
"${C99BIN_CMD[@]}"
exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo "C99Bin: Compilation successful"
else
    echo "C99Bin: Compilation failed with exit code $exit_code"
fi

exit $exit_code
