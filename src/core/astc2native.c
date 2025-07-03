/**
 * astc2native.c - ASTC到Native转换库实现
 *
 * 正确的设计：将ASTC格式的Runtime虚拟机转换为可执行的.native文件
 * 流程: runtime.astc (ASTC虚拟机) → (JIT编译/解释器生成) → runtime{arch}{bits}.native
 *
 * 架构设计：
 * 1. 解析ASTC格式的Runtime虚拟机代码
 * 2. 生成包含ASTC解释器的机器码
 * 3. 嵌入libc转发表和ASTC指令处理
 * 4. 输出完整的Runtime.rt文件
 */

// TODO: [Module] 实现延迟链接和符号解析机制
// TODO: [Module] 支持增量编译和代码缓存策略
// TODO: [Module] 添加跨模块优化支持
// TODO: [Module] JIT编译中实现动态符号查找

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "astc2native.h"
#include "c2astc.h"
#include "codegen.h"
#include "codegen_x64.h"
#include "codegen_arm64.h"

// 前向声明
int compile_ast_node_to_machine_code(struct ASTNode* node, CodeGen* gen);
int generate_rtme_file(uint8_t* code, size_t code_size, const char* output_file);
int generate_pe_executable(uint8_t* code, size_t code_size, const char* output_file);

// ===============================================
// 架构检测实现
// ===============================================

/**
 * 检测当前运行时架构
 */
TargetArch detect_runtime_architecture(void) {
    // 使用编译时宏检测架构
    #if defined(_M_X64) || defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64)
        return ARCH_X86_64;
    #elif defined(_M_IX86) || defined(__i386__) || defined(__i386) || defined(i386)
        return ARCH_X86_32;
    #elif defined(_M_ARM64) || defined(__aarch64__)
        return ARCH_ARM64;
    #elif defined(_M_ARM) || defined(__arm__) || defined(__arm)
        return ARCH_ARM32;
    #else
        printf("Warning: Unknown architecture detected, defaulting to x86_64\n");
        return ARCH_X86_64; // 默认为x86_64而不是UNKNOWN
    #endif
}

/**
 * 从字符串解析目标架构
 */
TargetArch parse_target_architecture(const char* arch_str) {
    if (!arch_str) return detect_runtime_architecture();

    if (strcmp(arch_str, "x86_64") == 0 || strcmp(arch_str, "amd64") == 0) {
        return ARCH_X86_64;
    } else if (strcmp(arch_str, "x86_32") == 0 || strcmp(arch_str, "i386") == 0) {
        return ARCH_X86_32;
    } else if (strcmp(arch_str, "arm64") == 0 || strcmp(arch_str, "aarch64") == 0) {
        return ARCH_ARM64;
    } else if (strcmp(arch_str, "arm32") == 0 || strcmp(arch_str, "arm") == 0) {
        return ARCH_ARM32;
    } else {
        printf("Warning: Unknown architecture '%s', using runtime detection\n", arch_str);
        return detect_runtime_architecture();
    }
}

/**
 * 检查架构是否支持
 */
bool is_architecture_supported(TargetArch arch) {
    switch (arch) {
        case ARCH_X86_64:
        case ARCH_ARM64:
        case ARCH_X86_32:
        case ARCH_ARM32:
            return true;
        default:
            return false;
    }
}

/**
 * 获取架构名称字符串
 */
const char* get_architecture_name(TargetArch arch) {
    switch (arch) {
        case ARCH_X86_32: return "x86_32";
        case ARCH_X86_64: return "x86_64";
        case ARCH_ARM32:  return "arm32";
        case ARCH_ARM64:  return "arm64";
        default:          return "unknown";
    }
}

// ===============================================
// 代码生成器实现
// ===============================================

CodeGen* old_codegen_init(void) {
    CodeGen* gen = malloc(sizeof(CodeGen));
    if (!gen) return NULL;

    gen->code_capacity = 4096;
    gen->code = malloc(gen->code_capacity);
    if (!gen->code) {
        free(gen);
        return NULL;
    }
    gen->code_size = 0;
    gen->target_arch = detect_runtime_architecture();
    return gen;
}

// 新的ASTC代码生成器实现
CodeGen* astc_codegen_init(TargetArch target_arch) {
    CodeGen* gen = malloc(sizeof(CodeGen));
    if (!gen) return NULL;

    gen->code_capacity = 4096;
    gen->code = malloc(gen->code_capacity);
    if (!gen->code) {
        free(gen);
        return NULL;
    }

    gen->code_size = 0;

    // 设置目标架构
    if (target_arch == ARCH_UNKNOWN) {
        gen->target_arch = detect_runtime_architecture();
    } else {
        gen->target_arch = target_arch;
    }

    printf("Initialized code generator for architecture: %s\n",
           get_architecture_name(gen->target_arch));

    return gen;
}

void astc_codegen_free(CodeGen* gen) {
    if (!gen) return;
    if (gen->code) {
        free(gen->code);
    }
    free(gen);
}

void old_codegen_free(CodeGen* gen) {
    if (gen) {
        if (gen->code) {
            free(gen->code);
        }
        free(gen);
    }
}

void emit_byte(CodeGen* gen, uint8_t byte) {
    if (!gen) return;

    if (gen->code_size >= gen->code_capacity) {
        gen->code_capacity *= 2;
        uint8_t* new_code = realloc(gen->code, gen->code_capacity);
        if (!new_code) return; // 处理内存分配失败
        gen->code = new_code;
    }
    gen->code[gen->code_size++] = byte;
}

void emit_int32(CodeGen* gen, int32_t value) {
    emit_byte(gen, value & 0xFF);
    emit_byte(gen, (value >> 8) & 0xFF);
    emit_byte(gen, (value >> 16) & 0xFF);
    emit_byte(gen, (value >> 24) & 0xFF);
}

void emit_int64(CodeGen* gen, int64_t value) {
    emit_int32(gen, (int32_t)(value & 0xFFFFFFFF));
    emit_int32(gen, (int32_t)((value >> 32) & 0xFFFFFFFF));
}

// ===============================================
// 架构特定的代码生成函数
// ===============================================

// x86_64架构的代码生成函数
void emit_x86_64_function_prologue(CodeGen* gen) {
    emit_byte(gen, 0x55);        // push rbp
    emit_byte(gen, 0x48);        // mov rbp, rsp
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xe5);
}

void emit_x86_64_function_epilogue(CodeGen* gen) {
    emit_byte(gen, 0x5d);        // pop rbp
    emit_byte(gen, 0xc3);        // ret
}

void emit_x86_64_load_immediate(CodeGen* gen, int32_t value) {
    emit_byte(gen, 0xb8);        // mov eax, immediate
    emit_int32(gen, value);
}

void emit_x86_64_return(CodeGen* gen) {
    emit_byte(gen, 0xc3);        // ret
}

// ARM64架构的代码生成函数
void emit_arm64_function_prologue(CodeGen* gen) {
    // stp x29, x30, [sp, #-16]!
    emit_byte(gen, 0xfd); emit_byte(gen, 0x7b); emit_byte(gen, 0xbf); emit_byte(gen, 0xa9);
    // mov x29, sp
    emit_byte(gen, 0xfd); emit_byte(gen, 0x03); emit_byte(gen, 0x00); emit_byte(gen, 0x91);
}

void emit_arm64_function_epilogue(CodeGen* gen) {
    // ldp x29, x30, [sp], #16
    emit_byte(gen, 0xfd); emit_byte(gen, 0x7b); emit_byte(gen, 0xc1); emit_byte(gen, 0xa8);
    // ret
    emit_byte(gen, 0xc0); emit_byte(gen, 0x03); emit_byte(gen, 0x5f); emit_byte(gen, 0xd6);
}

void emit_arm64_load_immediate(CodeGen* gen, int32_t value) {
    // mov w0, #immediate (简化版，只支持16位立即数)
    uint16_t imm16 = (uint16_t)(value & 0xFFFF);
    emit_byte(gen, 0x00 | (imm16 & 0x1F));
    emit_byte(gen, 0x80 | ((imm16 >> 5) & 0x7F));
    emit_byte(gen, 0x80 | ((imm16 >> 12) & 0x0F));
    emit_byte(gen, 0x52);
}

void emit_arm64_return(CodeGen* gen) {
    // ret
    emit_byte(gen, 0xc0); emit_byte(gen, 0x03); emit_byte(gen, 0x5f); emit_byte(gen, 0xd6);
}

// x86_64架构的其他指令
void emit_x86_64_nop(CodeGen* gen) {
    emit_byte(gen, 0x90);        // nop
}

void emit_x86_64_store_local(CodeGen* gen, uint32_t var_index) {
    // pop rax; mov [rbp-8*var_index], rax
    emit_byte(gen, 0x58);        // pop rax
    emit_byte(gen, 0x48);        // mov [rbp-offset], rax
    emit_byte(gen, 0x89);
    emit_byte(gen, 0x45);
    emit_byte(gen, (uint8_t)(-(int32_t)(8 * var_index)));
}

void emit_x86_64_load_local(CodeGen* gen, uint32_t var_index) {
    // mov rax, [rbp-8*var_index]; push rax
    emit_byte(gen, 0x48);        // mov rax, [rbp-offset]
    emit_byte(gen, 0x8b);
    emit_byte(gen, 0x45);
    emit_byte(gen, (uint8_t)(-(int32_t)(8 * var_index)));
    emit_byte(gen, 0x50);        // push rax
}

void emit_x86_64_jump(CodeGen* gen, uint32_t target) {
    // jmp rel32 (简化版)
    emit_byte(gen, 0xe9);        // jmp rel32
    emit_int32(gen, (int32_t)target);
}

void emit_x86_64_jump_if_false(CodeGen* gen, uint32_t target) {
    // pop rax; test rax, rax; jz target
    emit_byte(gen, 0x58);        // pop rax
    emit_byte(gen, 0x48);        // test rax, rax
    emit_byte(gen, 0x85);
    emit_byte(gen, 0xc0);
    emit_byte(gen, 0x0f);        // jz rel32
    emit_byte(gen, 0x84);
    emit_int32(gen, (int32_t)target);
}

void emit_x86_64_call_user(CodeGen* gen, uint32_t func_addr) {
    // call rel32 (简化版)
    emit_byte(gen, 0xe8);        // call rel32
    emit_int32(gen, (int32_t)func_addr);
}

// ARM64架构的其他指令
void emit_arm64_nop(CodeGen* gen) {
    // nop
    emit_byte(gen, 0x1f); emit_byte(gen, 0x20); emit_byte(gen, 0x03); emit_byte(gen, 0xd5);
}

void emit_arm64_store_local(CodeGen* gen, uint32_t var_index) {
    // str x0, [x29, #-offset] (简化版)
    uint16_t offset = (uint16_t)(8 * var_index);
    emit_byte(gen, 0xa0 | (offset & 0x1F));
    emit_byte(gen, 0x83 | ((offset >> 5) & 0x07));
    emit_byte(gen, 0x1f);
    emit_byte(gen, 0xf8);
}

void emit_arm64_load_local(CodeGen* gen, uint32_t var_index) {
    // ldr x0, [x29, #-offset] (简化版)
    uint16_t offset = (uint16_t)(8 * var_index);
    emit_byte(gen, 0xa0 | (offset & 0x1F));
    emit_byte(gen, 0x83 | ((offset >> 5) & 0x07));
    emit_byte(gen, 0x5f);
    emit_byte(gen, 0xf8);
}

void emit_arm64_jump(CodeGen* gen, uint32_t target) {
    // b target (简化版)
    emit_byte(gen, 0x00 | (target & 0x1F));
    emit_byte(gen, 0x00 | ((target >> 5) & 0xFF));
    emit_byte(gen, 0x00 | ((target >> 13) & 0xFF));
    emit_byte(gen, 0x14 | ((target >> 21) & 0x1F));
}

void emit_arm64_jump_if_false(CodeGen* gen, uint32_t target) {
    // cbz x0, target (简化版)
    emit_byte(gen, 0x00 | (target & 0x1F));
    emit_byte(gen, 0x00 | ((target >> 5) & 0xFF));
    emit_byte(gen, 0x00 | ((target >> 13) & 0x07));
    emit_byte(gen, 0xb4 | ((target >> 16) & 0x1F));
}

void emit_arm64_call_user(CodeGen* gen, uint32_t func_addr) {
    // bl func_addr (简化版)
    emit_byte(gen, 0x00 | (func_addr & 0x1F));
    emit_byte(gen, 0x00 | ((func_addr >> 5) & 0xFF));
    emit_byte(gen, 0x00 | ((func_addr >> 13) & 0xFF));
    emit_byte(gen, 0x94 | ((func_addr >> 21) & 0x1F));
}

// x86_32架构的基本指令（简化版，复用x86_64的大部分逻辑）
void emit_x86_32_function_prologue(CodeGen* gen) {
    emit_byte(gen, 0x55);        // push ebp
    emit_byte(gen, 0x89);        // mov ebp, esp
    emit_byte(gen, 0xe5);
}

void emit_x86_32_function_epilogue(CodeGen* gen) {
    emit_byte(gen, 0x5d);        // pop ebp
    emit_byte(gen, 0xc3);        // ret
}

void emit_x86_32_load_immediate(CodeGen* gen, int32_t value) {
    emit_byte(gen, 0xb8);        // mov eax, immediate
    emit_int32(gen, value);
}

void emit_x86_32_nop(CodeGen* gen) {
    emit_byte(gen, 0x90);        // nop
}

// ARM32架构的基本指令（简化版）
void emit_arm32_function_prologue(CodeGen* gen) {
    // push {fp, lr}
    emit_byte(gen, 0x00); emit_byte(gen, 0x48); emit_byte(gen, 0x2d); emit_byte(gen, 0xe9);
    // add fp, sp, #4
    emit_byte(gen, 0x04); emit_byte(gen, 0xb0); emit_byte(gen, 0x8d); emit_byte(gen, 0xe2);
}

void emit_arm32_function_epilogue(CodeGen* gen) {
    // pop {fp, pc}
    emit_byte(gen, 0x00); emit_byte(gen, 0x88); emit_byte(gen, 0xbd); emit_byte(gen, 0xe8);
}

void emit_arm32_load_immediate(CodeGen* gen, int32_t value) {
    // mov r0, #immediate (简化版，只支持8位立即数)
    uint8_t imm8 = (uint8_t)(value & 0xFF);
    emit_byte(gen, imm8); emit_byte(gen, 0x00); emit_byte(gen, 0xa0); emit_byte(gen, 0xe3);
}

void emit_arm32_nop(CodeGen* gen) {
    // nop (mov r0, r0)
    emit_byte(gen, 0x00); emit_byte(gen, 0x00); emit_byte(gen, 0xa0); emit_byte(gen, 0xe1);
}

// 架构特定的代码生成表
typedef struct {
    void (*emit_function_prologue)(CodeGen* gen);
    void (*emit_function_epilogue)(CodeGen* gen);
    void (*emit_load_immediate)(CodeGen* gen, int32_t value);
    void (*emit_return)(CodeGen* gen);
    void (*emit_nop)(CodeGen* gen);
    void (*emit_store_local)(CodeGen* gen, uint32_t var_index);
    void (*emit_load_local)(CodeGen* gen, uint32_t var_index);
    void (*emit_jump)(CodeGen* gen, uint32_t target);
    void (*emit_jump_if_false)(CodeGen* gen, uint32_t target);
    void (*emit_call_user)(CodeGen* gen, uint32_t func_addr);
} ArchCodeGenTable;

// x86_64代码生成表
static ArchCodeGenTable x86_64_table = {
    .emit_function_prologue = emit_x86_64_function_prologue,
    .emit_function_epilogue = emit_x86_64_function_epilogue,
    .emit_load_immediate = emit_x86_64_load_immediate,
    .emit_return = emit_x86_64_return,
    .emit_nop = emit_x86_64_nop,
    .emit_store_local = emit_x86_64_store_local,
    .emit_load_local = emit_x86_64_load_local,
    .emit_jump = emit_x86_64_jump,
    .emit_jump_if_false = emit_x86_64_jump_if_false,
    .emit_call_user = emit_x86_64_call_user
};

// ARM64代码生成表
static ArchCodeGenTable arm64_table = {
    .emit_function_prologue = emit_arm64_function_prologue,
    .emit_function_epilogue = emit_arm64_function_epilogue,
    .emit_load_immediate = emit_arm64_load_immediate,
    .emit_return = emit_arm64_return,
    .emit_nop = emit_arm64_nop,
    .emit_store_local = emit_arm64_store_local,
    .emit_load_local = emit_arm64_load_local,
    .emit_jump = emit_arm64_jump,
    .emit_jump_if_false = emit_arm64_jump_if_false,
    .emit_call_user = emit_arm64_call_user
};

// x86_32代码生成表
static ArchCodeGenTable x86_32_table = {
    .emit_function_prologue = emit_x86_32_function_prologue,
    .emit_function_epilogue = emit_x86_32_function_epilogue,
    .emit_load_immediate = emit_x86_32_load_immediate,
    .emit_return = emit_x86_32_function_epilogue, // 复用epilogue
    .emit_nop = emit_x86_32_nop,
    .emit_store_local = emit_x86_64_store_local, // 复用x86_64版本
    .emit_load_local = emit_x86_64_load_local,   // 复用x86_64版本
    .emit_jump = emit_x86_64_jump,               // 复用x86_64版本
    .emit_jump_if_false = emit_x86_64_jump_if_false, // 复用x86_64版本
    .emit_call_user = emit_x86_64_call_user      // 复用x86_64版本
};

// ARM32代码生成表
static ArchCodeGenTable arm32_table = {
    .emit_function_prologue = emit_arm32_function_prologue,
    .emit_function_epilogue = emit_arm32_function_epilogue,
    .emit_load_immediate = emit_arm32_load_immediate,
    .emit_return = emit_arm32_function_epilogue, // 复用epilogue
    .emit_nop = emit_arm32_nop,
    .emit_store_local = emit_arm64_store_local,  // 复用ARM64版本
    .emit_load_local = emit_arm64_load_local,    // 复用ARM64版本
    .emit_jump = emit_arm64_jump,                // 复用ARM64版本
    .emit_jump_if_false = emit_arm64_jump_if_false, // 复用ARM64版本
    .emit_call_user = emit_arm64_call_user       // 复用ARM64版本
};

// 获取架构特定的代码生成表
ArchCodeGenTable* get_arch_codegen_table(TargetArch arch) {
    switch (arch) {
        case ARCH_X86_64:
            return &x86_64_table;
        case ARCH_ARM64:
            return &arm64_table;
        case ARCH_X86_32:
            return &x86_32_table;
        case ARCH_ARM32:
            return &arm32_table;
        default:
            // 默认使用x86_64表
            printf("Warning: Unknown architecture, using x86_64 as default\n");
            return &x86_64_table;
    }
}

// ===============================================
// 代码生成优化框架
// ===============================================

// 优化级别枚举
typedef enum {
    OPT_NONE = 0,      // 无优化
    OPT_BASIC = 1,     // 基础优化
    OPT_STANDARD = 2,  // 标准优化
    OPT_AGGRESSIVE = 3 // 激进优化
} OptimizationLevel;

// 优化统计信息
typedef struct {
    int dead_code_eliminated;
    int constants_folded;
    int redundant_moves_removed;
    int instructions_combined;
    int register_allocations_optimized;
} OptimizationStats;

// 增强的代码生成器
typedef struct {
    CodeGen* base_gen;
    OptimizationLevel opt_level;
    OptimizationStats stats;
    bool enable_register_allocation;
    bool enable_constant_folding;
    bool enable_dead_code_elimination;
    bool enable_instruction_combining;
    uint32_t last_constant_value;
    bool has_pending_constant;
} EnhancedCodeGen;

// 创建增强的代码生成器
EnhancedCodeGen* create_enhanced_codegen(TargetArch arch, OptimizationLevel opt_level) {
    EnhancedCodeGen* enhanced = malloc(sizeof(EnhancedCodeGen));
    if (!enhanced) return NULL;

    enhanced->base_gen = astc_codegen_init(arch);
    if (!enhanced->base_gen) {
        free(enhanced);
        return NULL;
    }

    enhanced->opt_level = opt_level;
    memset(&enhanced->stats, 0, sizeof(OptimizationStats));

    // 根据优化级别设置优化选项
    enhanced->enable_register_allocation = (opt_level >= OPT_BASIC);
    enhanced->enable_constant_folding = (opt_level >= OPT_BASIC);
    enhanced->enable_dead_code_elimination = (opt_level >= OPT_STANDARD);
    enhanced->enable_instruction_combining = (opt_level >= OPT_AGGRESSIVE);

    enhanced->has_pending_constant = false;
    enhanced->last_constant_value = 0;

    printf("Enhanced code generator initialized with optimization level %d\n", opt_level);
    return enhanced;
}

// 释放增强的代码生成器
void free_enhanced_codegen(EnhancedCodeGen* enhanced) {
    if (!enhanced) return;

    printf("Optimization statistics:\n");
    printf("  Dead code eliminated: %d\n", enhanced->stats.dead_code_eliminated);
    printf("  Constants folded: %d\n", enhanced->stats.constants_folded);
    printf("  Redundant moves removed: %d\n", enhanced->stats.redundant_moves_removed);
    printf("  Instructions combined: %d\n", enhanced->stats.instructions_combined);
    printf("  Register allocations optimized: %d\n", enhanced->stats.register_allocations_optimized);

    if (enhanced->base_gen) {
        astc_codegen_free(enhanced->base_gen);
    }
    free(enhanced);
}

// 常量折叠优化
bool try_constant_folding(EnhancedCodeGen* enhanced, uint8_t opcode, uint32_t operand) {
    if (!enhanced->enable_constant_folding) return false;

    if (opcode == 0x10) { // CONST_I32
        if (enhanced->has_pending_constant) {
            // 连续的常量可能可以合并
            enhanced->stats.constants_folded++;
            return true;
        }
        enhanced->has_pending_constant = true;
        enhanced->last_constant_value = operand;
        return false;
    }

    if (enhanced->has_pending_constant && opcode == 0x20) { // ADD with constant
        // 可以优化为 add reg, immediate
        enhanced->has_pending_constant = false;
        enhanced->stats.constants_folded++;
        return true;
    }

    enhanced->has_pending_constant = false;
    return false;
}

// 死代码消除
bool is_dead_code_instruction(uint8_t opcode) {
    // 简单的死代码检测
    switch (opcode) {
        case 0x00: // NOP
            return true;
        default:
            return false;
    }
}

// ===============================================
// 代码生成辅助函数
// ===============================================

// 优化的指令生成函数
void enhanced_emit_const_i32(EnhancedCodeGen* enhanced, uint32_t value) {
    ArchCodeGenTable* table = get_arch_codegen_table(enhanced->base_gen->target_arch);

    // 常量优化
    if (enhanced->enable_constant_folding && value == 0) {
        // 使用xor reg, reg代替mov reg, 0（更短更快）
        if (enhanced->base_gen->target_arch == ARCH_X86_64) {
            emit_byte(enhanced->base_gen, 0x48); // REX.W
            emit_byte(enhanced->base_gen, 0x31); // xor
            emit_byte(enhanced->base_gen, 0xc0); // eax, eax
            enhanced->stats.instructions_combined++;
        } else {
            table->emit_load_immediate(enhanced->base_gen, value);
        }
    } else if (enhanced->enable_constant_folding && value == 1) {
        // 使用inc指令代替mov reg, 1
        if (enhanced->base_gen->target_arch == ARCH_X86_64) {
            emit_byte(enhanced->base_gen, 0x48); // REX.W
            emit_byte(enhanced->base_gen, 0x31); // xor eax, eax
            emit_byte(enhanced->base_gen, 0xc0);
            emit_byte(enhanced->base_gen, 0x48); // REX.W
            emit_byte(enhanced->base_gen, 0xff); // inc eax
            emit_byte(enhanced->base_gen, 0xc0);
            enhanced->stats.instructions_combined++;
        } else {
            table->emit_load_immediate(enhanced->base_gen, value);
        }
    } else {
        table->emit_load_immediate(enhanced->base_gen, value);
    }
}

void enhanced_emit_add(EnhancedCodeGen* enhanced) {
    ArchCodeGenTable* table = get_arch_codegen_table(enhanced->base_gen->target_arch);

    if (enhanced->base_gen->target_arch == ARCH_X86_64) {
        // 优化的x86_64加法：pop rbx; pop rax; add rax, rbx; push rax
        emit_byte(enhanced->base_gen, 0x5b); // pop rbx
        emit_byte(enhanced->base_gen, 0x58); // pop rax
        emit_byte(enhanced->base_gen, 0x48); // REX.W
        emit_byte(enhanced->base_gen, 0x01); // add rax, rbx
        emit_byte(enhanced->base_gen, 0xd8);
        emit_byte(enhanced->base_gen, 0x50); // push rax
    } else {
        // 使用架构特定的实现
        table->emit_nop(enhanced->base_gen); // 简化实现
    }
}

void enhanced_emit_libc_call(EnhancedCodeGen* enhanced, uint16_t func_id, uint16_t arg_count) {
    if (enhanced->base_gen->target_arch == ARCH_X86_64) {
        // 优化的libc调用：直接调用而不是通过查找表
        if (enhanced->enable_instruction_combining && func_id == 0x0030) { // printf
            // 特殊优化printf调用
            emit_byte(enhanced->base_gen, 0x48); // mov rax, printf_addr
            emit_byte(enhanced->base_gen, 0xb8);
            emit_int32(enhanced->base_gen, 0x12345678); // 占位符地址
            emit_int32(enhanced->base_gen, 0);
            emit_byte(enhanced->base_gen, 0xff); // call rax
            emit_byte(enhanced->base_gen, 0xd0);
            enhanced->stats.instructions_combined++;
        } else {
            // 标准libc调用
            emit_byte(enhanced->base_gen, 0xb8); // mov eax, func_id
            emit_int32(enhanced->base_gen, func_id);
            emit_byte(enhanced->base_gen, 0x50); // push rax
            emit_byte(enhanced->base_gen, 0xb8); // mov eax, arg_count
            emit_int32(enhanced->base_gen, arg_count);
            emit_byte(enhanced->base_gen, 0x50); // push rax
        }
    }
}

// 编译常量表达式（借鉴TinyCC的立即数处理）
static void compile_constant(CodeGen* gen, struct ASTNode* node) {
    if (node->type == ASTC_EXPR_CONSTANT && node->data.constant.type == ASTC_TYPE_INT) {
        // mov eax, immediate
        emit_byte(gen, 0xb8);
        emit_int32(gen, node->data.constant.int_val);
    }
}

// 编译表达式（借鉴TinyCC的表达式编译）
static void compile_expression(CodeGen* gen, struct ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTC_EXPR_CONSTANT:
            compile_constant(gen, node);
            break;
        default:
            // 默认返回0
            emit_byte(gen, 0xb8);  // mov eax, 0
            emit_int32(gen, 0);
            break;
    }
}

// 编译return语句（借鉴TinyCC的函数返回处理）
static void compile_return(CodeGen* gen, struct ASTNode* node) {
    if (!node) return;

    if (node->data.return_stmt.value) {
        // 编译返回值表达式
        compile_expression(gen, node->data.return_stmt.value);
    }
    // ret指令
    emit_byte(gen, 0xc3);
}

// 编译语句（借鉴TinyCC的语句编译）
static void compile_statement(CodeGen* gen, struct ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case ASTC_RETURN_STMT:
            compile_return(gen, node);
            break;
        case ASTC_COMPOUND_STMT:
            // 编译复合语句中的所有子语句
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                compile_statement(gen, node->data.compound_stmt.statements[i]);
            }
            break;
        default:
            break;
    }
}

// 编译函数（借鉴TinyCC的函数编译）
static void compile_function(CodeGen* gen, struct ASTNode* node) {
    if (!node) return;

    // 函数序言
    emit_byte(gen, 0x55);        // push rbp
    emit_byte(gen, 0x48);        // mov rbp, rsp
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xe5);

    // 编译函数体
    if (node->data.func_decl.body) {
        compile_statement(gen, node->data.func_decl.body);
    }

    // 如果没有显式return，添加默认返回
    emit_byte(gen, 0xb8);        // mov eax, 0
    emit_int32(gen, 0);
    emit_byte(gen, 0x5d);        // pop rbp
    emit_byte(gen, 0xc3);        // ret
}

// 编译整个翻译单元（包含多个函数）
static void compile_runtime_from_translation_unit(CodeGen* gen, struct ASTNode* node) {
    if (!gen || !node) return;
    
    printf("Compiling runtime from translation unit...\n");

    // 遍历翻译单元中的所有声明
    if (node->type == ASTC_TRANSLATION_UNIT && node->data.translation_unit.declarations) {
        int func_count = 0;

        // 遍历声明数组
        for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
            struct ASTNode* decl = node->data.translation_unit.declarations[i];

            if (decl && decl->type == ASTC_FUNC_DECL) {
                printf("  Compiling function: %s\n", decl->data.func_decl.name);

                // 为每个函数生成标签和代码
                if (strcmp(decl->data.func_decl.name, "evolver0_runtime_main") == 0) {
                    // 这是主入口函数，放在开头
                    compile_function(gen, decl);
                    func_count++;
                } else {
                    // 其他函数
                    compile_function(gen, decl);
                    func_count++;
                }
            }
        }

        printf("  Compiled %d functions from translation unit\n", func_count);
    } else {
        printf("  Warning: Not a valid translation unit\n");
    }
}

// ===============================================
// 公开API实现
// ===============================================

// 架构特定的指令生成函数指针
typedef void (*emit_nop_func)(CodeGen* gen);
typedef void (*emit_halt_func)(CodeGen* gen);
typedef void (*emit_const_i32_func)(CodeGen* gen, uint32_t value);
typedef void (*emit_binary_op_func)(CodeGen* gen);
typedef void (*emit_libc_call_func)(CodeGen* gen, uint16_t func_id, uint16_t arg_count);
typedef void (*emit_function_prologue_func)(CodeGen* gen);
typedef void (*emit_function_epilogue_func)(CodeGen* gen);

// 架构特定的代码生成函数表
typedef struct {
    emit_nop_func emit_nop;
    emit_halt_func emit_halt;
    emit_const_i32_func emit_const_i32;
    emit_binary_op_func emit_add;
    emit_binary_op_func emit_sub;
    emit_binary_op_func emit_mul;
    emit_binary_op_func emit_div;
    emit_libc_call_func emit_libc_call;
    emit_function_prologue_func emit_function_prologue;
    emit_function_epilogue_func emit_function_epilogue;
} ArchCodegenTable;

// 获取架构特定的代码生成函数表
ArchCodegenTable* get_arch_codegen_table(TargetArch arch) {
    static ArchCodegenTable x64_table = {0};
    static ArchCodegenTable x86_table = {0};
    static ArchCodegenTable arm64_table = {0};
    static ArchCodegenTable arm32_table = {0};

    switch (arch) {
        case ARCH_X86_64:
            if (!x64_table.emit_nop) {
                // 初始化x64函数表
                x64_table.emit_nop = x64_emit_nop;
                x64_table.emit_halt = x64_emit_halt_with_return_value;
                x64_table.emit_const_i32 = x64_emit_const_i32;
                x64_table.emit_add = x64_emit_binary_op_add;
                x64_table.emit_sub = x64_emit_binary_op_sub;
                x64_table.emit_mul = x64_emit_binary_op_mul;
                x64_table.emit_div = x64_emit_div;
                x64_table.emit_libc_call = x64_emit_libc_call;
                x64_table.emit_function_prologue = x64_emit_function_prologue;
                x64_table.emit_function_epilogue = x64_emit_function_epilogue;
            }
            return &x64_table;

        case ARCH_X86_32:
            // TODO: 实现x86_32支持
            printf("Warning: x86_32 architecture not fully implemented, using x64 fallback\n");
            return get_arch_codegen_table(ARCH_X86_64);

        case ARCH_ARM64:
            if (!arm64_table.emit_nop) {
                // 初始化ARM64函数表
                arm64_table.emit_nop = arm64_emit_nop;
                arm64_table.emit_halt = arm64_emit_halt_with_return_value;
                arm64_table.emit_const_i32 = arm64_emit_const_i32;
                arm64_table.emit_add = arm64_emit_binary_op_add;
                arm64_table.emit_sub = arm64_emit_binary_op_sub;
                arm64_table.emit_mul = arm64_emit_binary_op_mul;
                arm64_table.emit_div = arm64_emit_div;
                arm64_table.emit_libc_call = arm64_emit_libc_call;
                arm64_table.emit_function_prologue = arm64_emit_function_prologue;
                arm64_table.emit_function_epilogue = arm64_emit_function_epilogue;
            }
            return &arm64_table;

        case ARCH_ARM32:
            // TODO: 实现ARM32支持
            printf("Warning: ARM32 architecture not implemented, using x64 fallback\n");
            return get_arch_codegen_table(ARCH_X86_64);

        default:
            printf("Warning: Unknown architecture, using x64 fallback\n");
            return get_arch_codegen_table(ARCH_X86_64);
    }
}

// ASTC JIT编译器 - 将ASTC字节码指令翻译成二进制机器码
// 使用架构特定的codegen函数，支持跨平台
void compile_astc_instruction_to_machine_code(CodeGen* gen, uint8_t opcode, uint8_t* operands, size_t operand_len) {
    ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);

    switch (opcode) {
        case 0x00: // NOP
            table->emit_nop(gen);
            break;

        case 0x01: // HALT
            table->emit_halt(gen);
            break;

        case 0x10: // CONST_I32 (优化版本)
            if (operand_len >= 4) {
                uint32_t value = *(uint32_t*)operands;

                // 优化：特殊值使用更高效的指令
                if (gen->target_arch == ARCH_X86_64) {
                    if (value == 0) {
                        // xor eax, eax; push rax (比 mov eax, 0; push rax 更快)
                        emit_byte(gen, 0x48); // REX.W
                        emit_byte(gen, 0x31); // xor eax, eax
                        emit_byte(gen, 0xc0);
                        emit_byte(gen, 0x50); // push rax
                    } else if (value <= 127) {
                        // push imm8 (比 mov + push 更短)
                        emit_byte(gen, 0x6a); // push imm8
                        emit_byte(gen, (uint8_t)value);
                    } else {
                        table->emit_const_i32(gen, value);
                    }
                } else {
                    table->emit_const_i32(gen, value);
                }
            }
            break;

        case 0x20: // ADD (优化版本)
            if (gen->target_arch == ARCH_X86_64) {
                // 优化的x86_64加法实现
                emit_byte(gen, 0x5b); // pop rbx
                emit_byte(gen, 0x58); // pop rax
                emit_byte(gen, 0x48); // REX.W
                emit_byte(gen, 0x01); // add rax, rbx
                emit_byte(gen, 0xd8);
                emit_byte(gen, 0x50); // push rax
            } else {
                table->emit_add(gen);
            }
            break;

        case 0x21: // SUB
            table->emit_sub(gen);
            break;

        case 0x22: // MUL
            table->emit_mul(gen);
            break;

        case 0x23: // DIV
            table->emit_div(gen);
            break;

        case 0x12: // CONST_STRING
            // 字符串常量指令 - 将字符串地址压入栈
            if (operand_len >= 4) {
                uint32_t str_len = *(uint32_t*)operands;
                // 简化实现：将字符串数据地址压入栈
                table->emit_const_i32(gen, (uint32_t)(uintptr_t)(operands + 4));
            }
            break;

        case 0x30: // STORE_LOCAL
            // 存储到局部变量
            if (operand_len >= 4) {
                uint32_t var_index = *(uint32_t*)operands;
                // 简化实现：将栈顶值存储到局部变量槽
                // pop rax; mov [rbp-8*var_index], rax
                table->emit_store_local(gen, var_index);
            } else {
                table->emit_nop(gen);
            }
            break;

        case 0x31: // LOAD_LOCAL
            // 加载局部变量
            if (operand_len >= 4) {
                uint32_t var_index = *(uint32_t*)operands;
                // 简化实现：从局部变量槽加载值到栈
                // mov rax, [rbp-8*var_index]; push rax
                table->emit_load_local(gen, var_index);
            } else {
                table->emit_nop(gen);
            }
            break;

        case 0x40: // JUMP
            // 无条件跳转
            if (operand_len >= 4) {
                uint32_t target = *(uint32_t*)operands;
                table->emit_jump(gen, target);
            } else {
                table->emit_nop(gen);
            }
            break;

        case 0x41: // JUMP_IF_FALSE
            // 条件跳转
            if (operand_len >= 4) {
                uint32_t target = *(uint32_t*)operands;
                table->emit_jump_if_false(gen, target);
            } else {
                table->emit_nop(gen);
            }
            break;

        case 0x50: // CALL_USER
            // 用户函数调用
            if (operand_len >= 4) {
                uint32_t func_addr = *(uint32_t*)operands;
                table->emit_call_user(gen, func_addr);
            } else {
                table->emit_nop(gen);
            }
            break;

        case 0xF0: // LIBC_CALL (优化版本)
            if (operand_len >= 4) {
                uint16_t func_id = *(uint16_t*)operands;
                uint16_t arg_count = *(uint16_t*)(operands + 2);

                if (gen->target_arch == ARCH_X86_64) {
                    // 优化：常用函数直接调用
                    if (func_id == 0x0030) { // printf
                        // 优化的printf调用：减少查找开销
                        emit_byte(gen, 0x48); // mov rax, printf_addr
                        emit_byte(gen, 0xb8);
                        emit_int32(gen, 0x12345678); // 占位符地址
                        emit_int32(gen, 0);
                        emit_byte(gen, 0xff); // call rax
                        emit_byte(gen, 0xd0);
                    } else {
                        // 标准libc调用
                        emit_byte(gen, 0xb8); // mov eax, func_id
                        emit_int32(gen, func_id);
                        emit_byte(gen, 0x50); // push rax
                        emit_byte(gen, 0xb8); // mov eax, arg_count
                        emit_int32(gen, arg_count);
                        emit_byte(gen, 0x50); // push rax
                    }
                } else {
                    table->emit_libc_call(gen, func_id, arg_count);
                }
            }
            break;

        default:
            // 未知指令，生成nop
            printf("Warning: Unknown ASTC opcode 0x%02X, generating NOP\n", opcode);
            table->emit_nop(gen);
            break;
    }
}

// ASTC JIT编译器 - 将ASTC字节码指令翻译成汇编代码
// 使用符合命名规范的proper codegen架构
void compile_astc_instruction_to_asm(CodeGenerator* cg, uint8_t opcode, uint8_t* operands, size_t operand_count) {
    char temp_buffer[256];

    switch (opcode) {
        case 0x00: // NOP
            codegen_append_public(cg, "    nop\n");
            break;

        case 0x01: // HALT
            codegen_append_public(cg, "    mov rsp, rbp\n");
            codegen_append_public(cg, "    pop rbp\n");
            codegen_append_public(cg, "    ret\n");
            break;

        case 0x10: // CONST_I32
            if (operand_count >= 4) {
                uint32_t value = *(uint32_t*)operands;
                sprintf(temp_buffer, "    mov eax, %u\n", value);
                codegen_append_public(cg, temp_buffer);
                codegen_append_public(cg, "    push rax\n");
            }
            break;

        case 0x20: // ADD
            codegen_append_public(cg, "    pop rbx\n");
            codegen_append_public(cg, "    pop rax\n");
            codegen_append_public(cg, "    add rax, rbx\n");
            codegen_append_public(cg, "    push rax\n");
            break;

        case 0x21: // SUB
            codegen_append_public(cg, "    pop rbx\n");
            codegen_append_public(cg, "    pop rax\n");
            codegen_append_public(cg, "    sub rax, rbx\n");
            codegen_append_public(cg, "    push rax\n");
            break;

        case 0x22: // MUL
            codegen_append_public(cg, "    pop rbx\n");
            codegen_append_public(cg, "    pop rax\n");
            codegen_append_public(cg, "    imul rax, rbx\n");
            codegen_append_public(cg, "    push rax\n");
            break;

        case 0xF0: // LIBC_CALL
            // 生成libc调用的机器码
            // 这里需要调用libc转发函数
            // 简化版本：调用printf
            if (operand_count >= 4) {
                uint16_t func_id = *(uint16_t*)operands;
                uint16_t arg_count = *(uint16_t*)(operands + 2);

                // 根据func_id生成对应的libc调用
                if (func_id == 0x0030) { // LIBC_PRINTF
                    sprintf(temp_buffer, "    ; LIBC_PRINTF call (func_id=%u, args=%u)\n", func_id, arg_count);
                    codegen_append_public(cg, temp_buffer);
                    codegen_append_public(cg, "    call printf\n");
                }
            }
            break;

        default:
            // 未知指令，生成NOP
            codegen_append_public(cg, "    nop\n");
            break;
    }
}

// ASTC JIT编译主函数 - 类似TinyCC的代码生成
int compile_astc_to_machine_code(uint8_t* astc_data, size_t astc_size, CodeGen* gen) {
    printf("JIT compiling ASTC bytecode to %s machine code...\n",
           get_architecture_name(gen->target_arch));

    // 跳过ASTC头部
    if (astc_size < 16 || memcmp(astc_data, "ASTC", 4) != 0) {
        printf("Error: Invalid ASTC format\n");
        return 1;
    }

    uint32_t* header = (uint32_t*)astc_data;
    uint32_t version = header[1];
    uint32_t data_size = header[2];
    uint32_t entry_point = header[3];

    printf("ASTC version: %u, data_size: %u, entry_point: %u\n", version, data_size, entry_point);

    // 生成函数序言
    ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
    table->emit_function_prologue(gen);

    // 尝试反序列化AST
    uint8_t* ast_data = astc_data + 16;
    size_t ast_data_size = astc_size - 16;

    struct ASTNode* ast = c2astc_deserialize(ast_data, ast_data_size);
    if (ast) {
        printf("JIT compiling AST to x64 machine code...\n");
        // 编译AST节点到机器码
        compile_ast_node_to_machine_code(ast, gen);
        ast_free(ast);
    } else {
        printf("Warning: Failed to deserialize AST, trying bytecode mode...\n");
        // 回退到字节码模式
        uint8_t* code = astc_data + 16;
        size_t code_size = astc_size - 16;
        size_t pc = 0;

        while (pc < code_size) {
            uint8_t opcode = code[pc++];

            // 根据指令类型确定操作数长度
            size_t operand_len = 0;
            switch (opcode) {
                case 0x10: operand_len = 4; break; // CONST_I32
                case 0x12: // CONST_STRING - 需要读取长度字段
                    if (pc + 4 <= code_size) {
                        uint32_t str_len = *(uint32_t*)&code[pc];
                        operand_len = 4 + str_len; // 长度字段 + 字符串数据
                    }
                    break;
                case 0x30: operand_len = 4; break; // STORE_LOCAL
                case 0x31: operand_len = 4; break; // LOAD_LOCAL
                case 0x40: operand_len = 4; break; // JUMP
                case 0x41: operand_len = 4; break; // JUMP_IF_FALSE
                case 0x50: operand_len = 4; break; // CALL_USER
                case 0xF0: operand_len = 4; break; // LIBC_CALL
                default: operand_len = 0; break;
            }

            uint8_t* operands = (pc + operand_len <= code_size) ? &code[pc] : NULL;

            // JIT编译这条指令到机器码
            compile_astc_instruction_to_machine_code(gen, opcode, operands, operand_len);

            pc += operand_len;
        }
    }

    // 如果没有显式的HALT，添加默认返回
    table->emit_function_epilogue(gen);

    printf("JIT compilation completed: %zu ASTC bytes → %zu machine code bytes\n",
           ast_data_size, gen->code_size);

    return 0;
}

// 优化的JIT编译入口函数
int optimized_jit_compile_astc_to_machine_code(uint8_t* astc_data, size_t astc_size,
                                              CodeGen* gen, OptimizationLevel opt_level) {
    if (!astc_data || !gen || astc_size < 16) {
        return 1;
    }

    printf("Starting optimized JIT compilation (level %d)...\n", opt_level);

    // 创建增强的代码生成器
    EnhancedCodeGen* enhanced = create_enhanced_codegen(gen->target_arch, opt_level);
    if (!enhanced) {
        return 1;
    }

    // 复制基础生成器的状态
    enhanced->base_gen->code = gen->code;
    enhanced->base_gen->code_size = gen->code_size;
    enhanced->base_gen->code_capacity = gen->code_capacity;

    // 解析ASTC头
    if (strncmp((char*)astc_data, "ASTC", 4) != 0) {
        printf("Error: Invalid ASTC format\n");
        free_enhanced_codegen(enhanced);
        return 1;
    }

    uint32_t version = *(uint32_t*)(astc_data + 4);
    uint32_t data_size = *(uint32_t*)(astc_data + 8);
    uint32_t entry_point = *(uint32_t*)(astc_data + 12);

    printf("ASTC version: %u, data_size: %u, entry_point: %u\n", version, data_size, entry_point);

    // 生成函数序言
    ArchCodeGenTable* table = get_arch_codegen_table(gen->target_arch);
    table->emit_function_prologue(enhanced->base_gen);

    // 编译ASTC字节码
    uint8_t* code = astc_data + 16;
    size_t code_size = data_size;
    size_t pc = 0;

    while (pc < code_size) {
        uint8_t opcode = code[pc++];

        // 死代码消除
        if (enhanced->enable_dead_code_elimination && is_dead_code_instruction(opcode)) {
            enhanced->stats.dead_code_eliminated++;
            continue;
        }

        // 确定操作数长度
        size_t operand_len = 0;
        switch (opcode) {
            case 0x10: operand_len = 4; break; // CONST_I32
            case 0x20: case 0x21: case 0x22: case 0x23: operand_len = 0; break; // 算术运算
            case 0x30: case 0x31: operand_len = 4; break; // 局部变量操作
            case 0x40: case 0x41: operand_len = 4; break; // 跳转指令
            case 0x50: operand_len = 4; break; // 用户函数调用
            case 0xF0: operand_len = 4; break; // LIBC调用
            default: operand_len = 0; break;
        }

        uint8_t* operands = (pc + operand_len <= code_size) ? &code[pc] : NULL;

        // 尝试常量折叠
        if (operands && try_constant_folding(enhanced, opcode, *(uint32_t*)operands)) {
            pc += operand_len;
            continue;
        }

        // 编译指令（使用原有的优化版本）
        compile_astc_instruction_to_machine_code(enhanced->base_gen, opcode, operands, operand_len);

        pc += operand_len;
    }

    // 生成函数尾声
    table->emit_function_epilogue(enhanced->base_gen);

    // 更新原始生成器的状态
    gen->code_size = enhanced->base_gen->code_size;

    printf("Optimized JIT compilation completed: %zu ASTC bytes → %zu machine code bytes\n",
           astc_size, gen->code_size);

    // 清理
    enhanced->base_gen->code = NULL; // 防止重复释放
    free_enhanced_codegen(enhanced);

    return 0;
}

int generate_runtime_file(uint8_t* code, size_t code_size, const char* output_file) {
    // 检查输出文件扩展名，决定生成格式
    const char* ext = strrchr(output_file, '.');
    bool generate_exe = (ext && strcmp(ext, ".exe") == 0);

    printf("DEBUG: output_file='%s', ext='%s', generate_exe=%d\n",
           output_file, ext ? ext : "NULL", generate_exe);

    if (generate_exe) {
        // 生成真正的PE可执行文件
        printf("DEBUG: Generating PE executable\n");
        return generate_pe_executable(code, code_size, output_file);
    } else {
        // 生成RTME格式文件
        printf("DEBUG: Generating RTME file\n");
        return generate_rtme_file(code, code_size, output_file);
    }
}

// 生成RTME格式文件（原有功能）
int generate_rtme_file(uint8_t* code, size_t code_size, const char* output_file) {
    FILE* fp = fopen(output_file, "wb");
    if (!fp) {
        printf("Error: Cannot open output file %s\n", output_file);
        return 1;
    }

    // 创建运行时文件头
    RuntimeHeader header;
    strncpy(header.magic, "RTME", 4);
    header.version = 1;
    header.size = (uint32_t)code_size;
    header.entry_point = sizeof(RuntimeHeader); // 入口点在header之后

    // 写入文件头
    fwrite(&header, sizeof(header), 1, fp);

    // 写入代码
    fwrite(code, 1, code_size, fp);

    fclose(fp);
    printf("Generated RTME runtime file: %s (%zu bytes + header)\n", output_file, code_size);
    return 0;
}

// 生成PE可执行文件
int generate_pe_executable(uint8_t* code, size_t code_size, const char* output_file) {
    printf("Generating PE executable: %s (%zu bytes machine code)\n", output_file, code_size);

    FILE* fp = fopen(output_file, "wb");
    if (!fp) {
        printf("Error: Cannot create PE executable %s\n", output_file);
        return 1;
    }

    // 正确的PE文件结构
    // 1. DOS头 (64字节)
    uint8_t dos_header[64] = {
        0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00, // MZ signature + e_cblp, e_cp
        0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, // e_crlc, e_cparhdr, e_minalloc, e_maxalloc
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_ss, e_sp, e_csum, e_ip
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_cs, e_lfarlc, e_ovno, e_res[0]
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_res[1], e_res[2], e_res[3], e_oemid
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_oeminfo, e_res2[0], e_res2[1], e_res2[2]
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // e_res2[3], e_res2[4], e_res2[5], e_res2[6]
        0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00  // e_res2[7], e_res2[8], e_res2[9], e_lfanew=0x80
    };

    // 2. PE签名 (4字节)
    uint8_t pe_signature[4] = { 0x50, 0x45, 0x00, 0x00 }; // "PE\0\0"

    // 3. COFF文件头 (20字节)
    uint8_t coff_header[20] = {
        0x64, 0x86,                                     // Machine (x64)
        0x01, 0x00,                                     // NumberOfSections (1)
        0x00, 0x00, 0x00, 0x00,                         // TimeDateStamp
        0x00, 0x00, 0x00, 0x00,                         // PointerToSymbolTable
        0x00, 0x00, 0x00, 0x00,                         // NumberOfSymbols
        0xF0, 0x00,                                     // SizeOfOptionalHeader (240字节)
        0x22, 0x00                                      // Characteristics (EXECUTABLE_IMAGE | LARGE_ADDRESS_AWARE)
    };

    // 4. Optional Header (240字节，x64版本)
    uint8_t optional_header[240] = {
        // 标准字段
        0x0B, 0x02,                                     // Magic (PE32+)
        0x0E, 0x00,                                     // MajorLinkerVersion, MinorLinkerVersion
        0x00, 0x10, 0x00, 0x00,                         // SizeOfCode (4096字节)
        0x00, 0x00, 0x00, 0x00,                         // SizeOfInitializedData
        0x00, 0x00, 0x00, 0x00,                         // SizeOfUninitializedData
        0x00, 0x10, 0x00, 0x00,                         // AddressOfEntryPoint (0x1000)
        0x00, 0x10, 0x00, 0x00,                         // BaseOfCode (0x1000)

        // Windows特定字段
        0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, // ImageBase (0x400000)
        0x00, 0x10, 0x00, 0x00,                         // SectionAlignment (4096)
        0x00, 0x02, 0x00, 0x00,                         // FileAlignment (512)
        0x06, 0x00, 0x00, 0x00,                         // MajorOSVersion, MinorOSVersion
        0x00, 0x00, 0x06, 0x00,                         // MajorImageVersion, MinorImageVersion
        0x06, 0x00, 0x00, 0x00,                         // MajorSubsystemVersion, MinorSubsystemVersion
        0x00, 0x00, 0x00, 0x00,                         // Win32VersionValue
        0x00, 0x20, 0x00, 0x00,                         // SizeOfImage (8192字节)
        0x00, 0x04, 0x00, 0x00,                         // SizeOfHeaders (1024字节)
        0x00, 0x00, 0x00, 0x00,                         // CheckSum
        0x03, 0x00,                                     // Subsystem (CONSOLE)
        0x00, 0x00,                                     // DllCharacteristics
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SizeOfStackReserve (1MB)
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SizeOfStackCommit (64KB)
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SizeOfHeapReserve (1MB)
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // SizeOfHeapCommit (4KB)
        0x00, 0x00, 0x00, 0x00,                         // LoaderFlags
        0x10, 0x00, 0x00, 0x00,                         // NumberOfRvaAndSizes (16)

        // 数据目录 (16个条目，每个8字节)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Export Table
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Import Table
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Resource Table
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Exception Table
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Certificate Table
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Base Relocation Table
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Debug
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Architecture
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Global Ptr
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // TLS Table
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Load Config Table
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Bound Import
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // IAT
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Delay Import Descriptor
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // COM+ Runtime Header
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Reserved
    };

    // 可选头（简化版）
    uint8_t optional_header[240] = {0};
    optional_header[0] = 0x0B;  // Magic (PE32+)
    optional_header[1] = 0x02;
    optional_header[4] = 0x01;  // MajorLinkerVersion
    optional_header[16] = 0x00; // AddressOfEntryPoint (低位)
    optional_header[17] = 0x10; // AddressOfEntryPoint (高位) = 0x1000
    optional_header[20] = 0x00; // BaseOfCode
    optional_header[21] = 0x10; // BaseOfCode = 0x1000

    // ImageBase (0x140000000 for x64)
    optional_header[24] = 0x00;
    optional_header[25] = 0x00;
    optional_header[26] = 0x00;
    optional_header[27] = 0x00;
    optional_header[28] = 0x01;
    optional_header[29] = 0x00;
    optional_header[30] = 0x00;
    optional_header[31] = 0x40;

    // SectionAlignment, FileAlignment
    optional_header[32] = 0x00; optional_header[33] = 0x10; // 0x1000
    optional_header[36] = 0x00; optional_header[37] = 0x02; // 0x200

    // Subsystem (Console = 3)
    optional_header[68] = 0x03;

    // 写入DOS头
    fwrite(dos_header, 1, 64, fp);

    // 填充到PE头位置
    uint8_t padding[64] = {0};
    fwrite(padding, 1, 64, fp);

    // 写入PE头
    fwrite(pe_header, 1, 24, fp);
    fwrite(optional_header, 1, 240, fp);

    // 节表
    uint8_t section_header[40] = {
        '.', 't', 'e', 'x', 't', 0, 0, 0,              // Name
        0x00, 0x10, 0x00, 0x00,                         // VirtualSize
        0x00, 0x10, 0x00, 0x00,                         // VirtualAddress
        0x00, 0x02, 0x00, 0x00,                         // SizeOfRawData
        0x00, 0x04, 0x00, 0x00,                         // PointerToRawData
        0x00, 0x00, 0x00, 0x00,                         // PointerToRelocations
        0x00, 0x00, 0x00, 0x00,                         // PointerToLinenumbers
        0x00, 0x00, 0x00, 0x00,                         // NumberOfRelocations, NumberOfLinenumbers
        0x20, 0x00, 0x00, 0x60                          // Characteristics (CODE | EXECUTE | READ)
    };

    fwrite(section_header, 1, 40, fp);

    // 填充到代码段开始位置 (0x400)
    long current_pos = ftell(fp);
    long padding_size = 0x400 - current_pos;
    if (padding_size > 0) {
        uint8_t* pad = calloc(1, padding_size);
        fwrite(pad, 1, padding_size, fp);
        free(pad);
    }

    // 写入机器码
    fwrite(code, 1, code_size, fp);

    // 填充到512字节对齐
    long final_pos = ftell(fp);
    long final_padding = ((final_pos + 511) & ~511) - final_pos;
    if (final_padding > 0) {
        uint8_t* pad = calloc(1, final_padding);
        fwrite(pad, 1, final_padding, fp);
        free(pad);
    }

    fclose(fp);
    printf("Generated PE executable: %s\n", output_file);
    return 0;
}

int compile_astc_to_runtime_bin(const char* astc_file, const char* output_file) {
    FILE* fp = fopen(astc_file, "rb");
    if (!fp) {
        printf("Error: Cannot open ASTC file %s\n", astc_file);
        return 1;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    size_t astc_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 读取ASTC文件内容
    unsigned char* astc_data = (unsigned char*)malloc(astc_size);
    if (!astc_data) {
        printf("Error: Memory allocation failed\n");
        fclose(fp);
        return 1;
    }

    size_t read_size = fread(astc_data, 1, astc_size, fp);
    fclose(fp);

    if (read_size != astc_size) {
        printf("Error: Failed to read ASTC file\n");
        free(astc_data);
        return 1;
    }

    printf("ASTC file size: %zu bytes\n", astc_size);

    // 创建代码生成器（自动检测架构）
    CodeGen* gen = astc_codegen_init(ARCH_UNKNOWN);
    if (!gen) {
        printf("Error: Failed to initialize code generator\n");
        free(astc_data);
        return 1;
    }

    // 使用新的JIT编译器：ASTC字节码 → 目标架构机器码
    if (compile_astc_to_machine_code(astc_data, astc_size, gen) != 0) {
        printf("Error: JIT compilation failed\n");
        free(astc_data);
        old_codegen_free(gen);
        return 1;
    }

    free(astc_data);

    // 生成运行时文件
    int result = generate_runtime_file(gen->code, gen->code_size, output_file);

    // 释放资源
    old_codegen_free(gen);

    return result;
}

int compile_c_to_runtime_bin(const char* c_file, const char* output_file) {
    // 首先将C文件编译成ASTC
    C2AstcOptions options = c2astc_default_options();
    struct ASTNode* ast = c2astc_convert_file(c_file, &options);

    if (!ast) {
        printf("Error: Failed to convert C file to ASTC\n");
        return 1;
    }

    // 创建代码生成器
    CodeGen* gen = old_codegen_init();
    if (!gen) {
        printf("Error: Failed to initialize code generator\n");
        ast_free(ast);
        return 1;
    }

    // 使用JIT编译器处理ASTC
    // 这里应该将C源码先转换为ASTC，然后JIT编译
    printf("Warning: C to runtime conversion should use C→ASTC→JIT pipeline\n");
    printf("Generating minimal runtime stub for compatibility\n");

    // 生成最小的Runtime机器码
    emit_byte(gen, 0x55);        // push rbp
    emit_byte(gen, 0x48);        // mov rbp, rsp
    emit_byte(gen, 0x89);
    emit_byte(gen, 0xe5);
    emit_byte(gen, 0xb8);        // mov eax, 42
    emit_int32(gen, 42);
    emit_byte(gen, 0x5d);        // pop rbp
    emit_byte(gen, 0xc3);        // ret

    // 生成运行时文件
    int result = generate_runtime_file(gen->code, gen->code_size, output_file);

    // 释放资源
    old_codegen_free(gen);
    ast_free(ast);

    return result;
}

// 编译AST节点到机器码
int compile_ast_node_to_machine_code(struct ASTNode* node, CodeGen* gen) {
    if (!node || !gen) {
        return 1;
    }

    switch (node->type) {
        case ASTC_TRANSLATION_UNIT:
            // 编译翻译单元中的所有声明
            for (int i = 0; i < node->data.translation_unit.declaration_count; i++) {
                compile_ast_node_to_machine_code(node->data.translation_unit.declarations[i], gen);
            }
            break;

        case ASTC_FUNC_DECL:
            // 编译函数声明
            if (node->data.func_decl.has_body) {
                compile_ast_node_to_machine_code(node->data.func_decl.body, gen);
            }
            break;

        case ASTC_COMPOUND_STMT:
            // 编译复合语句中的所有语句
            for (int i = 0; i < node->data.compound_stmt.statement_count; i++) {
                compile_ast_node_to_machine_code(node->data.compound_stmt.statements[i], gen);
            }
            break;

        case ASTC_EXPR_STMT:
            // 编译表达式语句
            compile_ast_node_to_machine_code(node->data.expr_stmt.expr, gen);
            break;

        case ASTC_CALL_EXPR:
            // 编译函数调用表达式
            printf("Found function call expression\n");

            // 检查AST中的libc标记
            if (node->data.call_expr.is_libc_call) {
                printf("Generating LIBC_CALL with ID: 0x%04X, args: %d\n",
                       node->data.call_expr.libc_func_id, node->data.call_expr.arg_count);

                // 生成LIBC_CALL指令
                ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
                table->emit_libc_call(gen, node->data.call_expr.libc_func_id, node->data.call_expr.arg_count);
            } else {
                // 普通函数调用
                if (node->data.call_expr.callee &&
                    node->data.call_expr.callee->type == ASTC_EXPR_IDENTIFIER) {
                    const char* func_name = node->data.call_expr.callee->data.identifier.name;
                    printf("Regular function call: %s\n", func_name);
                    // TODO: 处理普通函数调用
                }
            }
            break;

        case ASTC_RETURN_STMT:
            // 编译return语句
            {
                ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
                if (node->data.return_stmt.value) {
                    compile_ast_node_to_machine_code(node->data.return_stmt.value, gen);
                    table->emit_halt(gen);
                } else {
                    // 返回0
                    table->emit_const_i32(gen, 0);
                    table->emit_halt(gen);
                }
            }
            break;

        case ASTC_EXPR_CONSTANT:
            // 编译常量表达式
            if (node->data.constant.type == ASTC_TYPE_INT) {
                ArchCodegenTable* table = get_arch_codegen_table(gen->target_arch);
                table->emit_const_i32(gen, (uint32_t)node->data.constant.int_val);
            }
            break;

        default:
            // 其他节点类型暂时忽略
            printf("Ignoring AST node type: %d\n", node->type);
            break;
    }

    return 0;
}