/**
 * c2astc_ultra_minimal.c - 超简化的C到ASTC编译器
 * 专门设计为可以被c99bin正确编译的版本
 * 只支持最基本的C程序：int main() { return N; }
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 超简化的ASTC文件生成
int create_astc_file(const char* output_file, int return_value) {
    FILE* f = fopen(output_file, "wb");
    if (!f) {
        printf("Error: Cannot create %s\n", output_file);
        return 1;
    }
    
    // ASTC头部 (20字节)
    fwrite("ASTC", 4, 1, f);           // 魔数
    int version = 1;
    fwrite(&version, 4, 1, f);        // 版本
    int flags = 0;
    fwrite(&flags, 4, 1, f);          // 标志
    int entry = 0;
    fwrite(&entry, 4, 1, f);          // 入口点
    int source_size = 0;
    fwrite(&source_size, 4, 1, f);    // 源码大小
    
    // ASTC指令 (8字节)
    unsigned char instr[8] = {
        0x41, (unsigned char)(return_value & 0xFF), 0x00, 0x00,  // i32.const return_value
        0x0F, 0x00, 0x00, 0x00   // return
    };
    fwrite(instr, 8, 1, f);
    
    fclose(f);
    printf("Generated ASTC file: %s (28 bytes)\n", output_file);
    return 0;
}

// 超简化的C源码解析
int parse_return_value(const char* source_file) {
    FILE* f = fopen(source_file, "r");
    if (!f) {
        printf("Error: Cannot read %s\n", source_file);
        return -1;
    }
    
    char line[256];
    int return_value = 0;
    
    // 查找 "return N;" 模式
    while (fgets(line, sizeof(line), f)) {
        char* return_pos = strstr(line, "return");
        if (return_pos) {
            // 简单解析返回值
            char* num_start = return_pos + 6; // 跳过 "return"
            while (*num_start == ' ' || *num_start == '\t') num_start++;
            
            if (*num_start >= '0' && *num_start <= '9') {
                return_value = atoi(num_start);
                break;
            }
        }
    }
    
    fclose(f);
    return return_value;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input.c> <output.astc>\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    
    printf("Ultra-minimal C2ASTC compiler\n");
    printf("Input: %s\n", input_file);
    printf("Output: %s\n", output_file);
    
    // 解析返回值
    int return_value = parse_return_value(input_file);
    if (return_value < 0) {
        printf("Error: Failed to parse source file\n");
        return 1;
    }
    
    printf("Detected return value: %d\n", return_value);
    
    // 生成ASTC文件
    if (create_astc_file(output_file, return_value) != 0) {
        return 1;
    }
    
    printf("Compilation successful!\n");
    return 0;
}
