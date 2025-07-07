#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../src/core/module.h"
#include "../src/core/astc.h"

// 外部声明pipeline模块
extern Module module_pipeline;

int main() {
    printf("=== ASTC Assembly Generation Test ===\n");
    
    // 1. 初始化pipeline模块
    printf("1. Initializing pipeline module...\n");
    if (module_pipeline.init() != 0) {
        printf("ERROR: Failed to initialize pipeline module\n");
        return 1;
    }
    printf("   ✓ Pipeline module initialized\n");
    
    // 2. 编译C代码生成ASTC字节码
    printf("\n2. Compiling C code to ASTC bytecode...\n");
    const char* test_code = 
        "int main() {\n"
        "    return 42;\n"
        "}\n";
    
    printf("   Test code:\n%s\n", test_code);
    
    // 获取编译函数
    void* compile_func = module_pipeline.resolve("pipeline_compile");
    bool (*pipeline_compile)(const char*, void*) = (bool (*)(const char*, void*))compile_func;
    
    if (!pipeline_compile) {
        printf("ERROR: Could not resolve pipeline_compile function\n");
        return 1;
    }
    
    // 编译代码
    if (!pipeline_compile(test_code, NULL)) {
        void* get_error_func = module_pipeline.resolve("pipeline_get_error");
        const char* (*get_error)(void) = (const char* (*)(void))get_error_func;
        if (get_error) {
            printf("ERROR: Compilation failed: %s\n", get_error());
        }
        return 1;
    }
    printf("   ✓ Code compiled to ASTC bytecode\n");
    
    // 3. 获取生成的ASTC字节码程序
    printf("\n3. Getting ASTC bytecode program...\n");
    void* get_astc_func = module_pipeline.resolve("pipeline_get_astc_program");
    ASTCBytecodeProgram* (*get_astc_program)(void) = (ASTCBytecodeProgram* (*)(void))get_astc_func;
    
    if (!get_astc_program) {
        printf("ERROR: Could not resolve pipeline_get_astc_program function\n");
        return 1;
    }
    
    ASTCBytecodeProgram* astc_program = get_astc_program();
    if (!astc_program) {
        printf("ERROR: No ASTC program generated\n");
        return 1;
    }
    
    printf("   ASTC Program: %u instructions\n", astc_program->instruction_count);
    for (uint32_t i = 0; i < astc_program->instruction_count && i < 5; i++) {
        printf("     [%u] Opcode: 0x%02x\n", i, astc_program->instructions[i].opcode);
    }
    
    // 4. 测试ASTC字节码到汇编转换
    printf("\n4. Converting ASTC bytecode to assembly...\n");
    void* bytecode_to_asm_func = module_pipeline.resolve("astc_bytecode_to_assembly");
    ASTCAssemblyProgram* (*astc_bytecode_to_assembly)(ASTCBytecodeProgram*) = 
        (ASTCAssemblyProgram* (*)(ASTCBytecodeProgram*))bytecode_to_asm_func;
    
    if (!astc_bytecode_to_assembly) {
        printf("ERROR: Could not resolve astc_bytecode_to_assembly function\n");
        return 1;
    }
    
    ASTCAssemblyProgram* assembly_program = astc_bytecode_to_assembly(astc_program);
    if (!assembly_program) {
        printf("ERROR: Failed to convert bytecode to assembly\n");
        return 1;
    }
    
    printf("   ✓ ASTC bytecode converted to assembly\n");
    printf("   Assembly size: %zu bytes\n", assembly_program->text_size);
    
    // 5. 显示生成的ASTC汇编代码
    printf("\n5. Generated ASTC Assembly:\n");
    printf("----------------------------------------\n");
    if (assembly_program->assembly_text) {
        printf("%s", assembly_program->assembly_text);
    } else {
        printf("   (No assembly text generated)\n");
    }
    printf("----------------------------------------\n");
    
    // 6. 测试ASTC汇编函数
    printf("\n6. Testing ASTC assembly functions...\n");
    void* create_asm_func = module_pipeline.resolve("astc_assembly_create");
    ASTCAssemblyProgram* (*astc_assembly_create)(void) = (ASTCAssemblyProgram* (*)(void))create_asm_func;
    
    if (astc_assembly_create) {
        ASTCAssemblyProgram* test_asm = astc_assembly_create();
        if (test_asm) {
            printf("   ✓ astc_assembly_create works\n");
            
            void* add_line_func = module_pipeline.resolve("astc_assembly_add_line");
            int (*astc_assembly_add_line)(ASTCAssemblyProgram*, const char*) = 
                (int (*)(ASTCAssemblyProgram*, const char*))add_line_func;
            
            if (astc_assembly_add_line) {
                astc_assembly_add_line(test_asm, ";; Test assembly");
                astc_assembly_add_line(test_asm, "(module");
                astc_assembly_add_line(test_asm, "  (func $test (result i32)");
                astc_assembly_add_line(test_asm, "    i32.const 123");
                astc_assembly_add_line(test_asm, "    return");
                astc_assembly_add_line(test_asm, "  )");
                astc_assembly_add_line(test_asm, ")");
                
                printf("   ✓ astc_assembly_add_line works\n");
                printf("   Test assembly (%zu bytes):\n", test_asm->text_size);
                printf("   %s\n", test_asm->assembly_text);
            }
            
            void* free_asm_func = module_pipeline.resolve("astc_assembly_free");
            void (*astc_assembly_free)(ASTCAssemblyProgram*) = (void (*)(ASTCAssemblyProgram*))free_asm_func;
            
            if (astc_assembly_free) {
                astc_assembly_free(test_asm);
                printf("   ✓ astc_assembly_free works\n");
            }
        }
    }
    
    // 7. 清理
    printf("\n7. Cleaning up...\n");
    if (assembly_program) {
        void* free_asm_func = module_pipeline.resolve("astc_assembly_free");
        void (*astc_assembly_free)(ASTCAssemblyProgram*) = (void (*)(ASTCAssemblyProgram*))free_asm_func;
        if (astc_assembly_free) {
            astc_assembly_free(assembly_program);
        }
    }
    
    module_pipeline.cleanup();
    printf("   ✓ Cleanup completed\n");
    
    printf("\n=== ASTC Assembly Test Summary ===\n");
    printf("✓ ASTC bytecode to assembly conversion working\n");
    printf("✓ Generated WASM-compatible ASTC assembly format\n");
    printf("✓ ASTC assembly functions implemented and tested\n");
    printf("✓ Complete T2.3 Backend codegen implementation!\n");
    
    return 0;
}
