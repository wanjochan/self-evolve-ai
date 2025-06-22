/**
 * evolver0_codegen.inc.c - 机器码生成模块
 * 这个文件被 evolver0.c 包含，提供x86-64机器码生成功能
 */

// ====================================
// x86-64 寄存器定义
// ====================================

typedef enum {
    REG_RAX = 0,
    REG_RCX = 1,
    REG_RDX = 2,
    REG_RBX = 3,
    REG_RSP = 4,
    REG_RBP = 5,
    REG_RSI = 6,
    REG_RDI = 7,
    REG_R8 = 8,
    REG_R9 = 9,
    REG_R10 = 10,
    REG_R11 = 11,
    REG_R12 = 12,
    REG_R13 = 13,
    REG_R14 = 14,
    REG_R15 = 15
} X86Register;

// ====================================
// 代码生成器结构体
// ====================================

typedef struct {
    unsigned char *code;    // 机器码缓冲区
    size_t size;           // 当前代码大小
    size_t capacity;       // 缓冲区容量
    
    // 标签管理
    struct {
        char *name;
        size_t offset;
    } *labels;
    int label_count;
    int label_capacity;
    
    // 函数信息
    struct {
        char *name;
        size_t start_offset;
        size_t stack_size;
        int param_count;
    } *functions;
    int function_count;
    int function_capacity;
    int current_function;
    
    // 局部变量管理
    struct {
        char *name;
        int offset;     // 相对于RBP的偏移
        Type *type;
    } *locals;
    int local_count;
    int local_capacity;
    int stack_offset;
    
    // 字符串常量
    struct {
        char *str;
        size_t offset;
    } *strings;
    int string_count;
    int string_capacity;
} CodeGenerator;

// ====================================
// 机器码生成辅助函数
// ====================================

static void codegen_init(CodeGenerator *gen) {
    gen->code = malloc(4096);
    gen->capacity = 4096;
    gen->size = 0;
    
    gen->labels = NULL;
    gen->label_count = 0;
    gen->label_capacity = 0;
    
    gen->functions = NULL;
    gen->function_count = 0;
    gen->function_capacity = 0;
    gen->current_function = -1;
    
    gen->locals = NULL;
    gen->local_count = 0;
    gen->local_capacity = 0;
    gen->stack_offset = 0;
    
    gen->strings = NULL;
    gen->string_count = 0;
    gen->string_capacity = 0;
}

static void codegen_free(CodeGenerator *gen) {
    free(gen->code);
    
    for (int i = 0; i < gen->label_count; i++) {
        free(gen->labels[i].name);
    }
    free(gen->labels);
    
    for (int i = 0; i < gen->function_count; i++) {
        free(gen->functions[i].name);
    }
    free(gen->functions);
    
    for (int i = 0; i < gen->local_count; i++) {
        free(gen->locals[i].name);
    }
    free(gen->locals);
    
    for (int i = 0; i < gen->string_count; i++) {
        free(gen->strings[i].str);
    }
    free(gen->strings);
}

static void codegen_ensure_capacity(CodeGenerator *gen, size_t needed) {
    if (gen->size + needed > gen->capacity) {
        gen->capacity = (gen->size + needed) * 2;
        gen->code = realloc(gen->code, gen->capacity);
    }
}

static void emit_bytes(CodeGenerator *gen, const unsigned char *bytes, size_t count) {
    codegen_ensure_capacity(gen, count);
    memcpy(gen->code + gen->size, bytes, count);
    gen->size += count;
}

static void emit_byte(CodeGenerator *gen, unsigned char byte) {
    emit_bytes(gen, &byte, 1);
}

static void emit_word(CodeGenerator *gen, unsigned short word) {
    emit_bytes(gen, (unsigned char*)&word, 2);
}

static void emit_dword(CodeGenerator *gen, unsigned int dword) {
    emit_bytes(gen, (unsigned char*)&dword, 4);
}

static void emit_qword(CodeGenerator *gen, unsigned long long qword) {
    emit_bytes(gen, (unsigned char*)&qword, 8);
}

// ====================================
// x86-64 指令编码
// ====================================

// REX前缀
static unsigned char rex_prefix(int w, int r, int x, int b) {
    return 0x40 | (w << 3) | (r << 2) | (x << 1) | b;
}

// ModR/M字节
static unsigned char modrm_byte(int mod, int reg, int rm) {
    return (mod << 6) | ((reg & 7) << 3) | (rm & 7);
}

// SIB字节
static unsigned char sib_byte(int scale, int index, int base) {
    return (scale << 6) | ((index & 7) << 3) | (base & 7);
}

// PUSH指令
static void emit_push(CodeGenerator *gen, X86Register reg) {
    if (reg >= REG_R8) {
        emit_byte(gen, rex_prefix(0, 0, 0, 1));
    }
    emit_byte(gen, 0x50 + (reg & 7));
}

// POP指令
static void emit_pop(CodeGenerator *gen, X86Register reg) {
    if (reg >= REG_R8) {
        emit_byte(gen, rex_prefix(0, 0, 0, 1));
    }
    emit_byte(gen, 0x58 + (reg & 7));
}

// MOV reg, imm64
static void emit_mov_reg_imm64(CodeGenerator *gen, X86Register reg, long long value) {
    emit_byte(gen, rex_prefix(1, 0, 0, reg >= REG_R8));
    emit_byte(gen, 0xB8 + (reg & 7));
    emit_qword(gen, value);
}

// MOV reg, imm32 (符号扩展到64位)
static void emit_mov_reg_imm32(CodeGenerator *gen, X86Register reg, int value) {
    if (reg >= REG_R8) {
        emit_byte(gen, rex_prefix(1, 0, 0, 1));
    } else {
        emit_byte(gen, 0x48); // REX.W
    }
    emit_byte(gen, 0xC7);
    emit_byte(gen, modrm_byte(3, 0, reg & 7));
    emit_dword(gen, value);
}

// MOV reg1, reg2
static void emit_mov_reg_reg(CodeGenerator *gen, X86Register dst, X86Register src) {
    emit_byte(gen, rex_prefix(1, src >= REG_R8, 0, dst >= REG_R8));
    emit_byte(gen, 0x89);
    emit_byte(gen, modrm_byte(3, src & 7, dst & 7));
}

// MOV [reg+offset], reg
static void emit_mov_mem_reg(CodeGenerator *gen, X86Register base, int offset, X86Register src) {
    emit_byte(gen, rex_prefix(1, src >= REG_R8, 0, base >= REG_R8));
    emit_byte(gen, 0x89);
    
    if (offset == 0 && (base & 7) != REG_RBP) {
        emit_byte(gen, modrm_byte(0, src & 7, base & 7));
    } else if (offset >= -128 && offset <= 127) {
        emit_byte(gen, modrm_byte(1, src & 7, base & 7));
        emit_byte(gen, offset);
    } else {
        emit_byte(gen, modrm_byte(2, src & 7, base & 7));
        emit_dword(gen, offset);
    }
    
    // 处理RSP和R12需要SIB字节
    if ((base & 7) == REG_RSP) {
        emit_byte(gen, sib_byte(0, REG_RSP, REG_RSP));
    }
}

// MOV reg, [reg+offset]
static void emit_mov_reg_mem(CodeGenerator *gen, X86Register dst, X86Register base, int offset) {
    emit_byte(gen, rex_prefix(1, dst >= REG_R8, 0, base >= REG_R8));
    emit_byte(gen, 0x8B);
    
    if (offset == 0 && (base & 7) != REG_RBP) {
        emit_byte(gen, modrm_byte(0, dst & 7, base & 7));
    } else if (offset >= -128 && offset <= 127) {
        emit_byte(gen, modrm_byte(1, dst & 7, base & 7));
        emit_byte(gen, offset);
    } else {
        emit_byte(gen, modrm_byte(2, dst & 7, base & 7));
        emit_dword(gen, offset);
    }
    
    // 处理RSP和R12需要SIB字节
    if ((base & 7) == REG_RSP) {
        emit_byte(gen, sib_byte(0, REG_RSP, REG_RSP));
    }
}

// ADD reg1, reg2
static void emit_add_reg_reg(CodeGenerator *gen, X86Register dst, X86Register src) {
    emit_byte(gen, rex_prefix(1, src >= REG_R8, 0, dst >= REG_R8));
    emit_byte(gen, 0x01);
    emit_byte(gen, modrm_byte(3, src & 7, dst & 7));
}

// SUB reg1, reg2
static void emit_sub_reg_reg(CodeGenerator *gen, X86Register dst, X86Register src) {
    emit_byte(gen, rex_prefix(1, src >= REG_R8, 0, dst >= REG_R8));
    emit_byte(gen, 0x29);
    emit_byte(gen, modrm_byte(3, src & 7, dst & 7));
}

// IMUL reg1, reg2
static void emit_imul_reg_reg(CodeGenerator *gen, X86Register dst, X86Register src) {
    emit_byte(gen, rex_prefix(1, dst >= REG_R8, 0, src >= REG_R8));
    emit_byte(gen, 0x0F);
    emit_byte(gen, 0xAF);
    emit_byte(gen, modrm_byte(3, dst & 7, src & 7));
}

// CQO (符号扩展RAX到RDX:RAX)
static void emit_cqo(CodeGenerator *gen) {
    emit_byte(gen, 0x48);
    emit_byte(gen, 0x99);
}

// IDIV reg
static void emit_idiv_reg(CodeGenerator *gen, X86Register reg) {
    emit_byte(gen, rex_prefix(1, 0, 0, reg >= REG_R8));
    emit_byte(gen, 0xF7);
    emit_byte(gen, modrm_byte(3, 7, reg & 7));
}

// CMP reg1, reg2
static void emit_cmp_reg_reg(CodeGenerator *gen, X86Register reg1, X86Register reg2) {
    emit_byte(gen, rex_prefix(1, reg2 >= REG_R8, 0, reg1 >= REG_R8));
    emit_byte(gen, 0x39);
    emit_byte(gen, modrm_byte(3, reg2 & 7, reg1 & 7));
}

// JMP rel32
static void emit_jmp(CodeGenerator *gen, int offset) {
    emit_byte(gen, 0xE9);
    emit_dword(gen, offset - 5); // 相对于下一条指令
}

// Jcc rel32 (条件跳转)
static void emit_jcc(CodeGenerator *gen, unsigned char condition, int offset) {
    emit_byte(gen, 0x0F);
    emit_byte(gen, 0x80 | condition);
    emit_dword(gen, offset - 6); // 相对于下一条指令
}

// CALL rel32
static void emit_call(CodeGenerator *gen, int offset) {
    emit_byte(gen, 0xE8);
    emit_dword(gen, offset - 5); // 相对于下一条指令
}

// RET
static void emit_ret(CodeGenerator *gen) {
    emit_byte(gen, 0xC3);
}

// SYSCALL
static void emit_syscall(CodeGenerator *gen) {
    emit_byte(gen, 0x0F);
    emit_byte(gen, 0x05);
}

// ====================================
// 函数序言和尾声
// ====================================

static void emit_function_prologue(CodeGenerator *gen) {
    // push rbp
    emit_push(gen, REG_RBP);
    // mov rbp, rsp
    emit_mov_reg_reg(gen, REG_RBP, REG_RSP);
    
    // 预留局部变量空间（稍后调整）
    gen->stack_offset = 0;
}

static void emit_function_epilogue(CodeGenerator *gen) {
    // mov rsp, rbp
    emit_mov_reg_reg(gen, REG_RSP, REG_RBP);
    // pop rbp
    emit_pop(gen, REG_RBP);
    // ret
    emit_ret(gen);
}

// 调整栈空间
static void emit_stack_adjustment(CodeGenerator *gen, int bytes) {
    if (bytes == 0) return;
    
    emit_byte(gen, 0x48); // REX.W
    emit_byte(gen, 0x81);
    
    if (bytes > 0) {
        // sub rsp, bytes
        emit_byte(gen, modrm_byte(3, 5, REG_RSP));
    } else {
        // add rsp, -bytes
        emit_byte(gen, modrm_byte(3, 0, REG_RSP));
        bytes = -bytes;
    }
    
    emit_dword(gen, bytes);
}

// ====================================
// AST到机器码的转换
// ====================================

static void codegen_expression(CodeGenerator *gen, ASTNode *expr);
static void codegen_statement(CodeGenerator *gen, ASTNode *stmt);
static void codegen_declaration(CodeGenerator *gen, ASTNode *decl);

// 查找局部变量
static int find_local_variable(CodeGenerator *gen, const char *name) {
    for (int i = 0; i < gen->local_count; i++) {
        if (strcmp(gen->locals[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// 添加局部变量
static int add_local_variable(CodeGenerator *gen, const char *name, Type *type) {
    if (gen->local_count >= gen->local_capacity) {
        gen->local_capacity = gen->local_capacity ? gen->local_capacity * 2 : 8;
        gen->locals = realloc(gen->locals, gen->local_capacity * sizeof(gen->locals[0]));
    }
    
    int size = type ? type->size : 8; // 默认8字节
    gen->stack_offset += size;
    
    gen->locals[gen->local_count].name = strdup(name);
    gen->locals[gen->local_count].offset = -gen->stack_offset;
    gen->locals[gen->local_count].type = type;
    
    return gen->local_count++;
}

// 生成表达式的代码（结果在RAX中）
static void codegen_expression(CodeGenerator *gen, ASTNode *expr) {
    if (!expr) return;
    
    switch (expr->node_type) {
        case NODE_INTEGER_LITERAL:
            // 加载整数常量到RAX
            emit_mov_reg_imm32(gen, REG_RAX, expr->value.int_val);
            break;
            
        case NODE_IDENTIFIER: {
            // 加载变量值到RAX
            int var_idx = find_local_variable(gen, expr->data.decl.name);
            if (var_idx >= 0) {
                emit_mov_reg_mem(gen, REG_RAX, REG_RBP, gen->locals[var_idx].offset);
            } else {
                // 假设是全局变量或函数，暂时返回0
                emit_mov_reg_imm32(gen, REG_RAX, 0);
            }
            break;
        }
            
        case NODE_BINARY_OP: {
            // 计算左操作数（结果在RAX）
            codegen_expression(gen, expr->data.expr.lhs);
            // 保存RAX到栈上
            emit_push(gen, REG_RAX);
            
            // 计算右操作数（结果在RAX）
            codegen_expression(gen, expr->data.expr.rhs);
            
            // 将右操作数移到RCX
            emit_mov_reg_reg(gen, REG_RCX, REG_RAX);
            // 恢复左操作数到RAX
            emit_pop(gen, REG_RAX);
            
            // 执行操作
            switch (expr->data.expr.expr_type) {
                case EXPR_BINARY:
                    // 需要更具体的操作符信息
                    emit_add_reg_reg(gen, REG_RAX, REG_RCX);
                    break;
                default:
                    emit_add_reg_reg(gen, REG_RAX, REG_RCX);
                    break;
            }
            break;
        }
            
        case NODE_ASSIGNMENT: {
            // 计算右值（结果在RAX）
            codegen_expression(gen, expr->data.expr.rhs);
            
            // 存储到左值
            if (expr->data.expr.lhs->node_type == NODE_IDENTIFIER) {
                int var_idx = find_local_variable(gen, expr->data.expr.lhs->data.decl.name);
                if (var_idx >= 0) {
                    emit_mov_mem_reg(gen, REG_RBP, gen->locals[var_idx].offset, REG_RAX);
                }
            }
            // 赋值表达式的值是右值
            break;
        }
            
        case NODE_FUNCTION_CALL: {
            // 简化的函数调用：只支持printf("Hello, World!\n")
            if (expr->data.expr.lhs->node_type == NODE_IDENTIFIER &&
                strcmp(expr->data.expr.lhs->data.decl.name, "printf") == 0) {
                
                // 系统调用write
                emit_mov_reg_imm32(gen, REG_RAX, 1);  // sys_write
                emit_mov_reg_imm32(gen, REG_RDI, 1);  // stdout
                
                // 假设第一个参数是字符串字面量
                if (expr->data.expr.num_args > 0 && 
                    expr->data.expr.args[0]->node_type == NODE_STRING_LITERAL) {
                    // 暂时使用固定地址
                    emit_mov_reg_imm64(gen, REG_RSI, 0x402000); // 字符串地址
                    emit_mov_reg_imm32(gen, REG_RDX, 
                        strlen(expr->data.expr.args[0]->value.str_val.str));
                }
                
                emit_syscall(gen);
            }
            break;
        }
            
        default:
            // 其他表达式类型暂不支持
            emit_mov_reg_imm32(gen, REG_RAX, 0);
            break;
    }
}

// 生成语句的代码
static void codegen_statement(CodeGenerator *gen, ASTNode *stmt) {
    if (!stmt) return;
    
    switch (stmt->node_type) {
        case NODE_COMPOUND_STMT: {
            // 复合语句
            if (stmt->data.stmt.body) {
                ASTNode **stmts = (ASTNode**)stmt->data.stmt.body;
                int count = stmt->data.stmt.has_else;
                for (int i = 0; i < count; i++) {
                    codegen_statement(gen, stmts[i]);
                }
            }
            break;
        }
            
        case NODE_EXPRESSION_STMT:
            // 表达式语句
            if (stmt->data.stmt.cond) {
                codegen_expression(gen, stmt->data.stmt.cond);
            }
            break;
            
        case NODE_RETURN_STMT:
            // return语句
            if (stmt->data.stmt.cond) {
                codegen_expression(gen, stmt->data.stmt.cond);
            } else {
                emit_mov_reg_imm32(gen, REG_RAX, 0);
            }
            emit_function_epilogue(gen);
            break;
            
        case NODE_IF_STMT: {
            // if语句
            size_t else_label = gen->size; // 临时标签
            size_t end_label = gen->size;
            
            // 计算条件
            codegen_expression(gen, stmt->data.stmt.cond);
            
            // 测试结果
            emit_byte(gen, 0x48); // REX.W
            emit_byte(gen, 0x85); // TEST
            emit_byte(gen, modrm_byte(3, REG_RAX, REG_RAX));
            
            // 如果为假，跳转到else部分
            size_t jz_offset = gen->size;
            emit_byte(gen, 0x0F);
            emit_byte(gen, 0x84); // JZ
            emit_dword(gen, 0); // 占位符
            
            // then部分
            codegen_statement(gen, stmt->data.stmt.then);
            
            if (stmt->data.stmt.else_) {
                // 跳过else部分
                size_t jmp_offset = gen->size;
                emit_byte(gen, 0xE9); // JMP
                emit_dword(gen, 0); // 占位符
                
                // 修正JZ跳转目标
                else_label = gen->size;
                *(int*)(gen->code + jz_offset + 2) = else_label - (jz_offset + 6);
                
                // else部分
                codegen_statement(gen, stmt->data.stmt.else_);
                
                // 修正JMP跳转目标
                end_label = gen->size;
                *(int*)(gen->code + jmp_offset + 1) = end_label - (jmp_offset + 5);
            } else {
                // 修正JZ跳转目标
                end_label = gen->size;
                *(int*)(gen->code + jz_offset + 2) = end_label - (jz_offset + 6);
            }
            break;
        }
            
        case NODE_WHILE_STMT: {
            // while循环
            size_t loop_start = gen->size;
            
            // 计算条件
            codegen_expression(gen, stmt->data.stmt.cond);
            
            // 测试结果
            emit_byte(gen, 0x48); // REX.W
            emit_byte(gen, 0x85); // TEST
            emit_byte(gen, modrm_byte(3, REG_RAX, REG_RAX));
            
            // 如果为假，跳出循环
            size_t jz_offset = gen->size;
            emit_byte(gen, 0x0F);
            emit_byte(gen, 0x84); // JZ
            emit_dword(gen, 0); // 占位符
            
            // 循环体
            codegen_statement(gen, stmt->data.stmt.body);
            
            // 跳回循环开始
            emit_byte(gen, 0xE9); // JMP
            emit_dword(gen, loop_start - (gen->size + 4));
            
            // 修正JZ跳转目标
            size_t loop_end = gen->size;
            *(int*)(gen->code + jz_offset + 2) = loop_end - (jz_offset + 6);
            break;
        }
            
        case NODE_DECLARATION:
            // 处理局部变量声明
            codegen_declaration(gen, stmt);
            break;
            
        default:
            // 其他语句类型暂不支持
            break;
    }
}

// 生成声明的代码
static void codegen_declaration(CodeGenerator *gen, ASTNode *decl) {
    if (!decl) return;
    
    switch (decl->node_type) {
        case NODE_DECLARATION:
            // 变量声明
            if (decl->data.decl.name) {
                add_local_variable(gen, decl->data.decl.name, 
                                 decl->data.decl.type ? decl->data.decl.type->type : NULL);
                
                // 如果有初始化器
                if (decl->data.decl.init) {
                    // 创建一个赋值表达式
                    ASTNode assign = {
                        .node_type = NODE_ASSIGNMENT,
                        .data.expr.lhs = create_identifier_node(decl->data.decl.name, 0, 0),
                        .data.expr.rhs = decl->data.decl.init
                    };
                    codegen_expression(gen, &assign);
                }
            }
            break;
            
        case NODE_FUNCTION_DECL:
        case NODE_FUNCTION_DEF:
            // 函数定义
            if (decl->data.decl.name && decl->data.decl.body) {
                // 添加函数到函数表
                if (gen->function_count >= gen->function_capacity) {
                    gen->function_capacity = gen->function_capacity ? gen->function_capacity * 2 : 8;
                    gen->functions = realloc(gen->functions, 
                                           gen->function_capacity * sizeof(gen->functions[0]));
                }
                
                gen->current_function = gen->function_count;
                gen->functions[gen->function_count].name = strdup(decl->data.decl.name);
                gen->functions[gen->function_count].start_offset = gen->size;
                gen->function_count++;
                
                // 重置局部变量
                gen->local_count = 0;
                gen->stack_offset = 0;
                
                // 生成函数序言
                emit_function_prologue(gen);
                
                // 记录栈调整位置
                size_t stack_adjust_offset = gen->size;
                emit_stack_adjustment(gen, 0); // 占位符
                
                // 生成函数体
                codegen_statement(gen, decl->data.decl.body);
                
                // 如果没有显式return，添加默认的
                if (gen->code[gen->size - 1] != 0xC3) { // RET
                    emit_mov_reg_imm32(gen, REG_RAX, 0);
                    emit_function_epilogue(gen);
                }
                
                // 修正栈调整
                if (gen->stack_offset > 0) {
                    // 对齐到16字节
                    int stack_size = (gen->stack_offset + 15) & ~15;
                    gen->functions[gen->current_function].stack_size = stack_size;
                    
                    // 更新栈调整指令
                    gen->code[stack_adjust_offset] = 0x48; // REX.W
                    gen->code[stack_adjust_offset + 1] = 0x81; // SUB
                    gen->code[stack_adjust_offset + 2] = modrm_byte(3, 5, REG_RSP);
                    *(int*)(gen->code + stack_adjust_offset + 3) = stack_size;
                }
            }
            break;
            
        default:
            break;
    }
}

// ====================================
// 主代码生成函数
// ====================================

int generate_x86_64_code(ASTNode *ast, unsigned char **code_out, size_t *size_out) {
    CodeGenerator gen;
    codegen_init(&gen);
    
    // 生成代码段开始的填充（用于对齐）
    emit_byte(&gen, 0x90); // NOP
    
    // 处理翻译单元
    if (ast && ast->node_type == NODE_TRANSLATION_UNIT) {
        if (ast->data.stmt.body) {
            ASTNode **decls = (ASTNode**)ast->data.stmt.body;
            int count = ast->data.stmt.has_else;
            
            for (int i = 0; i < count; i++) {
                if (decls[i]->node_type == NODE_FUNCTION_DEF ||
                    decls[i]->node_type == NODE_FUNCTION_DECL) {
                    codegen_declaration(&gen, decls[i]);
                }
            }
        }
    }
    
    // 找到main函数
    int main_offset = -1;
    for (int i = 0; i < gen.function_count; i++) {
        if (strcmp(gen.functions[i].name, "main") == 0) {
            main_offset = gen.functions[i].start_offset;
            break;
        }
    }
    
    if (main_offset < 0) {
        // 如果没有main函数，生成一个默认的
        emit_mov_reg_imm32(&gen, REG_RAX, 42);
        emit_ret(&gen);
        main_offset = 1; // 跳过开头的NOP
    }
    
    *code_out = gen.code;
    *size_out = gen.size;
    
    // 不释放gen.code，因为它被返回了
    // 但需要释放其他资源
    for (int i = 0; i < gen.label_count; i++) {
        free(gen.labels[i].name);
    }
    free(gen.labels);
    
    for (int i = 0; i < gen.function_count; i++) {
        free(gen.functions[i].name);
    }
    free(gen.functions);
    
    for (int i = 0; i < gen.local_count; i++) {
        free(gen.locals[i].name);
    }
    free(gen.locals);
    
    for (int i = 0; i < gen.string_count; i++) {
        free(gen.strings[i].str);
    }
    free(gen.strings);
    
    return main_offset;
}

// ====================================
// ELF文件生成
// ====================================

#define ELF_HEADER_SIZE 64
#define PROGRAM_HEADER_SIZE 56
#define SECTION_HEADER_SIZE 64

typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

int generate_elf_executable(const char *filename, unsigned char *code, size_t code_size, int entry_offset) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        return -1;
    }
    
    // 基地址
    const uint64_t base_addr = 0x400000;
    const uint64_t code_addr = base_addr + 0x1000;
    
    // ELF头
    Elf64_Ehdr ehdr = {
        .e_ident = {
            0x7F, 'E', 'L', 'F',  // 魔数
            2,                     // 64位
            1,                     // 小端
            1,                     // 当前版本
            0,                     // System V ABI
            0,                     // ABI版本
            0, 0, 0, 0, 0, 0, 0  // 填充
        },
        .e_type = 2,              // ET_EXEC
        .e_machine = 0x3E,        // x86-64
        .e_version = 1,           // 当前版本
        .e_entry = code_addr + entry_offset,
        .e_phoff = ELF_HEADER_SIZE,
        .e_shoff = 0,             // 没有节头表
        .e_flags = 0,
        .e_ehsize = ELF_HEADER_SIZE,
        .e_phentsize = PROGRAM_HEADER_SIZE,
        .e_phnum = 2,             // 两个程序头
        .e_shentsize = 0,
        .e_shnum = 0,
        .e_shstrndx = 0
    };
    
    // 程序头1：加载ELF头和程序头
    Elf64_Phdr phdr1 = {
        .p_type = 1,              // PT_LOAD
        .p_flags = 4,             // PF_R
        .p_offset = 0,
        .p_vaddr = base_addr,
        .p_paddr = base_addr,
        .p_filesz = ELF_HEADER_SIZE + 2 * PROGRAM_HEADER_SIZE,
        .p_memsz = ELF_HEADER_SIZE + 2 * PROGRAM_HEADER_SIZE,
        .p_align = 0x1000
    };
    
    // 程序头2：加载代码段
    Elf64_Phdr phdr2 = {
        .p_type = 1,              // PT_LOAD
        .p_flags = 5,             // PF_R | PF_X
        .p_offset = 0x1000,
        .p_vaddr = code_addr,
        .p_paddr = code_addr,
        .p_filesz = code_size,
        .p_memsz = code_size,
        .p_align = 0x1000
    };
    
    // 写入ELF头
    fwrite(&ehdr, 1, ELF_HEADER_SIZE, f);
    
    // 写入程序头
    fwrite(&phdr1, 1, PROGRAM_HEADER_SIZE, f);
    fwrite(&phdr2, 1, PROGRAM_HEADER_SIZE, f);
    
    // 填充到代码段
    long current_pos = ftell(f);
    while (current_pos < 0x1000) {
        fputc(0, f);
        current_pos++;
    }
    
    // 写入代码
    fwrite(code, 1, code_size, f);
    
    fclose(f);
    
    // 设置可执行权限
    chmod(filename, 0755);
    
    return 0;
}

// ====================================
// PE文件生成 (Windows可执行文件)
// ====================================

// PE文件格式常量
#define PE_DOS_HEADER_SIZE 64
#define PE_SIGNATURE_SIZE 4
#define PE_FILE_HEADER_SIZE 20
#define PE_OPTIONAL_HEADER_SIZE 224  // PE32格式
#define PE_SECTION_HEADER_SIZE 40

// PE文件格式结构体
typedef struct {
    // DOS头
    uint16_t e_magic;          // MZ签名
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;         // PE头偏移
} DOS_HEADER;

typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} PE_FILE_HEADER;

typedef struct {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
} PE_OPTIONAL_HEADER;

typedef struct {
    uint32_t VirtualAddress;
    uint32_t Size;
} PE_DATA_DIRECTORY;

typedef struct {
    uint8_t  Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} PE_SECTION_HEADER;

// DOS存根程序
static const unsigned char dos_stub[] = {
    0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68,
    0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72, 0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F,
    0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E, 0x20, 0x69, 0x6E, 0x20, 0x44, 0x4F, 0x53, 0x20,
    0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

int generate_pe_executable(const char *filename, unsigned char *code, size_t code_size, int entry_offset) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("无法创建文件");
        return -1;
    }

    // 基地址和对齐设置
    const uint64_t image_base = 0x140000000;  // 64位Windows默认基址
    const uint32_t section_alignment = 0x1000;  // 4KB
    const uint32_t file_alignment = 0x200;      // 512字节
    const uint32_t headers_size = 0x400;        // 头部大小1KB

    // 计算代码段大小（文件对齐）
    uint32_t code_size_aligned = (code_size + file_alignment - 1) & ~(file_alignment - 1);
    
    // 计算镜像大小（内存对齐）
    uint32_t code_virtual_size = (code_size + section_alignment - 1) & ~(section_alignment - 1);
    uint32_t image_size = section_alignment + code_virtual_size;

    // 1. DOS头
    DOS_HEADER dos_header = {0};
    dos_header.e_magic = 0x5A4D;  // "MZ"
    dos_header.e_lfanew = PE_DOS_HEADER_SIZE + sizeof(dos_stub);
    fwrite(&dos_header, 1, sizeof(DOS_HEADER), f);

    // 2. DOS存根
    fwrite(dos_stub, 1, sizeof(dos_stub), f);

    // 3. PE签名
    uint32_t pe_signature = 0x00004550;  // "PE\0\0"
    fwrite(&pe_signature, 1, sizeof(uint32_t), f);

    // 4. PE文件头
    PE_FILE_HEADER file_header = {0};
    file_header.Machine = 0x8664;  // x86-64
    file_header.NumberOfSections = 1;  // 只有一个代码段
    file_header.TimeDateStamp = (uint32_t)time(NULL);
    file_header.SizeOfOptionalHeader = sizeof(PE_OPTIONAL_HEADER) + 16 * sizeof(PE_DATA_DIRECTORY);
    file_header.Characteristics = 0x0022;  // 可执行 + 大地址感知
    fwrite(&file_header, 1, sizeof(PE_FILE_HEADER), f);

    // 5. PE可选头 (64位版本)
    PE_OPTIONAL_HEADER64 optional_header = {0};
    optional_header.Magic = 0x20B;  // PE32+ (64位)
    optional_header.MajorLinkerVersion = 1;
    optional_header.MinorLinkerVersion = 0;
    optional_header.SizeOfCode = code_size_aligned;
    optional_header.SizeOfInitializedData = 0;
    optional_header.SizeOfUninitializedData = 0;
    optional_header.AddressOfEntryPoint = section_alignment + entry_offset;  // 代码入口点
    optional_header.BaseOfCode = section_alignment;
    optional_header.ImageBase = image_base;
    optional_header.SectionAlignment = section_alignment;
    optional_header.FileAlignment = file_alignment;
    optional_header.MajorOperatingSystemVersion = 6;
    optional_header.MinorOperatingSystemVersion = 0;
    optional_header.MajorImageVersion = 0;
    optional_header.MinorImageVersion = 0;
    optional_header.MajorSubsystemVersion = 6;
    optional_header.MinorSubsystemVersion = 0;
    optional_header.Win32VersionValue = 0;
    optional_header.SizeOfImage = image_size;
    optional_header.SizeOfHeaders = headers_size;
    optional_header.CheckSum = 0;
    optional_header.Subsystem = 3;  // 控制台应用
    optional_header.DllCharacteristics = 0x140;  // 动态基址 + NX兼容
    optional_header.SizeOfStackReserve = 0x100000;  // 1MB
    optional_header.SizeOfStackCommit = 0x1000;     // 4KB
    optional_header.SizeOfHeapReserve = 0x100000;   // 1MB
    optional_header.SizeOfHeapCommit = 0x1000;      // 4KB
    optional_header.LoaderFlags = 0;
    optional_header.NumberOfRvaAndSizes = 16;
    fwrite(&optional_header, 1, sizeof(PE_OPTIONAL_HEADER64), f);

    // 6. 数据目录
    PE_DATA_DIRECTORY data_directories[16] = {0};
    fwrite(data_directories, 1, 16 * sizeof(PE_DATA_DIRECTORY), f);

    // 7. 节表
    PE_SECTION_HEADER section_header = {0};
    memcpy(section_header.Name, ".text\0\0\0", 8);  // 代码段名称
    section_header.VirtualSize = code_size;
    section_header.VirtualAddress = section_alignment;  // 第一个节从4KB开始
    section_header.SizeOfRawData = code_size_aligned;
    section_header.PointerToRawData = headers_size;
    section_header.PointerToRelocations = 0;
    section_header.PointerToLinenumbers = 0;
    section_header.NumberOfRelocations = 0;
    section_header.NumberOfLinenumbers = 0;
    section_header.Characteristics = 0x60000020;  // 代码段，可执行，可读
    fwrite(&section_header, 1, sizeof(PE_SECTION_HEADER), f);

    // 8. 填充到头部结束
    long current_pos = ftell(f);
    while (current_pos < headers_size) {
        fputc(0, f);
        current_pos++;
    }

    // 9. 写入代码段
    fwrite(code, 1, code_size, f);

    // 10. 填充到文件对齐
    current_pos = ftell(f);
    uint32_t aligned_end = (current_pos + file_alignment - 1) & ~(file_alignment - 1);
    while (current_pos < aligned_end) {
        fputc(0, f);
        current_pos++;
    }

    fclose(f);
    printf("成功生成PE可执行文件: %s\n", filename);
    return 0;
}

// 添加64位PE头结构定义
typedef struct {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;           // 注意：PE32+使用64位基址
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;  // 注意：PE32+使用64位大小
    uint64_t SizeOfStackCommit;   // 注意：PE32+使用64位大小
    uint64_t SizeOfHeapReserve;   // 注意：PE32+使用64位大小
    uint64_t SizeOfHeapCommit;    // 注意：PE32+使用64位大小
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
} PE_OPTIONAL_HEADER64;