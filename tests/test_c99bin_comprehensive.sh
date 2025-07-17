#!/bin/bash

# test_c99bin_comprehensive.sh - Comprehensive C99Bin Testing Framework
# T2-T4 Implementation: Complete test suite for c99bin toolchain validation
# 
# This script provides a comprehensive testing framework for the c99bin toolchain,
# covering all aspects from basic syntax to complex integration scenarios.

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Test configuration
SCRIPT_DIR="$(dirname "$0")"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_RESULTS_DIR="$SCRIPT_DIR/c99bin_comprehensive_results"
TEST_TEMP_DIR="/tmp/c99bin_comprehensive_test"

# Test counters and statistics
declare -A test_stats=(
    [total]=0
    [passed]=0
    [failed]=0
    [skipped]=0
    [c99bin_success]=0
    [gcc_fallback]=0
    [expected_fail]=0
)

declare -A category_stats=(
    [syntax]=0
    [compilation]=0
    [functionality]=0
    [integration]=0
    [performance]=0
    [compatibility]=0
    [error_handling]=0
)

# Test framework functions
print_header() {
    echo -e "${MAGENTA}=== $1 ===${NC}"
}

print_subheader() {
    echo -e "${CYAN}--- $1 ---${NC}"
}

print_test_start() {
    echo -e "${BLUE}Testing:${NC} $1"
}

print_result() {
    local test_name="$1"
    local result="$2"
    local method="$3"
    local details="$4"
    local category="$5"

    test_stats[total]=$((test_stats[total] + 1))
    
    if [ -n "$category" ]; then
        category_stats[$category]=$((category_stats[$category] + 1))
    fi

    case "$result" in
        "PASS")
            test_stats[passed]=$((test_stats[passed] + 1))
            if [ "$method" = "c99bin" ]; then
                test_stats[c99bin_success]=$((test_stats[c99bin_success] + 1))
                echo -e "${GREEN}✅ PASS${NC} [$method] $test_name"
            else
                test_stats[gcc_fallback]=$((test_stats[gcc_fallback] + 1))
                echo -e "${YELLOW}✅ PASS${NC} [$method] $test_name"
            fi
            ;;
        "FAIL")
            test_stats[failed]=$((test_stats[failed] + 1))
            echo -e "${RED}❌ FAIL${NC} [$method] $test_name"
            if [ -n "$details" ]; then
                echo -e "${RED}   Details: $details${NC}"
            fi
            ;;
        "EXPECTED_FAIL")
            test_stats[expected_fail]=$((test_stats[expected_fail] + 1))
            test_stats[passed]=$((test_stats[passed] + 1))  # Count as success
            echo -e "${BLUE}⚠️ EXPECTED_FAIL${NC} [$method] $test_name"
            if [ -n "$details" ]; then
                echo -e "${BLUE}   Details: $details${NC}"
            fi
            ;;
        "SKIP")
            test_stats[skipped]=$((test_stats[skipped] + 1))
            echo -e "${YELLOW}⏭️ SKIP${NC} [$method] $test_name"
            if [ -n "$details" ]; then
                echo -e "${YELLOW}   Reason: $details${NC}"
            fi
            ;;
    esac
}

# Test execution with timeout and error handling
run_test() {
    local test_name="$1"
    local command="$2"
    local timeout_seconds="${3:-30}"
    local expected_method="${4:-c99bin}"
    local expected_exit_code="${5:-0}"
    local category="${6:-functionality}"

    print_test_start "$test_name"

    # Create isolated test environment
    local test_dir="$TEST_TEMP_DIR/$(echo "$test_name" | tr ' ' '_' | tr -cd '[:alnum:]_-')"
    mkdir -p "$test_dir"
    
    # Run command with timeout in isolated environment
    local actual_exit_code
    if timeout "$timeout_seconds" bash -c "cd '$test_dir' && $command" >/dev/null 2>&1; then
        actual_exit_code=$?
    else
        actual_exit_code=$?
    fi

    # Evaluate result
    if [ $actual_exit_code -eq 124 ]; then
        print_result "$test_name" "FAIL" "$expected_method" "Timeout after ${timeout_seconds}s" "$category"
        return 1
    elif [ $actual_exit_code -eq $expected_exit_code ]; then
        print_result "$test_name" "PASS" "$expected_method" "" "$category"
        return 0
    else
        print_result "$test_name" "FAIL" "$expected_method" "Exit code: $actual_exit_code (expected: $expected_exit_code)" "$category"
        return 1
    fi
}

# Create test source file
create_test_file() {
    local filename="$1"
    local content="$2"
    local test_dir="$3"
    
    echo "$content" > "$test_dir/$filename"
}

# Check if c99bin toolchain is available
check_prerequisites() {
    print_header "Prerequisites Check"
    
    local missing_tools=()
    
    # Check c99bin compiler
    if [ ! -f "$PROJECT_ROOT/tools/c99bin" ]; then
        missing_tools+=("tools/c99bin")
    fi
    
    # Check c99bin.sh wrapper
    if [ ! -f "$PROJECT_ROOT/c99bin.sh" ]; then
        missing_tools+=("c99bin.sh")
    fi
    
    # Check cc.sh for fallback testing
    if [ ! -f "$PROJECT_ROOT/cc.sh" ]; then
        missing_tools+=("cc.sh")
    fi
    
    if [ ${#missing_tools[@]} -gt 0 ]; then
        echo -e "${RED}❌ Missing required tools:${NC}"
        for tool in "${missing_tools[@]}"; do
            echo -e "${RED}   - $tool${NC}"
        done
        echo -e "${RED}Please build the c99bin toolchain first.${NC}"
        return 1
    fi
    
    echo -e "${GREEN}✅ All required tools found${NC}"
    return 0
}

# Initialize test environment
initialize_test_environment() {
    print_header "Test Environment Initialization"
    
    # Clean and create test directories
    rm -rf "$TEST_RESULTS_DIR" "$TEST_TEMP_DIR"
    mkdir -p "$TEST_RESULTS_DIR" "$TEST_TEMP_DIR"
    
    # Create test log
    exec 1> >(tee -a "$TEST_RESULTS_DIR/test_execution.log")
    exec 2> >(tee -a "$TEST_RESULTS_DIR/test_errors.log" >&2)
    
    echo "Test environment initialized at: $(date)"
    echo "Results directory: $TEST_RESULTS_DIR"
    echo "Temporary directory: $TEST_TEMP_DIR"
    echo ""
}

# Generate final test report
generate_final_report() {
    print_header "Final Test Report"
    
    local total=${test_stats[total]}
    local passed=${test_stats[passed]}
    local failed=${test_stats[failed]}
    local skipped=${test_stats[skipped]}
    local c99bin_success=${test_stats[c99bin_success]}
    local gcc_fallback=${test_stats[gcc_fallback]}
    local expected_fail=${test_stats[expected_fail]}
    
    echo "Test Execution Summary:"
    echo "======================"
    echo "Total tests: $total"
    echo "Passed: $passed"
    echo "Failed: $failed"
    echo "Skipped: $skipped"
    echo "Expected failures: $expected_fail"
    echo ""
    echo "Compilation Method Distribution:"
    echo "==============================="
    echo "C99Bin successes: $c99bin_success"
    echo "GCC fallbacks: $gcc_fallback"
    echo ""
    echo "Test Category Distribution:"
    echo "=========================="
    for category in "${!category_stats[@]}"; do
        echo "$category: ${category_stats[$category]}"
    done
    echo ""
    
    if [ $total -gt 0 ]; then
        local success_rate=$((passed * 100 / total))
        local c99bin_rate=$((c99bin_success * 100 / total))
        local failure_rate=$((failed * 100 / total))
        
        echo "Success rate: ${success_rate}%"
        echo "C99Bin usage rate: ${c99bin_rate}%"
        echo "Failure rate: ${failure_rate}%"
        echo ""
        
        # Save detailed report
        cat > "$TEST_RESULTS_DIR/comprehensive_test_report.txt" << EOF
C99Bin Comprehensive Test Report - $(date)
==========================================
Total tests: $total
Passed: $passed
Failed: $failed
Skipped: $skipped
Expected failures: $expected_fail

Success rate: ${success_rate}%
C99Bin usage rate: ${c99bin_rate}%
Failure rate: ${failure_rate}%

Compilation Methods:
- C99Bin successes: $c99bin_success
- GCC fallbacks: $gcc_fallback

Test Categories:
$(for category in "${!category_stats[@]}"; do echo "- $category: ${category_stats[$category]}"; done)
EOF
        
        # Determine overall result
        if [ $success_rate -ge 90 ]; then
            echo -e "${GREEN}🎉 EXCELLENT: Test suite passed with ${success_rate}% success rate!${NC}"
            return 0
        elif [ $success_rate -ge 80 ]; then
            echo -e "${GREEN}✅ GOOD: Test suite passed with ${success_rate}% success rate${NC}"
            return 0
        elif [ $success_rate -ge 70 ]; then
            echo -e "${YELLOW}⚠️ ACCEPTABLE: Test suite passed with ${success_rate}% success rate${NC}"
            return 0
        else
            echo -e "${RED}❌ POOR: Test suite failed with only ${success_rate}% success rate${NC}"
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
        echo "Cleaning up test results and temporary files..."
        rm -rf "$TEST_RESULTS_DIR" "$TEST_TEMP_DIR"
        echo "Cleanup completed"
    fi
}

# Main execution control
main() {
    echo -e "${MAGENTA}C99Bin Comprehensive Testing Framework${NC}"
    echo -e "${MAGENTA}=====================================${NC}"
    echo ""
    
    # Initialize
    if ! check_prerequisites; then
        exit 1
    fi
    
    initialize_test_environment
    
    # Load and execute test modules
    source "$SCRIPT_DIR/test_c99bin_syntax.sh"
    source "$SCRIPT_DIR/test_c99bin_compilation.sh"
    source "$SCRIPT_DIR/test_c99bin_functionality.sh"
    source "$SCRIPT_DIR/test_c99bin_integration.sh"
    source "$SCRIPT_DIR/test_c99bin_performance.sh"
    source "$SCRIPT_DIR/test_c99bin_compatibility.sh"
    source "$SCRIPT_DIR/test_c99bin_error_handling.sh"
    
    # Execute test suites
    test_syntax_support || true
    test_compilation_features || true
    test_functionality_features || true
    test_integration_features || true
    test_performance_features || true
    test_compatibility_features || true
    test_error_handling_features || true
    
    # Generate final report
    generate_final_report
}

# Execute main function or cleanup
if [ "$1" = "clean" ]; then
    cleanup clean
else
    main "$@"
fi
