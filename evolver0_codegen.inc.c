/**
 * evolver0_codegen.inc.c - 代码生成器模块
 * 被 evolver0.c 包含
 */

#ifndef EVOLVER0_CODEGEN_INC_C
#define EVOLVER0_CODEGEN_INC_C

#include <stdint.h>

// ====================================
// x86-64 指令编码
// ====================================

// CodeGen结构已在主文件定义，这里只是扩展定义
#if 0
typedef struct {
    uint8_t *code;
    size_t size;
    size_t capacity;
    
    // 标签和重定位
    struct {
        char *name;
        size_t offset;
    } labels[1024];
    int label_count;
    
    struct {
        char *label;
        size_t offset;
        int type; // 0=相对跳转, 1=绝对地址
    } relocations[1024];
    int reloc_count;
    
    // 当前函数状态
    int stack_offset;
    int max_stack_size;
    
    // 局部变量
    struct {
        char *name;
        int offset; // 相对于RBP的偏移
        TypeInfo *type;
    } locals[256];
    int local_count;
    
    // 字符串常量
    struct {
        char *str;
        char *label;
    } strings[256];
    int string_count;
    
} CodeGen;
#endif

// 扩展CodeGen结构，添加新字段
typedef struct {
    unsigned char *code;
    size_t size;
    size_t capacity;
    
    // 标签管理
    struct {
        char *name;
        size_t offset;
    } *labels;
    int label_count;
    int label_capacity;
    
    // 重定位
    struct {
        char *label;
        size_t offset;
        int type;
    } relocations[1024];
    int reloc_count;
    
    // 局部变量
    struct {
        char *name;
        int offset;  // 相对于RBP的偏移
        TypeInfo *type;
    } *locals;
    int local_count;
    int local_capacity;
    int stack_offset;
    int max_stack_size;
    
    // 字符串常量
    struct {
        char *str;
        char *label;
    } strings[256];
    int string_count;
    
    // 当前函数信息
    char *current_function;
    bool in_main;
} CodeGen;

// ====================================
// 辅助函数
// ====================================

static void init_codegen(CodeGen *gen) {
    gen->code = (uint8_t*)malloc(4096);
    gen->size = 0;
    gen->capacity = 4096;
    
    // 初始化标签数组
    gen->label_capacity = 1024;
    gen->labels = malloc(sizeof(*gen->labels) * gen->label_capacity);
    gen->label_count = 0;
    
    gen->reloc_count = 0;
    gen->stack_offset = 0;
    gen->max_stack_size = 0;
    
    // 初始化局部变量数组
    gen->local_capacity = 256;
    gen->locals = malloc(sizeof(*gen->locals) * gen->local_capacity);
    gen->local_count = 0;
    
    gen->string_count = 0;
    gen->current_function = NULL;
    gen->in_main = false;
}

static void free_codegen(CodeGen *gen) {
    free(gen->code);
    
    // 释放标签数组
    for (int i = 0; i < gen->label_count; i++) {
        free(gen->labels[i].name);
    }
    free(gen->labels);
    
    // 释放重定位
    for (int i = 0; i < gen->reloc_count; i++) {
        free(gen->relocations[i].label);
    }
    
    // 释放局部变量数组
    for (int i = 0; i < gen->local_count; i++) {
        free(gen->locals[i].name);
    }
    free(gen->locals);
    
    // 释放字符串常量
    for (int i = 0; i < gen->string_count; i++) {
        free(gen->strings[i].str);
        free(gen->strings[i].label);
    }
    
    free(gen->current_function);
}

static void emit_byte(CodeGen *gen, uint8_t byte) {
    if (gen->size >= gen->capacity) {
        gen->capacity *= 2;
        gen->code = (uint8_t*)realloc(gen->code, gen->capacity);
    }
    gen->code[gen->size++] = byte;
}

static void emit_bytes(CodeGen *gen, const uint8_t *bytes, size_t count) {
    for (size_t i = 0; i < count; i++) {
        emit_byte(gen, bytes[i]);
    }
}

static void emit_int32(CodeGen *gen, int32_t value) {
    emit_byte(gen, value & 0xFF);
    emit_byte(gen, (value >> 8) & 0xFF);
    emit_byte(gen, (value >> 16) & 0xFF);
    emit_byte(gen, (value >> 24) & 0xFF);
}

static void emit_int64(CodeGen *gen, int64_t value) {
    emit_int32(gen, value & 0xFFFFFFFF);
    emit_int32(gen, (value >> 32) & 0xFFFFFFFF);
}

// ====================================
// 标签和重定位
// ====================================

static void add_label(CodeGen *gen, const char *name) {
    if (gen->label_count >= gen->label_capacity) {
        gen->label_capacity *= 2;
        gen->labels = realloc(gen->labels, sizeof(*gen->labels) * gen->label_capacity);
    }
    
    gen->labels[gen->label_count].name = strdup(name);
    gen->labels[gen->label_count].offset = gen->size;
    gen->label_count++;
}

static size_t find_label(CodeGen *gen, const char *name) {
    for (int i = 0; i < gen->label_count; i++) {
        if (strcmp(gen->labels[i].name, name) == 0) {
            return gen->labels[i].offset;
        }
    }
    return (size_t)-1;
}

static void add_relocation(CodeGen *gen, const char *label, int type) {
    if (gen->reloc_count >= 1024) return;
    
    gen->relocations[gen->reloc_count].label = strdup(label);
    gen->relocations[gen->reloc_count].offset = gen->size;
    gen->relocations[gen->reloc_count].type = type;
    gen->reloc_count++;
}

static void resolve_relocations(CodeGen *gen) {
    for (int i = 0; i < gen->reloc_count; i++) {
        size_t label_offset = find_label(gen, gen->relocations[i].label);
        if (label_offset == (size_t)-1) {
            fprintf(stderr, "未定义的标签: %s\n", gen->relocations[i].label);
            continue;
        }
        
        size_t reloc_offset = gen->relocations[i].offset;
        
        if (gen->relocations[i].type == 0) {
            // 相对跳转
            int32_t rel = label_offset - (reloc_offset + 4);
            memcpy(gen->code + reloc_offset, &rel, 4);
        } else {
            // 绝对地址
            memcpy(gen->code + reloc_offset, &label_offset, 8);
        }
    }
}

// ====================================
// 局部变量管理
// ====================================

static int add_local(CodeGen *gen, const char *name, TypeInfo *type) {
    if (gen->local_count >= gen->local_capacity) {
        gen->local_capacity *= 2;
        gen->locals = realloc(gen->locals, sizeof(*gen->locals) * gen->local_capacity);
    }
    
    int size = type ? type->size : 8;
    int alignment = type ? type->alignment : 8;
    
    // 对齐
    gen->stack_offset = (gen->stack_offset + alignment - 1) & ~(alignment - 1);
    gen->stack_offset += size;
    
    gen->locals[gen->local_count].name = strdup(name);
    gen->locals[gen->local_count].offset = -gen->stack_offset;
    gen->locals[gen->local_count].type = type;
    gen->local_count++;
    
    if (gen->stack_offset > gen->max_stack_size) {
        gen->max_stack_size = gen->stack_offset;
    }
    
    return -gen->stack_offset;
}

static int find_local(CodeGen *gen, const char *name) {
    for (int i = gen->local_count - 1; i >= 0; i--) {
        if (strcmp(gen->locals[i].name, name) == 0) {
            return gen->locals[i].offset;
        }
    }
    return 0;
}

// ====================================
// x86-64 指令生成
// ====================================

// MOV reg, imm
static void emit_mov_reg_imm(CodeGen *gen, int reg, int64_t imm) {
    if (imm >= -2147483648LL && imm <= 2147483647LL) {
        // MOV r32, imm32 (带符号扩展到64位)
        if (reg >= 8) {
            emit_byte(gen, 0x41);
        }
        emit_byte(gen, 0xB8 + (reg & 7));
        emit_int32(gen, imm);
    } else {
        // MOV r64, imm64
        emit_byte(gen, 0x48 | (reg >= 8 ? 0x01 : 0x00));
        emit_byte(gen, 0xB8 + (reg & 7));
        emit_int64(gen, imm);
    }
}

// PUSH reg
static void emit_push(CodeGen *gen, int reg) {
    if (reg >= 8) {
        emit_byte(gen, 0x41);
    }
    emit_byte(gen, 0x50 + (reg & 7));
}

// POP reg
static void emit_pop(CodeGen *gen, int reg) {
    if (reg >= 8) {
        emit_byte(gen, 0x41);
    }
    emit_byte(gen, 0x58 + (reg & 7));
}

// ADD dst, src
static void emit_add(CodeGen *gen, int dst, int src) {
    emit_byte(gen, 0x48 | (dst >= 8 ? 0x04 : 0x00) | (src >= 8 ? 0x01 : 0x00));
    emit_byte(gen, 0x01);
    emit_byte(gen, 0xC0 | ((src & 7) << 3) | (dst & 7));
}

// SUB dst, src
static void emit_sub(CodeGen *gen, int dst, int src) {
    emit_byte(gen, 0x48 | (dst >= 8 ? 0x04 : 0x00) | (src >= 8 ? 0x01 : 0x00));
    emit_byte(gen, 0x29);
    emit_byte(gen, 0xC0 | ((src & 7) << 3) | (dst & 7));
}

// IMUL dst, src
static void emit_imul(CodeGen *gen, int dst, int src) {
    emit_byte(gen, 0x48 | (dst >= 8 ? 0x04 : 0x00) | (src >= 8 ? 0x01 : 0x00));
    emit_byte(gen, 0x0F);
    emit_byte(gen, 0xAF);
    emit_byte(gen, 0xC0 | ((dst & 7) << 3) | (src & 7));
}

// CQO (符号扩展RAX到RDX:RAX)
static void emit_cqo(CodeGen *gen) {
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x99);
}

// IDIV src
static void emit_idiv(CodeGen *gen, int src) {
    emit_byte(gen, 0x48 | (src >= 8 ? 0x01 : 0x00));
    emit_byte(gen, 0xF7);
    emit_byte(gen, 0xF8 | (src & 7));
}

// MOV dst, [rbp+offset]
static void emit_mov_reg_mem(CodeGen *gen, int reg, int offset) {
    emit_byte(gen, 0x48 | (reg >= 8 ? 0x04 : 0x00));
    emit_byte(gen, 0x8B);
    
    if (offset == 0) {
        emit_byte(gen, 0x45 | ((reg & 7) << 3));
    } else if (offset >= -128 && offset <= 127) {
        emit_byte(gen, 0x45 | ((reg & 7) << 3));
        emit_byte(gen, offset);
    } else {
        emit_byte(gen, 0x85 | ((reg & 7) << 3));
        emit_int32(gen, offset);
    }
}

// MOV [rbp+offset], reg
static void emit_mov_mem_reg(CodeGen *gen, int offset, int reg) {
    emit_byte(gen, 0x48 | (reg >= 8 ? 0x04 : 0x00));
    emit_byte(gen, 0x89);
    
    if (offset == 0) {
        emit_byte(gen, 0x45 | ((reg & 7) << 3));
    } else if (offset >= -128 && offset <= 127) {
        emit_byte(gen, 0x45 | ((reg & 7) << 3));
        emit_byte(gen, offset);
    } else {
        emit_byte(gen, 0x85 | ((reg & 7) << 3));
        emit_int32(gen, offset);
    }
}

// CMP reg1, reg2
static void emit_cmp(CodeGen *gen, int reg1, int reg2) {
    emit_byte(gen, 0x48 | (reg1 >= 8 ? 0x01 : 0x00) | (reg2 >= 8 ? 0x04 : 0x00));
    emit_byte(gen, 0x39);
    emit_byte(gen, 0xC0 | ((reg2 & 7) << 3) | (reg1 & 7));
}

// SETcc reg
static void emit_setcc(CodeGen *gen, int cc, int reg) {
    if (reg >= 4) {
        emit_byte(gen, reg >= 8 ? 0x41 : 0x40);
    }
    emit_byte(gen, 0x0F);
    emit_byte(gen, 0x90 + cc);
    emit_byte(gen, 0xC0 | (reg & 7));
}

// MOVZX reg64, reg8
static void emit_movzx(CodeGen *gen, int dst, int src) {
    emit_byte(gen, 0x48 | (dst >= 8 ? 0x04 : 0x00) | (src >= 8 ? 0x01 : 0x00));
    emit_byte(gen, 0x0F);
    emit_byte(gen, 0xB6);
    emit_byte(gen, 0xC0 | ((dst & 7) << 3) | (src & 7));
}

// JMP rel32
static void emit_jmp(CodeGen *gen, const char *label) {
    emit_byte(gen, 0xE9);
    add_relocation(gen, label, 0);
    emit_int32(gen, 0);
}

// Jcc rel32
static void emit_jcc(CodeGen *gen, int cc, const char *label) {
    emit_byte(gen, 0x0F);
    emit_byte(gen, 0x80 + cc);
    add_relocation(gen, label, 0);
    emit_int32(gen, 0);
}

// CALL func
static void emit_call(CodeGen *gen, const char *func) {
    emit_byte(gen, 0xE8);
    add_relocation(gen, func, 0);
    emit_int32(gen, 0);
}

// RET
static void emit_ret(CodeGen *gen) {
    emit_byte(gen, 0xC3);
}

// ====================================
// 寄存器定义
// ====================================

typedef enum {
    RAX = 0, RCX = 1, RDX = 2, RBX = 3,
    RSP = 4, RBP = 5, RSI = 6, RDI = 7,
    R8 = 8, R9 = 9, R10 = 10, R11 = 11,
    R12 = 12, R13 = 13, R14 = 14, R15 = 15
} Register;

// ====================================
// 表达式代码生成
// ====================================

static void gen_expression(CodeGen *gen, ASTNode *node);

static void gen_binary_expr(CodeGen *gen, ASTNode *node) {
    // 计算左操作数
    gen_expression(gen, node->data.binary.left);
    emit_push(gen, RAX);
    
    // 计算右操作数
    gen_expression(gen, node->data.binary.right);
    
    // 右操作数在RAX，弹出左操作数到RCX
    emit_pop(gen, RCX);
    
    switch (node->data.binary.op) {
        case OP_ADD:
            emit_add(gen, RAX, RCX);
            break;
            
        case OP_SUB:
            // 交换操作数：结果 = 左 - 右
            emit_byte(gen, 0x48); // REX.W
            emit_byte(gen, 0x89); // MOV
            emit_byte(gen, 0xC2); // RDX, RAX
            emit_byte(gen, 0x48); // REX.W
            emit_byte(gen, 0x89); // MOV
            emit_byte(gen, 0xC8); // RAX, RCX
            emit_sub(gen, RAX, RDX);
            break;
            
        case OP_MUL:
            emit_imul(gen, RAX, RCX);
            break;
            
        case OP_DIV:
        case OP_MOD:
            // 交换操作数
            emit_byte(gen, 0x48); // REX.W
            emit_byte(gen, 0x89); // MOV
            emit_byte(gen, 0xC2); // RDX, RAX
            emit_byte(gen, 0x48); // REX.W
            emit_byte(gen, 0x89); // MOV
            emit_byte(gen, 0xC8); // RAX, RCX
            emit_cqo(gen);
            emit_idiv(gen, RDX);
            if (node->data.binary.op == OP_MOD) {
                // 余数在RDX
                emit_byte(gen, 0x48); // REX.W
                emit_byte(gen, 0x89); // MOV
                emit_byte(gen, 0xD0); // RAX, RDX
            }
            break;
            
        case OP_LT:
        case OP_GT:
        case OP_LE:
        case OP_GE:
        case OP_EQ:
        case OP_NE:
            emit_cmp(gen, RCX, RAX);
            int cc;
            switch (node->data.binary.op) {
                case OP_LT: cc = 0x0C; break; // L
                case OP_GT: cc = 0x0F; break; // G
                case OP_LE: cc = 0x0E; break; // LE
                case OP_GE: cc = 0x0D; break; // GE
                case OP_EQ: cc = 0x04; break; // E
                case OP_NE: cc = 0x05; break; // NE
            }
            emit_setcc(gen, cc, RAX);
            emit_movzx(gen, RAX, RAX);
            break;
            
        default:
            fprintf(stderr, "未实现的二元操作: %d\n", node->data.binary.op);
    }
}

static void gen_unary_expr(CodeGen *gen, ASTNode *node) {
    switch (node->data.unary.op) {
        case OP_MINUS:
            gen_expression(gen, node->data.unary.operand);
            // NEG RAX
            emit_byte(gen, 0x48);
            emit_byte(gen, 0xF7);
            emit_byte(gen, 0xD8);
            break;
            
        case OP_NOT:
            gen_expression(gen, node->data.unary.operand);
            // TEST RAX, RAX
            emit_byte(gen, 0x48);
            emit_byte(gen, 0x85);
            emit_byte(gen, 0xC0);
            // SETE AL
            emit_byte(gen, 0x0F);
            emit_byte(gen, 0x94);
            emit_byte(gen, 0xC0);
            // MOVZX RAX, AL
            emit_movzx(gen, RAX, RAX);
            break;
            
        default:
            fprintf(stderr, "未实现的一元操作: %d\n", node->data.unary.op);
    }
}

static void gen_assignment_expr(CodeGen *gen, ASTNode *node) {
    // 计算右值
    gen_expression(gen, node->data.assignment.right);
    
    // 存储到左值
    if (node->data.assignment.left->type == AST_IDENTIFIER) {
        const char *name = node->data.assignment.left->data.identifier.name;
        int offset = find_local(gen, name);
        if (offset != 0) {
            emit_mov_mem_reg(gen, offset, RAX);
        } else {
            fprintf(stderr, "未定义的变量: %s\n", name);
        }
    } else {
        fprintf(stderr, "不支持的左值类型\n");
    }
}

static void gen_call_expr(CodeGen *gen, ASTNode *node) {
    // 计算参数（简化：只支持前6个参数）
    const int arg_regs[] = {RDI, RSI, RDX, RCX, R8, R9};
    
    for (int i = node->data.call.arg_count - 1; i >= 0; i--) {
        gen_expression(gen, node->data.call.args[i]);
        emit_push(gen, RAX);
    }
    
    for (int i = 0; i < node->data.call.arg_count && i < 6; i++) {
        emit_pop(gen, arg_regs[i]);
    }
    
    // 调用函数
    if (node->data.call.function->type == AST_IDENTIFIER) {
        emit_call(gen, node->data.call.function->data.identifier.name);
    } else {
        fprintf(stderr, "不支持的函数调用形式\n");
    }
}

static void gen_expression(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_INTEGER_LITERAL:
            emit_mov_reg_imm(gen, RAX, node->value.int_val);
            break;
            
        case AST_IDENTIFIER:
            {
                const char *name = node->data.identifier.name;
                int offset = find_local(gen, name);
                if (offset != 0) {
                    emit_mov_reg_mem(gen, RAX, offset);
                } else {
                    fprintf(stderr, "未定义的变量: %s\n", name);
                    emit_mov_reg_imm(gen, RAX, 0);
                }
            }
            break;
            
        case AST_BINARY_EXPR:
            gen_binary_expr(gen, node);
            break;
            
        case AST_UNARY_EXPR:
            gen_unary_expr(gen, node);
            break;
            
        case AST_ASSIGNMENT_EXPR:
            gen_assignment_expr(gen, node);
            break;
            
        case AST_CALL_EXPR:
            gen_call_expr(gen, node);
            break;
            
        default:
            fprintf(stderr, "未实现的表达式类型: %d\n", node->type);
    }
}

// ====================================
// 语句代码生成
// ====================================

static void gen_statement(CodeGen *gen, ASTNode *node);

static void gen_compound_stmt(CodeGen *gen, ASTNode *node) {
    for (int i = 0; i < node->data.generic.child_count; i++) {
        gen_statement(gen, node->data.generic.children[i]);
    }
}

static void gen_expression_stmt(CodeGen *gen, ASTNode *node) {
    if (node->data.generic.child_count > 0) {
        gen_expression(gen, node->data.generic.children[0]);
    }
}

static void gen_if_stmt(CodeGen *gen, ASTNode *node) {
    static int if_counter = 0;
    char else_label[32], end_label[32];
    
    snprintf(else_label, sizeof(else_label), ".L_else_%d", if_counter);
    snprintf(end_label, sizeof(end_label), ".L_endif_%d", if_counter);
    if_counter++;
    
    // 计算条件
    gen_expression(gen, node->data.if_stmt.condition);
    
    // TEST RAX, RAX
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x85);
    emit_byte(gen, 0xC0);
    
    // JE else_label
    emit_jcc(gen, 0x04, else_label);
    
    // then分支
    gen_statement(gen, node->data.if_stmt.then_stmt);
    
    if (node->data.if_stmt.else_stmt) {
        emit_jmp(gen, end_label);
        add_label(gen, else_label);
        gen_statement(gen, node->data.if_stmt.else_stmt);
        add_label(gen, end_label);
    } else {
        add_label(gen, else_label);
    }
}

static void gen_while_stmt(CodeGen *gen, ASTNode *node) {
    static int while_counter = 0;
    char loop_label[32], end_label[32];
    
    snprintf(loop_label, sizeof(loop_label), ".L_while_%d", while_counter);
    snprintf(end_label, sizeof(end_label), ".L_endwhile_%d", while_counter);
    while_counter++;
    
    add_label(gen, loop_label);
    
    // 计算条件
    gen_expression(gen, node->data.while_stmt.condition);
    
    // TEST RAX, RAX
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x85);
    emit_byte(gen, 0xC0);
    
    // JE end_label
    emit_jcc(gen, 0x04, end_label);
    
    // 循环体
    gen_statement(gen, node->data.while_stmt.body);
    
    // JMP loop_label
    emit_jmp(gen, loop_label);
    
    add_label(gen, end_label);
}

static void gen_for_stmt(CodeGen *gen, ASTNode *node) {
    static int for_counter = 0;
    char loop_label[32], cond_label[32], end_label[32];
    
    snprintf(cond_label, sizeof(cond_label), ".L_for_cond_%d", for_counter);
    snprintf(loop_label, sizeof(loop_label), ".L_for_loop_%d", for_counter);
    snprintf(end_label, sizeof(end_label), ".L_for_end_%d", for_counter);
    for_counter++;
    
    // 初始化
    if (node->data.for_stmt.init) {
        if (node->data.for_stmt.init->type == AST_VAR_DECL) {
            gen_statement(gen, node->data.for_stmt.init);
        } else {
            gen_expression(gen, node->data.for_stmt.init);
        }
    }
    
    add_label(gen, cond_label);
    
    // 条件
    if (node->data.for_stmt.condition) {
        gen_expression(gen, node->data.for_stmt.condition);
        
        // TEST RAX, RAX
        emit_byte(gen, 0x48);
        emit_byte(gen, 0x85);
        emit_byte(gen, 0xC0);
        
        // JE end_label
        emit_jcc(gen, 0x04, end_label);
    }
    
    // 循环体
    gen_statement(gen, node->data.for_stmt.body);
    
    // 增量
    if (node->data.for_stmt.increment) {
        gen_expression(gen, node->data.for_stmt.increment);
    }
    
    // JMP cond_label
    emit_jmp(gen, cond_label);
    
    add_label(gen, end_label);
}

static void gen_return_stmt(CodeGen *gen, ASTNode *node) {
    if (node->data.return_stmt.value) {
        gen_expression(gen, node->data.return_stmt.value);
    } else {
        emit_mov_reg_imm(gen, RAX, 0);
    }
    
    // 直接生成函数尾声，不跳转
    // MOV RSP, RBP
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xEC);
    
    emit_pop(gen, RBP);
    emit_ret(gen);
}

static void gen_var_decl(CodeGen *gen, ASTNode *node) {
    // 分配局部变量
    add_local(gen, node->data.var_decl.name, node->type_info);
    
    // 如果有初始化
    if (node->data.var_decl.init) {
        gen_expression(gen, node->data.var_decl.init);
        int offset = find_local(gen, node->data.var_decl.name);
        emit_mov_mem_reg(gen, offset, RAX);
    }
}

static void gen_statement(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_COMPOUND_STMT:
            gen_compound_stmt(gen, node);
            break;
            
        case AST_EXPRESSION_STMT:
            gen_expression_stmt(gen, node);
            break;
            
        case AST_IF_STMT:
            gen_if_stmt(gen, node);
            break;
            
        case AST_WHILE_STMT:
            gen_while_stmt(gen, node);
            break;
            
        case AST_FOR_STMT:
            gen_for_stmt(gen, node);
            break;
            
        case AST_RETURN_STMT:
            gen_return_stmt(gen, node);
            break;
            
        case AST_VAR_DECL:
            gen_var_decl(gen, node);
            break;
            
        default:
            fprintf(stderr, "未实现的语句类型: %d\n", node->type);
    }
    
    // 处理链表中的下一个声明
    if (node->next) {
        gen_statement(gen, node->next);
    }
}

// ====================================
// 函数代码生成
// ====================================

static void gen_function(CodeGen *gen, ASTNode *node) {
    // 添加函数标签
    add_label(gen, node->data.function.name);
    
    // 重置局部变量
    gen->local_count = 0;
    gen->stack_offset = 0;
    gen->max_stack_size = 0;
    
    // 函数序言
    emit_push(gen, RBP);
    // MOV RBP, RSP
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xE5);
    
    // 保存函数序言后的位置，用于后续调整栈空间
    size_t stack_adjustment_pos = gen->size;
    // SUB RSP, imm32 (占位)
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x81);
    emit_byte(gen, 0xEC);
    emit_int32(gen, 0);
    
    // 处理参数
    const int arg_regs[] = {RDI, RSI, RDX, RCX, R8, R9};
    for (int i = 0; i < node->data.function.param_count && i < 6; i++) {
        ASTNode *param = node->data.function.params[i];
        if (param->type == AST_PARAM_DECL) {
            int offset = add_local(gen, param->data.var_decl.name, param->type_info);
            emit_mov_mem_reg(gen, offset, arg_regs[i]);
        }
    }
    
    // 生成函数体
    if (node->data.function.body) {
        gen_statement(gen, node->data.function.body);
    }
    
    // 如果函数体没有显式返回，添加默认返回
    // 检查最后生成的字节是否是RET (0xC3)
    if (gen->size == 0 || gen->code[gen->size - 1] != 0xC3) {
        // 函数结尾标签
        add_label(gen, ".L_func_epilogue");
        
        // 函数尾声
        // MOV RSP, RBP
        emit_byte(gen, 0x48);
        emit_byte(gen, 0x89);
        emit_byte(gen, 0xEC);
        
        emit_pop(gen, RBP);
        emit_ret(gen);
    }
    
    // 回填栈空间大小
    int32_t stack_size = (gen->max_stack_size + 15) & ~15; // 16字节对齐
    memcpy(gen->code + stack_adjustment_pos + 3, &stack_size, 4);
}

// ====================================
// 顶层代码生成
// ====================================

static void gen_translation_unit(CodeGen *gen, ASTNode *node) {
    bool has_main = false;
    
    // 第一遍：查找main函数
    for (int i = 0; i < node->data.generic.child_count; i++) {
        ASTNode *decl = node->data.generic.children[i];
        if (decl->type == AST_FUNCTION_DEF && 
            strcmp(decl->data.function.name, "main") == 0) {
            has_main = true;
            break;
        }
    }
    
    // 如果有main函数，先生成_start函数
    if (has_main) {
        // _start:
        add_label(gen, "_start");
        
        // 调用main
        emit_call(gen, "main");
        
        // 将返回值移到RDI作为退出码
        // MOV RDI, RAX
        emit_byte(gen, 0x48);
        emit_byte(gen, 0x89);
        emit_byte(gen, 0xC7);
        
        // 调用exit系统调用
        // MOV RAX, 60  ; exit系统调用号
        emit_mov_reg_imm(gen, RAX, 60);
        
        // SYSCALL
        emit_byte(gen, 0x0F);
        emit_byte(gen, 0x05);
    }
    
    // 第二遍：生成所有函数
    for (int i = 0; i < node->data.generic.child_count; i++) {
        ASTNode *decl = node->data.generic.children[i];
        
        if (decl->type == AST_FUNCTION_DEF) {
            gen_function(gen, decl);
        } else {
            fprintf(stderr, "忽略非函数定义的顶层声明\n");
        }
    }
    
    // 解析重定位
    resolve_relocations(gen);
}

// ====================================
// 公共接口
// ====================================

static uint8_t* generate_code(ASTNode *ast, size_t *code_size) {
    CodeGen gen;
    init_codegen(&gen);
    
    gen_translation_unit(&gen, ast);
    
    *code_size = gen.size;
    uint8_t *code = (uint8_t*)malloc(gen.size);
    memcpy(code, gen.code, gen.size);
    
    free_codegen(&gen);
    
    return code;
}

// ====================================
// 调试辅助
// ====================================

static void disassemble_code(const uint8_t *code, size_t size) {
    printf("=== 生成的机器码 ===\n");
    printf("大小: %zu 字节\n", size);
    
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            if (i > 0) printf("\n");
            printf("%04zx: ", i);
        }
        printf("%02x ", code[i]);
    }
    printf("\n");
    printf("==================\n");
}

#endif // EVOLVER0_CODEGEN_INC_C