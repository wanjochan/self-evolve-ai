#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 从simple_parser_test.c复用的结构定义
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_RETURN_STMT,
    AST_VAR_DECL,
    AST_BINARY_OP,
    AST_NUMBER,
    AST_IDENTIFIER
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char *value;
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
} ASTNode;

// WASM字节码缓冲区
typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} WasmBuffer;

// WASM操作码
#define WASM_OP_I32_CONST   0x41
#define WASM_OP_I32_ADD     0x6A
#define WASM_OP_I32_SUB     0x6B
#define WASM_OP_I32_MUL     0x6C
#define WASM_OP_I32_DIV_S   0x6D
#define WASM_OP_RETURN      0x0F
#define WASM_OP_LOCAL_GET   0x20
#define WASM_OP_LOCAL_SET   0x21
#define WASM_OP_END         0x0B

// 缓冲区操作
static void wasm_buffer_init(WasmBuffer *buffer) {
    buffer->data = malloc(1024);
    buffer->size = 0;
    buffer->capacity = 1024;
}

static void wasm_write_byte(WasmBuffer *buffer, uint8_t byte) {
    if (buffer->size >= buffer->capacity) {
        buffer->capacity *= 2;
        buffer->data = realloc(buffer->data, buffer->capacity);
    }
    buffer->data[buffer->size++] = byte;
}

static void wasm_write_leb128_u32(WasmBuffer *buffer, uint32_t value) {
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        wasm_write_byte(buffer, byte);
    } while (value != 0);
}

static void wasm_write_leb128_i32(WasmBuffer *buffer, int32_t value) {
    int more = 1;
    while (more) {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if ((value == 0 && (byte & 0x40) == 0) || 
            (value == -1 && (byte & 0x40) != 0)) {
            more = 0;
        } else {
            byte |= 0x80;
        }
        wasm_write_byte(buffer, byte);
    }
}

// 简单的变量表（名称到局部变量索引的映射）
typedef struct {
    char *name;
    int index;
} Variable;

typedef struct {
    Variable vars[16];
    int count;
} VariableTable;

static int find_or_add_variable(VariableTable *table, const char *name) {
    // 查找现有变量
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->vars[i].name, name) == 0) {
            return table->vars[i].index;
        }
    }
    
    // 添加新变量
    if (table->count < 16) {
        table->vars[table->count].name = strdup(name);
        table->vars[table->count].index = table->count;
        return table->count++;
    }
    
    printf("Error: Too many variables\n");
    return -1;
}

// AST到WASM转换器
static void generate_wasm_expression(ASTNode *node, WasmBuffer *buffer, VariableTable *vars) {
    if (!node) return;
    
    switch (node->type) {
        case AST_NUMBER:
            // 生成 i32.const 指令
            wasm_write_byte(buffer, WASM_OP_I32_CONST);
            wasm_write_leb128_i32(buffer, atoi(node->value));
            printf("  Generated: i32.const %s\n", node->value);
            break;
            
        case AST_IDENTIFIER:
            // 生成 local.get 指令
            int var_index = find_or_add_variable(vars, node->value);
            if (var_index >= 0) {
                wasm_write_byte(buffer, WASM_OP_LOCAL_GET);
                wasm_write_leb128_u32(buffer, var_index);
                printf("  Generated: local.get %d (%s)\n", var_index, node->value);
            }
            break;
            
        case AST_BINARY_OP:
            // 生成左操作数
            generate_wasm_expression(node->left, buffer, vars);
            // 生成右操作数
            generate_wasm_expression(node->right, buffer, vars);
            
            // 生成操作符指令
            if (strcmp(node->value, "+") == 0) {
                wasm_write_byte(buffer, WASM_OP_I32_ADD);
                printf("  Generated: i32.add\n");
            } else if (strcmp(node->value, "-") == 0) {
                wasm_write_byte(buffer, WASM_OP_I32_SUB);
                printf("  Generated: i32.sub\n");
            } else if (strcmp(node->value, "*") == 0) {
                wasm_write_byte(buffer, WASM_OP_I32_MUL);
                printf("  Generated: i32.mul\n");
            } else if (strcmp(node->value, "/") == 0) {
                wasm_write_byte(buffer, WASM_OP_I32_DIV_S);
                printf("  Generated: i32.div_s\n");
            }
            break;
            
        default:
            printf("Warning: Unsupported expression type: %d\n", node->type);
            break;
    }
}

static void generate_wasm_statement(ASTNode *node, WasmBuffer *buffer, VariableTable *vars) {
    if (!node) return;
    
    switch (node->type) {
        case AST_VAR_DECL:
            printf("Processing variable declaration: %s\n", node->value);
            // 记录变量名（索引在find_or_add_variable中分配）
            int var_index = find_or_add_variable(vars, node->value);
            
            if (node->right) {
                // 有初始化表达式
                printf("  Generating initialization expression:\n");
                generate_wasm_expression(node->right, buffer, vars);
                
                // 存储到局部变量
                wasm_write_byte(buffer, WASM_OP_LOCAL_SET);
                wasm_write_leb128_u32(buffer, var_index);
                printf("  Generated: local.set %d (%s)\n", var_index, node->value);
            }
            break;
            
        case AST_RETURN_STMT:
            printf("Processing return statement:\n");
            if (node->left) {
                // 生成返回值表达式
                printf("  Generating return expression:\n");
                generate_wasm_expression(node->left, buffer, vars);
            }
            
            // 生成return指令
            wasm_write_byte(buffer, WASM_OP_RETURN);
            printf("  Generated: return\n");
            break;
            
        default:
            printf("Warning: Unsupported statement type: %d\n", node->type);
            break;
    }
}

static void generate_wasm_function(ASTNode *node, WasmBuffer *buffer) {
    if (!node || node->type != AST_FUNCTION) return;
    
    printf("\nGenerating WASM for function: %s\n", node->value);
    
    VariableTable vars = {0};
    
    // 函数体开始
    printf("Function body:\n");
    
    // 遍历函数中的所有语句
    ASTNode *stmt = node->left;
    while (stmt) {
        generate_wasm_statement(stmt, buffer, &vars);
        stmt = stmt->next;
    }
    
    // 函数结束标记
    wasm_write_byte(buffer, WASM_OP_END);
    printf("  Generated: end\n");
    
    printf("Function %s completed. Local variables: %d\n", node->value, vars.count);
    
    // 清理变量表
    for (int i = 0; i < vars.count; i++) {
        free(vars.vars[i].name);
    }
}

// 生成完整的WASM模块
static void generate_wasm_module(ASTNode *ast, const char *output_file) {
    if (!ast || ast->type != AST_PROGRAM) return;
    
    WasmBuffer buffer;
    wasm_buffer_init(&buffer);
    
    printf("Generating WASM module...\n");
    
    // WASM魔数和版本
    uint8_t header[] = {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00};
    for (size_t i = 0; i < sizeof(header); i++) {
        wasm_write_byte(&buffer, header[i]);
    }
    
    // 类型段 (section 1)
    wasm_write_byte(&buffer, 0x01); // type section
    wasm_write_leb128_u32(&buffer, 0x05); // section size (placeholder)
    wasm_write_leb128_u32(&buffer, 0x01); // 1 type
    wasm_write_byte(&buffer, 0x60);       // func type
    wasm_write_leb128_u32(&buffer, 0x00); // 0 params
    wasm_write_leb128_u32(&buffer, 0x01); // 1 result
    wasm_write_byte(&buffer, 0x7F);       // i32
    
    // 函数段 (section 3)
    wasm_write_byte(&buffer, 0x03); // function section
    wasm_write_leb128_u32(&buffer, 0x02); // section size
    wasm_write_leb128_u32(&buffer, 0x01); // 1 function
    wasm_write_leb128_u32(&buffer, 0x00); // type index 0
    
    // 导出段 (section 7)
    wasm_write_byte(&buffer, 0x07); // export section
    wasm_write_leb128_u32(&buffer, 0x08); // section size
    wasm_write_leb128_u32(&buffer, 0x01); // 1 export
    wasm_write_leb128_u32(&buffer, 0x04); // name length
    wasm_write_byte(&buffer, 'm');
    wasm_write_byte(&buffer, 'a');
    wasm_write_byte(&buffer, 'i');
    wasm_write_byte(&buffer, 'n');
    wasm_write_byte(&buffer, 0x00);       // export kind (function)
    wasm_write_leb128_u32(&buffer, 0x00); // function index 0
    
    // 代码段 (section 10)
    wasm_write_byte(&buffer, 0x0A); // code section
    
    // 临时缓冲区用于函数体
    WasmBuffer func_buffer;
    wasm_buffer_init(&func_buffer);
    
    // 生成函数体
    ASTNode *func = ast->left;
    if (func && func->type == AST_FUNCTION) {
        // 局部变量声明（这里简化为2个i32变量）
        wasm_write_leb128_u32(&func_buffer, 0x01); // 1 local group
        wasm_write_leb128_u32(&func_buffer, 0x02); // 2 locals
        wasm_write_byte(&func_buffer, 0x7F);       // i32 type
        
        generate_wasm_function(func, &func_buffer);
    }
    
    // 写入代码段大小和内容
    wasm_write_leb128_u32(&buffer, func_buffer.size + 2); // section size
    wasm_write_leb128_u32(&buffer, 0x01);                  // 1 function
    wasm_write_leb128_u32(&buffer, func_buffer.size);     // function size
    
    // 复制函数体
    for (size_t i = 0; i < func_buffer.size; i++) {
        wasm_write_byte(&buffer, func_buffer.data[i]);
    }
    
    // 写入文件
    FILE *file = fopen(output_file, "wb");
    if (file) {
        fwrite(buffer.data, 1, buffer.size, file);
        fclose(file);
        printf("\n✓ WASM module written to %s (%zu bytes)\n", output_file, buffer.size);
    } else {
        printf("✗ Failed to write %s\n", output_file);
    }
    
    // 清理
    free(buffer.data);
    free(func_buffer.data);
}

// 创建测试AST（模拟simple_parser_test.c的输出）
static ASTNode* create_test_ast() {
    // 创建程序节点
    ASTNode *program = calloc(1, sizeof(ASTNode));
    program->type = AST_PROGRAM;
    program->value = strdup("program");
    
    // 创建main函数
    ASTNode *main_func = calloc(1, sizeof(ASTNode));
    main_func->type = AST_FUNCTION;
    main_func->value = strdup("main");
    
    // 变量声明: int x = 42;
    ASTNode *var_x = calloc(1, sizeof(ASTNode));
    var_x->type = AST_VAR_DECL;
    var_x->value = strdup("x");
    
    ASTNode *const_42 = calloc(1, sizeof(ASTNode));
    const_42->type = AST_NUMBER;
    const_42->value = strdup("42");
    var_x->right = const_42;
    
    // 变量声明: int y = x + 10;
    ASTNode *var_y = calloc(1, sizeof(ASTNode));
    var_y->type = AST_VAR_DECL;
    var_y->value = strdup("y");
    
    ASTNode *add_op = calloc(1, sizeof(ASTNode));
    add_op->type = AST_BINARY_OP;
    add_op->value = strdup("+");
    
    ASTNode *id_x = calloc(1, sizeof(ASTNode));
    id_x->type = AST_IDENTIFIER;
    id_x->value = strdup("x");
    
    ASTNode *const_10 = calloc(1, sizeof(ASTNode));
    const_10->type = AST_NUMBER;
    const_10->value = strdup("10");
    
    add_op->left = id_x;
    add_op->right = const_10;
    var_y->right = add_op;
    
    // 返回语句: return y * 2;
    ASTNode *return_stmt = calloc(1, sizeof(ASTNode));
    return_stmt->type = AST_RETURN_STMT;
    return_stmt->value = strdup("return");
    
    ASTNode *mul_op = calloc(1, sizeof(ASTNode));
    mul_op->type = AST_BINARY_OP;
    mul_op->value = strdup("*");
    
    ASTNode *id_y = calloc(1, sizeof(ASTNode));
    id_y->type = AST_IDENTIFIER;
    id_y->value = strdup("y");
    
    ASTNode *const_2 = calloc(1, sizeof(ASTNode));
    const_2->type = AST_NUMBER;
    const_2->value = strdup("2");
    
    mul_op->left = id_y;
    mul_op->right = const_2;
    return_stmt->left = mul_op;
    
    // 链接语句
    var_x->next = var_y;
    var_y->next = return_stmt;
    
    // 链接到函数
    main_func->left = var_x;
    program->left = main_func;
    
    return program;
}

int main() {
    printf("AST到WASM转换器测试\n");
    printf("===================\n\n");
    
    // 创建测试AST
    ASTNode *ast = create_test_ast();
    
    printf("输入AST结构 (模拟):\n");
    printf("PROGRAM\n");
    printf("  FUNCTION: main\n");
    printf("    VAR_DECL: x\n");
    printf("      NUMBER: 42\n");
    printf("    VAR_DECL: y\n");
    printf("      BINARY_OP: +\n");
    printf("        IDENTIFIER: x\n");
    printf("        NUMBER: 10\n");
    printf("    RETURN\n");
    printf("      BINARY_OP: *\n");
    printf("        IDENTIFIER: y\n");
    printf("        NUMBER: 2\n");
    printf("\n");
    
    // 生成WASM模块
    generate_wasm_module(ast, "test.wasm");
    
    printf("\n✓ AST到WASM转换测试完成！\n");
    printf("生成的WASM文件应该实现以下逻辑:\n");
    printf("1. 将42存储到局部变量0 (x)\n");
    printf("2. 计算x + 10并存储到局部变量1 (y)\n");
    printf("3. 返回y * 2的结果\n");
    printf("预期结果: (42 + 10) * 2 = 104\n");
    
    return 0;
}