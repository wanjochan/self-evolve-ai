/**
 * multi_arch_backend.h - 多架构后端支持
 * 
 * 统一的多架构代码生成系统
 */

#ifndef MULTI_ARCH_BACKEND_H
#define MULTI_ARCH_BACKEND_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "astc2native.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// 扩展的架构支持
// ===============================================

// 使用已有的TargetArch定义，并扩展更多架构
typedef TargetArch ArchType;

// 扩展的架构类型
#define ARCH_RISCV32 5
#define ARCH_RISCV64 6
#define ARCH_MIPS32 7
#define ARCH_MIPS64 8
#define ARCH_WASM32 9
#define ARCH_WASM64 10

typedef enum {
    ENDIAN_LITTLE = 0,
    ENDIAN_BIG = 1
} EndianType;

typedef enum {
    ABI_SYSV = 1,       // System V ABI (Linux, Unix)
    ABI_WIN64 = 2,      // Windows x64 ABI
    ABI_AAPCS = 3,      // ARM AAPCS
    ABI_RISCV = 4,      // RISC-V ABI
    ABI_MIPS = 5,       // MIPS ABI
    ABI_WASM = 6        // WebAssembly ABI
} AbiType;

// ===============================================
// 架构特性描述
// ===============================================

typedef struct {
    ArchType arch;
    const char* name;
    const char* description;
    uint32_t word_size;         // 字长(字节)
    uint32_t pointer_size;      // 指针大小(字节)
    uint32_t register_count;    // 通用寄存器数量
    uint32_t stack_alignment;   // 栈对齐要求
    EndianType endianness;      // 字节序
    AbiType default_abi;        // 默认ABI
    bool has_fpu;              // 是否有浮点单元
    bool has_vector;           // 是否有向量指令
    bool has_atomic;           // 是否有原子指令
    bool supports_pic;         // 是否支持位置无关代码
} ArchInfo;

// ===============================================
// 架构特定的代码生成接口
// ===============================================

typedef struct ArchCodegen {
    ArchType arch;
    
    // 基础代码生成
    int (*emit_prologue)(void* gen);
    int (*emit_epilogue)(void* gen);
    int (*emit_nop)(void* gen);
    int (*emit_halt)(void* gen);
    
    // 常量加载
    int (*emit_const_i32)(void* gen, uint32_t value);
    int (*emit_const_i64)(void* gen, uint64_t value);
    int (*emit_const_f32)(void* gen, float value);
    int (*emit_const_f64)(void* gen, double value);
    int (*emit_const_string)(void* gen, const char* str, uint32_t len);
    
    // 算术运算
    int (*emit_add_i32)(void* gen);
    int (*emit_sub_i32)(void* gen);
    int (*emit_mul_i32)(void* gen);
    int (*emit_div_i32)(void* gen);
    int (*emit_mod_i32)(void* gen);
    
    // 位运算
    int (*emit_and)(void* gen);
    int (*emit_or)(void* gen);
    int (*emit_xor)(void* gen);
    int (*emit_not)(void* gen);
    int (*emit_shl)(void* gen);
    int (*emit_shr)(void* gen);
    
    // 比较运算
    int (*emit_eq)(void* gen);
    int (*emit_ne)(void* gen);
    int (*emit_lt)(void* gen);
    int (*emit_le)(void* gen);
    int (*emit_gt)(void* gen);
    int (*emit_ge)(void* gen);
    
    // 内存操作
    int (*emit_load_local)(void* gen, uint32_t offset);
    int (*emit_store_local)(void* gen, uint32_t offset);
    int (*emit_load_global)(void* gen, uint32_t address);
    int (*emit_store_global)(void* gen, uint32_t address);
    int (*emit_load_indirect)(void* gen);
    int (*emit_store_indirect)(void* gen);
    
    // 控制流
    int (*emit_jump)(void* gen, uint32_t target);
    int (*emit_jump_if_true)(void* gen, uint32_t target);
    int (*emit_jump_if_false)(void* gen, uint32_t target);
    int (*emit_call)(void* gen, uint32_t target);
    int (*emit_call_indirect)(void* gen);
    int (*emit_return)(void* gen);
    
    // 函数调用
    int (*emit_libc_call)(void* gen, uint32_t func_id);
    int (*emit_syscall)(void* gen, uint32_t syscall_num);
    
    // 栈操作
    int (*emit_push)(void* gen);
    int (*emit_pop)(void* gen);
    int (*emit_dup)(void* gen);
    int (*emit_swap)(void* gen);
    
    // 类型转换
    int (*emit_i32_to_i64)(void* gen);
    int (*emit_i64_to_i32)(void* gen);
    int (*emit_i32_to_f32)(void* gen);
    int (*emit_f32_to_i32)(void* gen);
    
    // 架构特定优化
    int (*optimize_instruction_sequence)(void* gen, uint8_t* opcodes, size_t count);
    int (*emit_optimized_loop)(void* gen, uint32_t start, uint32_t end);
    int (*emit_optimized_call)(void* gen, uint32_t target, uint32_t arg_count);
    
} ArchCodegen;

// ===============================================
// 多架构后端管理器
// ===============================================

typedef struct MultiArchBackend {
    ArchCodegen* codegens[16];  // 支持的架构代码生成器
    uint32_t arch_count;        // 支持的架构数量
    ArchType current_arch;      // 当前目标架构
    ArchInfo* arch_infos[16];   // 架构信息
    
    // 运行时架构检测
    ArchType host_arch;         // 宿主架构
    bool cross_compilation;     // 是否交叉编译
    
    // 优化选项
    bool enable_arch_specific_opts; // 启用架构特定优化
    bool enable_cross_arch_compat;  // 启用跨架构兼容
    
    // 统计信息
    uint32_t total_compilations;
    uint32_t arch_specific_opts_applied;
    uint32_t cross_arch_calls;
    
} MultiArchBackend;

// ===============================================
// 多架构后端API
// ===============================================

/**
 * 初始化多架构后端
 */
MultiArchBackend* multi_arch_backend_init(void);

/**
 * 释放多架构后端
 */
void multi_arch_backend_free(MultiArchBackend* backend);

/**
 * 注册架构代码生成器
 */
int multi_arch_register_codegen(MultiArchBackend* backend, ArchType arch, ArchCodegen* codegen);

/**
 * 获取架构代码生成器
 */
ArchCodegen* multi_arch_get_codegen(MultiArchBackend* backend, ArchType arch);

/**
 * 设置目标架构
 */
int multi_arch_set_target(MultiArchBackend* backend, ArchType arch);

/**
 * 获取架构信息
 */
ArchInfo* multi_arch_get_arch_info(MultiArchBackend* backend, ArchType arch);

/**
 * 检测运行时架构
 */
ArchType multi_arch_detect_host_architecture(void);

/**
 * 检查架构兼容性
 */
bool multi_arch_is_compatible(ArchType source, ArchType target);

/**
 * 编译ASTC到多架构机器码
 */
int multi_arch_compile_astc(MultiArchBackend* backend, uint8_t* astc_data, 
                           size_t astc_size, ArchType target_arch, 
                           uint8_t** output_code, size_t* output_size);

// ===============================================
// 架构特定实现
// ===============================================

/**
 * 创建x86_64代码生成器
 */
ArchCodegen* create_x86_64_codegen(void);

/**
 * 创建ARM64代码生成器
 */
ArchCodegen* create_arm64_codegen(void);

/**
 * 创建RISC-V代码生成器
 */
ArchCodegen* create_riscv64_codegen(void);

/**
 * 创建WebAssembly代码生成器
 */
ArchCodegen* create_wasm32_codegen(void);

// ===============================================
// 架构信息查询
// ===============================================

/**
 * 获取架构名称
 */
const char* multi_arch_get_name(ArchType arch);

/**
 * 获取架构描述
 */
const char* multi_arch_get_description(ArchType arch);

/**
 * 获取支持的架构列表
 */
void multi_arch_list_supported_architectures(MultiArchBackend* backend);

/**
 * 检查架构特性支持
 */
bool multi_arch_supports_feature(ArchType arch, const char* feature);

// ===============================================
// 交叉编译支持
// ===============================================

/**
 * 启用交叉编译模式
 */
int multi_arch_enable_cross_compilation(MultiArchBackend* backend, 
                                       ArchType host, ArchType target);

/**
 * 生成交叉编译工具链
 */
int multi_arch_generate_cross_toolchain(MultiArchBackend* backend, 
                                       ArchType target, const char* output_dir);

/**
 * 验证交叉编译结果
 */
int multi_arch_validate_cross_compiled_code(uint8_t* code, size_t size, ArchType arch);

// ===============================================
// 性能优化
// ===============================================

/**
 * 应用架构特定优化
 */
int multi_arch_apply_arch_optimizations(MultiArchBackend* backend, 
                                       uint8_t* code, size_t* size, ArchType arch);

/**
 * 生成优化报告
 */
void multi_arch_generate_optimization_report(MultiArchBackend* backend, 
                                            const char* filename);

/**
 * 获取性能统计
 */
typedef struct {
    uint32_t total_instructions;
    uint32_t arch_specific_instructions;
    uint32_t optimized_instructions;
    uint32_t cross_arch_calls;
    float optimization_ratio;
    uint64_t compilation_time_us;
} MultiArchStats;

void multi_arch_get_stats(MultiArchBackend* backend, MultiArchStats* stats);

// ===============================================
// 调试和诊断
// ===============================================

/**
 * 打印多架构后端状态
 */
void multi_arch_print_status(MultiArchBackend* backend);

/**
 * 反汇编生成的机器码
 */
int multi_arch_disassemble_code(uint8_t* code, size_t size, ArchType arch, 
                               char* output, size_t output_size);

/**
 * 验证生成的机器码
 */
bool multi_arch_validate_machine_code(uint8_t* code, size_t size, ArchType arch);

#ifdef __cplusplus
}
#endif

#endif // MULTI_ARCH_BACKEND_H
