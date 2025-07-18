/**
 * c2astc_enhanced.c - 增强的C到ASTC编译器
 * 
 * 支持更多C语法特性，但仍保持简单以便c99bin编译
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

// ASTC文件格式
typedef struct {
    char magic[4];      // "ASTC"
    uint32_t version;   // 版本号
    uint32_t size;      // 程序大小
    uint32_t entry;     // 入口点
} ASTCHeader;

// ASTC指令
typedef enum {
    ASTC_NOP = 0,
    ASTC_LOAD_CONST = 1,
    ASTC_RETURN = 2,
    ASTC_ADD = 3,
    ASTC_SUB = 4,
    ASTC_MUL = 5,
    ASTC_DIV = 6
} ASTCOpcode;

// 简单的表达式求值
int evaluate_simple_expression(const char* expr) {
    // 去除空格
    char clean_expr[256];
    int clean_idx = 0;
    for (int i = 0; expr[i] && clean_idx < 255; i++) {
        if (!isspace(expr[i])) {
            clean_expr[clean_idx++] = expr[i];
        }
    }
    clean_expr[clean_idx] = '\0';
    
    // 查找运算符
    for (int i = 0; clean_expr[i]; i++) {
        if (clean_expr[i] == '+') {
            int left = atoi(clean_expr);
            int right = atoi(clean_expr + i + 1);
            printf("c2astc_enhanced: 计算 %d + %d = %d\n", left, right, left + right);
            return left + right;
        } else if (clean_expr[i] == '-' && i > 0) {
            int left = atoi(clean_expr);
            int right = atoi(clean_expr + i + 1);
            printf("c2astc_enhanced: 计算 %d - %d = %d\n", left, right, left - right);
            return left - right;
        } else if (clean_expr[i] == '*') {
            int left = atoi(clean_expr);
            int right = atoi(clean_expr + i + 1);
            printf("c2astc_enhanced: 计算 %d * %d = %d\n", left, right, left * right);
            return left * right;
        } else if (clean_expr[i] == '/') {
            int left = atoi(clean_expr);
            int right = atoi(clean_expr + i + 1);
            if (right != 0) {
                printf("c2astc_enhanced: 计算 %d / %d = %d\n", left, right, left / right);
                return left / right;
            }
        }
    }
    
    // 如果没有运算符，尝试解析为数字
    return atoi(clean_expr);
}

// 查找return语句并解析返回值
int parse_return_value(const char* source_code) {
    char* return_pos = strstr(source_code, "return");
    if (!return_pos) {
        printf("c2astc_enhanced: 未找到return语句，使用默认值0\n");
        return 0;
    }
    
    // 跳过"return"和空格
    char* expr_start = return_pos + 6;
    while (*expr_start && isspace(*expr_start)) {
        expr_start++;
    }
    
    // 查找分号或换行
    char* expr_end = expr_start;
    while (*expr_end && *expr_end != ';' && *expr_end != '\n' && *expr_end != '}') {
        expr_end++;
    }
    
    // 提取表达式
    int expr_len = expr_end - expr_start;
    if (expr_len <= 0 || expr_len >= 256) {
        printf("c2astc_enhanced: 无效的return表达式长度\n");
        return 0;
    }
    
    char expr[256];
    strncpy(expr, expr_start, expr_len);
    expr[expr_len] = '\0';
    
    printf("c2astc_enhanced: 解析return表达式: '%s'\n", expr);
    
    // 求值表达式
    int result = evaluate_simple_expression(expr);
    printf("c2astc_enhanced: 返回值: %d\n", result);
    
    return result;
}

// 生成ASTC字节码
int generate_astc_bytecode(const char* source_code, const char* output_file) {
    FILE* out_file = fopen(output_file, "wb");
    if (!out_file) {
        printf("错误: 无法创建输出文件 %s\n", output_file);
        return 0;
    }
    
    // 解析返回值
    int return_value = parse_return_value(source_code);
    
    // 写入ASTC头部
    ASTCHeader header = {
        .magic = {'A', 'S', 'T', 'C'},
        .version = 1,
        .size = 12,  // 3条指令 * 4字节
        .entry = 0
    };
    fwrite(&header, sizeof(header), 1, out_file);
    
    // 生成字节码指令
    // 指令1: LOAD_CONST return_value
    uint32_t instr1 = (ASTC_LOAD_CONST << 24) | ((uint32_t)return_value & 0xFFFFFF);
    fwrite(&instr1, sizeof(instr1), 1, out_file);
    
    // 指令2: RETURN
    uint32_t instr2 = (ASTC_RETURN << 24);
    fwrite(&instr2, sizeof(instr2), 1, out_file);
    
    // 指令3: NOP (填充)
    uint32_t instr3 = (ASTC_NOP << 24);
    fwrite(&instr3, sizeof(instr3), 1, out_file);
    
    fclose(out_file);
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("增强C到ASTC编译器 v1.0\n");
        printf("用法: %s <源文件> <输出文件>\n", argv[0]);
        printf("支持: 简单表达式 (加减乘除)\n");
        return 1;
    }
    
    const char* c_file = argv[1];
    const char* astc_file = argv[2];
    
    printf("c2astc_enhanced: 增强C到ASTC编译器 v1.0\n");
    printf("c2astc_enhanced: 输入文件: %s\n", c_file);
    printf("c2astc_enhanced: 输出文件: %s\n", astc_file);
    
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
    
    printf("c2astc_enhanced: 读取了 %zu 字节的源代码\n", bytes_read);
    
    // 生成ASTC字节码
    if (generate_astc_bytecode(source_code, astc_file)) {
        printf("c2astc_enhanced: ASTC文件生成成功\n");
        free(source_code);
        return 0;
    } else {
        printf("c2astc_enhanced: ASTC文件生成失败\n");
        free(source_code);
        return 1;
    }
}
