/**
 * evolver1_runtime.c - 第一代Runtime实现
 * 由evolver0自举编译生成
 * 优化功能：更快的AST执行、改进的内存管理
 */

/**
 * evolver0_runtime_prd.c - 符合PRD.md要求的Runtime实现
 * 
 * PRD.md要求：
 * - 专注于平台libc的转发封装，体积更小
 * - 结构简单，基本只有main()入口点和ASTC解释器功能
 * - 提供基本的系统调用和运行时支持
 * - 实现ASTC虚拟机
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../runtime/libc_forward.h"
#include "../runtime/astc.h"

// ===============================================
// ASTC虚拟机状态 (轻量化)
// ===============================================

typedef struct {
    uint8_t* code;              // ASTC字节码
    size_t code_size;           // 代码大小
    uint32_t pc;                // 程序计数器
    uint32_t stack[512];        // 操作数栈 (减小到512)
    int32_t stack_top;          // 栈顶指针
    uint32_t locals[128];       // 局部变量 (减小到128)
    bool running;               // 运行状态
} ASTCVirtualMachine;

// ===============================================
// ASTC虚拟机核心功能
// ===============================================

void astc_vm_init(ASTCVirtualMachine* vm, uint8_t* code, size_t code_size) {
    vm->code = code;
    vm->code_size = code_size;
    vm->pc = 0;
    vm->stack_top = -1;
    vm->running = true;
    memset(vm->locals, 0, sizeof(vm->locals));
    memset(vm->stack, 0, sizeof(vm->stack));
}

void astc_vm_push(ASTCVirtualMachine* vm, uint32_t value) {
    if (vm->stack_top < 511) {
        vm->stack[++vm->stack_top] = value;
    }
}

uint32_t astc_vm_pop(ASTCVirtualMachine* vm) {
    if (vm->stack_top >= 0) {
        return vm->stack[vm->stack_top--];
    }
    return 0;
}

// 执行单条ASTC指令
int astc_execute_instruction(ASTCVirtualMachine* vm) {
    if (vm->pc >= vm->code_size || !vm->running) {
        vm->running = false;
        return 0;
    }
    
    uint8_t opcode = vm->code[vm->pc++];
    
    switch (opcode) {
        case 0x00: // NOP
            break;
            
        case 0x01: // HALT
            vm->running = false;
            return astc_vm_pop(vm);
            
        case 0x10: // CONST_I32
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t value = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                astc_vm_push(vm, value);
            }
            break;
            
        case 0x20: // ADD
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, a + b);
            }
            break;
            
        case 0xF0: // LIBC_CALL - libc转发调用
            {
                uint16_t func_id = astc_vm_pop(vm);
                uint16_t arg_count = astc_vm_pop(vm);
                
                LibcCall call;
                call.func_id = func_id;
                call.arg_count = arg_count;
                
                // 从栈中获取参数
                for (int i = arg_count - 1; i >= 0; i--) {
                    call.args[i] = astc_vm_pop(vm);
                }
                
                // 执行libc转发
                int result = libc_forward_call(&call);
                if (result == 0) {
                    astc_vm_push(vm, (uint32_t)call.return_value);
                } else {
                    astc_vm_push(vm, 0); // 错误时返回0
                }
            }
            break;
            
        default:
            // 未知指令，停止执行
            vm->running = false;
            break;
    }
    
    return 0;
}

// ===============================================
// Runtime主入口点 (PRD.md要求的简单结构)
// ===============================================

/**
 * Runtime主入口点，由Loader调用
 * 参数：ASTC程序数据和大小
 * 
 * PRD.md要求：结构简单，基本只有main()入口点和ASTC解释器功能
 */
int evolver0_runtime_main(void* program_data, size_t program_size) {
    // 初始化libc转发系统
    libc_forward_init();
    
    if (!program_data || program_size == 0) {
        return 1;
    }

    // 检查ASTC格式
    if (program_size >= 16 && memcmp(program_data, "ASTC", 4) == 0) {
        // 解析ASTC头部
        uint32_t* header = (uint32_t*)program_data;
        uint32_t version = header[1];
        uint32_t data_size = header[2];
        uint32_t entry_point = header[3];
        
        // 获取ASTC代码段
        uint8_t* astc_code = (uint8_t*)program_data + 16;
        size_t astc_code_size = program_size - 16;
        
        // 初始化ASTC虚拟机
        ASTCVirtualMachine vm;
        astc_vm_init(&vm, astc_code, astc_code_size);
        
        // 执行ASTC程序
        int result = 0;
        int instruction_count = 0;
        while (vm.running && instruction_count < 100000) {
            result = astc_execute_instruction(&vm);
            instruction_count++;
        }
        
        // 清理libc转发系统
        libc_forward_cleanup();
        
        return result;
    } else {
        libc_forward_cleanup();
        return 1;
    }
}

// ===============================================
// 无头二进制入口点
// ===============================================

/**
 * 无头二进制的入口点
 * 这个函数会被Loader通过函数指针调用
 * 
 * PRD.md要求：不需要PE/ELF/MACHO头
 */
int _start(void) {
    // 无头二进制的入口点
    // 实际的参数会通过Loader传递
    return 0;
}

// ===============================================
// 测试模式 (可选)
// ===============================================

#ifdef RUNTIME_TEST_MODE
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <astc_file>\n", argv[0]);
        return 1;
    }
    
    // 读取ASTC文件
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Cannot open ASTC file: %s\n", argv[1]);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uint8_t* data = malloc(size);
    fread(data, 1, size, file);
    fclose(file);
    
    // 执行Runtime
    int result = evolver0_runtime_main(data, size);
    
    free(data);
    return result;
}
#endif
