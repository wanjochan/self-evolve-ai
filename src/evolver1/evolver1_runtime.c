/**
 * evolver1_runtime.c - evolver1运行时 (基于evolver0改进)
 * 
 * 基于evolver0_runtime的改进版本，增强ASTC虚拟机性能
 * 
 * 主要改进：
 * 1. 更高效的指令执行引擎
 * 2. 改进的内存管理
 * 3. 增强的调试和分析功能
 * 4. 更完整的ASTC指令支持
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// 引入公共组件
#include "../runtime/astc.h"
#include "../runtime/runtime.h"
#include "../tools/c2astc.h"

// ===============================================
// evolver1增强的运行时状态
// ===============================================

#define EVOLVER1_STACK_SIZE 2048      // 增大栈大小
#define EVOLVER1_LOCALS_SIZE 512      // 增大局部变量空间
#define EVOLVER1_GLOBALS_SIZE 256     // 全局变量空间
#define EVOLVER1_MEMORY_SIZE (64*1024) // 64KB内存空间

typedef struct {
    // 基础状态
    unsigned char* astc_data;
    size_t astc_size;
    size_t pc;                    // 程序计数器
    
    // 增强的执行栈
    int32_t stack[EVOLVER1_STACK_SIZE];
    int stack_top;
    
    // 增强的变量空间
    int32_t locals[EVOLVER1_LOCALS_SIZE];
    int32_t globals[EVOLVER1_GLOBALS_SIZE];
    
    // 增强的内存管理
    unsigned char memory[EVOLVER1_MEMORY_SIZE];
    size_t memory_used;
    
    // 执行状态
    int exit_code;
    bool running;
    bool debug_mode;
    
    // 性能统计 (evolver1新增)
    uint64_t instruction_count;
    uint64_t function_calls;
    uint64_t memory_allocations;
    
    // 错误处理 (evolver1新增)
    char error_message[256];
    bool has_error;
} Evolver1Runtime;

// ===============================================
// 增强的运行时管理
// ===============================================

Evolver1Runtime* evolver1_runtime_init(unsigned char* astc_data, size_t astc_size, bool debug_mode) {
    Evolver1Runtime* rt = malloc(sizeof(Evolver1Runtime));
    if (!rt) return NULL;
    
    // 初始化基础状态
    rt->astc_data = astc_data;
    rt->astc_size = astc_size;
    rt->pc = 0;
    rt->stack_top = 0;
    rt->exit_code = 0;
    rt->running = true;
    rt->debug_mode = debug_mode;
    rt->memory_used = 0;
    rt->has_error = false;
    
    // 初始化性能统计
    rt->instruction_count = 0;
    rt->function_calls = 0;
    rt->memory_allocations = 0;
    
    // 清空内存空间
    memset(rt->stack, 0, sizeof(rt->stack));
    memset(rt->locals, 0, sizeof(rt->locals));
    memset(rt->globals, 0, sizeof(rt->globals));
    memset(rt->memory, 0, sizeof(rt->memory));
    memset(rt->error_message, 0, sizeof(rt->error_message));
    
    if (debug_mode) {
        printf("evolver1_runtime: Initialized with enhanced features\n");
        printf("evolver1_runtime: Stack size: %d, Locals: %d, Memory: %d KB\n",
               EVOLVER1_STACK_SIZE, EVOLVER1_LOCALS_SIZE, EVOLVER1_MEMORY_SIZE/1024);
    }
    
    return rt;
}

void evolver1_runtime_free(Evolver1Runtime* rt) {
    if (rt) {
        if (rt->debug_mode) {
            printf("evolver1_runtime: Performance stats:\n");
            printf("  Instructions executed: %llu\n", rt->instruction_count);
            printf("  Function calls: %llu\n", rt->function_calls);
            printf("  Memory allocations: %llu\n", rt->memory_allocations);
        }
        free(rt);
    }
}

// ===============================================
// 增强的栈操作
// ===============================================

bool evolver1_push(Evolver1Runtime* rt, int32_t value) {
    if (rt->stack_top >= EVOLVER1_STACK_SIZE) {
        snprintf(rt->error_message, sizeof(rt->error_message), "Stack overflow");
        rt->has_error = true;
        return false;
    }
    rt->stack[rt->stack_top++] = value;
    return true;
}

int32_t evolver1_pop(Evolver1Runtime* rt) {
    if (rt->stack_top <= 0) {
        snprintf(rt->error_message, sizeof(rt->error_message), "Stack underflow");
        rt->has_error = true;
        return 0;
    }
    return rt->stack[--rt->stack_top];
}

int32_t evolver1_peek(Evolver1Runtime* rt, int offset) {
    int index = rt->stack_top - 1 - offset;
    if (index < 0 || index >= rt->stack_top) {
        snprintf(rt->error_message, sizeof(rt->error_message), "Invalid stack access");
        rt->has_error = true;
        return 0;
    }
    return rt->stack[index];
}

// ===============================================
// 增强的内存操作
// ===============================================

bool evolver1_memory_alloc(Evolver1Runtime* rt, size_t size, uint32_t* address) {
    if (rt->memory_used + size > EVOLVER1_MEMORY_SIZE) {
        snprintf(rt->error_message, sizeof(rt->error_message), "Out of memory");
        rt->has_error = true;
        return false;
    }
    
    *address = rt->memory_used;
    rt->memory_used += size;
    rt->memory_allocations++;
    
    if (rt->debug_mode) {
        printf("evolver1_runtime: Allocated %zu bytes at address 0x%X\n", size, *address);
    }
    
    return true;
}

bool evolver1_memory_read(Evolver1Runtime* rt, uint32_t address, void* buffer, size_t size) {
    if (address + size > EVOLVER1_MEMORY_SIZE) {
        snprintf(rt->error_message, sizeof(rt->error_message), "Memory read out of bounds");
        rt->has_error = true;
        return false;
    }
    
    memcpy(buffer, rt->memory + address, size);
    return true;
}

bool evolver1_memory_write(Evolver1Runtime* rt, uint32_t address, const void* data, size_t size) {
    if (address + size > EVOLVER1_MEMORY_SIZE) {
        snprintf(rt->error_message, sizeof(rt->error_message), "Memory write out of bounds");
        rt->has_error = true;
        return false;
    }
    
    memcpy(rt->memory + address, data, size);
    return true;
}

// ===============================================
// 增强的指令执行
// ===============================================

uint8_t evolver1_read_u8(Evolver1Runtime* rt) {
    if (rt->pc >= rt->astc_size) {
        rt->running = false;
        return 0;
    }
    return rt->astc_data[rt->pc++];
}

int32_t evolver1_read_i32(Evolver1Runtime* rt) {
    if (rt->pc + 4 > rt->astc_size) {
        rt->running = false;
        return 0;
    }
    int32_t value = *(int32_t*)(rt->astc_data + rt->pc);
    rt->pc += 4;
    return value;
}

bool evolver1_execute_instruction(Evolver1Runtime* rt) {
    if (rt->pc >= rt->astc_size || rt->has_error) {
        rt->running = false;
        return false;
    }
    
    uint8_t opcode = evolver1_read_u8(rt);
    rt->instruction_count++;
    
    if (rt->debug_mode) {
        printf("evolver1_runtime: PC=%zu, Opcode=0x%02X, Stack=%d\n", 
               rt->pc-1, opcode, rt->stack_top);
    }
    
    switch (opcode) {
        case 0x41: // i32.const
            {
                int32_t value = evolver1_read_i32(rt);
                if (!evolver1_push(rt, value)) return false;
                if (rt->debug_mode) {
                    printf("  i32.const %d\n", value);
                }
            }
            break;
            
        case 0x6A: // i32.add
            {
                int32_t b = evolver1_pop(rt);
                int32_t a = evolver1_pop(rt);
                if (rt->has_error) return false;
                if (!evolver1_push(rt, a + b)) return false;
                if (rt->debug_mode) {
                    printf("  i32.add %d + %d = %d\n", a, b, a + b);
                }
            }
            break;
            
        case 0x6B: // i32.sub
            {
                int32_t b = evolver1_pop(rt);
                int32_t a = evolver1_pop(rt);
                if (rt->has_error) return false;
                if (!evolver1_push(rt, a - b)) return false;
                if (rt->debug_mode) {
                    printf("  i32.sub %d - %d = %d\n", a, b, a - b);
                }
            }
            break;
            
        case 0x6C: // i32.mul
            {
                int32_t b = evolver1_pop(rt);
                int32_t a = evolver1_pop(rt);
                if (rt->has_error) return false;
                if (!evolver1_push(rt, a * b)) return false;
                if (rt->debug_mode) {
                    printf("  i32.mul %d * %d = %d\n", a, b, a * b);
                }
            }
            break;
            
        case 0x20: // local.get
            {
                uint8_t index = evolver1_read_u8(rt);
                if (index >= EVOLVER1_LOCALS_SIZE) {
                    snprintf(rt->error_message, sizeof(rt->error_message), 
                            "Local variable index out of bounds: %d", index);
                    rt->has_error = true;
                    return false;
                }
                if (!evolver1_push(rt, rt->locals[index])) return false;
                if (rt->debug_mode) {
                    printf("  local.get %d = %d\n", index, rt->locals[index]);
                }
            }
            break;
            
        case 0x21: // local.set
            {
                uint8_t index = evolver1_read_u8(rt);
                if (index >= EVOLVER1_LOCALS_SIZE) {
                    snprintf(rt->error_message, sizeof(rt->error_message), 
                            "Local variable index out of bounds: %d", index);
                    rt->has_error = true;
                    return false;
                }
                rt->locals[index] = evolver1_pop(rt);
                if (rt->has_error) return false;
                if (rt->debug_mode) {
                    printf("  local.set %d = %d\n", index, rt->locals[index]);
                }
            }
            break;
            
        case 0x10: // call
            {
                uint32_t func_index = evolver1_read_i32(rt);
                rt->function_calls++;
                if (rt->debug_mode) {
                    printf("  call function %u\n", func_index);
                }
                // 简化的函数调用处理
            }
            break;
            
        case 0x0F: // return
            {
                if (rt->stack_top > 0) {
                    rt->exit_code = evolver1_pop(rt);
                }
                rt->running = false;
                if (rt->debug_mode) {
                    printf("  return %d\n", rt->exit_code);
                }
            }
            break;
            
        default:
            if (rt->debug_mode) {
                printf("  unknown opcode: 0x%02X\n", opcode);
            }
            // 对于未知指令，继续执行而不是停止
            break;
    }
    
    return !rt->has_error;
}

// ===============================================
// 主执行函数
// ===============================================

int evolver1_runtime_execute(Evolver1Runtime* rt) {
    if (!rt) return -1;
    
    printf("evolver1_runtime: Starting enhanced ASTC execution\n");
    
    // 验证ASTC头部
    if (rt->astc_size < 8) {
        printf("evolver1_runtime: Error - ASTC file too small\n");
        return -1;
    }
    
    if (memcmp(rt->astc_data, "ASTC", 4) != 0) {
        printf("evolver1_runtime: Error - Invalid ASTC magic number\n");
        return -1;
    }
    
    int version = *(int*)(rt->astc_data + 4);
    printf("evolver1_runtime: ASTC version: %d\n", version);
    
    // 跳过头部
    rt->pc = 8;
    
    // 执行指令循环
    while (rt->running && rt->instruction_count < 100000) { // 防止无限循环
        if (!evolver1_execute_instruction(rt)) {
            if (rt->has_error) {
                printf("evolver1_runtime: Execution error: %s\n", rt->error_message);
                return -1;
            }
            break;
        }
    }
    
    if (rt->instruction_count >= 100000) {
        printf("evolver1_runtime: Warning - Maximum instruction limit reached\n");
    }
    
    printf("evolver1_runtime: Execution completed\n");
    printf("evolver1_runtime: Instructions executed: %llu\n", rt->instruction_count);
    printf("evolver1_runtime: Exit code: %d\n", rt->exit_code);
    
    return rt->exit_code;
}

// ===============================================
// 主函数 (用于独立测试)
// ===============================================

int main(int argc, char* argv[]) {
    printf("evolver1_runtime v1.0 - Enhanced ASTC Runtime\n");
    
    bool debug_mode = false;
    char* astc_file = NULL;
    
    // 解析参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
        } else if (argv[i][0] != '-') {
            astc_file = argv[i];
        }
    }
    
    if (!astc_file) {
        printf("Usage: %s [--debug] <program.astc>\n", argv[0]);
        return 1;
    }
    
    // 加载ASTC文件
    FILE* file = fopen(astc_file, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", astc_file);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    unsigned char* data = malloc(size);
    if (!data) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    
    fread(data, 1, size, file);
    fclose(file);
    
    // 创建运行时并执行
    Evolver1Runtime* rt = evolver1_runtime_init(data, size, debug_mode);
    if (!rt) {
        printf("Error: Runtime initialization failed\n");
        free(data);
        return 1;
    }
    
    int exit_code = evolver1_runtime_execute(rt);
    
    evolver1_runtime_free(rt);
    free(data);
    
    return exit_code;
}
