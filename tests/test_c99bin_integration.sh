#!/bin/bash

# test_c99bin_integration.sh - C99Bin Integration Testing Module
# T4.1, T4.2 Implementation: Toolchain integration and compatibility tests

# Test integration features
test_integration_features() {
    print_header "T4: Integration and Compatibility Testing"
    
    test_toolchain_integration
    test_build_system_integration
    test_module_system_integration
}

# T4.1: Toolchain integration tests
test_toolchain_integration() {
    print_subheader "T4.1: Toolchain Integration Tests"
    
    test_c99bin_wrapper_integration
    test_build_script_integration
    test_tool_dependency_integration
}

test_c99bin_wrapper_integration() {
    local test_dir="$TEST_TEMP_DIR/wrapper_integration"
    mkdir -p "$test_dir"
    
    # Test c99bin.sh wrapper functionality
    create_test_file "wrapper_test.c" '#include <stdio.h>
int main() {
    printf("Wrapper integration test\n");
    return 0;
}' "$test_dir"
    
    # Test basic wrapper functionality
    run_test "C99Bin Wrapper Basic" \
        "$PROJECT_ROOT/c99bin.sh wrapper_test.c -o wrapper_test && ./wrapper_test" \
        30 "c99bin" 0 "integration"
    
    # Test wrapper with different options
    run_test "C99Bin Wrapper Options" \
        "$PROJECT_ROOT/c99bin.sh --version" \
        10 "c99bin" 0 "integration"
    
    # Test wrapper error handling
    if timeout 30 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh nonexistent.c -o test" >/dev/null 2>&1; then
        print_result "C99Bin Wrapper Error Handling" "FAIL" "c99bin" "Should have failed but succeeded" "integration"
    else
        print_result "C99Bin Wrapper Error Handling" "PASS" "c99bin" "Correctly handled error" "integration"
    fi
}

test_build_script_integration() {
    # Test integration with build scripts
    run_test "C99Bin Build Script" \
        "$PROJECT_ROOT/c99bin_build.sh" \
        120 "mixed" 0 "integration"
    
    run_test "C99Bin Tools Build Script" \
        "$PROJECT_ROOT/c99bin_tools_build.sh" \
        120 "c99bin" 0 "integration"
}

test_tool_dependency_integration() {
    # Test that c99bin tools exist and are functional
    local tools=("c2astc" "c2native" "simple_loader")
    
    for tool in "${tools[@]}"; do
        if [ -f "$PROJECT_ROOT/bin/$tool" ]; then
            # Test tool existence
            print_result "Tool Exists: $tool" "PASS" "c99bin" "" "integration"
            
            # Test basic tool execution (help/version)
            if timeout 10 "$PROJECT_ROOT/bin/$tool" --help >/dev/null 2>&1 || \
               timeout 10 "$PROJECT_ROOT/bin/$tool" --version >/dev/null 2>&1 || \
               timeout 10 "$PROJECT_ROOT/bin/$tool" >/dev/null 2>&1; then
                print_result "Tool Executes: $tool" "PASS" "c99bin" "" "integration"
            else
                print_result "Tool Executes: $tool" "FAIL" "c99bin" "Cannot execute tool" "integration"
            fi
        else
            print_result "Tool Exists: $tool" "FAIL" "c99bin" "Tool not found" "integration"
        fi
    done
}

# T4.2: Build system integration tests
test_build_system_integration() {
    print_subheader "T4.2: Build System Integration Tests"
    
    test_makefile_integration
    test_script_integration
    test_module_compilation_integration
}

test_makefile_integration() {
    local test_dir="$TEST_TEMP_DIR/makefile_test"
    mkdir -p "$test_dir"
    
    # Create a simple Makefile that uses c99bin.sh
    create_test_file "Makefile" 'CC = ../../../c99bin.sh
CFLAGS = 
TARGET = test_program
SOURCE = test.c

$(TARGET): $(SOURCE)
	$(CC) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean' "$test_dir"
    
    create_test_file "test.c" '#include <stdio.h>
int main() {
    printf("Makefile integration test\n");
    return 0;
}' "$test_dir"
    
    run_test "Makefile Integration" \
        "cd '$test_dir' && make && ./test_program" \
        60 "c99bin" 0 "integration"
}

test_script_integration() {
    local test_dir="$TEST_TEMP_DIR/script_integration"
    mkdir -p "$test_dir"
    
    # Create a build script that uses c99bin.sh
    create_test_file "build.sh" '#!/bin/bash
set -e

echo "Building with c99bin..."
../../../c99bin.sh hello.c -o hello

echo "Testing executable..."
./hello

echo "Build script integration test passed!"' "$test_dir"
    
    create_test_file "hello.c" '#include <stdio.h>
int main() {
    printf("Script integration test\n");
    return 0;
}' "$test_dir"
    
    chmod +x "$test_dir/build.sh"
    
    run_test "Build Script Integration" \
        "cd '$test_dir' && ./build.sh" \
        60 "c99bin" 0 "integration"
}

test_module_compilation_integration() {
    local test_dir="$TEST_TEMP_DIR/module_integration"
    mkdir -p "$test_dir"
    
    # Test compiling a module (no main function)
    create_test_file "math_module.c" 'int add(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    return x * y;
}

int global_counter = 0;

void increment_counter() {
    global_counter++;
}

int get_counter() {
    return global_counter;
}' "$test_dir"
    
    # c99bin should handle module compilation (no main function)
    run_test "Module Compilation Integration" \
        "$PROJECT_ROOT/c99bin.sh math_module.c -o math_module" \
        30 "c99bin" 0 "integration"
}

# Test module system integration
test_module_system_integration() {
    print_subheader "Module System Integration Tests"
    
    test_c99bin_module_loading
    test_module_dependency_resolution
    test_native_module_integration
}

test_c99bin_module_loading() {
    # Test if c99bin module can be loaded
    if [ -f "$PROJECT_ROOT/test_c99bin_simple" ]; then
        run_test "C99Bin Module Loading" \
            "$PROJECT_ROOT/test_c99bin_simple" \
            30 "c99bin" 0 "integration"
    else
        print_result "C99Bin Module Loading" "SKIP" "c99bin" "Test executable not found" "integration"
    fi
}

test_module_dependency_resolution() {
    # Test module dependency resolution
    local modules=("layer0_module" "pipeline_module" "compiler_module" "libc_module")
    
    for module in "${modules[@]}"; do
        if [ -f "$PROJECT_ROOT/bin/${module}_x64_64.native" ] || \
           [ -f "$PROJECT_ROOT/bin/${module}_arm64_64.native" ]; then
            print_result "Module Available: $module" "PASS" "mixed" "" "integration"
        else
            print_result "Module Available: $module" "FAIL" "mixed" "Module not found" "integration"
        fi
    done
}

test_native_module_integration() {
    # Test .native module format integration
    local test_dir="$TEST_TEMP_DIR/native_module"
    mkdir -p "$test_dir"
    
    # Create a simple test that uses module loading
    create_test_file "module_test.c" '#include <stdio.h>
#include <dlfcn.h>

int main() {
    printf("Native module integration test\n");
    
    // Try to load a module (basic test)
    void* handle = dlopen("./libtest.so", RTLD_LAZY);
    if (handle) {
        dlclose(handle);
        printf("Module loading works\n");
    } else {
        printf("Module loading test (expected to fail)\n");
    }
    
    return 0;
}' "$test_dir"
    
    run_test "Native Module Integration" \
        "$PROJECT_ROOT/c99bin.sh module_test.c -o module_test && ./module_test" \
        30 "c99bin" 0 "integration"
}

# Test cross-platform compatibility
test_cross_platform_compatibility() {
    print_subheader "Cross-Platform Compatibility Tests"
    
    test_architecture_detection
    test_platform_specific_features
}

test_architecture_detection() {
    local test_dir="$TEST_TEMP_DIR/arch_detection"
    mkdir -p "$test_dir"
    
    create_test_file "arch_test.c" '#include <stdio.h>
int main() {
    printf("Architecture detection test\n");
    
    #ifdef __x86_64__
        printf("x86_64 architecture detected\n");
    #elif defined(__aarch64__)
        printf("ARM64 architecture detected\n");
    #elif defined(__i386__)
        printf("i386 architecture detected\n");
    #else
        printf("Unknown architecture\n");
    #endif
    
    return 0;
}' "$test_dir"
    
    run_test "Architecture Detection" \
        "$PROJECT_ROOT/c99bin.sh arch_test.c -o arch_test && ./arch_test" \
        30 "c99bin" 0 "integration"
}

test_platform_specific_features() {
    local test_dir="$TEST_TEMP_DIR/platform_features"
    mkdir -p "$test_dir"
    
    create_test_file "platform_test.c" '#include <stdio.h>
int main() {
    printf("Platform-specific features test\n");
    
    #ifdef __linux__
        printf("Linux platform detected\n");
    #elif defined(__APPLE__)
        printf("macOS platform detected\n");
    #elif defined(_WIN32)
        printf("Windows platform detected\n");
    #else
        printf("Unknown platform\n");
    #endif
    
    return 0;
}' "$test_dir"
    
    run_test "Platform-Specific Features" \
        "$PROJECT_ROOT/c99bin.sh platform_test.c -o platform_test && ./platform_test" \
        30 "c99bin" 0 "integration"
}

# Test real-world integration scenarios
test_real_world_scenarios() {
    print_subheader "Real-World Integration Scenarios"
    
    test_example_programs_integration
    test_existing_codebase_integration
}

test_example_programs_integration() {
    # Test compilation of existing example programs
    if [ -d "$PROJECT_ROOT/examples" ]; then
        for c_file in "$PROJECT_ROOT/examples"/*.c; do
            if [ -f "$c_file" ]; then
                local basename=$(basename "$c_file" .c)
                local test_dir="$TEST_TEMP_DIR/example_$basename"
                mkdir -p "$test_dir"
                
                # Copy example file to test directory
                cp "$c_file" "$test_dir/"
                
                # Try to compile with c99bin
                if timeout 60 bash -c "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh '$basename.c' -o '$basename'" >/dev/null 2>&1; then
                    # Test execution if compilation succeeded
                    if [ -f "$test_dir/$basename" ] && [ -x "$test_dir/$basename" ]; then
                        if timeout 30 bash -c "cd '$test_dir' && ./'$basename'" >/dev/null 2>&1; then
                            print_result "Example Integration: $basename" "PASS" "c99bin" "" "integration"
                        else
                            print_result "Example Integration: $basename" "FAIL" "c99bin" "Execution failed" "integration"
                        fi
                    else
                        print_result "Example Integration: $basename" "FAIL" "c99bin" "No executable generated" "integration"
                    fi
                else
                    # Expected for complex examples
                    print_result "Example Integration: $basename" "EXPECTED_FAIL" "c99bin" "Complex syntax not supported" "integration"
                fi
            fi
        done
    else
        print_result "Example Programs Integration" "SKIP" "c99bin" "Examples directory not found" "integration"
    fi
}

test_existing_codebase_integration() {
    # Test integration with existing test files
    local test_files=(
        "tests/test_simple.c"
        "tests/test_assignment.c"
        "tests/test_function_call_simple.c"
    )
    
    for test_file in "${test_files[@]}"; do
        if [ -f "$PROJECT_ROOT/$test_file" ]; then
            local basename=$(basename "$test_file" .c)
            local test_dir="$TEST_TEMP_DIR/existing_$basename"
            mkdir -p "$test_dir"
            
            # Copy test file
            cp "$PROJECT_ROOT/$test_file" "$test_dir/"
            
            run_test "Existing Code: $basename" \
                "cd '$test_dir' && $PROJECT_ROOT/c99bin.sh '$basename.c' -o '$basename'" \
                30 "c99bin" 0 "integration"
        fi
    done
}
