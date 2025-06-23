/**
 * test2.c - 测试复杂的C语言结构转换为ASTC
 */

#include <stdio.h>
#include <stdlib.h>
#include "c2astc.h"

int main() {
    printf("测试2: 复杂C语言结构转换为ASTC\n");
    printf("================================\n\n");
    
    // 包含结构体、循环、函数定义等的C代码
    const char *test_code = 
        "// 复杂的C程序\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n\n"
        "// 结构体定义\n"
        "typedef struct {\n"
        "    int id;\n"
        "    char *name;\n"
        "    float score;\n"
        "} Student;\n\n"
        "// 函数声明\n"
        "void print_student(Student *s);\n"
        "Student *create_student(int id, const char *name, float score);\n\n"
        "// 主函数\n"
        "int main() {\n"
        "    int num_students = 3;\n"
        "    Student *students[3];\n\n"
        "    // 创建学生\n"
        "    students[0] = create_student(1, \"Alice\", 92.5);\n"
        "    students[1] = create_student(2, \"Bob\", 85.0);\n"
        "    students[2] = create_student(3, \"Charlie\", 78.5);\n\n"
        "    // 使用循环打印学生信息\n"
        "    for (int i = 0; i < num_students; i++) {\n"
        "        print_student(students[i]);\n"
        "    }\n\n"
        "    // 释放内存\n"
        "    for (int i = 0; i < num_students; i++) {\n"
        "        free(students[i]);\n"
        "    }\n\n"
        "    return 0;\n"
        "}\n\n"
        "// 函数定义\n"
        "void print_student(Student *s) {\n"
        "    printf(\"ID: %d, Name: %s, Score: %.1f\\n\", s->id, s->name, s->score);\n"
        "}\n\n"
        "Student *create_student(int id, const char *name, float score) {\n"
        "    Student *s = (Student *)malloc(sizeof(Student));\n"
        "    s->id = id;\n"
        "    s->name = strdup(name);\n"
        "    s->score = score;\n"
        "    return s;\n"
        "}\n";
    
    printf("测试代码:\n%s\n", test_code);
    
    // 使用默认选项
    C2AstcOptions options = c2astc_default_options();
    
    // 转换为ASTC
    printf("转换为ASTC...\n");
    struct ASTNode *ast = c2astc_convert(test_code, &options);
    
    if (!ast) {
        const char *error = c2astc_get_error();
        printf("转换失败: %s\n", error ? error : "未知错误");
        return 1;
    }
    
    printf("转换成功!\n\n");
    
    // 打印AST结构
    printf("AST结构:\n");
    ast_print(ast, 0);
    
    // 序列化为二进制
    size_t binary_size;
    printf("\n序列化为二进制...\n");
    unsigned char *binary = c2astc_serialize(ast, &binary_size);
    
    if (!binary) {
        const char *error = c2astc_get_error();
        printf("序列化失败: %s\n", error ? error : "未知错误");
        ast_free(ast);
        return 1;
    }
    
    printf("序列化成功! 二进制大小: %zu 字节\n", binary_size);
    
    // 清理资源
    c2astc_free(binary);
    ast_free(ast);
    
    printf("\n测试2完成!\n");
    return 0;
} 