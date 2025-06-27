/**
 * evolver0_runtime_enhanced.c - 增强的Runtime实现
 * 
 * 集成libc转发系统的Runtime层，符合PRD.md轻量化设计
 * 目标：为脱离TinyCC提供完整的C语言执行环境
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// 引入增强的ASTC虚拟机
#include "../runtime/enhanced_astc_vm.h"
#include "../runtime/libc_forward.h"
#include "../runtime/astc.h"

// ASTC头部结构定义
typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t size;          // 数据大小
    uint32_t entry_point;   // 入口点
} ASTCHeader;

// ===============================================
// Runtime主入口点
// ===============================================

/**
 * Runtime的主入口点，由Loader调用
 * 参数：ASTC程序数据和大小
 */
int evolver0_runtime_main(void* program_data, size_t program_size) {
    printf("=== Enhanced Evolver0 Runtime ===\n");
    printf("Runtime: Starting enhanced ASTC Virtual Machine\n");
    printf("Runtime: Program data size: %zu bytes\n", program_size);
    printf("Runtime: libc转发系统已集成\n");

    if (!program_data || program_size == 0) {
        printf("Runtime: Error - No program data\n");
        return 1;
    }

    // 检查ASTC格式
    if (program_size >= 8 && memcmp(program_data, "ASTC", 4) == 0) {
        printf("Runtime: Valid ASTC program detected\n");
        
        // 解析ASTC头部
        ASTCHeader* header = (ASTCHeader*)program_data;
        uint32_t version = header->version;
        uint32_t data_size = header->size;
        uint32_t entry_point = header->entry_point;
        
        printf("Runtime: ASTC version: %u\n", version);
        printf("Runtime: Data size: %u bytes\n", data_size);
        printf("Runtime: Entry point: %u\n", entry_point);
        
        // 提取ASTC代码段
        uint8_t* astc_code = (uint8_t*)program_data + sizeof(ASTCHeader);
        size_t astc_code_size = program_size - sizeof(ASTCHeader);
        
        // 初始化增强ASTC虚拟机
        EnhancedASTCVM vm;
        int init_result = enhanced_astc_vm_init(&vm, astc_code, astc_code_size, NULL, 0);
        if (init_result != 0) {
            printf("Runtime: Failed to initialize enhanced ASTC VM\n");
            return 2;
        }
        
        printf("Runtime: Enhanced ASTC VM initialized successfully\n");
        printf("Runtime: Starting program execution...\n");
        
        // 启用调试模式（可选）
        enhanced_astc_vm_set_debug(&vm, false);
        
        // 执行ASTC程序
        int exit_code = enhanced_astc_vm_run(&vm);
        
        printf("Runtime: Program execution completed\n");
        printf("Runtime: Exit code: %d\n", exit_code);
        printf("Runtime: Instructions executed: %llu\n", vm.instruction_count);
        
        // 显示libc调用统计
        LibcStats stats;
        libc_get_stats(&stats);
        if (stats.total_calls > 0) {
            printf("Runtime: libc calls made: %llu\n", stats.total_calls);
            printf("  - Memory allocations: %llu\n", stats.malloc_calls);
            printf("  - File operations: %llu\n", stats.file_operations);
            printf("  - String operations: %llu\n", stats.string_operations);
        }
        
        // 清理虚拟机
        enhanced_astc_vm_cleanup(&vm);
        
        return exit_code;
    } else {
        printf("Runtime: Invalid program format (expected ASTC)\n");
        return 1;
    }
}

// ===============================================
// 简化的测试程序生成器
// ===============================================

/**
 * 生成一个简单的ASTC测试程序
 * 用于验证增强虚拟机的功能
 */
int generate_test_astc_program(const char* output_file) {
    printf("Generating test ASTC program: %s\n", output_file);
    
    FILE* fp = fopen(output_file, "wb");
    if (!fp) {
        printf("Error: Cannot create output file\n");
        return 1;
    }
    
    // 写入ASTC头部
    ASTCHeader header;
    memcpy(header.magic, "ASTC", 4);
    header.version = 1;
    header.size = 0; // 稍后更新
    header.entry_point = sizeof(ASTCHeader);
    
    fwrite(&header, sizeof(header), 1, fp);
    
    // 生成简单的ASTC程序
    // 程序功能：调用printf输出"Hello from Enhanced ASTC VM!"
    
    uint8_t program[] = {
        // 加载字符串常量地址（模拟）
        ASTC_CONST_I64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // 字符串地址
        
        // 调用printf
        ASTC_LIBC_CALL, 
        (uint8_t)(LIBC_PRINTF & 0xFF), (uint8_t)((LIBC_PRINTF >> 8) & 0xFF), // func_id
        0x01, 0x00, // arg_count = 1
        
        // 加载返回值
        ASTC_CONST_I32, 0x00, 0x00, 0x00, 0x00, // 返回值 0
        
        // 程序结束
        ASTC_RETURN
    };
    
    fwrite(program, sizeof(program), 1, fp);
    
    // 更新头部中的大小信息
    header.size = sizeof(program);
    fseek(fp, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, fp);
    
    fclose(fp);
    
    printf("Test ASTC program generated: %zu bytes\n", sizeof(ASTCHeader) + sizeof(program));
    return 0;
}

// ===============================================
// 测试和调试功能
// ===============================================

/**
 * 测试增强Runtime的功能
 */
int test_enhanced_runtime(void) {
    printf("=== Testing Enhanced Runtime ===\n");
    
    // 生成测试程序
    const char* test_file = "test_enhanced.astc";
    if (generate_test_astc_program(test_file) != 0) {
        printf("Failed to generate test program\n");
        return 1;
    }
    
    // 读取测试程序
    FILE* fp = fopen(test_file, "rb");
    if (!fp) {
        printf("Failed to open test program\n");
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    uint8_t* program_data = malloc(file_size);
    if (!program_data) {
        printf("Failed to allocate memory\n");
        fclose(fp);
        return 1;
    }
    
    fread(program_data, 1, file_size, fp);
    fclose(fp);
    
    // 执行测试程序
    printf("\nExecuting test program...\n");
    int result = evolver0_runtime_main(program_data, file_size);
    
    free(program_data);
    
    printf("\nTest completed with result: %d\n", result);
    return result;
}

// ===============================================
// 主函数（用于独立测试）
// ===============================================

#ifdef STANDALONE_TEST
int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        return test_enhanced_runtime();
    }
    
    printf("Enhanced Evolver0 Runtime\n");
    printf("Usage: %s test  - Run built-in tests\n", argv[0]);
    printf("This runtime is designed to be called by evolver0_loader\n");
    
    return 0;
}
#endif
