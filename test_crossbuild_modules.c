/**
 * test_crossbuild_modules.c - Cross-compilation Modules Test
 *
 * Tests the newly developed cross-platform compilation modules:
 * - PE Generator (Windows)
 * - Mach-O Generator (macOS) 
 * - x86_32 Code Generator
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test function declarations (external modules)
extern int pe_generator_test(void);
extern int macho_generator_test(void);
extern int x86_32_codegen_test(void);

// Test results tracking
typedef struct {
    const char* module_name;
    int (*test_function)(void);
    int result;
    const char* description;
} ModuleTest;

int main(void) {
    printf("🧪 CROSS-COMPILATION MODULES TEST\n");
    printf("==================================\n");
    printf("Testing work_id=stage1crossbuild Task 1 deliverables\n\n");
    
    // Define test modules
    ModuleTest tests[] = {
        {
            "PE Generator",
            pe_generator_test,
            -1,
            "Windows PE32/PE32+ executable file format generator"
        },
        {
            "Mach-O Generator", 
            macho_generator_test,
            -1,
            "macOS Mach-O executable file format generator"
        },
        {
            "x86_32 CodeGen",
            x86_32_codegen_test,
            -1,
            "x86 32-bit assembly code generator"
        }
    };
    
    int num_tests = sizeof(tests) / sizeof(ModuleTest);
    int passed_tests = 0;
    int failed_tests = 0;
    
    // Run all tests
    for (int i = 0; i < num_tests; i++) {
        printf("🔍 Testing %s...\n", tests[i].module_name);
        printf("   Description: %s\n", tests[i].description);
        
        // Run the test
        tests[i].result = tests[i].test_function();
        
        if (tests[i].result == 0) {
            printf("   ✅ %s: PASSED\n", tests[i].module_name);
            passed_tests++;
        } else {
            printf("   ❌ %s: FAILED (code: %d)\n", tests[i].module_name, tests[i].result);
            failed_tests++;
        }
        printf("\n");
    }
    
    // Summary
    printf("📊 TEST SUMMARY\n");
    printf("===============\n");
    printf("Total Tests: %d\n", num_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", failed_tests);
    printf("Success Rate: %.1f%%\n", (float)passed_tests / num_tests * 100.0f);
    
    // Detailed results
    printf("\n📋 DETAILED RESULTS\n");
    printf("===================\n");
    
    for (int i = 0; i < num_tests; i++) {
        char status = tests[i].result == 0 ? '✅' : '❌';
        printf("%c %s: %s\n", status, tests[i].module_name, 
               tests[i].result == 0 ? "Ready for production" : "Needs fixes");
    }
    
    // Cross-platform capability assessment
    printf("\n🌍 CROSS-PLATFORM CAPABILITY ASSESSMENT\n");
    printf("========================================\n");
    
    int pe_ready = 0, macho_ready = 0, x86_ready = 0;
    
    for (int i = 0; i < num_tests; i++) {
        if (tests[i].result == 0) {
            if (strcmp(tests[i].module_name, "PE Generator") == 0) pe_ready = 1;
            if (strcmp(tests[i].module_name, "Mach-O Generator") == 0) macho_ready = 1;
            if (strcmp(tests[i].module_name, "x86_32 CodeGen") == 0) x86_ready = 1;
        }
    }
    
    printf("Target Platform Support:\n");
    printf("  🖥️  Windows x64: %s (PE Generator: %s)\n", 
           pe_ready ? "✅ Ready" : "❌ Not Ready",
           pe_ready ? "Working" : "Failed");
           
    printf("  🖥️  Windows x86: %s (PE + x86_32: %s)\n", 
           (pe_ready && x86_ready) ? "✅ Ready" : "❌ Not Ready",
           (pe_ready && x86_ready) ? "Working" : "Incomplete");
           
    printf("  🍎 macOS ARM64: %s (Mach-O Generator: %s)\n", 
           macho_ready ? "✅ Ready" : "❌ Not Ready",
           macho_ready ? "Working" : "Failed");
           
    printf("  🐧 Linux x64: ✅ Ready (Already supported)\n");
    
    // Task 1 completion assessment
    printf("\n🎯 TASK 1 COMPLETION ASSESSMENT\n");
    printf("==============================\n");
    printf("work_id=stage1crossbuild Task 1: C99Bin交叉编译器扩展\n");
    
    float completion_percentage = (float)passed_tests / num_tests * 100.0f;
    
    if (completion_percentage >= 100.0f) {
        printf("🎉 STATUS: TASK 1 COMPLETE (100%%)\n");
        printf("✅ All cross-compilation modules working\n");
        printf("✅ Windows PE file format support ready\n");
        printf("✅ macOS Mach-O file format support ready\n");
        printf("✅ x86_32 architecture support ready\n");
        printf("\n🚀 READY FOR TASK 2: Layer 1跨平台Simple Loader\n");
    } else if (completion_percentage >= 66.0f) {
        printf("🔄 STATUS: TASK 1 MOSTLY COMPLETE (%.0f%%)\n", completion_percentage);
        printf("⚠️  Some modules need fixes before proceeding\n");
        printf("🔧 Recommended: Fix failing modules\n");
    } else {
        printf("🚨 STATUS: TASK 1 INCOMPLETE (%.0f%%)\n", completion_percentage);
        printf("❌ Multiple critical failures\n");
        printf("🛠️  Recommended: Debug and fix all modules\n");
    }
    
    // File generation verification
    printf("\n📁 GENERATED FILES VERIFICATION\n");
    printf("===============================\n");
    
    const char* expected_files[] = {
        "test_pe_output.exe",
        "test_macho_output", 
        "test_x86_32_output.s"
    };
    
    for (int i = 0; i < 3; i++) {
        FILE* file = fopen(expected_files[i], "r");
        if (file) {
            fclose(file);
            printf("✅ %s: Generated successfully\n", expected_files[i]);
        } else {
            printf("❌ %s: Not generated\n", expected_files[i]);
        }
    }
    
    // Next steps recommendation
    printf("\n📋 NEXT STEPS RECOMMENDATION\n");
    printf("============================\n");
    
    if (passed_tests == num_tests) {
        printf("🎯 All Task 1 modules working perfectly!\n");
        printf("🚀 Proceed to Task 2: Layer 1跨平台构建\n");
        printf("   - Build Windows版Simple Loader\n");
        printf("   - Build macOS版Simple Loader\n");
        printf("   - Implement统一跨平台检测逻辑\n");
    } else {
        printf("🔧 Fix failing modules before proceeding:\n");
        for (int i = 0; i < num_tests; i++) {
            if (tests[i].result != 0) {
                printf("   - Debug and fix %s\n", tests[i].module_name);
            }
        }
    }
    
    // Final status
    if (passed_tests == num_tests) {
        printf("\n🏆 WORK_ID=STAGE1CROSSBUILD TASK 1: SUCCESS!\n");
        printf("Cross-compilation foundation ready for multi-platform support!\n");
        return 0;
    } else {
        printf("\n⚠️  WORK_ID=STAGE1CROSSBUILD TASK 1: NEEDS WORK\n");
        printf("Some modules require debugging before full deployment.\n");
        return 1;
    }
}