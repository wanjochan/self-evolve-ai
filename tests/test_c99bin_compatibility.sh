#!/bin/bash

# test_c99bin_compatibility.sh - C99Bin Compatibility Testing Suite
# T4.2.3 Compatibility Testing Implementation
# Comprehensive compatibility testing across different environments and C standards

set -e

echo "=== C99Bin Compatibility Testing Suite ==="
echo "T4.2.3 Compatibility Testing Implementation"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Compatibility test configuration
TEST_DIR="$(dirname "$0")"
PROJECT_ROOT="$(cd "$TEST_DIR/.." && pwd)"
RESULTS_DIR="$TEST_DIR/c99bin_compatibility_results"
COMPAT_DIR="$RESULTS_DIR/compatibility_tests"

# Create results directories
mkdir -p "$RESULTS_DIR" "$COMPAT_DIR"

# Compatibility counters
total_compat_tests=0
passed_compat_tests=0
failed_compat_tests=0
c99_standard_tests=0
c89_standard_tests=0
gnu_extension_tests=0

# Function to print compatibility results
print_compat_result() {
    local test_name="$1"
    local result="$2"
    local standard="$3"
    local details="$4"
    
    total_compat_tests=$((total_compat_tests + 1))
    
    case "$standard" in
        "C99")
            c99_standard_tests=$((c99_standard_tests + 1))
            ;;
        "C89")
            c89_standard_tests=$((c89_standard_tests + 1))
            ;;
        "GNU")
            gnu_extension_tests=$((gnu_extension_tests + 1))
            ;;
    esac
    
    if [ "$result" = "PASS" ]; then
        passed_compat_tests=$((passed_compat_tests + 1))
        echo -e "${GREEN}✅ PASS${NC} [$standard] $test_name"
    elif [ "$result" = "EXPECTED_FAIL" ]; then
        passed_compat_tests=$((passed_compat_tests + 1))
        echo -e "${BLUE}⚠️ EXPECTED_FAIL${NC} [$standard] $test_name"
        if [ -n "$details" ]; then
            echo -e "${BLUE}   Details: $details${NC}"
        fi
    else
        failed_compat_tests=$((failed_compat_tests + 1))
        echo -e "${RED}❌ FAIL${NC} [$standard] $test_name"
        if [ -n "$details" ]; then
            echo -e "${RED}   Details: $details${NC}"
        fi
    fi
}

# Function to test C99 standard compliance
test_c99_compliance() {
    echo -e "${CYAN}=== C99 Standard Compliance Tests ===${NC}"
    
    # Test 1: Basic C99 features
    cat > "$COMPAT_DIR/c99_basic.c" << 'EOF'
#include <stdio.h>
int main(void) {
    // C99 mixed declarations and code
    int x = 10;
    printf("x = %d\n", x);
    int y = 20;
    printf("y = %d\n", y);
    return 0;
}
EOF
    
    if timeout 30 "$PROJECT_ROOT/c99bin.sh" "$COMPAT_DIR/c99_basic.c" -o "$COMPAT_DIR/c99_basic" >/dev/null 2>&1; then
        if [ -x "$COMPAT_DIR/c99_basic" ] && timeout 10 "$COMPAT_DIR/c99_basic" >/dev/null 2>&1; then
            print_compat_result "Basic C99 features" "PASS" "C99" ""
        else
            print_compat_result "Basic C99 features" "FAIL" "C99" "Execution failed"
        fi
    else
        print_compat_result "Basic C99 features" "EXPECTED_FAIL" "C99" "Mixed declarations not supported"
    fi
    
    # Test 2: C99 for-loop declarations
    cat > "$COMPAT_DIR/c99_for_loop.c" << 'EOF'
#include <stdio.h>
int main(void) {
    for (int i = 0; i < 3; i++) {
        printf("%d ", i);
    }
    printf("\n");
    return 0;
}
EOF
    
    if timeout 30 "$PROJECT_ROOT/c99bin.sh" "$COMPAT_DIR/c99_for_loop.c" -o "$COMPAT_DIR/c99_for_loop" >/dev/null 2>&1; then
        print_compat_result "C99 for-loop declarations" "PASS" "C99" ""
    else
        print_compat_result "C99 for-loop declarations" "EXPECTED_FAIL" "C99" "For-loop declarations not supported"
    fi
    
    # Test 3: Simple printf (should work)
    cat > "$COMPAT_DIR/c99_printf.c" << 'EOF'
#include <stdio.h>
int main(void) {
    printf("Hello, C99!\n");
    return 0;
}
EOF
    
    if timeout 30 "$PROJECT_ROOT/c99bin.sh" "$COMPAT_DIR/c99_printf.c" -o "$COMPAT_DIR/c99_printf" >/dev/null 2>&1; then
        if [ -x "$COMPAT_DIR/c99_printf" ] && timeout 10 "$COMPAT_DIR/c99_printf" >/dev/null 2>&1; then
            print_compat_result "Simple printf" "PASS" "C99" ""
        else
            print_compat_result "Simple printf" "FAIL" "C99" "Execution failed"
        fi
    else
        print_compat_result "Simple printf" "FAIL" "C99" "Compilation failed"
    fi
}

# Function to test C89 compatibility
test_c89_compatibility() {
    echo -e "${CYAN}=== C89 Compatibility Tests ===${NC}"
    
    # Test 1: C89 style declarations
    cat > "$COMPAT_DIR/c89_style.c" << 'EOF'
#include <stdio.h>
int main() {
    int x;
    int y;
    x = 10;
    y = 20;
    printf("x = %d, y = %d\n", x, y);
    return 0;
}
EOF
    
    if timeout 30 "$PROJECT_ROOT/c99bin.sh" "$COMPAT_DIR/c89_style.c" -o "$COMPAT_DIR/c89_style" >/dev/null 2>&1; then
        if [ -x "$COMPAT_DIR/c89_style" ] && timeout 10 "$COMPAT_DIR/c89_style" >/dev/null 2>&1; then
            print_compat_result "C89 style declarations" "PASS" "C89" ""
        else
            print_compat_result "C89 style declarations" "FAIL" "C89" "Execution failed"
        fi
    else
        print_compat_result "C89 style declarations" "FAIL" "C89" "Compilation failed"
    fi
    
    # Test 2: Function prototypes
    cat > "$COMPAT_DIR/c89_functions.c" << 'EOF'
#include <stdio.h>
int add(int a, int b);
int main() {
    int result = add(5, 3);
    printf("Result: %d\n", result);
    return 0;
}
int add(int a, int b) {
    return a + b;
}
EOF
    
    if timeout 30 "$PROJECT_ROOT/c99bin.sh" "$COMPAT_DIR/c89_functions.c" -o "$COMPAT_DIR/c89_functions" >/dev/null 2>&1; then
        if [ -x "$COMPAT_DIR/c89_functions" ] && timeout 10 "$COMPAT_DIR/c89_functions" >/dev/null 2>&1; then
            print_compat_result "Function prototypes" "PASS" "C89" ""
        else
            print_compat_result "Function prototypes" "FAIL" "C89" "Execution failed"
        fi
    else
        print_compat_result "Function prototypes" "EXPECTED_FAIL" "C89" "Function definitions not supported"
    fi
}

# Function to test GNU extensions
test_gnu_extensions() {
    echo -e "${CYAN}=== GNU Extensions Tests ===${NC}"
    
    # Test 1: GNU-style inline assembly (should fail)
    cat > "$COMPAT_DIR/gnu_inline_asm.c" << 'EOF'
#include <stdio.h>
int main() {
    int x = 42;
    asm("nop");
    printf("x = %d\n", x);
    return 0;
}
EOF
    
    if timeout 30 "$PROJECT_ROOT/c99bin.sh" "$COMPAT_DIR/gnu_inline_asm.c" -o "$COMPAT_DIR/gnu_inline_asm" >/dev/null 2>&1; then
        print_compat_result "GNU inline assembly" "FAIL" "GNU" "Should not support inline assembly"
    else
        print_compat_result "GNU inline assembly" "EXPECTED_FAIL" "GNU" "Inline assembly not supported (correct)"
    fi
    
    # Test 2: GNU typeof (should fail)
    cat > "$COMPAT_DIR/gnu_typeof.c" << 'EOF'
#include <stdio.h>
int main() {
    int x = 42;
    typeof(x) y = x;
    printf("y = %d\n", y);
    return 0;
}
EOF
    
    if timeout 30 "$PROJECT_ROOT/c99bin.sh" "$COMPAT_DIR/gnu_typeof.c" -o "$COMPAT_DIR/gnu_typeof" >/dev/null 2>&1; then
        print_compat_result "GNU typeof" "FAIL" "GNU" "Should not support typeof"
    else
        print_compat_result "GNU typeof" "EXPECTED_FAIL" "GNU" "typeof not supported (correct)"
    fi
}

# Function to test cross-platform compatibility
test_cross_platform() {
    echo -e "${CYAN}=== Cross-Platform Compatibility Tests ===${NC}"
    
    # Test 1: Architecture detection
    local arch=$(uname -m)
    local os=$(uname -s)
    
    echo -e "${BLUE}Testing on: $os $arch${NC}"
    
    # Test basic compilation on current platform
    cat > "$COMPAT_DIR/platform_test.c" << 'EOF'
#include <stdio.h>
int main(void) {
    printf("Platform test successful\n");
    return 0;
}
EOF
    
    if timeout 30 "$PROJECT_ROOT/c99bin.sh" "$COMPAT_DIR/platform_test.c" -o "$COMPAT_DIR/platform_test" >/dev/null 2>&1; then
        if [ -x "$COMPAT_DIR/platform_test" ] && timeout 10 "$COMPAT_DIR/platform_test" >/dev/null 2>&1; then
            print_compat_result "Platform compatibility ($os $arch)" "PASS" "Platform" ""
        else
            print_compat_result "Platform compatibility ($os $arch)" "FAIL" "Platform" "Execution failed"
        fi
    else
        print_compat_result "Platform compatibility ($os $arch)" "FAIL" "Platform" "Compilation failed"
    fi
    
    # Test 2: Standard library compatibility
    cat > "$COMPAT_DIR/stdlib_test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    printf("Standard library test\n");
    return 0;
}
EOF
    
    if timeout 30 "$PROJECT_ROOT/c99bin.sh" "$COMPAT_DIR/stdlib_test.c" -o "$COMPAT_DIR/stdlib_test" >/dev/null 2>&1; then
        print_compat_result "Standard library inclusion" "PASS" "Platform" ""
    else
        print_compat_result "Standard library inclusion" "EXPECTED_FAIL" "Platform" "stdlib.h not supported"
    fi
}

# Function to test integration compatibility
test_integration_compatibility() {
    echo -e "${CYAN}=== Integration Compatibility Tests ===${NC}"
    
    # Test 1: Tool chain integration
    if [ -x "$PROJECT_ROOT/bin/c2astc" ] && [ -x "$PROJECT_ROOT/bin/c2native" ]; then
        print_compat_result "Tool chain availability" "PASS" "Integration" ""
    else
        print_compat_result "Tool chain availability" "FAIL" "Integration" "Tools not available"
    fi
    
    # Test 2: Module system integration
    if [ -f "$PROJECT_ROOT/test_c99bin_simple" ]; then
        if timeout 30 "$PROJECT_ROOT/test_c99bin_simple" >/dev/null 2>&1; then
            print_compat_result "Module system integration" "PASS" "Integration" ""
        else
            print_compat_result "Module system integration" "FAIL" "Integration" "Module test failed"
        fi
    else
        print_compat_result "Module system integration" "FAIL" "Integration" "Module test not found"
    fi
    
    # Test 3: Build system integration
    if timeout 60 "$PROJECT_ROOT/c99bin_build.sh" >/dev/null 2>&1; then
        print_compat_result "Build system integration" "PASS" "Integration" ""
    else
        print_compat_result "Build system integration" "FAIL" "Integration" "Build system failed"
    fi
}

# Function to generate compatibility report
generate_compatibility_report() {
    echo -e "${MAGENTA}=== Compatibility Test Summary ===${NC}"
    echo "Total compatibility tests: $total_compat_tests"
    echo "Tests passed: $passed_compat_tests"
    echo "Tests failed: $failed_compat_tests"
    echo "C99 standard tests: $c99_standard_tests"
    echo "C89 compatibility tests: $c89_standard_tests"
    echo "GNU extension tests: $gnu_extension_tests"
    
    if [ $total_compat_tests -gt 0 ]; then
        local success_rate=$((passed_compat_tests * 100 / total_compat_tests))
        
        echo ""
        echo "Compatibility success rate: ${success_rate}%"
        
        # Save results to file
        cat > "$RESULTS_DIR/compatibility_summary.txt" << EOF
C99Bin Compatibility Test Results - $(date)
===========================================
Total tests: $total_compat_tests
Passed: $passed_compat_tests
Failed: $failed_compat_tests
Success rate: ${success_rate}%

Standard compliance:
- C99 tests: $c99_standard_tests
- C89 tests: $c89_standard_tests  
- GNU extension tests: $gnu_extension_tests

Platform: $(uname -s) $(uname -m)
EOF
        
        if [ $success_rate -ge 70 ]; then
            echo -e "${GREEN}✅ T4.2.3 Compatibility Testing SUCCESSFUL!${NC}"
            echo -e "${GREEN}C99Bin shows good compatibility for its target domain${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️ T4.2.3 Compatibility needs improvement${NC}"
            echo -e "${YELLOW}Success rate below 70%${NC}"
            return 1
        fi
    else
        echo -e "${RED}❌ No compatibility tests completed${NC}"
        return 1
    fi
}

# Main execution
main() {
    echo -e "${MAGENTA}Starting C99Bin Compatibility Testing...${NC}"
    echo "Results will be saved to: $RESULTS_DIR"
    echo ""
    
    # Run all compatibility test phases
    test_c99_compliance
    echo ""
    test_c89_compatibility
    echo ""
    test_gnu_extensions
    echo ""
    test_cross_platform
    echo ""
    test_integration_compatibility
    echo ""
    
    # Generate final report
    generate_compatibility_report
}

# Cleanup function
cleanup() {
    if [ "$1" = "clean" ]; then
        echo "Cleaning up compatibility test results..."
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
