/**
 * evolver0_codegen.inc.c - evolver0的代码生成模块
 * 这个文件被evolver0.c包含
 */

// ====================================
// x86-64 代码生成
// ====================================

typedef struct {
    unsigned char *code;    // 代码缓冲区
    size_t size;           // 当前代码大小
    size_t capacity;       // 缓冲区容量
    
    // 符号表
    struct {
        char *name;
        int offset;       // 相对于栈指针的偏移
        int is_global;
    } symbols[256];
    int symbol_count;
    
    // 栈帧信息
    int stack_offset;     // 当前栈偏移
    int local_count;      // 局部变量数量
} CodeGenerator;

// ====================================
// 代码生成辅助函数
// ====================================

static CodeGenerator* create_codegen() {
    CodeGenerator *gen = calloc(1, sizeof(CodeGenerator));
    gen->capacity = 4096;
    gen->code = malloc(gen->capacity);
    gen->size = 0;
    gen->stack_offset = 0;
    gen->local_count = 0;
    gen->symbol_count = 0;
    return gen;
}

static void free_codegen(CodeGenerator *gen) {
    if (!gen) return;
    free(gen->code);
    free(gen);
}

static void ensure_capacity(CodeGenerator *gen, size_t needed) {
    if (gen->size + needed > gen->capacity) {
        gen->capacity = (gen->capacity + needed) * 2;
        gen->code = realloc(gen->code, gen->capacity);
    }
}

static void emit(CodeGenerator *gen, unsigned char byte) {
    ensure_capacity(gen, 1);
    gen->code[gen->size++] = byte;
}

static void emit_bytes(CodeGenerator *gen, const unsigned char *bytes, size_t count) {
    ensure_capacity(gen, count);
    memcpy(gen->code + gen->size, bytes, count);
    gen->size += count;
}

static void emit_int32(CodeGenerator *gen, int32_t value) {
    emit(gen, value & 0xFF);
    emit(gen, (value >> 8) & 0xFF);
    emit(gen, (value >> 16) & 0xFF);
    emit(gen, (value >> 24) & 0xFF);
}

static void emit_int64(CodeGenerator *gen, int64_t value) {
    emit_int32(gen, value & 0xFFFFFFFF);
    emit_int32(gen, (value >> 32) & 0xFFFFFFFF);
}

// ====================================
// x86-64 指令编码
// ====================================

// push rax
static void emit_push_rax(CodeGenerator *gen) {
    emit(gen, 0x50);
}

// pop rax
static void emit_pop_rax(CodeGenerator *gen) {
    emit(gen, 0x58);
}

// push rbp
static void emit_push_rbp(CodeGenerator *gen) {
    emit(gen, 0x55);
}

// pop rbp
static void emit_pop_rbp(CodeGenerator *gen) {
    emit(gen, 0x5D);
}

// mov rbp, rsp
static void emit_mov_rbp_rsp(CodeGenerator *gen) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x89);  // MOV r/m64, r64
    emit(gen, 0xE5);  // ModRM: rbp = rsp
}

// mov rsp, rbp
static void emit_mov_rsp_rbp(CodeGenerator *gen) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x89);  // MOV r/m64, r64
    emit(gen, 0xEC);  // ModRM: rsp = rbp
}

// mov rax, imm64
static void emit_mov_rax_imm64(CodeGenerator *gen, int64_t value) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0xB8);  // MOV rax, imm64
    emit_int64(gen, value);
}

// mov rdi, rax
static void emit_mov_rdi_rax(CodeGenerator *gen) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x89);  // MOV r/m64, r64
    emit(gen, 0xC7);  // ModRM: rdi = rax
}

// add rax, rbx (pop rbx; add rax, rbx)
static void emit_add_rax_rbx(CodeGenerator *gen) {
    emit(gen, 0x5B);  // pop rbx
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x01);  // ADD r/m64, r64
    emit(gen, 0xD8);  // ModRM: rax += rbx
}

// sub rax, rbx (pop rbx; sub rax, rbx; neg rax)
static void emit_sub_rax_rbx(CodeGenerator *gen) {
    emit(gen, 0x5B);  // pop rbx
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x29);  // SUB r/m64, r64
    emit(gen, 0xD8);  // ModRM: rax -= rbx
    emit(gen, 0x48);  // REX.W
    emit(gen, 0xF7);  // NEG r/m64
    emit(gen, 0xD8);  // ModRM: neg rax
}

// imul rax, rbx (pop rbx; imul rax, rbx)
static void emit_imul_rax_rbx(CodeGenerator *gen) {
    emit(gen, 0x5B);  // pop rbx
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x0F);  // Two-byte opcode prefix
    emit(gen, 0xAF);  // IMUL r64, r/m64
    emit(gen, 0xC3);  // ModRM: rax *= rbx
}

// xor rdx, rdx (clear rdx for division)
static void emit_xor_rdx_rdx(CodeGenerator *gen) {
    emit(gen, 0x48);  // REX.W
    emit(gen, 0x31);  // XOR r/m64, r64
    emit(gen, 0xD2);  // ModRM: rdx = 0
}

// idiv rbx (pop rbx; idiv rbx)
static void emit_idiv_rbx(CodeGenerator *gen) {
    emit(gen, 0x5B);  // pop rbx
    emit_xor_rdx_rdx(gen);  // clear rdx
    emit(gen, 0x48);  // REX.W
    emit(gen, 0xF7);  // IDIV r/m64
    emit(gen, 0xFB);  // ModRM: idiv rbx
}

// syscall
static void emit_syscall(CodeGenerator *gen) {
    emit(gen, 0x0F);
    emit(gen, 0x05);
}

// ret
static void emit_ret(CodeGenerator *gen) {
    emit(gen, 0xC3);
}

// ====================================
// AST代码生成
// ====================================

static void generate_expression(CodeGenerator *gen, ASTNode *node);
static void generate_statement(CodeGenerator *gen, ASTNode *node);

// 生成整数字面量
static void generate_integer_literal(CodeGenerator *gen, ASTNode *node) {
    emit_mov_rax_imm64(gen, node->value.int_val);
    emit_push_rax(gen);
}

// 生成二元操作
static void generate_binary_op(CodeGenerator *gen, ASTNode *node) {
    // 生成右操作数
    generate_expression(gen, node->data.expr.rhs);
    
    // 生成左操作数
    generate_expression(gen, node->data.expr.lhs);
    
    // 左操作数在rax，右操作数在栈上
    emit_pop_rax(gen);  // 左操作数 -> rax
    
    // 根据操作符生成代码
    BinaryOp op = OP_ADD; // 需要从node中获取实际操作符
    // TODO: 从node中提取实际的操作符
    
    switch (op) {
        case OP_ADD:
            emit_add_rax_rbx(gen);
            break;
            
        case OP_SUB:
            emit_sub_rax_rbx(gen);
            break;
            
        case OP_MUL:
            emit_imul_rax_rbx(gen);
            break;
            
        case OP_DIV:
            emit_idiv_rbx(gen);
            break;
            
        default:
            // 不支持的操作
            break;
    }
    
    emit_push_rax(gen);  // 结果压栈
}

// 生成表达式
static void generate_expression(CodeGenerator *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->node_type) {
        case NODE_INTEGER_LITERAL:
            generate_integer_literal(gen, node);
            break;
            
        case NODE_BINARY_OP:
            generate_binary_op(gen, node);
            break;
            
        case NODE_IDENTIFIER:
            // TODO: 实现变量访问
            emit_mov_rax_imm64(gen, 0);
            emit_push_rax(gen);
            break;
            
        case NODE_FUNCTION_CALL:
            // TODO: 实现函数调用
            break;
            
        default:
            break;
    }
}

// 生成return语句
static void generate_return_stmt(CodeGenerator *gen, ASTNode *node) {
    if (node->data.stmt.cond) {
        // 生成返回值表达式
        generate_expression(gen, node->data.stmt.cond);
        emit_pop_rax(gen);  // 结果 -> rax
    } else {
        // 无返回值，返回0
        emit_mov_rax_imm64(gen, 0);
    }
    
    // 使用syscall退出（对于main函数）
    emit_mov_rdi_rax(gen);      // 退出码 -> rdi
    emit_mov_rax_imm64(gen, 60); // sys_exit
    emit_syscall(gen);
}

// 生成复合语句
static void generate_compound_stmt(CodeGenerator *gen, ASTNode *node) {
    for (int i = 0; i < node->data.expr.num_args; i++) {
        generate_statement(gen, node->data.expr.args[i]);
    }
}

// 生成语句
static void generate_statement(CodeGenerator *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->node_type) {
        case NODE_RETURN_STMT:
            generate_return_stmt(gen, node);
            break;
            
        case NODE_COMPOUND_STMT:
            generate_compound_stmt(gen, node);
            break;
            
        case NODE_EXPRESSION_STMT:
            if (node->data.stmt.cond) {
                generate_expression(gen, node->data.stmt.cond);
                emit_pop_rax(gen);  // 丢弃表达式结果
            }
            break;
            
        case NODE_VAR_DECL:
            // TODO: 实现变量声明
            break;
            
        case NODE_IF_STMT:
            // TODO: 实现if语句
            break;
            
        case NODE_WHILE_STMT:
            // TODO: 实现while循环
            break;
            
        case NODE_FOR_STMT:
            // TODO: 实现for循环
            break;
            
        default:
            break;
    }
}

// 生成函数
static void generate_function(CodeGenerator *gen, ASTNode *node) {
    // 函数序言
    emit_push_rbp(gen);       // push rbp
    emit_mov_rbp_rsp(gen);    // mov rbp, rsp
    
    // TODO: 为局部变量分配栈空间
    
    // 生成函数体
    if (node->decl.body) {
        generate_statement(gen, node->decl.body);
    }
    
    // 如果函数没有显式返回，添加默认返回
    // 这里简化处理，假设所有函数都通过syscall退出
}

// 主代码生成函数
static int generate_x86_64_code(ASTNode *ast, unsigned char **out_code, size_t *out_size) {
    if (!ast || !out_code || !out_size) return -1;
    
    CodeGenerator *gen = create_codegen();
    
    // 遍历翻译单元，查找main函数
    int entry_offset = 0;
    bool found_main = false;
    
    if (ast->node_type == NODE_TRANSLATION_UNIT) {
        for (int i = 0; i < ast->data.expr.num_args; i++) {
            ASTNode *decl = ast->data.expr.args[i];
            
            if (decl->node_type == NODE_FUNCTION_DECL) {
                if (strcmp(decl->decl.name, "main") == 0) {
                    entry_offset = gen->size;
                    generate_function(gen, decl);
                    found_main = true;
                    break;
                }
            }
        }
    }
    
    if (!found_main) {
        free_codegen(gen);
        return -1;
    }
    
    // 复制生成的代码
    *out_code = malloc(gen->size);
    memcpy(*out_code, gen->code, gen->size);
    *out_size = gen->size;
    
    free_codegen(gen);
    return entry_offset;
}

// ====================================
// ELF文件生成
// ====================================

#define ELF_MAGIC 0x464C457F  // 0x7F, 'E', 'L', 'F'

typedef struct {
    uint32_t magic;
    uint8_t class;      // 1 = 32-bit, 2 = 64-bit
    uint8_t data;       // 1 = little-endian, 2 = big-endian
    uint8_t version;    // 1 = current
    uint8_t osabi;      // 0 = System V
    uint8_t abiversion; // 0
    uint8_t pad[7];     // 填充到16字节
} ElfIdent;

typedef struct {
    ElfIdent ident;
    uint16_t type;      // 2 = executable
    uint16_t machine;   // 62 = x86-64
    uint32_t version;   // 1 = current
    uint64_t entry;     // 入口点地址
    uint64_t phoff;     // 程序头表偏移
    uint64_t shoff;     // 节头表偏移（可选）
    uint32_t flags;     // 处理器特定标志
    uint16_t ehsize;    // ELF头大小
    uint16_t phentsize; // 程序头表项大小
    uint16_t phnum;     // 程序头表项数量
    uint16_t shentsize; // 节头表项大小
    uint16_t shnum;     // 节头表项数量
    uint16_t shstrndx;  // 节名字符串表索引
} Elf64Header;

typedef struct {
    uint32_t type;    // 段类型：1 = LOAD
    uint32_t flags;   // 段标志：1 = X, 2 = W, 4 = R
    uint64_t offset;  // 文件偏移
    uint64_t vaddr;   // 虚拟地址
    uint64_t paddr;   // 物理地址（通常与vaddr相同）
    uint64_t filesz;  // 文件中的大小
    uint64_t memsz;   // 内存中的大小
    uint64_t align;   // 对齐
} Elf64Phdr;

static int generate_elf_executable(const char *filename, unsigned char *code, size_t code_size, int entry_offset) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("无法创建输出文件");
        return -1;
    }
    
    // 基地址
    const uint64_t base_addr = 0x400000;
    const size_t page_size = 4096;
    
    // 计算大小
    size_t headers_size = sizeof(Elf64Header) + sizeof(Elf64Phdr);
    size_t total_size = headers_size + code_size;
    
    // 创建ELF头
    Elf64Header ehdr = {0};
    ehdr.ident.magic = ELF_MAGIC;
    ehdr.ident.class = 2;        // 64-bit
    ehdr.ident.data = 1;         // little-endian
    ehdr.ident.version = 1;      // current
    ehdr.ident.osabi = 0;        // System V
    ehdr.type = 2;               // ET_EXEC
    ehdr.machine = 62;           // EM_X86_64
    ehdr.version = 1;            // current
    ehdr.entry = base_addr + headers_size + entry_offset;  // 入口点
    ehdr.phoff = sizeof(Elf64Header);  // 程序头紧跟ELF头
    ehdr.shoff = 0;              // 无节头表
    ehdr.flags = 0;              // 无特殊标志
    ehdr.ehsize = sizeof(Elf64Header);
    ehdr.phentsize = sizeof(Elf64Phdr);
    ehdr.phnum = 1;              // 一个程序头
    ehdr.shentsize = 0;          // 无节头
    ehdr.shnum = 0;              // 无节
    ehdr.shstrndx = 0;           // 无节名表
    
    // 创建程序头
    Elf64Phdr phdr = {0};
    phdr.type = 1;               // PT_LOAD
    phdr.flags = 5;              // PF_X | PF_R (可执行|可读)
    phdr.offset = 0;             // 从文件开始
    phdr.vaddr = base_addr;      // 虚拟地址
    phdr.paddr = base_addr;      // 物理地址
    phdr.filesz = total_size;    // 文件大小
    phdr.memsz = total_size;     // 内存大小
    phdr.align = page_size;      // 页对齐
    
    // 写入ELF头
    fwrite(&ehdr, sizeof(ehdr), 1, f);
    
    // 写入程序头
    fwrite(&phdr, sizeof(phdr), 1, f);
    
    // 写入代码
    fwrite(code, code_size, 1, f);
    
    fclose(f);
    
    // 设置可执行权限
    if (chmod(filename, 0755) != 0) {
        perror("无法设置可执行权限");
        return -1;
    }
    
    return 0;
}