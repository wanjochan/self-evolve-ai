#!/bin/bash

# test_c99bin_functionality.sh - C99Bin Functionality Testing Module
# Core functionality tests for c99bin compiler features

# Test core functionality features
test_functionality_features() {
    print_header "T4: Core Functionality Testing"
    
    test_program_types
    test_printf_handling
    test_return_value_handling
    test_variable_operations
    test_mathematical_operations
}

# Test different program types that c99bin should handle
test_program_types() {
    print_subheader "Program Type Classification Tests"
    
    test_hello_world_programs
    test_return_value_programs
    test_mathematical_programs
    test_complex_programs
}

test_hello_world_programs() {
    local test_dir="$TEST_TEMP_DIR/hello_programs"
    mkdir -p "$test_dir"
    
    # Basic Hello World
    create_test_file "hello1.c" '#include <stdio.h>
int main() {
    printf("Hello World!\n");
    return 0;
}' "$test_dir"
    
    run_test "Basic Hello World" \
        "$PROJECT_ROOT/c99bin.sh hello1.c -o hello1 && ./hello1" \
        30 "c99bin" 0 "functionality"
    
    # Hello World with variables
    create_test_file "hello2.c" '#include <stdio.h>
int main() {
    char *message = "Hello from C99Bin!";
    printf("%s\n", message);
    return 0;
}' "$test_dir"
    
    run_test "Hello World with Variables" \
        "$PROJECT_ROOT/c99bin.sh hello2.c -o hello2 && ./hello2" \
        30 "c99bin" 0 "functionality"
    
    # Multiple printf statements
    create_test_file "hello3.c" '#include <stdio.h>
int main() {
    printf("Line 1\n");
    printf("Line 2\n");
    printf("Line 3\n");
    return 0;
}' "$test_dir"
    
    run_test "Multiple Printf Statements" \
        "$PROJECT_ROOT/c99bin.sh hello3.c -o hello3 && ./hello3" \
        30 "c99bin" 0 "functionality"
}

test_return_value_programs() {
    local test_dir="$TEST_TEMP_DIR/return_programs"
    mkdir -p "$test_dir"
    
    # Simple return value
    create_test_file "return1.c" 'int main() {
    return 42;
}' "$test_dir"
    
    run_test "Simple Return Value" \
        "$PROJECT_ROOT/c99bin.sh return1.c -o return1 && ./return1" \
        30 "c99bin" 42 "functionality"
    
    # Calculated return value
    create_test_file "return2.c" 'int main() {
    int a = 10;
    int b = 5;
    return a + b;
}' "$test_dir"
    
    run_test "Calculated Return Value" \
        "$PROJECT_ROOT/c99bin.sh return2.c -o return2 && ./return2" \
        30 "c99bin" 15 "functionality"
    
    # Conditional return value
    create_test_file "return3.c" 'int main() {
    int x = 20;
    if (x > 10) {
        return 1;
    } else {
        return 0;
    }
}' "$test_dir"
    
    run_test "Conditional Return Value" \
        "$PROJECT_ROOT/c99bin.sh return3.c -o return3 && ./return3" \
        30 "c99bin" 1 "functionality"
}

test_mathematical_programs() {
    local test_dir="$TEST_TEMP_DIR/math_programs"
    mkdir -p "$test_dir"
    
    # Basic arithmetic
    create_test_file "math1.c" 'int main() {
    int result = 5 + 3 * 2 - 1;
    return result;
}' "$test_dir"
    
    run_test "Basic Arithmetic" \
        "$PROJECT_ROOT/c99bin.sh math1.c -o math1 && ./math1" \
        30 "c99bin" 10 "functionality"
    
    # Division and modulo
    create_test_file "math2.c" 'int main() {
    int a = 17;
    int b = 5;
    int div = a / b;
    int mod = a % b;
    return div + mod;
}' "$test_dir"
    
    run_test "Division and Modulo" \
        "$PROJECT_ROOT/c99bin.sh math2.c -o math2 && ./math2" \
        30 "c99bin" 5 "functionality"
    
    # Mathematical functions
    create_test_file "math3.c" 'int square(int x) {
    return x * x;
}

int main() {
    int result = square(4) + square(3);
    return result;
}' "$test_dir"
    
    run_test "Mathematical Functions" \
        "$PROJECT_ROOT/c99bin.sh math3.c -o math3 && ./math3" \
        30 "c99bin" 25 "functionality"
}

test_complex_programs() {
    local test_dir="$TEST_TEMP_DIR/complex_programs"
    mkdir -p "$test_dir"
    
    # Program with loops and conditions
    create_test_file "complex1.c" 'int main() {
    int sum = 0;
    int i;
    
    for (i = 1; i <= 10; i++) {
        if (i % 2 == 0) {
            sum += i;
        }
    }
    
    return sum;  // 2+4+6+8+10 = 30
}' "$test_dir"
    
    run_test "Loops and Conditions" \
        "$PROJECT_ROOT/c99bin.sh complex1.c -o complex1 && ./complex1" \
        30 "c99bin" 30 "functionality"
    
    # Program with multiple functions
    create_test_file "complex2.c" 'int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

int main() {
    return fibonacci(6);  // Should be 8
}' "$test_dir"
    
    run_test "Multiple Functions (Fibonacci)" \
        "$PROJECT_ROOT/c99bin.sh complex2.c -o complex2 && ./complex2" \
        30 "c99bin" 8 "functionality"
}

# Test printf handling capabilities
test_printf_handling() {
    print_subheader "Printf Handling Tests"
    
    test_simple_printf
    test_formatted_printf
    test_multiple_printf
}

test_simple_printf() {
    local test_dir="$TEST_TEMP_DIR/printf_simple"
    mkdir -p "$test_dir"
    
    create_test_file "printf1.c" '#include <stdio.h>
int main() {
    printf("Simple message");
    return 0;
}' "$test_dir"
    
    run_test "Simple Printf" \
        "$PROJECT_ROOT/c99bin.sh printf1.c -o printf1 && ./printf1" \
        30 "c99bin" 0 "functionality"
}

test_formatted_printf() {
    local test_dir="$TEST_TEMP_DIR/printf_formatted"
    mkdir -p "$test_dir"
    
    create_test_file "printf2.c" '#include <stdio.h>
int main() {
    int x = 42;
    printf("The answer is %d\n", x);
    return 0;
}' "$test_dir"
    
    run_test "Formatted Printf" \
        "$PROJECT_ROOT/c99bin.sh printf2.c -o printf2 && ./printf2" \
        30 "c99bin" 0 "functionality"
}

test_multiple_printf() {
    local test_dir="$TEST_TEMP_DIR/printf_multiple"
    mkdir -p "$test_dir"
    
    create_test_file "printf3.c" '#include <stdio.h>
int main() {
    printf("First line\n");
    printf("Second line\n");
    printf("Third line\n");
    return 0;
}' "$test_dir"
    
    run_test "Multiple Printf" \
        "$PROJECT_ROOT/c99bin.sh printf3.c -o printf3 && ./printf3" \
        30 "c99bin" 0 "functionality"
}

# Test return value handling
test_return_value_handling() {
    print_subheader "Return Value Handling Tests"
    
    test_zero_return
    test_positive_return
    test_negative_return
    test_calculated_return
}

test_zero_return() {
    local test_dir="$TEST_TEMP_DIR/return_zero"
    mkdir -p "$test_dir"
    
    create_test_file "zero.c" 'int main() {
    return 0;
}' "$test_dir"
    
    run_test "Zero Return Value" \
        "$PROJECT_ROOT/c99bin.sh zero.c -o zero && ./zero" \
        30 "c99bin" 0 "functionality"
}

test_positive_return() {
    local test_dir="$TEST_TEMP_DIR/return_positive"
    mkdir -p "$test_dir"
    
    create_test_file "positive.c" 'int main() {
    return 123;
}' "$test_dir"
    
    run_test "Positive Return Value" \
        "$PROJECT_ROOT/c99bin.sh positive.c -o positive && ./positive" \
        30 "c99bin" 123 "functionality"
}

test_negative_return() {
    local test_dir="$TEST_TEMP_DIR/return_negative"
    mkdir -p "$test_dir"
    
    create_test_file "negative.c" 'int main() {
    return -1;
}' "$test_dir"
    
    run_test "Negative Return Value" \
        "$PROJECT_ROOT/c99bin.sh negative.c -o negative && ./negative" \
        30 "c99bin" 255 "functionality"  # -1 becomes 255 in exit code
}

test_calculated_return() {
    local test_dir="$TEST_TEMP_DIR/return_calc"
    mkdir -p "$test_dir"
    
    create_test_file "calc.c" 'int main() {
    int a = 7;
    int b = 3;
    return a * b + 1;
}' "$test_dir"
    
    run_test "Calculated Return Value" \
        "$PROJECT_ROOT/c99bin.sh calc.c -o calc && ./calc" \
        30 "c99bin" 22 "functionality"
}

# Test variable operations
test_variable_operations() {
    print_subheader "Variable Operations Tests"
    
    test_variable_declaration
    test_variable_assignment
    test_variable_arithmetic
}

test_variable_declaration() {
    local test_dir="$TEST_TEMP_DIR/var_decl"
    mkdir -p "$test_dir"
    
    create_test_file "vardecl.c" 'int main() {
    int x;
    int y = 10;
    char c = '\''A'\'';
    x = 5;
    return x + y;
}' "$test_dir"
    
    run_test "Variable Declaration" \
        "$PROJECT_ROOT/c99bin.sh vardecl.c -o vardecl && ./vardecl" \
        30 "c99bin" 15 "functionality"
}

test_variable_assignment() {
    local test_dir="$TEST_TEMP_DIR/var_assign"
    mkdir -p "$test_dir"
    
    create_test_file "assign.c" 'int main() {
    int x = 5;
    x = x + 3;
    x = x * 2;
    return x;
}' "$test_dir"
    
    run_test "Variable Assignment" \
        "$PROJECT_ROOT/c99bin.sh assign.c -o assign && ./assign" \
        30 "c99bin" 16 "functionality"
}

test_variable_arithmetic() {
    local test_dir="$TEST_TEMP_DIR/var_arith"
    mkdir -p "$test_dir"
    
    create_test_file "arith.c" 'int main() {
    int a = 10;
    int b = 3;
    int sum = a + b;
    int diff = a - b;
    int prod = a * b;
    int quot = a / b;
    return sum + diff + prod + quot;
}' "$test_dir"
    
    run_test "Variable Arithmetic" \
        "$PROJECT_ROOT/c99bin.sh arith.c -o arith && ./arith" \
        30 "c99bin" 49 "functionality"
}

# Test mathematical operations
test_mathematical_operations() {
    print_subheader "Mathematical Operations Tests"
    
    test_basic_arithmetic
    test_operator_precedence
    test_increment_decrement
}

test_basic_arithmetic() {
    local test_dir="$TEST_TEMP_DIR/basic_math"
    mkdir -p "$test_dir"
    
    create_test_file "basic.c" 'int main() {
    int result = 2 + 3 * 4 - 1;
    return result;
}' "$test_dir"
    
    run_test "Basic Arithmetic" \
        "$PROJECT_ROOT/c99bin.sh basic.c -o basic && ./basic" \
        30 "c99bin" 13 "functionality"
}

test_operator_precedence() {
    local test_dir="$TEST_TEMP_DIR/precedence"
    mkdir -p "$test_dir"
    
    create_test_file "precedence.c" 'int main() {
    int result = 2 + 3 * 4 / 2 - 1;
    return result;
}' "$test_dir"
    
    run_test "Operator Precedence" \
        "$PROJECT_ROOT/c99bin.sh precedence.c -o precedence && ./precedence" \
        30 "c99bin" 7 "functionality"
}

test_increment_decrement() {
    local test_dir="$TEST_TEMP_DIR/inc_dec"
    mkdir -p "$test_dir"
    
    create_test_file "incdec.c" 'int main() {
    int x = 5;
    x++;
    x++;
    x--;
    return x;
}' "$test_dir"
    
    run_test "Increment/Decrement" \
        "$PROJECT_ROOT/c99bin.sh incdec.c -o incdec && ./incdec" \
        30 "c99bin" 6 "functionality"
}
