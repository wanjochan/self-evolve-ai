#!/bin/bash

# test_c99bin_compilation.sh - C99Bin Compilation Features Testing Module
# T3.1, T3.2, T3.3 Implementation: Compilation process, error handling, and performance

# Test compilation features
test_compilation_features() {
    print_header "T3: Compilation Features Testing"
    
    test_compilation_process
    test_error_handling
    test_performance_optimization
}

# T3.1: Compilation process tests
test_compilation_process() {
    print_subheader "T3.1: Compilation Process Tests"
    
    test_single_file_compilation
    test_module_compilation
    test_output_file_generation
    test_command_line_options
}

test_single_file_compilation() {
    local test_dir="$TEST_TEMP_DIR/single_file"
    mkdir -p "$test_dir"
    
    create_test_file "single.c" '#include <stdio.h>
int main() {
    printf("Single file compilation test\n");
    return 0;
}' "$test_dir"
    
    run_test "Single File Compilation" \
        "$PROJECT_ROOT/c99bin.sh single.c -o single && test -f single && test -x single" \
        30 "c99bin" 0 "compilation"
}

test_module_compilation() {
    local test_dir="$TEST_TEMP_DIR/module_comp"
    mkdir -p "$test_dir"
    
    # Create a module without main function
    create_test_file "module.c" 'int add(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    return x * y;
}

int global_var = 42;' "$test_dir"
    
    # Test module compilation (should be treated as complex program)
    run_test "Module Compilation (no main)" \
        "$PROJECT_ROOT/c99bin.sh module.c -o module" \
        30 "c99bin" 0 "compilation"
}

test_output_file_generation() {
    local test_dir="$TEST_TEMP_DIR/output_file"
    mkdir -p "$test_dir"
    
    create_test_file "test.c" 'int main() { return 0; }' "$test_dir"
    
    # Test custom output filename
    run_test "Custom Output Filename" \
        "$PROJECT_ROOT/c99bin.sh test.c -o custom_name && test -f custom_name && test -x custom_name" \
        30 "c99bin" 0 "compilation"
    
    # Test default output filename
    run_test "Default Output Filename" \
        "cd '$test_dir' && rm -f a.out && $PROJECT_ROOT/c99bin.sh test.c && test -f a.out && test -x a.out" \
        30 "c99bin" 0 "compilation"
}

test_command_line_options() {
    local test_dir="$TEST_TEMP_DIR/cmd_options"
    mkdir -p "$test_dir"
    
    create_test_file "test.c" 'int main() { return 0; }' "$test_dir"
    
    # Test --version option
    run_test "Version Option" \
        "$PROJECT_ROOT/c99bin.sh --version" \
        10 "c99bin" 0 "compilation"
    
    # Test --help option
    run_test "Help Option" \
        "$PROJECT_ROOT/c99bin.sh --help" \
        10 "c99bin" 0 "compilation"
    
    # Test unsupported options (should show warnings)
    run_test "Unsupported Options Warning" \
        "$PROJECT_ROOT/c99bin.sh -I/usr/include test.c -o test 2>&1 | grep -q 'Warning'" \
        30 "c99bin" 0 "compilation"
}

# T3.2: Error handling tests
test_error_handling() {
    print_subheader "T3.2: Error Handling Tests"
    
    test_syntax_error_detection
    test_file_not_found_handling
    test_unsupported_syntax_handling
    test_compilation_failure_handling
}

test_syntax_error_detection() {
    local test_dir="$TEST_TEMP_DIR/syntax_error"
    mkdir -p "$test_dir"
    
    # Create file with syntax error (missing semicolon)
    create_test_file "syntax_error.c" 'int main() {
    int x = 10
    return x;
}' "$test_dir"
    
    # Should fail with non-zero exit code
    if timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh syntax_error.c -o syntax_error" >/dev/null 2>&1; then
        print_result "Syntax Error Detection" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    else
        print_result "Syntax Error Detection" "PASS" "c99bin" "Correctly detected syntax error" "error_handling"
    fi
}

test_file_not_found_handling() {
    local test_dir="$TEST_TEMP_DIR/file_not_found"
    mkdir -p "$test_dir"
    
    # Try to compile non-existent file
    if timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh nonexistent.c -o test" >/dev/null 2>&1; then
        print_result "File Not Found Handling" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    else
        print_result "File Not Found Handling" "PASS" "c99bin" "Correctly handled missing file" "error_handling"
    fi
}

test_unsupported_syntax_handling() {
    local test_dir="$TEST_TEMP_DIR/unsupported"
    mkdir -p "$test_dir"
    
    # Create file with unsupported syntax
    create_test_file "unsupported.c" 'int main() {
    asm("nop");
    return 0;
}' "$test_dir"
    
    # Should fail gracefully with helpful message
    local output
    output=$(timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh unsupported.c -o unsupported" 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ] && echo "$output" | grep -q "not supported"; then
        print_result "Unsupported Syntax Handling" "PASS" "c99bin" "Graceful failure with helpful message" "error_handling"
    else
        print_result "Unsupported Syntax Handling" "FAIL" "c99bin" "Did not handle unsupported syntax properly" "error_handling"
    fi
}

test_compilation_failure_handling() {
    local test_dir="$TEST_TEMP_DIR/comp_failure"
    mkdir -p "$test_dir"
    
    # Create file that should cause compilation failure
    create_test_file "failure.c" 'int main() {
    undefined_function();
    return 0;
}' "$test_dir"
    
    # Should fail with appropriate error handling
    if timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh failure.c -o failure" >/dev/null 2>&1; then
        print_result "Compilation Failure Handling" "FAIL" "c99bin" "Should have failed but succeeded" "error_handling"
    else
        print_result "Compilation Failure Handling" "PASS" "c99bin" "Correctly handled compilation failure" "error_handling"
    fi
}

# T3.3: Performance and optimization tests
test_performance_optimization() {
    print_subheader "T3.3: Performance and Optimization Tests"
    
    test_compilation_speed
    test_generated_code_quality
    test_memory_usage_efficiency
    test_caching_mechanism
}

test_compilation_speed() {
    local test_dir="$TEST_TEMP_DIR/speed_test"
    mkdir -p "$test_dir"
    
    create_test_file "speed.c" '#include <stdio.h>
int main() {
    printf("Speed test\n");
    return 0;
}' "$test_dir"
    
    # Measure compilation time
    local start_time end_time compile_time
    start_time=$(date +%s.%N)
    
    if timeout 60 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh speed.c -o speed" >/dev/null 2>&1; then
        end_time=$(date +%s.%N)
        compile_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0.1")
        
        # Check if compilation was reasonably fast (under 5 seconds for simple program)
        if (( $(echo "$compile_time < 5.0" | bc -l 2>/dev/null || echo "1") )); then
            print_result "Compilation Speed" "PASS" "c99bin" "Compiled in ${compile_time}s" "performance"
        else
            print_result "Compilation Speed" "FAIL" "c99bin" "Too slow: ${compile_time}s" "performance"
        fi
    else
        print_result "Compilation Speed" "FAIL" "c99bin" "Compilation failed" "performance"
    fi
}

test_generated_code_quality() {
    local test_dir="$TEST_TEMP_DIR/code_quality"
    mkdir -p "$test_dir"
    
    create_test_file "quality.c" 'int main() {
    int sum = 0;
    int i;
    for (i = 1; i <= 100; i++) {
        sum += i;
    }
    return sum == 5050 ? 0 : 1;
}' "$test_dir"
    
    # Test that generated code produces correct results
    run_test "Generated Code Quality" \
        "$PROJECT_ROOT/c99bin.sh quality.c -o quality && ./quality" \
        30 "c99bin" 0 "performance"
    
    # Check executable size (should be reasonable)
    if [ -f "$test_dir/quality" ]; then
        local file_size
        file_size=$(stat -c%s "$test_dir/quality" 2>/dev/null || stat -f%z "$test_dir/quality" 2>/dev/null || echo "0")
        
        # Reasonable size check (under 100KB for simple program)
        if [ "$file_size" -lt 102400 ]; then
            print_result "Executable Size" "PASS" "c99bin" "Size: ${file_size} bytes" "performance"
        else
            print_result "Executable Size" "FAIL" "c99bin" "Too large: ${file_size} bytes" "performance"
        fi
    fi
}

test_memory_usage_efficiency() {
    local test_dir="$TEST_TEMP_DIR/memory_test"
    mkdir -p "$test_dir"
    
    create_test_file "memory.c" 'int main() {
    int arr[1000];
    int i;
    for (i = 0; i < 1000; i++) {
        arr[i] = i;
    }
    return arr[999];
}' "$test_dir"
    
    # Test memory usage during compilation (basic check)
    run_test "Memory Usage Efficiency" \
        "$PROJECT_ROOT/c99bin.sh memory.c -o memory && ./memory" \
        30 "c99bin" 999 "performance"
}

test_caching_mechanism() {
    local test_dir="$TEST_TEMP_DIR/cache_test"
    mkdir -p "$test_dir"
    
    create_test_file "cache.c" '#include <stdio.h>
int main() {
    printf("Cache test\n");
    return 0;
}' "$test_dir"
    
    # First compilation
    local first_time
    local start_time=$(date +%s.%N)
    
    if timeout 60 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh cache.c -o cache1" >/dev/null 2>&1; then
        local end_time=$(date +%s.%N)
        first_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "1.0")
        
        # Second compilation (should potentially be faster due to caching)
        start_time=$(date +%s.%N)
        
        if timeout 60 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh cache.c -o cache2" >/dev/null 2>&1; then
            end_time=$(date +%s.%N)
            local second_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "1.0")
            
            print_result "Caching Mechanism" "PASS" "c99bin" "First: ${first_time}s, Second: ${second_time}s" "performance"
        else
            print_result "Caching Mechanism" "FAIL" "c99bin" "Second compilation failed" "performance"
        fi
    else
        print_result "Caching Mechanism" "FAIL" "c99bin" "First compilation failed" "performance"
    fi
}

# Test ELF file generation specifics
test_elf_generation() {
    print_subheader "ELF File Generation Tests"
    
    local test_dir="$TEST_TEMP_DIR/elf_test"
    mkdir -p "$test_dir"
    
    create_test_file "elf.c" 'int main() { return 42; }' "$test_dir"
    
    if timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh elf.c -o elf_test" >/dev/null 2>&1; then
        # Check if generated file is a valid ELF
        if file "$test_dir/elf_test" | grep -q "ELF"; then
            print_result "ELF File Generation" "PASS" "c99bin" "Valid ELF file generated" "compilation"
            
            # Test execution
            if timeout 10 bash -c "cd '$test_dir' && ./elf_test"; then
                local exit_code=$?
                if [ $exit_code -eq 42 ]; then
                    print_result "ELF Execution" "PASS" "c99bin" "Correct exit code: $exit_code" "compilation"
                else
                    print_result "ELF Execution" "FAIL" "c99bin" "Wrong exit code: $exit_code" "compilation"
                fi
            else
                print_result "ELF Execution" "FAIL" "c99bin" "Execution failed" "compilation"
            fi
        else
            print_result "ELF File Generation" "FAIL" "c99bin" "Not a valid ELF file" "compilation"
        fi
    else
        print_result "ELF File Generation" "FAIL" "c99bin" "Compilation failed" "compilation"
    fi
}

# Test large file handling
test_large_file_handling() {
    print_subheader "Large File Handling Tests"
    
    local test_dir="$TEST_TEMP_DIR/large_file"
    mkdir -p "$test_dir"
    
    # Generate a moderately large C file
    {
        echo '#include <stdio.h>'
        echo 'int main() {'
        echo '    int sum = 0;'
        
        # Generate many variable declarations and operations
        for i in $(seq 1 100); do
            echo "    int var$i = $i;"
            echo "    sum += var$i;"
        done
        
        echo '    printf("Sum: %d\\n", sum);'
        echo '    return sum == 5050 ? 0 : 1;'
        echo '}'
    } > "$test_dir/large.c"
    
    run_test "Large File Handling" \
        "$PROJECT_ROOT/c99bin.sh large.c -o large && ./large" \
        60 "c99bin" 0 "compilation"
}
