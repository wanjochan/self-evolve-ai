/**
 * Core Module Tests
 * 
 * Tests for the modular architecture based on PRD.md requirements.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 由于module.h可能不在标准路径，我们在这里定义必要的类型和函数
typedef struct module_t module_t;

// 函数声明
module_t* module_load(const char* name);
void module_unload(module_t* module);
void* module_get_symbol(module_t* module, const char* symbol_name);
module_t* module_find_loaded(const char* name);

// Test results tracking
int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) void test_##name() { \
    printf("Running test: %s\n", #name); \
    tests_run++; \
    int test_result = 1;

#define END_TEST(name) \
    if (test_result) { \
        printf("✓ Test passed: %s\n", #name); \
        tests_passed++; \
    } else { \
        printf("✗ Test failed: %s\n", #name); \
        tests_failed++; \
    } \
}

// Forward declarations of test functions
void test_module_loading();
void test_symbol_resolution();
void test_dependency_management();
void test_vm_module();
void test_memory_module();
void test_astc_module();
void test_native_module();
void test_utils_module();
void test_jit_module();
void test_c2astc_module();
void test_astc2native_module();
void test_codegen_module();
void test_module_integration();

int main() {
    printf("=== Core Module Tests ===\n");
    
    // Core module system tests
    test_module_loading();
    test_symbol_resolution();
    test_dependency_management();
    
    // Individual module tests
    test_vm_module();
    test_memory_module();
    test_astc_module();
    test_native_module();
    test_utils_module();
    test_jit_module();
    test_c2astc_module();
    test_astc2native_module();
    test_codegen_module();
    
    // Integration tests
    test_module_integration();
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}

// Test implementations

TEST(module_loading)
    // Test basic module loading
    module_t* mod = module_load("memory");
    test_result = mod != NULL;
    
    // Test loading non-existent module
    module_t* invalid_mod = module_load("nonexistent_module");
    test_result = test_result && (invalid_mod == NULL);
    
    // Cleanup
    if (mod) module_unload(mod);
END_TEST(module_loading)

TEST(symbol_resolution)
    // Load a module
    module_t* mod = module_load("memory");
    if (!mod) {
        test_result = 0;
        END_TEST(symbol_resolution);
        return;
    }
    
    // Resolve a known symbol
    void* sym = module_get_symbol(mod, "memory_alloc");
    test_result = sym != NULL;
    
    // Resolve an unknown symbol
    void* invalid_sym = module_get_symbol(mod, "nonexistent_symbol");
    test_result = test_result && (invalid_sym == NULL);
    
    // Cleanup
    module_unload(mod);
END_TEST(symbol_resolution)

TEST(dependency_management)
    // Load a module with dependencies
    module_t* mod = module_load("vm");
    if (!mod) {
        test_result = 0;
        END_TEST(dependency_management);
        return;
    }
    
    // Check if dependencies were automatically loaded
    module_t* memory_mod = module_find_loaded("memory");
    test_result = memory_mod != NULL;
    
    // Cleanup
    module_unload(mod);
END_TEST(dependency_management)

TEST(vm_module)
    module_t* mod = module_load("vm");
    test_result = mod != NULL;
    if (mod) module_unload(mod);
END_TEST(vm_module)

TEST(memory_module)
    module_t* mod = module_load("memory");
    test_result = mod != NULL;
    
    if (mod) {
        // Test memory allocation function
        void* (*memory_alloc)(size_t) = module_get_symbol(mod, "memory_alloc");
        void (*memory_free)(void*) = module_get_symbol(mod, "memory_free");
        
        if (memory_alloc && memory_free) {
            void* ptr = memory_alloc(100);
            test_result = ptr != NULL;
            memory_free(ptr);
        } else {
            test_result = 0;
        }
        
        module_unload(mod);
    }
END_TEST(memory_module)

TEST(astc_module)
    module_t* mod = module_load("astc");
    test_result = mod != NULL;
    if (mod) module_unload(mod);
END_TEST(astc_module)

TEST(native_module)
    module_t* mod = module_load("native");
    test_result = mod != NULL;
    if (mod) module_unload(mod);
END_TEST(native_module)

TEST(utils_module)
    module_t* mod = module_load("utils");
    test_result = mod != NULL;
    if (mod) module_unload(mod);
END_TEST(utils_module)

TEST(jit_module)
    module_t* mod = module_load("jit");
    test_result = mod != NULL;
    if (mod) module_unload(mod);
END_TEST(jit_module)

TEST(c2astc_module)
    module_t* mod = module_load("c2astc");
    test_result = mod != NULL;
    if (mod) module_unload(mod);
END_TEST(c2astc_module)

TEST(astc2native_module)
    module_t* mod = module_load("astc2native");
    test_result = mod != NULL;
    if (mod) module_unload(mod);
END_TEST(astc2native_module)

TEST(codegen_module)
    module_t* mod = module_load("codegen");
    test_result = mod != NULL;
    if (mod) module_unload(mod);
END_TEST(codegen_module)

TEST(module_integration)
    // Test the integration between multiple modules
    module_t* vm_mod = module_load("vm");
    module_t* astc_mod = module_load("astc");
    module_t* c2astc_mod = module_load("c2astc");
    
    test_result = (vm_mod != NULL) && (astc_mod != NULL) && (c2astc_mod != NULL);
    
    // Cleanup
    if (vm_mod) module_unload(vm_mod);
    if (astc_mod) module_unload(astc_mod);
    if (c2astc_mod) module_unload(c2astc_mod);
END_TEST(module_integration)
