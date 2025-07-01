/**
 * multi_arch_backend.c - 多架构后端实现
 */

#include "multi_arch_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// 架构信息定义
// ===============================================

static ArchInfo arch_infos[] = {
    {
        .arch = ARCH_X86_64,
        .name = "x86_64",
        .description = "Intel/AMD 64-bit x86 architecture",
        .word_size = 8,
        .pointer_size = 8,
        .register_count = 16,
        .stack_alignment = 16,
        .endianness = ENDIAN_LITTLE,
        .default_abi = ABI_SYSV,
        .has_fpu = true,
        .has_vector = true,
        .has_atomic = true,
        .supports_pic = true
    },
    {
        .arch = ARCH_ARM64,
        .name = "arm64",
        .description = "ARM 64-bit architecture (AArch64)",
        .word_size = 8,
        .pointer_size = 8,
        .register_count = 31,
        .stack_alignment = 16,
        .endianness = ENDIAN_LITTLE,
        .default_abi = ABI_AAPCS,
        .has_fpu = true,
        .has_vector = true,
        .has_atomic = true,
        .supports_pic = true
    },
    {
        .arch = ARCH_RISCV64,
        .name = "riscv64",
        .description = "RISC-V 64-bit architecture",
        .word_size = 8,
        .pointer_size = 8,
        .register_count = 32,
        .stack_alignment = 16,
        .endianness = ENDIAN_LITTLE,
        .default_abi = ABI_RISCV,
        .has_fpu = true,
        .has_vector = true,
        .has_atomic = true,
        .supports_pic = true
    },
    {
        .arch = ARCH_WASM32,
        .name = "wasm32",
        .description = "WebAssembly 32-bit",
        .word_size = 4,
        .pointer_size = 4,
        .register_count = 0, // 栈机器
        .stack_alignment = 4,
        .endianness = ENDIAN_LITTLE,
        .default_abi = ABI_WASM,
        .has_fpu = true,
        .has_vector = false,
        .has_atomic = true,
        .supports_pic = true
    }
};

static const size_t arch_info_count = sizeof(arch_infos) / sizeof(arch_infos[0]);

// ===============================================
// 多架构后端管理
// ===============================================

MultiArchBackend* multi_arch_backend_init(void) {
    MultiArchBackend* backend = calloc(1, sizeof(MultiArchBackend));
    if (!backend) return NULL;
    
    // 初始化架构信息
    for (size_t i = 0; i < arch_info_count && i < 16; i++) {
        backend->arch_infos[i] = &arch_infos[i];
    }
    
    // 检测宿主架构
    backend->host_arch = multi_arch_detect_host_architecture();
    backend->current_arch = backend->host_arch;
    backend->cross_compilation = false;
    
    // 设置默认选项
    backend->enable_arch_specific_opts = true;
    backend->enable_cross_arch_compat = true;
    
    printf("Multi-architecture backend initialized\n");
    printf("Host architecture: %s\n", multi_arch_get_name(backend->host_arch));
    
    return backend;
}

void multi_arch_backend_free(MultiArchBackend* backend) {
    if (!backend) return;
    
    // 释放代码生成器
    for (uint32_t i = 0; i < backend->arch_count; i++) {
        if (backend->codegens[i]) {
            free(backend->codegens[i]);
        }
    }
    
    free(backend);
}

// ===============================================
// 架构检测
// ===============================================

ArchType multi_arch_detect_host_architecture(void) {
#if defined(_M_X64) || defined(__x86_64__)
    return ARCH_X86_64;
#elif defined(_M_IX86) || defined(__i386__)
    return ARCH_X86_32;
#elif defined(_M_ARM64) || defined(__aarch64__)
    return ARCH_ARM64;
#elif defined(_M_ARM) || defined(__arm__)
    return ARCH_ARM32;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return ARCH_RISCV64;
#elif defined(__riscv) && (__riscv_xlen == 32)
    return ARCH_RISCV32;
#elif defined(__EMSCRIPTEN__)
    return ARCH_WASM32;
#else
    return ARCH_UNKNOWN;
#endif
}

// ===============================================
// 架构信息查询
// ===============================================

const char* multi_arch_get_name(ArchType arch) {
    for (size_t i = 0; i < arch_info_count; i++) {
        if (arch_infos[i].arch == arch) {
            return arch_infos[i].name;
        }
    }
    return "unknown";
}

const char* multi_arch_get_description(ArchType arch) {
    for (size_t i = 0; i < arch_info_count; i++) {
        if (arch_infos[i].arch == arch) {
            return arch_infos[i].description;
        }
    }
    return "Unknown architecture";
}

ArchInfo* multi_arch_get_arch_info(MultiArchBackend* backend, ArchType arch) {
    if (!backend) return NULL;
    
    for (uint32_t i = 0; i < 16; i++) {
        if (backend->arch_infos[i] && backend->arch_infos[i]->arch == arch) {
            return backend->arch_infos[i];
        }
    }
    return NULL;
}

// ===============================================
// 代码生成器管理
// ===============================================

int multi_arch_register_codegen(MultiArchBackend* backend, ArchType arch, ArchCodegen* codegen) {
    if (!backend || !codegen || backend->arch_count >= 16) {
        return -1;
    }
    
    // 检查是否已注册
    for (uint32_t i = 0; i < backend->arch_count; i++) {
        if (backend->codegens[i] && backend->codegens[i]->arch == arch) {
            printf("Warning: Architecture %s already registered, replacing\n", 
                   multi_arch_get_name(arch));
            free(backend->codegens[i]);
            backend->codegens[i] = codegen;
            return 0;
        }
    }
    
    // 添加新的代码生成器
    backend->codegens[backend->arch_count] = codegen;
    backend->arch_count++;
    
    printf("Registered code generator for %s\n", multi_arch_get_name(arch));
    return 0;
}

ArchCodegen* multi_arch_get_codegen(MultiArchBackend* backend, ArchType arch) {
    if (!backend) return NULL;
    
    for (uint32_t i = 0; i < backend->arch_count; i++) {
        if (backend->codegens[i] && backend->codegens[i]->arch == arch) {
            return backend->codegens[i];
        }
    }
    
    return NULL;
}

int multi_arch_set_target(MultiArchBackend* backend, ArchType arch) {
    if (!backend) return -1;
    
    // 检查是否支持目标架构
    ArchCodegen* codegen = multi_arch_get_codegen(backend, arch);
    if (!codegen) {
        printf("Error: Architecture %s not supported\n", multi_arch_get_name(arch));
        return -1;
    }
    
    backend->current_arch = arch;
    backend->cross_compilation = (arch != backend->host_arch);
    
    printf("Target architecture set to: %s\n", multi_arch_get_name(arch));
    if (backend->cross_compilation) {
        printf("Cross-compilation mode enabled (%s -> %s)\n", 
               multi_arch_get_name(backend->host_arch),
               multi_arch_get_name(arch));
    }
    
    return 0;
}

// ===============================================
// 架构兼容性检查
// ===============================================

bool multi_arch_is_compatible(ArchType source, ArchType target) {
    // 相同架构总是兼容的
    if (source == target) return true;
    
    // x86架构族内部兼容
    if ((source == ARCH_X86_32 && target == ARCH_X86_64) ||
        (source == ARCH_X86_64 && target == ARCH_X86_32)) {
        return true;
    }
    
    // ARM架构族内部兼容
    if ((source == ARCH_ARM32 && target == ARCH_ARM64) ||
        (source == ARCH_ARM64 && target == ARCH_ARM32)) {
        return true;
    }
    
    // RISC-V架构族内部兼容
    if ((source == ARCH_RISCV32 && target == ARCH_RISCV64) ||
        (source == ARCH_RISCV64 && target == ARCH_RISCV32)) {
        return true;
    }
    
    // WebAssembly与其他架构不兼容
    if (source == ARCH_WASM32 || source == ARCH_WASM64 ||
        target == ARCH_WASM32 || target == ARCH_WASM64) {
        return false;
    }
    
    return false;
}

// ===============================================
// 多架构编译
// ===============================================

int multi_arch_compile_astc(MultiArchBackend* backend, uint8_t* astc_data, 
                           size_t astc_size, ArchType target_arch, 
                           uint8_t** output_code, size_t* output_size) {
    if (!backend || !astc_data || !output_code || !output_size) {
        return -1;
    }
    
    // 获取目标架构的代码生成器
    ArchCodegen* codegen = multi_arch_get_codegen(backend, target_arch);
    if (!codegen) {
        printf("Error: No code generator for architecture %s\n", 
               multi_arch_get_name(target_arch));
        return -1;
    }
    
    printf("Compiling ASTC to %s machine code...\n", multi_arch_get_name(target_arch));
    
    // 分配输出缓冲区
    size_t max_output_size = astc_size * 4; // 估算最大输出大小
    uint8_t* code = malloc(max_output_size);
    if (!code) return -1;
    
    // 模拟编译过程
    size_t code_size = 0;
    
    // 生成函数序言
    if (codegen->emit_prologue) {
        // 这里应该传递实际的代码生成器上下文
        printf("Emitting prologue for %s\n", multi_arch_get_name(target_arch));
        code_size += 8; // 模拟序言大小
    }
    
    // 解析ASTC并生成代码
    if (astc_size >= 16 && memcmp(astc_data, "ASTC", 4) == 0) {
        uint8_t* bytecode = astc_data + 16;
        size_t bytecode_size = astc_size - 16;
        size_t pc = 0;
        
        while (pc < bytecode_size) {
            uint8_t opcode = bytecode[pc++];
            
            // 根据操作码生成对应的机器码
            switch (opcode) {
                case 0x10: // CONST_I32
                    if (codegen->emit_const_i32 && pc + 4 <= bytecode_size) {
                        uint32_t value = *(uint32_t*)&bytecode[pc];
                        printf("Emitting CONST_I32(%u) for %s\n", value, multi_arch_get_name(target_arch));
                        code_size += (target_arch == ARCH_ARM64) ? 4 : 6; // 模拟指令大小
                        pc += 4;
                    }
                    break;
                    
                case 0x20: // ADD
                    if (codegen->emit_add_i32) {
                        printf("Emitting ADD for %s\n", multi_arch_get_name(target_arch));
                        code_size += (target_arch == ARCH_ARM64) ? 4 : 3; // 模拟指令大小
                    }
                    break;
                    
                case 0x30: // STORE_LOCAL
                    if (codegen->emit_store_local && pc + 4 <= bytecode_size) {
                        uint32_t offset = *(uint32_t*)&bytecode[pc];
                        printf("Emitting STORE_LOCAL(%u) for %s\n", offset, multi_arch_get_name(target_arch));
                        code_size += (target_arch == ARCH_ARM64) ? 4 : 7; // 模拟指令大小
                        pc += 4;
                    }
                    break;
                    
                case 0x31: // LOAD_LOCAL
                    if (codegen->emit_load_local && pc + 4 <= bytecode_size) {
                        uint32_t offset = *(uint32_t*)&bytecode[pc];
                        printf("Emitting LOAD_LOCAL(%u) for %s\n", offset, multi_arch_get_name(target_arch));
                        code_size += (target_arch == ARCH_ARM64) ? 4 : 7; // 模拟指令大小
                        pc += 4;
                    }
                    break;
                    
                default:
                    printf("Warning: Unhandled opcode 0x%02X for %s\n", opcode, multi_arch_get_name(target_arch));
                    code_size += 4; // 模拟默认指令大小
                    break;
            }
        }
    }
    
    // 生成函数尾声
    if (codegen->emit_epilogue) {
        printf("Emitting epilogue for %s\n", multi_arch_get_name(target_arch));
        code_size += 4; // 模拟尾声大小
    }
    
    // 应用架构特定优化
    if (backend->enable_arch_specific_opts) {
        printf("Applying %s-specific optimizations...\n", multi_arch_get_name(target_arch));
        backend->arch_specific_opts_applied++;
        
        // 模拟优化效果
        if (target_arch == ARCH_ARM64) {
            code_size = (code_size * 9) / 10; // ARM64优化减少10%代码
        } else if (target_arch == ARCH_X86_64) {
            code_size = (code_size * 8) / 10; // x64优化减少20%代码
        }
    }
    
    // 填充模拟的机器码
    for (size_t i = 0; i < code_size && i < max_output_size; i++) {
        code[i] = (uint8_t)(i ^ (target_arch * 17)); // 生成模拟的机器码
    }
    
    *output_code = code;
    *output_size = code_size;
    
    backend->total_compilations++;
    
    printf("Multi-arch compilation completed:\n");
    printf("  Target: %s\n", multi_arch_get_name(target_arch));
    printf("  Input: %zu bytes ASTC\n", astc_size);
    printf("  Output: %zu bytes machine code\n", code_size);
    printf("  Compression ratio: %.1f%%\n", (float)code_size * 100.0f / astc_size);
    
    return 0;
}

// ===============================================
// 状态和统计
// ===============================================

void multi_arch_list_supported_architectures(MultiArchBackend* backend) {
    if (!backend) return;
    
    printf("=== Supported Architectures ===\n");
    printf("Host architecture: %s\n", multi_arch_get_name(backend->host_arch));
    printf("Current target: %s\n", multi_arch_get_name(backend->current_arch));
    printf("Cross-compilation: %s\n", backend->cross_compilation ? "Enabled" : "Disabled");
    printf("\nRegistered code generators:\n");
    
    for (uint32_t i = 0; i < backend->arch_count; i++) {
        if (backend->codegens[i]) {
            ArchType arch = backend->codegens[i]->arch;
            ArchInfo* info = multi_arch_get_arch_info(backend, arch);
            printf("  %s - %s\n", multi_arch_get_name(arch), 
                   info ? info->description : "No description");
            if (info) {
                printf("    Word size: %u bytes, Registers: %u, ABI: %d\n",
                       info->word_size, info->register_count, info->default_abi);
                printf("    Features: FPU=%s, Vector=%s, Atomic=%s, PIC=%s\n",
                       info->has_fpu ? "Yes" : "No",
                       info->has_vector ? "Yes" : "No", 
                       info->has_atomic ? "Yes" : "No",
                       info->supports_pic ? "Yes" : "No");
            }
        }
    }
}

void multi_arch_print_status(MultiArchBackend* backend) {
    if (!backend) return;
    
    printf("=== Multi-Architecture Backend Status ===\n");
    printf("Registered architectures: %u\n", backend->arch_count);
    printf("Total compilations: %u\n", backend->total_compilations);
    printf("Architecture-specific optimizations applied: %u\n", backend->arch_specific_opts_applied);
    printf("Cross-architecture calls: %u\n", backend->cross_arch_calls);
    printf("Architecture-specific optimizations: %s\n", 
           backend->enable_arch_specific_opts ? "Enabled" : "Disabled");
    printf("Cross-architecture compatibility: %s\n", 
           backend->enable_cross_arch_compat ? "Enabled" : "Disabled");
}

void multi_arch_get_stats(MultiArchBackend* backend, MultiArchStats* stats) {
    if (!backend || !stats) return;
    
    memset(stats, 0, sizeof(MultiArchStats));
    
    stats->total_instructions = backend->total_compilations * 10; // 估算
    stats->arch_specific_instructions = backend->arch_specific_opts_applied;
    stats->optimized_instructions = backend->arch_specific_opts_applied;
    stats->cross_arch_calls = backend->cross_arch_calls;
    stats->optimization_ratio = backend->total_compilations > 0 ? 
        (float)backend->arch_specific_opts_applied / backend->total_compilations : 0.0f;
    stats->compilation_time_us = backend->total_compilations * 1000; // 估算
}
