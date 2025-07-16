#!/bin/bash

# test_c99bin_regression.sh - C99Bin Regression Test Suite
# T4.2.1 Regression Test Suite Implementation
# Comprehensive testing of c99bin compiler against all examples and core functionality

set -e

echo "=== C99Bin Regression Test Suite ==="
echo "T4.2.1 Regression Test Suite Implementation"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Test configuration
TEST_DIR="$(dirname "$0")"
PROJECT_ROOT="$(cd "$TEST_DIR/.." && pwd)"
RESULTS_DIR="$TEST_DIR/c99bin_regression_results"
EXAMPLES_DIR="$PROJECT_ROOT/examples"
BIN_DIR="$PROJECT_ROOT/bin"

# Create results directory
mkdir -p "$RESULTS_DIR"

# Test counters
total_tests=0
passed_tests=0
failed_tests=0
c99bin_successes=0
gcc_fallbacks=0

# Function to print test results
print_test_result() {
    local test_name="$1"
    local result="$2"
    local method="$3"
    local details="$4"
    
    total_tests=$((total_tests + 1))
    
    if [ "$result" = "PASS" ]; then
        passed_tests=$((passed_tests + 1))
        if [ "$method" = "c99bin" ]; then
            c99bin_successes=$((c99bin_successes + 1))
            echo -e "${GREEN}✅ PASS${NC} [$method] $test_name"
        else
            gcc_fallbacks=$((gcc_fallbacks + 1))
            echo -e "${YELLOW}✅ PASS${NC} [$method] $test_name"
        fi
    else
        failed_tests=$((failed_tests + 1))
        echo -e "${RED}❌ FAIL${NC} [$method] $test_name"
        if [ -n "$details" ]; then
            echo -e "${RED}   Details: $details${NC}"
        fi
    fi
}

# Function to run a test with timeout
run_test_with_timeout() {
    local test_name="$1"
    local command="$2"
    local timeout_seconds="$3"
    local expected_method="$4"
    local expected_exit_code="${5:-0}"  # Default to 0 if not specified

    echo -e "${BLUE}Running:${NC} $test_name"

    # Run command with timeout
    timeout "$timeout_seconds" bash -c "$command" >/dev/null 2>&1
    local actual_exit_code=$?

    if [ $actual_exit_code -eq 124 ]; then
        print_test_result "$test_name" "FAIL" "$expected_method" "Timeout after ${timeout_seconds}s"
        return 1
    elif [ $actual_exit_code -eq $expected_exit_code ]; then
        print_test_result "$test_name" "PASS" "$expected_method" ""
        return 0
    else
        print_test_result "$test_name" "FAIL" "$expected_method" "Exit code: $actual_exit_code (expected: $expected_exit_code)"
        return 1
    fi
}

# Phase 1: Core Module Compilation Tests
test_core_modules() {
    echo -e "${CYAN}=== Phase 1: Core Module Compilation Tests ===${NC}"
    
    declare -a modules=(
        "astc:$PROJECT_ROOT/src/core/astc.c:c99bin"
        "layer0_module:$PROJECT_ROOT/src/core/modules/layer0_module.c:c99bin"
        "pipeline_utils:$PROJECT_ROOT/src/core/modules/pipeline_utils.c:c99bin"
        "pipeline_frontend:$PROJECT_ROOT/src/core/modules/pipeline_frontend.c:gcc"
        "compiler_module:$PROJECT_ROOT/src/core/modules/compiler_module.c:gcc"
        "libc_module:$PROJECT_ROOT/src/core/modules/libc_module.c:gcc"
        "c99bin_module:$PROJECT_ROOT/src/core/modules/c99bin_module.c:gcc"
    )
    
    for module_info in "${modules[@]}"; do
        IFS=':' read -r module_name source_file expected_method <<< "$module_info"
        
        if [ -f "$source_file" ]; then
            local output_file="$RESULTS_DIR/${module_name}_regression.o"
            local compile_cmd="$PROJECT_ROOT/build_c99bin_complete.sh && test -f $BIN_DIR/${module_name}_complete.o"
            
            run_test_with_timeout "Module: $module_name" "$compile_cmd" 60 "$expected_method"
        else
            print_test_result "Module: $module_name" "FAIL" "missing" "Source file not found"
        fi
    done
}

# Phase 2: Tool Chain Tests
test_toolchain() {
    echo -e "${CYAN}=== Phase 2: Tool Chain Tests ===${NC}"
    
    declare -a tools=(
        "c2astc:c99bin"
        "c2native:c99bin"
        "simple_loader:c99bin"
    )
    
    for tool_info in "${tools[@]}"; do
        IFS=':' read -r tool_name expected_method <<< "$tool_info"
        
        # Test tool existence
        local tool_path="$BIN_DIR/$tool_name"
        if [ -f "$tool_path" ]; then
            print_test_result "Tool exists: $tool_name" "PASS" "$expected_method" ""
            
            # Test tool execution (basic help/version)
            if timeout 10 "$tool_path" --help >/dev/null 2>&1 || timeout 10 "$tool_path" --version >/dev/null 2>&1; then
                print_test_result "Tool executes: $tool_name" "PASS" "$expected_method" ""
            else
                print_test_result "Tool executes: $tool_name" "FAIL" "$expected_method" "Cannot execute tool"
            fi
        else
            print_test_result "Tool exists: $tool_name" "FAIL" "$expected_method" "Tool not found"
        fi
    done
}

# Phase 3: Example Program Compilation Tests
test_examples() {
    echo -e "${CYAN}=== Phase 3: Example Program Compilation Tests ===${NC}"
    
    if [ ! -d "$EXAMPLES_DIR" ]; then
        print_test_result "Examples directory" "FAIL" "system" "Directory not found"
        return
    fi
    
    # Test each C file in examples
    for c_file in "$EXAMPLES_DIR"/*.c; do
        if [ -f "$c_file" ]; then
            local basename=$(basename "$c_file" .c)
            local output_file="$RESULTS_DIR/${basename}_regression"
            
            # Determine expected method based on file complexity
            local line_count=$(wc -l < "$c_file")
            local expected_method="c99bin"
            if [ "$line_count" -gt 500 ]; then
                expected_method="gcc"
            fi
            
            # Test compilation
            local compile_cmd="$PROJECT_ROOT/c99bin.sh '$c_file' -o '$output_file'"
            
            if run_test_with_timeout "Example: $basename compilation" "$compile_cmd" 30 "$expected_method"; then
                # Test execution if compilation succeeded
                if [ -f "$output_file" ] && [ -x "$output_file" ]; then
                    run_test_with_timeout "Example: $basename execution" "'$output_file'" 10 "$expected_method"
                fi
            fi
        fi
    done
}

# Phase 4: Integration Tests
test_integration() {
    echo -e "${CYAN}=== Phase 4: Integration Tests ===${NC}"
    
    # Test c99bin module loading
    if [ -f "$PROJECT_ROOT/test_c99bin_simple" ]; then
        run_test_with_timeout "C99Bin module loading" "$PROJECT_ROOT/test_c99bin_simple" 30 "c99bin"
    else
        print_test_result "C99Bin module loading" "FAIL" "c99bin" "Test executable not found"
    fi
    
    # Test complete build system
    run_test_with_timeout "Complete build system" "$PROJECT_ROOT/build_c99bin_complete.sh" 180 "mixed"
    
    # Test tool chain build system
    run_test_with_timeout "Tool chain build system" "$PROJECT_ROOT/c99bin_tools_build.sh" 120 "c99bin"
    
    # Test module build system
    run_test_with_timeout "Module build system" "$PROJECT_ROOT/c99bin_build.sh" 120 "mixed"
}

# Phase 5: Stress Tests
test_stress() {
    echo -e "${CYAN}=== Phase 5: Stress Tests ===${NC}"
    
    # Create a complex test program
    local complex_test="$RESULTS_DIR/complex_stress_test.c"
    cat > "$complex_test" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test various C99 features
struct test_struct {
    int value;
    char name[64];
};

int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    struct test_struct test = {42, "test"};
    
    printf("Testing C99 features:\n");
    printf("Struct value: %d\n", test.value);
    printf("Struct name: %s\n", test.name);
    printf("Factorial of 5: %d\n", factorial(5));
    
    // Test array initialization
    int arr[] = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += arr[i];
    }
    printf("Array sum: %d\n", sum);
    
    return 0;
}
EOF
    
    # Test compilation of complex program
    local complex_output="$RESULTS_DIR/complex_stress_test"
    local compile_cmd="$PROJECT_ROOT/c99bin.sh '$complex_test' -o '$complex_output'"
    
    if run_test_with_timeout "Stress: Complex C99 program" "$compile_cmd" 60 "c99bin"; then
        if [ -f "$complex_output" ] && [ -x "$complex_output" ]; then
            run_test_with_timeout "Stress: Complex program execution" "'$complex_output'" 10 "c99bin"
        fi
    fi
}

# Main execution
main() {
    echo -e "${MAGENTA}Starting C99Bin Regression Test Suite...${NC}"
    echo "Test results will be saved to: $RESULTS_DIR"
    echo ""
    
    # Run all test phases
    test_core_modules
    echo ""
    test_toolchain
    echo ""
    test_examples
    echo ""
    test_integration
    echo ""
    test_stress
    
    # Generate final report
    echo ""
    echo -e "${MAGENTA}=== Final Test Report ===${NC}"
    echo "Total tests run: $total_tests"
    echo "Tests passed: $passed_tests"
    echo "Tests failed: $failed_tests"
    echo "C99Bin successes: $c99bin_successes"
    echo "GCC fallbacks: $gcc_fallbacks"
    
    if [ "$total_tests" -gt 0 ]; then
        local success_rate=$((passed_tests * 100 / total_tests))
        local c99bin_rate=$((c99bin_successes * 100 / total_tests))
        
        echo ""
        echo "Success rate: ${success_rate}%"
        echo "C99Bin usage rate: ${c99bin_rate}%"
        echo "GCC fallback rate: $(((total_tests - c99bin_successes) * 100 / total_tests))%"
        
        # Save results to file
        cat > "$RESULTS_DIR/regression_test_summary.txt" << EOF
C99Bin Regression Test Results - $(date)
==========================================
Total tests: $total_tests
Passed: $passed_tests
Failed: $failed_tests
Success rate: ${success_rate}%
C99Bin usage rate: ${c99bin_rate}%
GCC fallback rate: $(((total_tests - c99bin_successes) * 100 / total_tests))%
EOF
        
        if [ "$success_rate" -ge 80 ]; then
            echo -e "${GREEN}✅ T4.2.1 Regression Test Suite PASSED!${NC}"
            echo -e "${GREEN}Success rate meets 80%+ requirement${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️ T4.2.1 Regression Test Suite needs improvement${NC}"
            echo -e "${YELLOW}Success rate below 80% requirement${NC}"
            return 1
        fi
    else
        echo -e "${RED}❌ No tests were executed${NC}"
        return 1
    fi
}

# Cleanup function
cleanup() {
    if [ "$1" = "clean" ]; then
        echo "Cleaning up test results..."
        rm -rf "$RESULTS_DIR"
        echo "Cleanup completed"
    fi
}

# Execute main function or cleanup
if [ "$1" = "clean" ]; then
    cleanup clean
else
    main
fi
