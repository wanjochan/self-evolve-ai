/**
 * c2astc.c - C语言到ASTC的转换库
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// 包含AST定义
#include "astc.h"
#include "evolver0_token.h"

// ===============================================
// 类型和结构定义
// ===============================================

typedef struct {
    const char *source;     // C源代码
    size_t length;          // 源代码长度
    size_t position;        // 当前处理位置
    int line;               // 当前行号
    int column;             // 当前列号
    char *error_msg;        // 错误信息
} C2AstcContext;

// WASM类型
typedef enum {
    WASM_TYPE_I32,
    WASM_TYPE_I64,
    WASM_TYPE_F32,
    WASM_TYPE_F64,
    WASM_TYPE_FUNCREF,
    WASM_TYPE_EXTERNREF,
    WASM_TYPE_V128,       // SIMD
    WASM_TYPE_VOID,       // 用于函数返回void
    WASM_TYPE_FUNC        // 函数类型
} WasmValueType;

// C类型到WASM类型映射
typedef struct {
    ASTNodeType c_type;     // C语言类型
    WasmValueType wasm_type; // 对应的WASM类型
    int size;               // 类型大小（字节）
    bool is_signed;         // 是否有符号
} TypeMapping;

// 类型映射表
static const TypeMapping type_map[] = {
    {ASTC_TYPE_VOID, WASM_TYPE_VOID, 0, false},
    {ASTC_TYPE_CHAR, WASM_TYPE_I32, 1, true},
    {ASTC_TYPE_SIGNED_CHAR, WASM_TYPE_I32, 1, true},
    {ASTC_TYPE_UNSIGNED_CHAR, WASM_TYPE_I32, 1, false},
    {ASTC_TYPE_SHORT, WASM_TYPE_I32, 2, true},
    {ASTC_TYPE_UNSIGNED_SHORT, WASM_TYPE_I32, 2, false},
    {ASTC_TYPE_INT, WASM_TYPE_I32, 4, true},
    {ASTC_TYPE_UNSIGNED_INT, WASM_TYPE_I32, 4, false},
    {ASTC_TYPE_LONG, WASM_TYPE_I64, 8, true},
    {ASTC_TYPE_UNSIGNED_LONG, WASM_TYPE_I64, 8, false},
    {ASTC_TYPE_LONG_LONG, WASM_TYPE_I64, 8, true},
    {ASTC_TYPE_UNSIGNED_LONG_LONG, WASM_TYPE_I64, 8, false},
    {ASTC_TYPE_FLOAT, WASM_TYPE_F32, 4, true},
    {ASTC_TYPE_DOUBLE, WASM_TYPE_F64, 8, true},
    {ASTC_TYPE_LONG_DOUBLE, WASM_TYPE_F64, 8, true},
    {ASTC_TYPE_BOOL, WASM_TYPE_I32, 1, false},
    {ASTC_TYPE_POINTER, WASM_TYPE_I32, 4, false}  // 默认32位指针
};

// 转换配置选项
typedef struct {
    bool optimize_level;        // 优化级别
    bool enable_extensions;     // 启用WASX扩展
    bool emit_debug_info;       // 生成调试信息
} C2AstcOptions;

// ===============================================
// 初始化和清理
// ===============================================

/**
 * 初始化C到ASTC转换上下文
 */
static C2AstcContext* init_c2astc_context(const char *source) {
    C2AstcContext *ctx = (C2AstcContext*)malloc(sizeof(C2AstcContext));
    if (ctx) {
        ctx->source = source;
        ctx->length = strlen(source);
        ctx->position = 0;
        ctx->line = 1;
        ctx->column = 1;
        ctx->error_msg = NULL;
    }
    return ctx;
}

/**
 * 释放转换上下文资源
 */
static void free_c2astc_context(C2AstcContext *ctx) {
    if (ctx) {
        if (ctx->error_msg) {
            free(ctx->error_msg);
        }
        free(ctx);
    }
}

/**
 * 设置错误信息
 */
static void set_error(C2AstcContext *ctx, const char *format, ...) {
    if (ctx) {
        va_list args;
        va_start(args, format);
        
        // 如果已有错误信息，先释放
        if (ctx->error_msg) {
            free(ctx->error_msg);
        }
        
        // 分配内存并格式化错误消息
        ctx->error_msg = (char*)malloc(256);
        if (ctx->error_msg) {
            vsnprintf(ctx->error_msg, 256, format, args);
        }
        
        va_end(args);
    }
}

// ===============================================
// C到ASTC转换辅助函数
// ===============================================

/**
 * 将C语言二元操作符转换为ASTC二元操作符节点类型
 */
static ASTNodeType c_binop_to_astc(int token_type) {
    switch (token_type) {
        // 算术运算符
        case '+': return ASTC_EXPR_ADD;
        case '-': return ASTC_EXPR_SUB;
        case '*': return ASTC_EXPR_MUL;
        case '/': return ASTC_EXPR_DIV;
        case '%': return ASTC_EXPR_MOD;
        
        // 比较运算符
        case TOKEN_EQ: return ASTC_EXPR_EQUAL;
        case TOKEN_NE: return ASTC_EXPR_NOT_EQUAL;
        case '<': return ASTC_EXPR_LESS;
        case TOKEN_LE: return ASTC_EXPR_LESS_EQUAL;
        case '>': return ASTC_EXPR_GREATER;
        case TOKEN_GE: return ASTC_EXPR_GREATER_EQUAL;
        
        // 逻辑运算符
        case TOKEN_LOGICAL_AND: return ASTC_EXPR_LOGICAL_AND;
        case TOKEN_LOGICAL_OR: return ASTC_EXPR_LOGICAL_OR;
        
        // 按位运算符
        case '&': return ASTC_EXPR_BIT_AND;
        case '|': return ASTC_EXPR_BIT_OR;
        case '^': return ASTC_EXPR_BIT_XOR;
        case TOKEN_SHL: return ASTC_EXPR_LEFT_SHIFT;
        case TOKEN_SHR: return ASTC_EXPR_RIGHT_SHIFT;
        
        // 赋值运算符
        case '=': return ASTC_EXPR_ASSIGN;
        case TOKEN_ADD_ASSIGN: return ASTC_EXPR_ADD_ASSIGN;
        case TOKEN_SUB_ASSIGN: return ASTC_EXPR_SUB_ASSIGN;
        case TOKEN_MUL_ASSIGN: return ASTC_EXPR_MUL_ASSIGN;
        case TOKEN_DIV_ASSIGN: return ASTC_EXPR_DIV_ASSIGN;
        case TOKEN_MOD_ASSIGN: return ASTC_EXPR_MOD_ASSIGN;
        case TOKEN_AND_ASSIGN: return ASTC_EXPR_BIT_AND_ASSIGN;
        case TOKEN_OR_ASSIGN: return ASTC_EXPR_BIT_OR_ASSIGN;
        case TOKEN_XOR_ASSIGN: return ASTC_EXPR_BIT_XOR_ASSIGN;
        case TOKEN_SHL_ASSIGN: return ASTC_EXPR_LEFT_SHIFT_ASSIGN;
        case TOKEN_SHR_ASSIGN: return ASTC_EXPR_RIGHT_SHIFT_ASSIGN;
        
        // 其他
        case ',': return ASTC_EXPR_COMMA;
        
        default: return ASTC_ERROR; // 未知或不支持的操作符
    }
}

/**
 * 将C语言一元操作符转换为ASTC一元操作符节点类型
 */
static ASTNodeType c_unaryop_to_astc(int token_type) {
    switch (token_type) {
        case '+': return ASTC_EXPR_PLUS;
        case '-': return ASTC_EXPR_MINUS;
        case '!': return ASTC_EXPR_LOGICAL_NOT;
        case '~': return ASTC_EXPR_BIT_NOT;
        case '*': return ASTC_EXPR_DEREF;
        case '&': return ASTC_EXPR_ADDR;
        case TOKEN_INC: return ASTC_EXPR_PRE_INC;
        case TOKEN_DEC: return ASTC_EXPR_PRE_DEC;
        case TOKEN_SIZEOF: return ASTC_EXPR_SIZEOF;
        default: return ASTC_ERROR;
    }
}

/**
 * 将C语言类型转换为WASM类型
 */
static WasmValueType c_type_to_wasm(ASTNodeType c_type) {
    for (int i = 0; i < sizeof(type_map) / sizeof(TypeMapping); i++) {
        if (type_map[i].c_type == c_type) {
            return type_map[i].wasm_type;
        }
    }
    return WASM_TYPE_I32; // 默认返回I32
}

// ===============================================
// 节点创建助手函数
// ===============================================

/**
 * 创建ASTC标识符表达式节点
 */
static struct ASTNode* create_identifier_expr(const char *name, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_IDENTIFIER, line, column);
    if (node) {
        node->data.identifier.name = strdup(name);
    }
    return node;
}

/**
 * 创建ASTC整数常量表达式节点
 */
static struct ASTNode* create_int_const_expr(int64_t value, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_CONSTANT, line, column);
    if (node) {
        node->data.constant.type = ASTC_TYPE_INT;
        node->data.constant.int_val = value;
    }
    return node;
}

/**
 * 创建ASTC浮点常量表达式节点
 */
static struct ASTNode* create_float_const_expr(double value, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_CONSTANT, line, column);
    if (node) {
        node->data.constant.type = ASTC_TYPE_FLOAT;
        node->data.constant.float_val = value;
    }
    return node;
}

/**
 * 创建ASTC字符串字面量表达式节点
 */
static struct ASTNode* create_string_literal_expr(const char *value, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_EXPR_STRING_LITERAL, line, column);
    if (node) {
        node->data.string_literal.value = strdup(value);
    }
    return node;
}

/**
 * 创建二元操作表达式节点
 */
static struct ASTNode* create_binary_expr(ASTNodeType op, struct ASTNode *left, struct ASTNode *right, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_BINARY_OP, line, column);
    if (node) {
        node->data.binary_op.op = op;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
    }
    return node;
}

/**
 * 创建一元操作表达式节点
 */
static struct ASTNode* create_unary_expr(ASTNodeType op, struct ASTNode *operand, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_UNARY_OP, line, column);
    if (node) {
        node->data.unary_op.op = op;
        node->data.unary_op.operand = operand;
    }
    return node;
}

/**
 * 创建函数调用表达式节点
 */
static struct ASTNode* create_call_expr(struct ASTNode *callee, struct ASTNode **args, int arg_count, int line, int column) {
    struct ASTNode *node = ast_create_node(ASTC_CALL_EXPR, line, column);
    if (node) {
        node->data.call_expr.callee = callee;
        node->data.call_expr.args = args;
        node->data.call_expr.arg_count = arg_count;
    }
    return node;
}

// ===============================================
// 主要转换函数
// ===============================================

/**
 * 将C源代码转换为ASTC表示
 * 
 * @param source C源代码
 * @param options 转换配置选项
 * @return 转换后的ASTC根节点，失败返回NULL
 */
struct ASTNode* c2astc_convert(const char *source, const C2AstcOptions *options) {
    if (!source) {
        return NULL;
    }
    
    // 创建上下文
    C2AstcContext *ctx = init_c2astc_context(source);
    if (!ctx) {
        return NULL;
    }
    
    // 通过词法分析转换为Token流
    // TODO: 在此集成evolver0的词法分析器
    
    // 递归下降解析
    // TODO: 在此调用evolver0解析器或重新实现解析
    
    // 转换为ASTC
    // TODO: 实现AST到ASTC的转换
    
    // 进行ASTC优化（如果启用）
    // TODO: 实现ASTC优化
    
    // 释放上下文
    free_c2astc_context(ctx);
    
    // 返回根节点（暂时返回NULL）
    return NULL;
}

/**
 * 打印C到ASTC转换库版本信息
 */
void c2astc_print_version(void) {
    printf("C to ASTC Converter v0.1.0\n");
    printf("Part of self-evolve-ai project\n");
}

// ===============================================
// API 函数
// ===============================================

/**
 * 从文件加载C源代码并转换为ASTC
 * 
 * @param filename C源文件名
 * @param options 转换配置选项
 * @return 转换后的ASTC根节点，失败返回NULL
 */
struct ASTNode* c2astc_convert_file(const char *filename, const C2AstcOptions *options) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return NULL;
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 分配内存读取文件内容
    char *source = (char*)malloc(file_size + 1);
    if (!source) {
        fclose(fp);
        return NULL;
    }
    
    // 读取文件内容
    size_t read_size = fread(source, 1, file_size, fp);
    source[read_size] = '\0';
    fclose(fp);
    
    // 转换为ASTC
    struct ASTNode *root = c2astc_convert(source, options);
    
    // 释放源代码内存
    free(source);
    
    return root;
}

/**
 * 默认转换选项
 */
C2AstcOptions c2astc_default_options(void) {
    C2AstcOptions options;
    options.optimize_level = 0;
    options.enable_extensions = true;
    options.emit_debug_info = false;
    return options;
}

// ===============================================
// 测试主函数 (如果作为独立程序编译)
// ===============================================

#ifdef C2ASTC_MAIN
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("用法: %s <C源文件>\n", argv[0]);
        return 1;
    }
    
    c2astc_print_version();
    
    C2AstcOptions options = c2astc_default_options();
    struct ASTNode *root = c2astc_convert_file(argv[1], &options);
    
    if (root) {
        printf("转换成功，打印ASTC树:\n");
        ast_print(root, 0);
        ast_free(root);
        return 0;
    } else {
        printf("转换失败\n");
        return 1;
    }
}
#endif // C2ASTC_MAIN 