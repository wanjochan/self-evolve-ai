#!/bin/bash

# test_c99bin_error_handling.sh - C99Bin Error Handling Testing Module
# T3.2 Implementation: Comprehensive error handling and graceful failure testing

# Test error handling features
test_error_handling_features() {
    print_header "Error Handling and Graceful Failure Testing"
    
    test_syntax_error_handling
    test_file_error_handling
    test_unsupported_feature_handling
    test_runtime_error_handling
}

# Test syntax error detection and handling
test_syntax_error_handling() {
    print_subheader "Syntax Error Handling Tests"
    
    test_missing_semicolon_error
    test_unmatched_braces_error
    test_invalid_syntax_error
    test_type_mismatch_error
}

test_missing_semicolon_error() {
    local test_dir="$TEST_TEMP_DIR/missing_semicolon"
    mkdir -p "$test_dir"
    
    create_test_file "missing_semicolon.c" 'int main() {
    int x = 10
    return x;
}' "$test_dir"
    
    # Should fail gracefully with helpful error message
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh missing_semicolon.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        if echo "$output" | grep -i "error\|syntax\|semicolon" >/dev/null; then
            print_result "Missing Semicolon Error" "PASS" "c99bin" "Correctly detected syntax error" "error_handling"
        else
            print_result "Missing Semicolon Error" "FAIL" "c99bin" "Error detected but message unclear" "error_handling"
        fi
    else
        print_result "Missing Semicolon Error" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_unmatched_braces_error() {
    local test_dir="$TEST_TEMP_DIR/unmatched_braces"
    mkdir -p "$test_dir"
    
    create_test_file "unmatched_braces.c" 'int main() {
    int x = 10;
    if (x > 5) {
        return 1;
    // Missing closing brace
}' "$test_dir"
    
    # Should fail gracefully
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh unmatched_braces.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        print_result "Unmatched Braces Error" "PASS" "c99bin" "Correctly detected syntax error" "error_handling"
    else
        print_result "Unmatched Braces Error" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_invalid_syntax_error() {
    local test_dir="$TEST_TEMP_DIR/invalid_syntax"
    mkdir -p "$test_dir"
    
    create_test_file "invalid_syntax.c" 'int main() {
    int x = 10;
    x =+ 5;  // Invalid operator
    return x;
}' "$test_dir"
    
    # Should fail gracefully
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh invalid_syntax.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        print_result "Invalid Syntax Error" "PASS" "c99bin" "Correctly detected syntax error" "error_handling"
    else
        print_result "Invalid Syntax Error" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_type_mismatch_error() {
    local test_dir="$TEST_TEMP_DIR/type_mismatch"
    mkdir -p "$test_dir"
    
    create_test_file "type_mismatch.c" 'int main() {
    int x = "string";  // Type mismatch
    return x;
}' "$test_dir"
    
    # Should fail gracefully
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh type_mismatch.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        print_result "Type Mismatch Error" "PASS" "c99bin" "Correctly detected type error" "error_handling"
    else
        print_result "Type Mismatch Error" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

# Test file-related error handling
test_file_error_handling() {
    print_subheader "File Error Handling Tests"
    
    test_file_not_found_error
    test_permission_denied_error
    test_empty_file_error
    test_binary_file_error
}

test_file_not_found_error() {
    local test_dir="$TEST_TEMP_DIR/file_not_found"
    mkdir -p "$test_dir"
    
    # Try to compile non-existent file
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh nonexistent.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        if echo "$output" | grep -i "not found\|no such file" >/dev/null; then
            print_result "File Not Found Error" "PASS" "c99bin" "Correctly handled missing file" "error_handling"
        else
            print_result "File Not Found Error" "FAIL" "c99bin" "Error detected but message unclear" "error_handling"
        fi
    else
        print_result "File Not Found Error" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_permission_denied_error() {
    local test_dir="$TEST_TEMP_DIR/permission_denied"
    mkdir -p "$test_dir"
    
    # Create a file and remove read permissions
    create_test_file "no_read.c" 'int main() { return 0; }' "$test_dir"
    chmod 000 "$test_dir/no_read.c" 2>/dev/null || true
    
    # Try to compile file without read permissions
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh no_read.c -o test" 2>&1)
    local exit_code=$?
    
    # Restore permissions for cleanup
    chmod 644 "$test_dir/no_read.c" 2>/dev/null || true
    
    if [ $exit_code -ne 0 ]; then
        print_result "Permission Denied Error" "PASS" "c99bin" "Correctly handled permission error" "error_handling"
    else
        print_result "Permission Denied Error" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_empty_file_error() {
    local test_dir="$TEST_TEMP_DIR/empty_file"
    mkdir -p "$test_dir"
    
    # Create empty file
    touch "$test_dir/empty.c"
    
    # Try to compile empty file
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh empty.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        print_result "Empty File Error" "PASS" "c99bin" "Correctly handled empty file" "error_handling"
    else
        print_result "Empty File Error" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_binary_file_error() {
    local test_dir="$TEST_TEMP_DIR/binary_file"
    mkdir -p "$test_dir"
    
    # Create a binary file (not C source)
    echo -e '\x00\x01\x02\x03\x04\x05' > "$test_dir/binary.c"
    
    # Try to compile binary file
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh binary.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        print_result "Binary File Error" "PASS" "c99bin" "Correctly handled binary file" "error_handling"
    else
        print_result "Binary File Error" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

# Test unsupported feature handling
test_unsupported_feature_handling() {
    print_subheader "Unsupported Feature Handling Tests"
    
    test_inline_assembly_handling
    test_pragma_directive_handling
    test_goto_statement_handling
    test_complex_preprocessor_handling
}

test_inline_assembly_handling() {
    local test_dir="$TEST_TEMP_DIR/inline_asm"
    mkdir -p "$test_dir"
    
    create_test_file "inline_asm.c" 'int main() {
    int result;
    asm("movl $42, %0" : "=r" (result));
    return result;
}' "$test_dir"
    
    # Should fail gracefully with helpful message
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh inline_asm.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        if echo "$output" | grep -i "not supported\|unsupported\|asm" >/dev/null; then
            print_result "Inline Assembly Handling" "PASS" "c99bin" "Correctly rejected with helpful message" "error_handling"
        else
            print_result "Inline Assembly Handling" "FAIL" "c99bin" "Rejected but message unclear" "error_handling"
        fi
    else
        print_result "Inline Assembly Handling" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_pragma_directive_handling() {
    local test_dir="$TEST_TEMP_DIR/pragma"
    mkdir -p "$test_dir"
    
    create_test_file "pragma.c" '#pragma once
#pragma pack(1)

int main() {
    return 0;
}' "$test_dir"
    
    # Should fail gracefully
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh pragma.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        if echo "$output" | grep -i "not supported\|unsupported\|pragma" >/dev/null; then
            print_result "Pragma Directive Handling" "PASS" "c99bin" "Correctly rejected with helpful message" "error_handling"
        else
            print_result "Pragma Directive Handling" "FAIL" "c99bin" "Rejected but message unclear" "error_handling"
        fi
    else
        print_result "Pragma Directive Handling" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_goto_statement_handling() {
    local test_dir="$TEST_TEMP_DIR/goto"
    mkdir -p "$test_dir"
    
    create_test_file "goto.c" 'int main() {
    int x = 0;
    
    goto skip;
    x = 1;
    
skip:
    return x;
}' "$test_dir"
    
    # Should fail gracefully
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh goto.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        if echo "$output" | grep -i "not supported\|unsupported\|goto" >/dev/null; then
            print_result "Goto Statement Handling" "PASS" "c99bin" "Correctly rejected with helpful message" "error_handling"
        else
            print_result "Goto Statement Handling" "FAIL" "c99bin" "Rejected but message unclear" "error_handling"
        fi
    else
        print_result "Goto Statement Handling" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_complex_preprocessor_handling() {
    local test_dir="$TEST_TEMP_DIR/complex_preprocessor"
    mkdir -p "$test_dir"
    
    create_test_file "complex_pp.c" '#define COMPLEX_MACRO(x, y) \
    do { \
        if (x > y) { \
            printf("x is greater\n"); \
        } else { \
            printf("y is greater\n"); \
        } \
    } while(0)

#include <stdio.h>

int main() {
    COMPLEX_MACRO(5, 3);
    return 0;
}' "$test_dir"
    
    # May fail due to complex preprocessor usage
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh complex_pp.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        print_result "Complex Preprocessor Handling" "EXPECTED_FAIL" "c99bin" "Complex preprocessor not supported" "error_handling"
    else
        print_result "Complex Preprocessor Handling" "PASS" "c99bin" "Unexpectedly handled complex preprocessor" "error_handling"
    fi
}

# Test runtime error handling
test_runtime_error_handling() {
    print_subheader "Runtime Error Handling Tests"
    
    test_compilation_timeout_handling
    test_memory_limit_handling
    test_output_directory_error
}

test_compilation_timeout_handling() {
    local test_dir="$TEST_TEMP_DIR/timeout"
    mkdir -p "$test_dir"
    
    # Create a program that might cause long compilation (complex recursion)
    create_test_file "timeout_test.c" 'int deeply_recursive(int n) {
    if (n <= 0) return 0;
    return deeply_recursive(n-1) + deeply_recursive(n-2) + deeply_recursive(n-3);
}

int main() {
    return deeply_recursive(20);
}' "$test_dir"
    
    # Test with short timeout
    local output
    output=$(timeout 5 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh timeout_test.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        print_result "Compilation Timeout Handling" "PASS" "c99bin" "Correctly timed out" "error_handling"
    elif [ $exit_code -eq 0 ]; then
        print_result "Compilation Timeout Handling" "PASS" "c99bin" "Completed within timeout" "error_handling"
    else
        print_result "Compilation Timeout Handling" "FAIL" "c99bin" "Failed for other reasons" "error_handling"
    fi
}

test_memory_limit_handling() {
    local test_dir="$TEST_TEMP_DIR/memory_limit"
    mkdir -p "$test_dir"
    
    # Create a program with large arrays (might stress memory)
    create_test_file "memory_test.c" 'int main() {
    int large_array1[10000];
    int large_array2[10000];
    int large_array3[10000];
    int i;
    
    for (i = 0; i < 10000; i++) {
        large_array1[i] = i;
        large_array2[i] = i * 2;
        large_array3[i] = i * 3;
    }
    
    return large_array1[9999] + large_array2[9999] + large_array3[9999];
}' "$test_dir"
    
    # Should handle memory usage gracefully
    run_test "Memory Limit Handling" \
        "$PROJECT_ROOT/c99bin.sh memory_test.c -o memory_test" \
        60 "c99bin" 0 "error_handling"
}

test_output_directory_error() {
    local test_dir="$TEST_TEMP_DIR/output_dir"
    mkdir -p "$test_dir"
    
    create_test_file "output_test.c" 'int main() { return 0; }' "$test_dir"
    
    # Try to output to non-existent directory
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh output_test.c -o /nonexistent/directory/test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        print_result "Output Directory Error" "PASS" "c99bin" "Correctly handled invalid output path" "error_handling"
    else
        print_result "Output Directory Error" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

# Test error message quality
test_error_message_quality() {
    print_subheader "Error Message Quality Tests"
    
    test_helpful_error_messages
    test_error_location_reporting
    test_suggestion_providing
}

test_helpful_error_messages() {
    local test_dir="$TEST_TEMP_DIR/helpful_errors"
    mkdir -p "$test_dir"
    
    create_test_file "helpful_test.c" 'int main() {
    int x = 10
    return x;
}' "$test_dir"
    
    # Check if error message is helpful
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh helpful_test.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        # Check for helpful keywords in error message
        if echo "$output" | grep -i "error\|syntax\|missing\|expected" >/dev/null; then
            print_result "Helpful Error Messages" "PASS" "c99bin" "Error message contains helpful information" "error_handling"
        else
            print_result "Helpful Error Messages" "FAIL" "c99bin" "Error message not helpful" "error_handling"
        fi
    else
        print_result "Helpful Error Messages" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_error_location_reporting() {
    local test_dir="$TEST_TEMP_DIR/error_location"
    mkdir -p "$test_dir"
    
    create_test_file "location_test.c" 'int main() {
    int x = 10;
    int y = 20
    return x + y;
}' "$test_dir"
    
    # Check if error message includes location information
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh location_test.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        # Check for line number or location information
        if echo "$output" | grep -E "line [0-9]+|:[0-9]+:" >/dev/null; then
            print_result "Error Location Reporting" "PASS" "c99bin" "Error message includes location" "error_handling"
        else
            print_result "Error Location Reporting" "FAIL" "c99bin" "Error message lacks location info" "error_handling"
        fi
    else
        print_result "Error Location Reporting" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}

test_suggestion_providing() {
    local test_dir="$TEST_TEMP_DIR/suggestions"
    mkdir -p "$test_dir"
    
    # Test with unsupported feature to see if suggestions are provided
    create_test_file "suggestion_test.c" 'int main() {
    asm("nop");
    return 0;
}' "$test_dir"
    
    # Check if error message provides suggestions
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh suggestion_test.c -o test" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        # Check for suggestion keywords
        if echo "$output" | grep -i "try\|consider\|use\|instead\|alternative" >/dev/null; then
            print_result "Suggestion Providing" "PASS" "c99bin" "Error message provides suggestions" "error_handling"
        else
            print_result "Suggestion Providing" "FAIL" "c99bin" "Error message lacks suggestions" "error_handling"
        fi
    else
        print_result "Suggestion Providing" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    fi
}
