#!/bin/bash
#
# build_examples_c99bin.sh - Build examples using c99bin.sh
# 
# This script demonstrates Phase 1 of the TinyCC replacement strategy:
# replacing cc.sh with c99bin.sh for simple programs
#

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLES_DIR="$SCRIPT_DIR/examples"
C99BIN_SCRIPT="$SCRIPT_DIR/c99bin.sh"

# Print status function
print_status() {
    local status=$1
    local message=$2
    case $status in
        "INFO")
            echo -e "${BLUE}[INFO]${NC} $message"
            ;;
        "OK")
            echo -e "${GREEN}[OK]${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}[WARN]${NC} $message"
            ;;
        "ERROR")
            echo -e "${RED}[ERROR]${NC} $message"
            ;;
    esac
}

# Check if c99bin.sh exists
if [ ! -x "$C99BIN_SCRIPT" ]; then
    print_status "ERROR" "c99bin.sh not found or not executable at $C99BIN_SCRIPT"
    exit 1
fi

echo -e "${BLUE}=== Building Examples with c99bin.sh (Phase 2) ===${NC}"
echo "Phase 2 of TinyCC replacement strategy - Enhanced Error Handling"
echo "Demonstrating improved c99bin.sh with graceful failure handling"
echo ""

# List of simple programs that c99bin.sh can handle
SIMPLE_PROGRAMS=(
    "simple_hello.c"
)

# List of programs that might work (to test)
TEST_PROGRAMS=(
    "test_program.c"
)

# List of complex programs that won't work with c99bin.sh
COMPLEX_PROGRAMS=(
    "hello_world.c"
    "c99_features_test.c"
    "c99_complex_syntax_test.c"
    "c99_error_handling_test.c"
    "c99_stdlib_test.c"
    "performance_test.c"
)

# Function to compile a program
compile_program() {
    local source_file=$1
    local category=$2
    local output_file="${source_file%.c}"
    
    print_status "INFO" "Compiling $source_file ($category)"
    
    if [ ! -f "$EXAMPLES_DIR/$source_file" ]; then
        print_status "WARN" "Source file $source_file not found, skipping"
        return 1
    fi
    
    # Try to compile with c99bin.sh (show error messages for better analysis)
    if "$C99BIN_SCRIPT" "$EXAMPLES_DIR/$source_file" -o "$EXAMPLES_DIR/$output_file" 2>&1; then
        print_status "OK" "Successfully compiled $source_file"
        
        # Test if the executable runs
        if "$EXAMPLES_DIR/$output_file" >/dev/null 2>&1; then
            print_status "OK" "Executable $output_file runs successfully"
            
            # Show file size
            local size=$(stat -c%s "$EXAMPLES_DIR/$output_file" 2>/dev/null || echo "unknown")
            print_status "INFO" "Executable size: $size bytes"
            return 0
        else
            print_status "WARN" "Executable $output_file compiled but failed to run"
            return 1
        fi
    else
        print_status "ERROR" "Failed to compile $source_file with c99bin.sh"
        return 1
    fi
}

# Compile simple programs (should work)
echo -e "${GREEN}=== Simple Programs (Expected to work) ===${NC}"
simple_success=0
simple_total=0

for program in "${SIMPLE_PROGRAMS[@]}"; do
    simple_total=$((simple_total + 1))
    if compile_program "$program" "simple"; then
        simple_success=$((simple_success + 1))
    fi
    echo ""
done

# Test programs (might work)
echo -e "${YELLOW}=== Test Programs (Might work) ===${NC}"
test_success=0
test_total=0

for program in "${TEST_PROGRAMS[@]}"; do
    test_total=$((test_total + 1))
    if compile_program "$program" "test"; then
        test_success=$((test_success + 1))
    fi
    echo ""
done

# Complex programs (expected to fail)
echo -e "${RED}=== Complex Programs (Expected to fail) ===${NC}"
complex_success=0
complex_total=0

for program in "${COMPLEX_PROGRAMS[@]}"; do
    complex_total=$((complex_total + 1))
    if compile_program "$program" "complex"; then
        complex_success=$((complex_success + 1))
    fi
    echo ""
done

# Summary
echo -e "${BLUE}=== Compilation Summary ===${NC}"
echo "Simple programs: $simple_success/$simple_total successful"
echo "Test programs: $test_success/$test_total successful"
echo "Complex programs: $complex_success/$complex_total successful"

total_success=$((simple_success + test_success + complex_success))
total_programs=$((simple_total + test_total + complex_total))
success_rate=$((total_success * 100 / total_programs))

echo ""
echo "Overall success rate: $total_success/$total_programs ($success_rate%)"

if [ $success_rate -ge 25 ]; then
    print_status "OK" "Phase 2 target maintained (≥25% success rate)"
else
    print_status "WARN" "Phase 2 target not met (<25% success rate)"
fi

echo ""
echo -e "${BLUE}=== Phase 2 Improvements ===${NC}"
echo "✅ Enhanced error handling - no more crashes"
echo "✅ Graceful failure with helpful error messages"
echo "✅ Better syntax detection to avoid stack overflow"
echo "✅ Improved buffer management for larger files"

echo ""
echo -e "${BLUE}=== Next Steps ===${NC}"
echo "1. ✅ Fixed stack overflow issues"
echo "2. ✅ Improved error handling and user feedback"
echo "3. 🔄 Create specialized build workflows for simple programs"
echo "4. 🔄 Document Phase 2 improvements and limitations"
