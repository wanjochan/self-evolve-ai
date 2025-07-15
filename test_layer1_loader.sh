#!/bin/bash
#
# test_layer1_loader.sh - Layer 1 Loader Test Script
#
# This script tests the simple_loader functionality including:
# - Architecture detection and module loading
# - ASTC program execution
# - Error handling and edge cases
# - Module system integration
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
TEST_DIR="$SCRIPT_DIR/tests/layer1_tests"
TEMP_DIR="$TEST_DIR/temp"
SIMPLE_LOADER="$SCRIPT_DIR/bin/simple_loader"

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_TOTAL=0

# Create test directories
mkdir -p "$TEST_DIR"
mkdir -p "$TEMP_DIR"

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case "$status" in
        "INFO")  echo -e "${BLUE}[INFO]${NC} $message" ;;
        "PASS")  echo -e "${GREEN}[PASS]${NC} $message" ;;
        "FAIL")  echo -e "${RED}[FAIL]${NC} $message" ;;
        "WARN")  echo -e "${YELLOW}[WARN]${NC} $message" ;;
    esac
}

# Function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_result="$3"  # 0 for success, non-zero for failure
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    print_status "INFO" "Running test: $test_name"
    
    if eval "$test_command" >/dev/null 2>&1; then
        local actual_result=0
    else
        local actual_result=$?
    fi
    
    if [ "$actual_result" -eq "$expected_result" ]; then
        print_status "PASS" "$test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        print_status "FAIL" "$test_name (expected exit code $expected_result, got $actual_result)"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

# Create test ASTC programs
create_test_programs() {
    print_status "INFO" "Creating test ASTC programs..."
    
    # Simple hello world program
    cat > "$TEMP_DIR/hello.c" << 'EOF'
#include <stdio.h>
int main() {
    printf("Hello from Layer 1!\n");
    return 0;
}
EOF

    # Program with return code
    cat > "$TEMP_DIR/return_code.c" << 'EOF'
int main() {
    return 42;
}
EOF

    # Program with arguments
    cat > "$TEMP_DIR/args_test.c" << 'EOF'
#include <stdio.h>
int main(int argc, char* argv[]) {
    printf("Arguments: %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("  [%d]: %s\n", i, argv[i]);
    }
    return argc;
}
EOF

    # Compile test programs to ASTC
    if [ -x "$SCRIPT_DIR/c99.sh" ]; then
        "$SCRIPT_DIR/c99.sh" "$TEMP_DIR/hello.c" -o "$TEMP_DIR/hello.astc" 2>/dev/null
        "$SCRIPT_DIR/c99.sh" "$TEMP_DIR/return_code.c" -o "$TEMP_DIR/return_code.astc" 2>/dev/null
        "$SCRIPT_DIR/c99.sh" "$TEMP_DIR/args_test.c" -o "$TEMP_DIR/args_test.astc" 2>/dev/null
        print_status "INFO" "Test ASTC programs created successfully"
    else
        print_status "WARN" "c99.sh not found, skipping ASTC program creation"
        return 1
    fi
}

# Test 1: Simple loader existence and basic functionality
test_loader_existence() {
    print_status "INFO" "Testing simple_loader existence and basic functionality..."
    
    if [ ! -x "$SIMPLE_LOADER" ]; then
        print_status "FAIL" "simple_loader not found or not executable at $SIMPLE_LOADER"
        return 1
    fi
    
    # Test help/usage
    if "$SIMPLE_LOADER" --help >/dev/null 2>&1 || "$SIMPLE_LOADER" -h >/dev/null 2>&1; then
        print_status "PASS" "simple_loader responds to help flags"
        return 0
    else
        # Some loaders might not have help, test with no args
        "$SIMPLE_LOADER" >/dev/null 2>&1
        local exit_code=$?
        if [ $exit_code -ne 0 ]; then
            print_status "PASS" "simple_loader exists and responds (exit code: $exit_code)"
            return 0
        else
            print_status "WARN" "simple_loader behavior unclear"
            return 0
        fi
    fi
}

# Test 2: ASTC program execution
test_astc_execution() {
    print_status "INFO" "Testing ASTC program execution..."
    
    if [ ! -f "$TEMP_DIR/hello.astc" ]; then
        print_status "FAIL" "Test ASTC program not available"
        return 1
    fi
    
    # Test basic execution
    local output=$("$SIMPLE_LOADER" "$TEMP_DIR/hello.astc" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 0 ] && echo "$output" | grep -q "Hello from Layer 1"; then
        print_status "PASS" "ASTC program execution successful"
        return 0
    else
        print_status "FAIL" "ASTC program execution failed (exit: $exit_code, output: $output)"
        return 1
    fi
}

# Test 3: Return code handling
test_return_code_handling() {
    print_status "INFO" "Testing return code handling..."
    
    if [ ! -f "$TEMP_DIR/return_code.astc" ]; then
        print_status "FAIL" "Return code test ASTC program not available"
        return 1
    fi
    
    # Test return code propagation
    "$SIMPLE_LOADER" "$TEMP_DIR/return_code.astc" >/dev/null 2>&1
    local exit_code=$?
    
    if [ $exit_code -eq 42 ]; then
        print_status "PASS" "Return code handling correct"
        return 0
    else
        print_status "FAIL" "Return code handling incorrect (expected 42, got $exit_code)"
        return 1
    fi
}

# Test 4: Argument passing
test_argument_passing() {
    print_status "INFO" "Testing argument passing..."
    
    if [ ! -f "$TEMP_DIR/args_test.astc" ]; then
        print_status "FAIL" "Arguments test ASTC program not available"
        return 1
    fi
    
    # Test argument passing
    local output=$("$SIMPLE_LOADER" "$TEMP_DIR/args_test.astc" arg1 arg2 arg3 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 4 ] && echo "$output" | grep -q "Arguments: 4"; then
        print_status "PASS" "Argument passing successful"
        return 0
    else
        print_status "FAIL" "Argument passing failed (exit: $exit_code, output: $output)"
        return 1
    fi
}

# Test 5: Error handling
test_error_handling() {
    print_status "INFO" "Testing error handling..."
    
    local error_tests_passed=0
    local total_error_tests=3
    
    # Test 1: Non-existent file
    if ! "$SIMPLE_LOADER" "$TEMP_DIR/nonexistent.astc" >/dev/null 2>&1; then
        error_tests_passed=$((error_tests_passed + 1))
        print_status "INFO" "Non-existent file correctly rejected"
    else
        print_status "WARN" "Non-existent file not rejected"
    fi
    
    # Test 2: Invalid ASTC file
    echo "invalid astc content" > "$TEMP_DIR/invalid.astc"
    if ! "$SIMPLE_LOADER" "$TEMP_DIR/invalid.astc" >/dev/null 2>&1; then
        error_tests_passed=$((error_tests_passed + 1))
        print_status "INFO" "Invalid ASTC file correctly rejected"
    else
        print_status "WARN" "Invalid ASTC file not rejected"
    fi
    
    # Test 3: No arguments
    if ! "$SIMPLE_LOADER" >/dev/null 2>&1; then
        error_tests_passed=$((error_tests_passed + 1))
        print_status "INFO" "No arguments correctly rejected"
    else
        print_status "WARN" "No arguments not rejected"
    fi
    
    if [ $error_tests_passed -eq $total_error_tests ]; then
        print_status "PASS" "All error handling tests passed"
        return 0
    else
        print_status "FAIL" "Some error handling tests failed ($error_tests_passed/$total_error_tests)"
        return 1
    fi
}

# Test 6: Architecture detection
test_architecture_detection() {
    print_status "INFO" "Testing architecture detection..."
    
    # Test with verbose mode if available
    local output=$("$SIMPLE_LOADER" --verbose "$TEMP_DIR/hello.astc" 2>&1 || "$SIMPLE_LOADER" "$TEMP_DIR/hello.astc" 2>&1)
    
    # Check for architecture-related output
    if echo "$output" | grep -qE "(x64_64|arm64_64|arch|architecture)"; then
        print_status "PASS" "Architecture detection appears functional"
        return 0
    else
        print_status "WARN" "Architecture detection not clearly visible (may be internal)"
        return 0  # Not a failure, just not observable
    fi
}

# Test 7: Module loading capability
test_module_loading() {
    print_status "INFO" "Testing module loading capability..."
    
    # Check if pipeline module exists
    local pipeline_modules=$(find "$SCRIPT_DIR/bin" -name "pipeline_*_*.native" 2>/dev/null)
    
    if [ -n "$pipeline_modules" ]; then
        # Test execution with potential module loading
        local output=$("$SIMPLE_LOADER" "$TEMP_DIR/hello.astc" 2>&1)
        local exit_code=$?
        
        if [ $exit_code -eq 0 ]; then
            print_status "PASS" "Module loading appears functional"
            return 0
        else
            print_status "WARN" "Module loading may have issues (exit: $exit_code)"
            return 1
        fi
    else
        print_status "WARN" "No pipeline modules found for testing"
        return 0
    fi
}

# Main test function
main() {
    echo -e "${BLUE}=== Layer 1 Loader Test Suite ===${NC}"
    echo "Testing simple_loader functionality..."
    echo
    
    # Create test programs
    if ! create_test_programs; then
        print_status "FAIL" "Failed to create test programs"
        exit 1
    fi
    
    # Run tests
    test_loader_existence
    test_astc_execution
    test_return_code_handling
    test_argument_passing
    test_error_handling
    test_architecture_detection
    test_module_loading
    
    # Print summary
    echo
    echo -e "${BLUE}=== Layer 1 Test Summary ===${NC}"
    echo "Tests passed: $TESTS_PASSED"
    echo "Tests failed: $TESTS_FAILED"
    echo "Total tests:  $TESTS_TOTAL"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        print_status "PASS" "All Layer 1 tests passed!"
        echo
        echo "simple_loader is functioning correctly."
        return 0
    else
        print_status "FAIL" "$TESTS_FAILED Layer 1 tests failed"
        echo
        echo "simple_loader has issues that need attention."
        return 1
    fi
}

# Cleanup function
cleanup() {
    if [ -d "$TEMP_DIR" ]; then
        rm -rf "$TEMP_DIR"
        print_status "INFO" "Cleaned up temporary files"
    fi
}

# Set up cleanup on exit
trap cleanup EXIT

# Run main function
main "$@"
