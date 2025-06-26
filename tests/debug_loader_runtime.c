/**
 * debug_loader_runtime.c - 调试Loader和Runtime的基本功能
 * 
 * 这个测试程序用于验证三层架构的基本工作流程
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 简化的ASTC头部结构
typedef struct {
    char magic[4];          // "ASTC"
    uint32_t version;       // 版本号
    uint32_t size;          // 数据大小
    uint32_t entry_point;   // 入口点
} ASTCHeader;

// 简化的Runtime头部结构
typedef struct {
    char magic[4];          // "RTME" 
    uint32_t version;       // 版本号
    uint32_t size;          // 代码大小
    uint32_t entry_point;   // 入口点偏移
} RuntimeHeader;

int main() {
    printf("=== Loader和Runtime调试测试 ===\n");
    
    // 测试1: 检查Runtime.bin文件格式
    printf("\n测试1: 检查Runtime.bin文件格式\n");
    FILE* runtime_file = fopen("evolver0_runtime.bin", "rb");
    if (!runtime_file) {
        printf("❌ 无法打开evolver0_runtime.bin\n");
        return 1;
    }
    
    RuntimeHeader runtime_header;
    if (fread(&runtime_header, sizeof(RuntimeHeader), 1, runtime_file) != 1) {
        printf("❌ 无法读取Runtime头部\n");
        fclose(runtime_file);
        return 1;
    }
    
    printf("Runtime Magic: %.4s\n", runtime_header.magic);
    printf("Runtime Version: %u\n", runtime_header.version);
    printf("Runtime Size: %u bytes\n", runtime_header.size);
    printf("Runtime Entry: %u\n", runtime_header.entry_point);
    
    if (memcmp(runtime_header.magic, "RTME", 4) == 0) {
        printf("✅ Runtime格式正确\n");
    } else {
        printf("❌ Runtime格式错误\n");
        fclose(runtime_file);
        return 1;
    }
    fclose(runtime_file);
    
    // 测试2: 检查Program.astc文件格式
    printf("\n测试2: 检查Program.astc文件格式\n");
    FILE* program_file = fopen("evolver0_program.astc", "rb");
    if (!program_file) {
        printf("❌ 无法打开evolver0_program.astc\n");
        return 1;
    }
    
    ASTCHeader astc_header;
    if (fread(&astc_header, sizeof(ASTCHeader), 1, program_file) != 1) {
        printf("❌ 无法读取ASTC头部\n");
        fclose(program_file);
        return 1;
    }
    
    printf("ASTC Magic: %.4s\n", astc_header.magic);
    printf("ASTC Version: %u\n", astc_header.version);
    printf("ASTC Size: %u bytes\n", astc_header.size);
    printf("ASTC Entry: %u\n", astc_header.entry_point);
    
    if (memcmp(astc_header.magic, "ASTC", 4) == 0) {
        printf("✅ ASTC格式正确\n");
    } else {
        printf("❌ ASTC格式错误\n");
        fclose(program_file);
        return 1;
    }
    fclose(program_file);
    
    // 测试3: 检查Loader可执行文件
    printf("\n测试3: 检查Loader可执行文件\n");
    FILE* loader_file = fopen("evolver0_loader.exe", "rb");
    if (!loader_file) {
        printf("❌ 无法打开evolver0_loader.exe\n");
        return 1;
    }
    
    // 检查PE头部
    char pe_header[2];
    if (fread(pe_header, 2, 1, loader_file) == 1) {
        if (memcmp(pe_header, "MZ", 2) == 0) {
            printf("✅ Loader是有效的PE可执行文件\n");
        } else {
            printf("❌ Loader不是有效的PE文件\n");
        }
    }
    fclose(loader_file);
    
    printf("\n=== 基本文件格式检查完成 ===\n");
    printf("建议：如果所有格式都正确，问题可能在于：\n");
    printf("1. Loader的参数解析逻辑\n");
    printf("2. Runtime机器码执行失败\n");
    printf("3. ASTC虚拟机无法正确解析Program数据\n");
    
    return 0;
}
