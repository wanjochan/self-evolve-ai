#!/bin/bash

# test_c99bin_syntax.sh - C99Bin Syntax Support Testing Module
# T2.1, T2.2, T2.3 Implementation: Basic C syntax, control flow, and data structures

# Test basic C syntax support
test_syntax_support() {
    print_header "T2: C Syntax Support Testing"
    
    test_basic_c_syntax
    test_control_flow_syntax
    test_data_structure_syntax
    test_unsupported_syntax
}

# T2.1: Basic C syntax tests
test_basic_c_syntax() {
    print_subheader "T2.1: Basic C Syntax Tests"
    
    # T2.1.1: Simple C programs
    test_simple_hello_world
    test_simple_return_value
    test_simple_variables
    test_simple_expressions
    
    # T2.1.2: Variable and expression tests
    test_variable_declarations
    test_arithmetic_expressions
    test_assignment_operations
    
    # T2.1.3: Function definition and calls
    test_function_definitions
    test_function_calls
    test_recursive_functions
}

test_simple_hello_world() {
    local test_dir="$TEST_TEMP_DIR/simple_hello"
    mkdir -p "$test_dir"
    
    create_test_file "hello.c" '#include <stdio.h>
int main() {
    printf("Hello World!\n");
    return 0;
}' "$test_dir"
    
    run_test "Simple Hello World" \
        "$PROJECT_ROOT/c99bin.sh hello.c -o hello && ./hello" \
        30 "c99bin" 0 "syntax"
}

test_simple_return_value() {
    local test_dir="$TEST_TEMP_DIR/simple_return"
    mkdir -p "$test_dir"
    
    create_test_file "return42.c" 'int main() {
    return 42;
}' "$test_dir"
    
    run_test "Simple Return Value" \
        "$PROJECT_ROOT/c99bin.sh return42.c -o return42 && ./return42" \
        30 "c99bin" 42 "syntax"
}

test_simple_variables() {
    local test_dir="$TEST_TEMP_DIR/simple_vars"
    mkdir -p "$test_dir"
    
    create_test_file "vars.c" 'int main() {
    int a = 10;
    int b = 20;
    int sum = a + b;
    return sum;
}' "$test_dir"
    
    run_test "Simple Variables" \
        "$PROJECT_ROOT/c99bin.sh vars.c -o vars && ./vars" \
        30 "c99bin" 30 "syntax"
}

test_simple_expressions() {
    local test_dir="$TEST_TEMP_DIR/simple_expr"
    mkdir -p "$test_dir"
    
    create_test_file "expr.c" 'int main() {
    int result = (5 + 3) * 2 - 1;
    return result;
}' "$test_dir"
    
    run_test "Simple Expressions" \
        "$PROJECT_ROOT/c99bin.sh expr.c -o expr && ./expr" \
        30 "c99bin" 15 "syntax"
}

test_variable_declarations() {
    local test_dir="$TEST_TEMP_DIR/var_decl"
    mkdir -p "$test_dir"
    
    create_test_file "vardecl.c" 'int main() {
    int x;
    int y = 5;
    char c = '\''A'\'';
    float f = 3.14;
    x = y + 1;
    return x;
}' "$test_dir"
    
    run_test "Variable Declarations" \
        "$PROJECT_ROOT/c99bin.sh vardecl.c -o vardecl && ./vardecl" \
        30 "c99bin" 6 "syntax"
}

test_arithmetic_expressions() {
    local test_dir="$TEST_TEMP_DIR/arithmetic"
    mkdir -p "$test_dir"
    
    create_test_file "arith.c" 'int main() {
    int a = 10;
    int b = 3;
    int add = a + b;
    int sub = a - b;
    int mul = a * b;
    int div = a / b;
    int mod = a % b;
    return add + sub + mul + div + mod;
}' "$test_dir"
    
    run_test "Arithmetic Expressions" \
        "$PROJECT_ROOT/c99bin.sh arith.c -o arith && ./arith" \
        30 "c99bin" 49 "syntax"
}

test_assignment_operations() {
    local test_dir="$TEST_TEMP_DIR/assignment"
    mkdir -p "$test_dir"
    
    create_test_file "assign.c" 'int main() {
    int x = 5;
    x += 3;  // x = 8
    x -= 2;  // x = 6
    x *= 2;  // x = 12
    x /= 3;  // x = 4
    return x;
}' "$test_dir"
    
    run_test "Assignment Operations" \
        "$PROJECT_ROOT/c99bin.sh assign.c -o assign && ./assign" \
        30 "c99bin" 4 "syntax"
}

test_function_definitions() {
    local test_dir="$TEST_TEMP_DIR/func_def"
    mkdir -p "$test_dir"
    
    create_test_file "funcdef.c" 'int add(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    return x * y;
}

int main() {
    int sum = add(5, 3);
    int product = multiply(4, 2);
    return sum + product;
}' "$test_dir"
    
    run_test "Function Definitions" \
        "$PROJECT_ROOT/c99bin.sh funcdef.c -o funcdef && ./funcdef" \
        30 "c99bin" 16 "syntax"
}

test_function_calls() {
    local test_dir="$TEST_TEMP_DIR/func_call"
    mkdir -p "$test_dir"
    
    create_test_file "funccall.c" 'int calculate(int a, int b, int c) {
    return a + b * c;
}

int main() {
    int result1 = calculate(1, 2, 3);  // 1 + 2*3 = 7
    int result2 = calculate(2, 3, 4);  // 2 + 3*4 = 14
    return result1 + result2;          // 7 + 14 = 21
}' "$test_dir"
    
    run_test "Function Calls" \
        "$PROJECT_ROOT/c99bin.sh funccall.c -o funccall && ./funccall" \
        30 "c99bin" 21 "syntax"
}

test_recursive_functions() {
    local test_dir="$TEST_TEMP_DIR/recursive"
    mkdir -p "$test_dir"
    
    create_test_file "recursive.c" 'int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    return factorial(4);  // 4! = 24
}' "$test_dir"
    
    run_test "Recursive Functions" \
        "$PROJECT_ROOT/c99bin.sh recursive.c -o recursive && ./recursive" \
        30 "c99bin" 24 "syntax"
}

# T2.2: Control flow syntax tests
test_control_flow_syntax() {
    print_subheader "T2.2: Control Flow Syntax Tests"
    
    test_if_else_statements
    test_for_loops
    test_while_loops
    test_nested_control_flow
}

test_if_else_statements() {
    local test_dir="$TEST_TEMP_DIR/if_else"
    mkdir -p "$test_dir"
    
    create_test_file "ifelse.c" 'int main() {
    int x = 10;
    int result = 0;
    
    if (x > 5) {
        result = 1;
    } else {
        result = 0;
    }
    
    if (x == 10) {
        result += 10;
    }
    
    return result;  // Should be 11
}' "$test_dir"
    
    run_test "If-Else Statements" \
        "$PROJECT_ROOT/c99bin.sh ifelse.c -o ifelse && ./ifelse" \
        30 "c99bin" 11 "syntax"
}

test_for_loops() {
    local test_dir="$TEST_TEMP_DIR/for_loop"
    mkdir -p "$test_dir"
    
    create_test_file "forloop.c" 'int main() {
    int sum = 0;
    int i;
    
    for (i = 1; i <= 5; i++) {
        sum += i;
    }
    
    return sum;  // 1+2+3+4+5 = 15
}' "$test_dir"
    
    run_test "For Loops" \
        "$PROJECT_ROOT/c99bin.sh forloop.c -o forloop && ./forloop" \
        30 "c99bin" 15 "syntax"
}

test_while_loops() {
    local test_dir="$TEST_TEMP_DIR/while_loop"
    mkdir -p "$test_dir"
    
    create_test_file "whileloop.c" 'int main() {
    int sum = 0;
    int i = 1;
    
    while (i <= 4) {
        sum += i;
        i++;
    }
    
    return sum;  // 1+2+3+4 = 10
}' "$test_dir"
    
    run_test "While Loops" \
        "$PROJECT_ROOT/c99bin.sh whileloop.c -o whileloop && ./whileloop" \
        30 "c99bin" 10 "syntax"
}

test_nested_control_flow() {
    local test_dir="$TEST_TEMP_DIR/nested_control"
    mkdir -p "$test_dir"
    
    create_test_file "nested.c" 'int main() {
    int result = 0;
    int i, j;
    
    for (i = 1; i <= 3; i++) {
        if (i % 2 == 1) {
            for (j = 1; j <= 2; j++) {
                result += i * j;
            }
        }
    }
    
    return result;  // (1*1 + 1*2) + (3*1 + 3*2) = 3 + 9 = 12
}' "$test_dir"
    
    run_test "Nested Control Flow" \
        "$PROJECT_ROOT/c99bin.sh nested.c -o nested && ./nested" \
        30 "c99bin" 12 "syntax"
}

# T2.3: Data structure syntax tests
test_data_structure_syntax() {
    print_subheader "T2.3: Data Structure Syntax Tests"
    
    test_arrays
    test_pointers
    test_structures
}

test_arrays() {
    local test_dir="$TEST_TEMP_DIR/arrays"
    mkdir -p "$test_dir"
    
    create_test_file "arrays.c" 'int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int sum = 0;
    int i;
    
    for (i = 0; i < 5; i++) {
        sum += arr[i];
    }
    
    return sum;  // 1+2+3+4+5 = 15
}' "$test_dir"
    
    run_test "Array Operations" \
        "$PROJECT_ROOT/c99bin.sh arrays.c -o arrays && ./arrays" \
        30 "c99bin" 15 "syntax"
}

test_pointers() {
    local test_dir="$TEST_TEMP_DIR/pointers"
    mkdir -p "$test_dir"
    
    create_test_file "pointers.c" 'int main() {
    int x = 42;
    int *ptr = &x;
    int value = *ptr;
    
    return value;  // Should be 42
}' "$test_dir"
    
    run_test "Pointer Operations" \
        "$PROJECT_ROOT/c99bin.sh pointers.c -o pointers && ./pointers" \
        30 "c99bin" 42 "syntax"
}

test_structures() {
    local test_dir="$TEST_TEMP_DIR/structures"
    mkdir -p "$test_dir"
    
    create_test_file "struct.c" 'struct Point {
    int x;
    int y;
};

int main() {
    struct Point p;
    p.x = 10;
    p.y = 20;
    
    return p.x + p.y;  // 10 + 20 = 30
}' "$test_dir"
    
    run_test "Structure Operations" \
        "$PROJECT_ROOT/c99bin.sh struct.c -o struct && ./struct" \
        30 "c99bin" 30 "syntax"
}

# Test unsupported syntax (should fail gracefully)
test_unsupported_syntax() {
    print_subheader "Unsupported Syntax Tests (Expected Failures)"
    
    test_inline_assembly
    test_pragma_directives
    test_goto_statements
}

test_inline_assembly() {
    local test_dir="$TEST_TEMP_DIR/inline_asm"
    mkdir -p "$test_dir"
    
    create_test_file "inline_asm.c" 'int main() {
    int result;
    asm("movl $42, %0" : "=r" (result));
    return result;
}' "$test_dir"
    
    # This should fail gracefully with c99bin
    if timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh inline_asm.c -o inline_asm" >/dev/null 2>&1; then
        print_result "Inline Assembly (unsupported)" "FAIL" "c99bin" "Should have failed but succeeded" "syntax"
    else
        print_result "Inline Assembly (unsupported)" "EXPECTED_FAIL" "c99bin" "Correctly rejected unsupported syntax" "syntax"
    fi
}

test_pragma_directives() {
    local test_dir="$TEST_TEMP_DIR/pragma"
    mkdir -p "$test_dir"
    
    create_test_file "pragma.c" '#pragma once
#pragma pack(1)

int main() {
    return 0;
}' "$test_dir"
    
    # This should fail gracefully with c99bin
    if timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh pragma.c -o pragma" >/dev/null 2>&1; then
        print_result "Pragma Directives (unsupported)" "FAIL" "c99bin" "Should have failed but succeeded" "syntax"
    else
        print_result "Pragma Directives (unsupported)" "EXPECTED_FAIL" "c99bin" "Correctly rejected unsupported syntax" "syntax"
    fi
}

test_goto_statements() {
    local test_dir="$TEST_TEMP_DIR/goto"
    mkdir -p "$test_dir"
    
    create_test_file "goto.c" 'int main() {
    int x = 0;
    
    goto skip;
    x = 1;
    
skip:
    return x;
}' "$test_dir"
    
    # This should fail gracefully with c99bin
    if timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh goto.c -o goto" >/dev/null 2>&1; then
        print_result "Goto Statements (unsupported)" "FAIL" "c99bin" "Should have failed but succeeded" "syntax"
    else
        print_result "Goto Statements (unsupported)" "EXPECTED_FAIL" "c99bin" "Correctly rejected unsupported syntax" "syntax"
    fi
}
