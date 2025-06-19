/**
 * evolver0_simple_codegen.c - 简化的x86-64代码生成器
 * 用于evolver0第零代编译器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// x86-64 寄存器
typedef enum {
    RAX = 0, RCX = 1, RDX = 2, RBX = 3,
    RSP = 4, RBP = 5, RSI = 6, RDI = 7,
    R8 = 8, R9 = 9, R10 = 10, R11 = 11,
    R12 = 12, R13 = 13, R14 = 14, R15 = 15
} X64Register;

// 代码缓冲区
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
    
    // 局部变量
    struct {
        char *name;
        int offset;  // 相对于RBP的偏移
    } *locals;
    int local_count;
    int local_capacity;
    int stack_offset;
} CodeBuffer;

// 创建代码缓冲区
static CodeBuffer* create_code_buffer() {
    CodeBuffer *buf = (CodeBuffer*)calloc(1, sizeof(CodeBuffer));
    buf->capacity = 1024;
    buf->code = (unsigned char*)malloc(buf->capacity);
    return buf;
}

// 释放代码缓冲区
static void free_code_buffer(CodeBuffer *buf) {
    if (!buf) return;
    
    free(buf->code);
    
    for (int i = 0; i < buf->label_count; i++) {
        free(buf->labels[i].name);
    }
    free(buf->labels);
    
    for (int i = 0; i < buf->local_count; i++) {
        free(buf->locals[i].name);
    }
    free(buf->locals);
    
    free(buf);
}

// 确保缓冲区有足够空间
static void ensure_capacity(CodeBuffer *buf, size_t needed) {
    if (buf->size + needed > buf->capacity) {
        while (buf->size + needed > buf->capacity) {
            buf->capacity *= 2;
        }
        buf->code = (unsigned char*)realloc(buf->code, buf->capacity);
    }
}

// 发出字节
static void emit_byte(CodeBuffer *buf, unsigned char byte) {
    ensure_capacity(buf, 1);
    buf->code[buf->size++] = byte;
}

// 发出多个字节
static void emit_bytes(CodeBuffer *buf, const unsigned char *bytes, size_t count) {
    ensure_capacity(buf, count);
    memcpy(buf->code + buf->size, bytes, count);
    buf->size += count;
}

// 发出32位整数（小端序）
static void emit_int32(CodeBuffer *buf, int value) {
    emit_byte(buf, value & 0xFF);
    emit_byte(buf, (value >> 8) & 0xFF);
    emit_byte(buf, (value >> 16) & 0xFF);
    emit_byte(buf, (value >> 24) & 0xFF);
}

// 发出64位整数（小端序）
static void emit_int64(CodeBuffer *buf, long long value) {
    emit_int32(buf, (int)value);
    emit_int32(buf, (int)(value >> 32));
}

// REX前缀
static void emit_rex(CodeBuffer *buf, int w, int r, int x, int b) {
    unsigned char rex = 0x40;
    if (w) rex |= 0x08;
    if (r) rex |= 0x04;
    if (x) rex |= 0x02;
    if (b) rex |= 0x01;
    if (rex != 0x40) {
        emit_byte(buf, rex);
    }
}

// MOV reg, imm32
static void emit_mov_reg_imm32(CodeBuffer *buf, X64Register reg, int value) {
    if (reg >= R8) {
        emit_rex(buf, 0, 0, 0, 1);
    }
    emit_byte(buf, 0xB8 + (reg & 7));
    emit_int32(buf, value);
}

// MOV reg, imm64
static void emit_mov_reg_imm64(CodeBuffer *buf, X64Register reg, long long value) {
    emit_rex(buf, 1, 0, 0, reg >= R8);
    emit_byte(buf, 0xB8 + (reg & 7));
    emit_int64(buf, value);
}

// PUSH reg
static void emit_push(CodeBuffer *buf, X64Register reg) {
    if (reg >= R8) {
        emit_rex(buf, 0, 0, 0, 1);
    }
    emit_byte(buf, 0x50 + (reg & 7));
}

// POP reg
static void emit_pop(CodeBuffer *buf, X64Register reg) {
    if (reg >= R8) {
        emit_rex(buf, 0, 0, 0, 1);
    }
    emit_byte(buf, 0x58 + (reg & 7));
}

// ADD reg1, reg2
static void emit_add_reg_reg(CodeBuffer *buf, X64Register dst, X64Register src) {
    emit_rex(buf, 1, src >= R8, 0, dst >= R8);
    emit_byte(buf, 0x01);
    emit_byte(buf, 0xC0 | ((src & 7) << 3) | (dst & 7));
}

// SUB reg1, reg2
static void emit_sub_reg_reg(CodeBuffer *buf, X64Register dst, X64Register src) {
    emit_rex(buf, 1, src >= R8, 0, dst >= R8);
    emit_byte(buf, 0x29);
    emit_byte(buf, 0xC0 | ((src & 7) << 3) | (dst & 7));
}

// IMUL reg1, reg2
static void emit_imul_reg_reg(CodeBuffer *buf, X64Register dst, X64Register src) {
    emit_rex(buf, 1, dst >= R8, 0, src >= R8);
    emit_byte(buf, 0x0F);
    emit_byte(buf, 0xAF);
    emit_byte(buf, 0xC0 | ((dst & 7) << 3) | (src & 7));
}

// CQO (sign extend RAX to RDX:RAX)
static void emit_cqo(CodeBuffer *buf) {
    emit_rex(buf, 1, 0, 0, 0);
    emit_byte(buf, 0x99);
}

// IDIV reg
static void emit_idiv_reg(CodeBuffer *buf, X64Register reg) {
    emit_rex(buf, 1, 0, 0, reg >= R8);
    emit_byte(buf, 0xF7);
    emit_byte(buf, 0xF8 | (reg & 7));
}

// MOV reg1, reg2
static void emit_mov_reg_reg(CodeBuffer *buf, X64Register dst, X64Register src) {
    emit_rex(buf, 1, src >= R8, 0, dst >= R8);
    emit_byte(buf, 0x89);
    emit_byte(buf, 0xC0 | ((src & 7) << 3) | (dst & 7));
}

// CMP reg1, reg2
static void emit_cmp_reg_reg(CodeBuffer *buf, X64Register reg1, X64Register reg2) {
    emit_rex(buf, 1, reg2 >= R8, 0, reg1 >= R8);
    emit_byte(buf, 0x39);
    emit_byte(buf, 0xC0 | ((reg2 & 7) << 3) | (reg1 & 7));
}

// JMP rel32
static void emit_jmp(CodeBuffer *buf, int offset) {
    emit_byte(buf, 0xE9);
    emit_int32(buf, offset);
}

// JE rel32
static void emit_je(CodeBuffer *buf, int offset) {
    emit_byte(buf, 0x0F);
    emit_byte(buf, 0x84);
    emit_int32(buf, offset);
}

// JNE rel32
static void emit_jne(CodeBuffer *buf, int offset) {
    emit_byte(buf, 0x0F);
    emit_byte(buf, 0x85);
    emit_int32(buf, offset);
}

// JL rel32
static void emit_jl(CodeBuffer *buf, int offset) {
    emit_byte(buf, 0x0F);
    emit_byte(buf, 0x8C);
    emit_int32(buf, offset);
}

// JG rel32
static void emit_jg(CodeBuffer *buf, int offset) {
    emit_byte(buf, 0x0F);
    emit_byte(buf, 0x8F);
    emit_int32(buf, offset);
}

// JLE rel32
static void emit_jle(CodeBuffer *buf, int offset) {
    emit_byte(buf, 0x0F);
    emit_byte(buf, 0x8E);
    emit_int32(buf, offset);
}

// JGE rel32
static void emit_jge(CodeBuffer *buf, int offset) {
    emit_byte(buf, 0x0F);
    emit_byte(buf, 0x8D);
    emit_int32(buf, offset);
}

// RET
static void emit_ret(CodeBuffer *buf) {
    emit_byte(buf, 0xC3);
}

// 函数序言
static void emit_function_prologue(CodeBuffer *buf) {
    emit_push(buf, RBP);
    emit_mov_reg_reg(buf, RBP, RSP);
}

// 函数尾声
static void emit_function_epilogue(CodeBuffer *buf) {
    emit_mov_reg_reg(buf, RSP, RBP);
    emit_pop(buf, RBP);
    emit_ret(buf);
}

// SUB RSP, imm32
static void emit_sub_rsp_imm32(CodeBuffer *buf, int value) {
    emit_rex(buf, 1, 0, 0, 0);
    emit_byte(buf, 0x81);
    emit_byte(buf, 0xEC);
    emit_int32(buf, value);
}

// MOV [RBP-offset], reg
static void emit_mov_local_reg(CodeBuffer *buf, int offset, X64Register reg) {
    emit_rex(buf, 1, reg >= R8, 0, 0);
    emit_byte(buf, 0x89);
    emit_byte(buf, 0x85 | ((reg & 7) << 3));
    emit_int32(buf, -offset);
}

// MOV reg, [RBP-offset]
static void emit_mov_reg_local(CodeBuffer *buf, X64Register reg, int offset) {
    emit_rex(buf, 1, reg >= R8, 0, 0);
    emit_byte(buf, 0x8B);
    emit_byte(buf, 0x85 | ((reg & 7) << 3));
    emit_int32(buf, -offset);
}

// 查找或创建局部变量
static int get_or_create_local(CodeBuffer *buf, const char *name) {
    // 查找现有变量
    for (int i = 0; i < buf->local_count; i++) {
        if (strcmp(buf->locals[i].name, name) == 0) {
            return buf->locals[i].offset;
        }
    }
    
    // 创建新变量
    if (buf->local_count >= buf->local_capacity) {
        buf->local_capacity = buf->local_capacity ? buf->local_capacity * 2 : 8;
        buf->locals = realloc(buf->locals, buf->local_capacity * sizeof(buf->locals[0]));
    }
    
    buf->stack_offset += 8;
    buf->locals[buf->local_count].name = strdup(name);
    buf->locals[buf->local_count].offset = buf->stack_offset;
    buf->local_count++;
    
    return buf->stack_offset;
}

// 创建唯一标签
static char* create_label(CodeBuffer *buf, const char *prefix) {
    static int label_counter = 0;
    char *label = malloc(64);
    snprintf(label, 64, "%s_%d", prefix, label_counter++);
    return label;
}

// 定义标签
static void define_label(CodeBuffer *buf, const char *name) {
    if (buf->label_count >= buf->label_capacity) {
        buf->label_capacity = buf->label_capacity ? buf->label_capacity * 2 : 8;
        buf->labels = realloc(buf->labels, buf->label_capacity * sizeof(buf->labels[0]));
    }
    
    buf->labels[buf->label_count].name = strdup(name);
    buf->labels[buf->label_count].offset = buf->size;
    buf->label_count++;
}

// 获取标签偏移
static int get_label_offset(CodeBuffer *buf, const char *name) {
    for (int i = 0; i < buf->label_count; i++) {
        if (strcmp(buf->labels[i].name, name) == 0) {
            return buf->labels[i].offset;
        }
    }
    return -1;
}

// 前向声明
static void codegen_expr(CodeBuffer *buf, SimpleASTNode *expr);
static void codegen_stmt(CodeBuffer *buf, SimpleASTNode *stmt);

// 生成表达式代码
static void codegen_expr(CodeBuffer *buf, SimpleASTNode *expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_INTEGER:
            emit_mov_reg_imm32(buf, RAX, (int)expr->data.int_value);
            break;
            
        case AST_IDENTIFIER: {
            int offset = get_or_create_local(buf, expr->data.str_value);
            emit_mov_reg_local(buf, RAX, offset);
            break;
        }
        
        case AST_BINARY_OP:
            // 计算左操作数
            codegen_expr(buf, expr->data.binary.left);
            emit_push(buf, RAX);
            
            // 计算右操作数
            codegen_expr(buf, expr->data.binary.right);
            emit_mov_reg_reg(buf, RCX, RAX);
            emit_pop(buf, RAX);
            
            // 执行操作
            switch (expr->data.binary.op) {
                case '+':
                    emit_add_reg_reg(buf, RAX, RCX);
                    break;
                case '-':
                    emit_sub_reg_reg(buf, RAX, RCX);
                    break;
                case '*':
                    emit_imul_reg_reg(buf, RAX, RCX);
                    break;
                case '/':
                    emit_cqo(buf);
                    emit_idiv_reg(buf, RCX);
                    break;
                case '<':
                    emit_cmp_reg_reg(buf, RAX, RCX);
                    emit_mov_reg_imm32(buf, RAX, 0);
                    emit_byte(buf, 0x0F);  // SETL AL
                    emit_byte(buf, 0x9C);
                    emit_byte(buf, 0xC0);
                    break;
                case '>':
                    emit_cmp_reg_reg(buf, RAX, RCX);
                    emit_mov_reg_imm32(buf, RAX, 0);
                    emit_byte(buf, 0x0F);  // SETG AL
                    emit_byte(buf, 0x9F);
                    emit_byte(buf, 0xC0);
                    break;
                case 'L': // <=
                    emit_cmp_reg_reg(buf, RAX, RCX);
                    emit_mov_reg_imm32(buf, RAX, 0);
                    emit_byte(buf, 0x0F);  // SETLE AL
                    emit_byte(buf, 0x9E);
                    emit_byte(buf, 0xC0);
                    break;
                case 'G': // >=
                    emit_cmp_reg_reg(buf, RAX, RCX);
                    emit_mov_reg_imm32(buf, RAX, 0);
                    emit_byte(buf, 0x0F);  // SETGE AL
                    emit_byte(buf, 0x9D);
                    emit_byte(buf, 0xC0);
                    break;
                case 'E': // ==
                    emit_cmp_reg_reg(buf, RAX, RCX);
                    emit_mov_reg_imm32(buf, RAX, 0);
                    emit_byte(buf, 0x0F);  // SETE AL
                    emit_byte(buf, 0x94);
                    emit_byte(buf, 0xC0);
                    break;
                case 'N': // !=
                    emit_cmp_reg_reg(buf, RAX, RCX);
                    emit_mov_reg_imm32(buf, RAX, 0);
                    emit_byte(buf, 0x0F);  // SETNE AL
                    emit_byte(buf, 0x95);
                    emit_byte(buf, 0xC0);
                    break;
            }
            break;
            
        case AST_UNARY_OP:
            codegen_expr(buf, expr->data.unary.operand);
            switch (expr->data.unary.op) {
                case '-':
                    // NEG RAX
                    emit_rex(buf, 1, 0, 0, 0);
                    emit_byte(buf, 0xF7);
                    emit_byte(buf, 0xD8);
                    break;
                case '!':
                    // TEST RAX, RAX
                    emit_rex(buf, 1, 0, 0, 0);
                    emit_byte(buf, 0x85);
                    emit_byte(buf, 0xC0);
                    // SETE AL
                    emit_mov_reg_imm32(buf, RAX, 0);
                    emit_byte(buf, 0x0F);
                    emit_byte(buf, 0x94);
                    emit_byte(buf, 0xC0);
                    break;
                case '~':
                    // NOT RAX
                    emit_rex(buf, 1, 0, 0, 0);
                    emit_byte(buf, 0xF7);
                    emit_byte(buf, 0xD0);
                    break;
            }
            break;
            
        case AST_ASSIGNMENT: {
            codegen_expr(buf, expr->data.assign.value);
            int offset = get_or_create_local(buf, expr->data.assign.name);
            emit_mov_local_reg(buf, offset, RAX);
            break;
        }
        
        case AST_CALL:
            // 简化：只支持无参数的函数调用
            // 实际实现需要更复杂的调用约定处理
            fprintf(stderr, "函数调用暂未实现: %s\n", expr->data.call.name);
            emit_mov_reg_imm32(buf, RAX, 0);
            break;
            
        default:
            fprintf(stderr, "未知的表达式类型: %d\n", expr->type);
            break;
    }
}

// 生成语句代码
static void codegen_stmt(CodeBuffer *buf, SimpleASTNode *stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_RETURN:
            if (stmt->data.ret.value) {
                codegen_expr(buf, stmt->data.ret.value);
            }
            emit_function_epilogue(buf);
            break;
            
        case AST_COMPOUND:
            for (int i = 0; i < stmt->data.compound.count; i++) {
                codegen_stmt(buf, stmt->data.compound.statements[i]);
            }
            break;
            
        case AST_EXPRESSION_STMT:
            codegen_expr(buf, stmt->data.ret.value);
            break;
            
        case AST_DECLARATION:
            if (stmt->data.decl.init) {
                codegen_expr(buf, stmt->data.decl.init);
                int offset = get_or_create_local(buf, stmt->data.decl.name);
                emit_mov_local_reg(buf, offset, RAX);
            }
            break;
            
        case AST_IF: {
            char *else_label = create_label(buf, "else");
            char *end_label = create_label(buf, "endif");
            
            // 计算条件
            codegen_expr(buf, stmt->data.if_stmt.cond);
            
            // 测试条件
            emit_rex(buf, 1, 0, 0, 0);
            emit_byte(buf, 0x85);  // TEST RAX, RAX
            emit_byte(buf, 0xC0);
            
            // 跳转到else（如果条件为假）
            size_t je_pos = buf->size;
            emit_je(buf, 0);  // 占位符
            
            // then分支
            codegen_stmt(buf, stmt->data.if_stmt.then_stmt);
            
            if (stmt->data.if_stmt.else_stmt) {
                // 跳转到结束
                size_t jmp_pos = buf->size;
                emit_jmp(buf, 0);  // 占位符
                
                // else分支
                define_label(buf, else_label);
                
                // 修正je跳转
                int je_offset = buf->size - je_pos - 6;
                memcpy(buf->code + je_pos + 2, &je_offset, 4);
                
                codegen_stmt(buf, stmt->data.if_stmt.else_stmt);
                
                // 结束标签
                define_label(buf, end_label);
                
                // 修正jmp跳转
                int jmp_offset = buf->size - jmp_pos - 5;
                memcpy(buf->code + jmp_pos + 1, &jmp_offset, 4);
            } else {
                // 修正je跳转
                int je_offset = buf->size - je_pos - 6;
                memcpy(buf->code + je_pos + 2, &je_offset, 4);
            }
            
            free(else_label);
            free(end_label);
            break;
        }
        
        case AST_WHILE: {
            char *loop_label = create_label(buf, "while_loop");
            char *end_label = create_label(buf, "while_end");
            
            // 循环开始
            define_label(buf, loop_label);
            
            // 计算条件
            codegen_expr(buf, stmt->data.while_stmt.cond);
            
            // 测试条件
            emit_rex(buf, 1, 0, 0, 0);
            emit_byte(buf, 0x85);  // TEST RAX, RAX
            emit_byte(buf, 0xC0);
            
            // 如果条件为假，跳出循环
            size_t je_pos = buf->size;
            emit_je(buf, 0);  // 占位符
            
            // 循环体
            codegen_stmt(buf, stmt->data.while_stmt.body);
            
            // 跳回循环开始
            int loop_offset = get_label_offset(buf, loop_label);
            emit_jmp(buf, loop_offset - buf->size - 5);
            
            // 循环结束
            define_label(buf, end_label);
            
            // 修正je跳转
            int je_offset = buf->size - je_pos - 6;
            memcpy(buf->code + je_pos + 2, &je_offset, 4);
            
            free(loop_label);
            free(end_label);
            break;
        }
        
        case AST_FOR: {
            char *loop_label = create_label(buf, "for_loop");
            char *cond_label = create_label(buf, "for_cond");
            char *end_label = create_label(buf, "for_end");
            
            // 初始化
            if (stmt->data.for_stmt.init) {
                codegen_stmt(buf, stmt->data.for_stmt.init);
            }
            
            // 条件标签
            define_label(buf, cond_label);
            
            // 计算条件
            if (stmt->data.for_stmt.cond) {
                codegen_expr(buf, stmt->data.for_stmt.cond);
                
                // 测试条件
                emit_rex(buf, 1, 0, 0, 0);
                emit_byte(buf, 0x85);  // TEST RAX, RAX
                emit_byte(buf, 0xC0);
                
                // 如果条件为假，跳出循环
                size_t je_pos = buf->size;
                emit_je(buf, 0);  // 占位符
                
                // 循环体
                codegen_stmt(buf, stmt->data.for_stmt.body);
                
                // 增量
                if (stmt->data.for_stmt.inc) {
                    codegen_expr(buf, stmt->data.for_stmt.inc);
                }
                
                // 跳回条件检查
                int cond_offset = get_label_offset(buf, cond_label);
                emit_jmp(buf, cond_offset - buf->size - 5);
                
                // 循环结束
                define_label(buf, end_label);
                
                // 修正je跳转
                int je_offset = buf->size - je_pos - 6;
                memcpy(buf->code + je_pos + 2, &je_offset, 4);
            } else {
                // 无条件循环
                codegen_stmt(buf, stmt->data.for_stmt.body);
                
                // 增量
                if (stmt->data.for_stmt.inc) {
                    codegen_expr(buf, stmt->data.for_stmt.inc);
                }
                
                // 跳回循环开始
                int cond_offset = get_label_offset(buf, cond_label);
                emit_jmp(buf, cond_offset - buf->size - 5);
            }
            
            free(loop_label);
            free(cond_label);
            free(end_label);
            break;
        }
        
        default:
            fprintf(stderr, "未知的语句类型: %d\n", stmt->type);
            break;
    }
}

// 生成函数代码
static void codegen_function(CodeBuffer *buf, SimpleASTNode *func) {
    if (func->type != AST_FUNCTION) return;
    
    // 重置局部变量
    buf->local_count = 0;
    buf->stack_offset = 0;
    
    // 函数序言
    emit_function_prologue(buf);
    
    // 为局部变量预留空间（简化：固定分配256字节）
    emit_sub_rsp_imm32(buf, 256);
    
    // 生成函数体
    codegen_stmt(buf, func->data.function.body);
    
    // 确保有返回（如果函数没有显式返回）
    if (buf->size == 0 || buf->code[buf->size - 1] != 0xC3) {
        emit_mov_reg_imm32(buf, RAX, 0);
        emit_function_epilogue(buf);
    }
}

// 生成程序代码
unsigned char* generate_simple_code(SimpleASTNode *ast, size_t *out_size, int *entry_offset) {
    if (!ast || ast->type != AST_PROGRAM) {
        return NULL;
    }
    
    CodeBuffer *buf = create_code_buffer();
    
    // 查找main函数并生成代码
    int main_found = 0;
    for (int i = 0; i < ast->data.compound.count; i++) {
        SimpleASTNode *func = ast->data.compound.statements[i];
        if (func->type == AST_FUNCTION && strcmp(func->data.function.name, "main") == 0) {
            *entry_offset = buf->size;
            codegen_function(buf, func);
            main_found = 1;
            break;
        }
    }
    
    if (!main_found) {
        fprintf(stderr, "错误：未找到main函数\n");
        free_code_buffer(buf);
        return NULL;
    }
    
    // 返回生成的代码
    unsigned char *code = malloc(buf->size);
    memcpy(code, buf->code, buf->size);
    *out_size = buf->size;
    
    free_code_buffer(buf);
    return code;
}