/**
 * debug_runtime.c - Runtime执行调试工具
 * 
 * 用于调试Runtime执行问题
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 简化的ASTC头部结构
typedef struct {
    uint32_t magic;      // "ASTC"
    uint32_t version;    // 版本号
    uint32_t data_size;  // 数据大小
    uint32_t entry_point; // 入口点
} ASTCHeader;

// 简化的Runtime头部结构
typedef struct {
    uint32_t magic;      // "RTME"
    uint32_t version;    // 版本号
    uint32_t size;       // 代码大小
    uint32_t entry_offset; // 入口点偏移
} RuntimeHeader;

void dump_astc_file(const char* filename) {
    printf("=== 分析ASTC文件: %s ===\n", filename);
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("错误: 无法打开文件\n");
        return;
    }
    
    // 读取文件大小
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    printf("文件大小: %zu 字节\n", file_size);
    
    if (file_size < sizeof(ASTCHeader)) {
        printf("错误: 文件太小，不是有效的ASTC文件\n");
        fclose(file);
        return;
    }
    
    // 读取ASTC头部
    ASTCHeader header;
    fread(&header, sizeof(header), 1, file);
    
    printf("Magic: 0x%08X (%c%c%c%c)\n", header.magic,
           (char)(header.magic & 0xFF),
           (char)((header.magic >> 8) & 0xFF),
           (char)((header.magic >> 16) & 0xFF),
           (char)((header.magic >> 24) & 0xFF));
    printf("Version: %u\n", header.version);
    printf("Data Size: %u\n", header.data_size);
    printf("Entry Point: %u\n", header.entry_point);
    
    // 读取前32字节的字节码
    printf("\n前32字节的字节码:\n");
    uint8_t bytecode[32];
    size_t read_size = (file_size - sizeof(header)) < 32 ? (file_size - sizeof(header)) : 32;
    fread(bytecode, 1, read_size, file);
    
    for (size_t i = 0; i < read_size; i++) {
        printf("%02X ", bytecode[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
    
    fclose(file);
}

void dump_runtime_file(const char* filename) {
    printf("\n=== 分析Runtime文件: %s ===\n", filename);
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("错误: 无法打开文件\n");
        return;
    }
    
    // 读取文件大小
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    printf("文件大小: %zu 字节\n", file_size);
    
    if (file_size < sizeof(RuntimeHeader)) {
        printf("错误: 文件太小，不是有效的Runtime文件\n");
        fclose(file);
        return;
    }
    
    // 读取Runtime头部
    RuntimeHeader header;
    fread(&header, sizeof(header), 1, file);
    
    printf("Magic: 0x%08X (%c%c%c%c)\n", header.magic,
           (char)(header.magic & 0xFF),
           (char)((header.magic >> 8) & 0xFF),
           (char)((header.magic >> 16) & 0xFF),
           (char)((header.magic >> 24) & 0xFF));
    printf("Version: %u\n", header.version);
    printf("Size: %u\n", header.size);
    printf("Entry Offset: %u\n", header.entry_offset);
    
    // 读取前32字节的机器码
    printf("\n前32字节的机器码:\n");
    uint8_t machine_code[32];
    size_t read_size = (file_size - sizeof(header)) < 32 ? (file_size - sizeof(header)) : 32;
    fread(machine_code, 1, read_size, file);
    
    for (size_t i = 0; i < read_size; i++) {
        printf("%02X ", machine_code[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
    
    fclose(file);
}

int main(int argc, char* argv[]) {
    printf("Runtime执行调试工具\n");
    printf("==================\n\n");
    
    if (argc < 2) {
        printf("用法: %s <astc_file> [runtime_file]\n", argv[0]);
        printf("示例: %s tests/minimal.astc bin/c99_runtime_x64_64.rt\n", argv[0]);
        return 1;
    }
    
    // 分析ASTC文件
    dump_astc_file(argv[1]);
    
    // 分析Runtime文件（如果提供）
    if (argc > 2) {
        dump_runtime_file(argv[2]);
    }
    
    printf("\n=== 调试建议 ===\n");
    printf("1. 检查ASTC文件的Magic是否为'ASTC'\n");
    printf("2. 检查Runtime文件的Magic是否为'RTME'\n");
    printf("3. 检查字节码是否合理\n");
    printf("4. 检查机器码是否正确生成\n");
    
    return 0;
}
