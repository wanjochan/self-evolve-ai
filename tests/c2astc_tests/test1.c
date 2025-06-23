/**
 * test1.c - 测试基本的C语言结构转换为ASTC
 */

#include <stdio.h>
#include <stdlib.h>
#include "c2astc.h"

int main() {
    printf("测试1: 基本C语言结构转换为ASTC\n");
    printf("================================\n\n");
    
    // 简单的C代码，包含变量声明、函数调用和控制流
    const char *test_code = 
        "// 简单的C程序\n"
        "#include <stdio.h>\n\n"
        "int main() {\n"
        "    int a = 10;\n"
        "    int b = 20;\n"
        "    int sum = a + b;\n\n"
        "    if (sum > 25) {\n"
        "        printf(\"Sum is greater than 25\\n\");\n"
        "    } else {\n"
        "        printf(\"Sum is not greater than 25\\n\");\n"
        "    }\n\n"
        "    return 0;\n"
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
    
    // 清理资源
    ast_free(ast);
    
    printf("\n测试1完成!\n");
    return 0;
} 