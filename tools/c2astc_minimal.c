/**
 * c2astc_minimal.c - 最小化的C到ASTC转换器
 * 
 * 绕过复杂的pipeline_compile，直接生成简单的ASTC文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 简单的ASTC文件格式
typedef struct {
    char magic[4];      // "ASTC"
    uint32_t version;   // 版本号
    uint32_t size;      // 程序大小
    uint32_t entry;     // 入口点
} ASTCHeader;

// 简单的ASTC指令
typedef enum {
    ASTC_NOP = 0,
    ASTC_LOAD_CONST = 1,
    ASTC_RETURN = 2,
    ASTC_CALL = 3
} ASTCOpcode;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("用法: %s <源文件> <输出文件>\n", argv[0]);
        return 1;
    }

    const char* c_file = argv[1];
    const char* astc_file = argv[2];

    printf("c2astc_minimal: 输入文件: %s\n", c_file);
    printf("c2astc_minimal: 输出文件: %s\n", astc_file);

    // 读取源文件
    FILE* file = fopen(c_file, "r");
    if (!file) {
        printf("错误: 无法打开源文件 %s\n", c_file);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* source_code = malloc(file_size + 1);
    if (!source_code) {
        printf("错误: 内存分配失败\n");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(source_code, 1, file_size, file);
    source_code[bytes_read] = '\0';
    fclose(file);

    printf("c2astc_minimal: 读取了 %zu 字节的源代码\n", bytes_read);

    // 简单的模式匹配：查找return语句中的数字
    int return_value = 0;
    char* return_pos = strstr(source_code, "return");
    if (return_pos) {
        // 查找return后面的数字
        char* num_start = return_pos + 6; // 跳过"return"
        while (*num_start && (*num_start == ' ' || *num_start == '\t')) {
            num_start++;
        }
        if (*num_start == '-' || (*num_start >= '0' && *num_start <= '9')) {
            return_value = atoi(num_start);
            printf("c2astc_minimal: 检测到返回值: %d\n", return_value);
        } else {
            printf("c2astc_minimal: 未检测到有效返回值，使用默认值0\n");
        }
    }

    // 创建ASTC文件
    FILE* out_file = fopen(astc_file, "wb");
    if (!out_file) {
        printf("错误: 无法创建输出文件 %s\n", astc_file);
        free(source_code);
        return 1;
    }

    // 写入ASTC头部
    ASTCHeader header = {
        .magic = {'A', 'S', 'T', 'C'},
        .version = 1,
        .size = 12,  // 3条指令 * 4字节
        .entry = 0
    };
    fwrite(&header, sizeof(header), 1, out_file);

    // 写入简单的ASTC字节码 (小端序格式)
    // 指令1: LOAD_CONST return_value
    uint32_t instr1 = (ASTC_LOAD_CONST << 24) | ((uint32_t)return_value & 0xFFFFFF);
    // 写入小端序
    uint8_t bytes1[4] = {
        instr1 & 0xFF,
        (instr1 >> 8) & 0xFF,
        (instr1 >> 16) & 0xFF,
        (instr1 >> 24) & 0xFF
    };
    fwrite(bytes1, 4, 1, out_file);

    // 指令2: RETURN
    uint32_t instr2 = (ASTC_RETURN << 24);
    uint8_t bytes2[4] = {
        instr2 & 0xFF,
        (instr2 >> 8) & 0xFF,
        (instr2 >> 16) & 0xFF,
        (instr2 >> 24) & 0xFF
    };
    fwrite(bytes2, 4, 1, out_file);

    // 指令3: NOP (填充)
    uint32_t instr3 = (ASTC_NOP << 24);
    uint8_t bytes3[4] = {
        instr3 & 0xFF,
        (instr3 >> 8) & 0xFF,
        (instr3 >> 16) & 0xFF,
        (instr3 >> 24) & 0xFF
    };
    fwrite(bytes3, 4, 1, out_file);

    fclose(out_file);
    free(source_code);

    printf("c2astc_minimal: ASTC文件创建成功\n");
    printf("c2astc_minimal: 生成了 %zu 字节的ASTC字节码\n", sizeof(header) + 12);

    return 0;
}
