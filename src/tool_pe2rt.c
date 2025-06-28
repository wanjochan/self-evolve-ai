/**
 * tool_pe2rt.c - PE文件到RTME格式转换工具
 * 
 * 从PE可执行文件中提取机器码并包装为RTME格式
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// RTME文件头
typedef struct {
    char magic[4];          // "RTME" 
    uint32_t version;       // 版本号
    uint32_t size;          // 代码大小
    uint32_t entry_point;   // 入口点偏移
} RuntimeHeader;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input.exe> <output.rt>\n", argv[0]);
        printf("Convert PE executable to RTME runtime format\n");
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    // 读取PE文件
    FILE* fp = fopen(input_file, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open input file: %s\n", input_file);
        return 1;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 读取整个文件
    uint8_t* pe_data = malloc(file_size);
    if (!pe_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fp);
        return 1;
    }

    fread(pe_data, 1, file_size, fp);
    fclose(fp);

    printf("PE file size: %zu bytes\n", file_size);

    // 简化实现：直接将整个PE文件作为"机器码"
    // TODO: 实际应该解析PE格式，提取.text段
    
    // 创建RTME文件
    FILE* out_fp = fopen(output_file, "wb");
    if (!out_fp) {
        fprintf(stderr, "Error: Cannot create output file: %s\n", output_file);
        free(pe_data);
        return 1;
    }

    // 写入RTME头
    RuntimeHeader header;
    strncpy(header.magic, "RTME", 4);
    header.version = 1;
    header.size = (uint32_t)file_size;
    header.entry_point = sizeof(RuntimeHeader);

    fwrite(&header, sizeof(header), 1, out_fp);

    // 写入PE数据（作为"机器码"）
    fwrite(pe_data, 1, file_size, out_fp);

    fclose(out_fp);
    free(pe_data);

    printf("Generated RTME file: %s (%zu bytes + header)\n", output_file, file_size);
    return 0;
}
