/**
 * enhanced_runtime_with_libc.c - 增强的Runtime，完整libc支持
 * 
 * 这个Runtime使用core_libc.c提供的完整libc转发功能
 * 支持malloc, free, printf等所有标准库函数
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// 引入libc转发系统
#include "runtime/core_libc.h"

// ===============================================
// ASTC虚拟机定义
// ===============================================

typedef struct {
    uint8_t* code;
    size_t code_size;
    size_t pc;              // 程序计数器
    uint32_t stack[1024];   // 栈
    int stack_top;          // 栈顶指针
    uint32_t locals[512];   // 局部变量
    bool running;           // 运行状态
} ASTCVirtualMachine;

// ===============================================
// 虚拟机栈操作
// ===============================================

void astc_vm_push(ASTCVirtualMachine* vm, uint32_t value) {
    if (vm->stack_top < 1023) {
        vm->stack[++vm->stack_top] = value;
    }
}

uint32_t astc_vm_pop(ASTCVirtualMachine* vm) {
    if (vm->stack_top >= 0) {
        return vm->stack[vm->stack_top--];
    }
    return 0;
}

void astc_vm_init(ASTCVirtualMachine* vm, uint8_t* code, size_t code_size) {
    vm->code = code;
    vm->code_size = code_size;
    vm->pc = 0;
    vm->stack_top = -1;
    vm->running = true;

    // 初始化局部变量
    for (int i = 0; i < 512; i++) {
        vm->locals[i] = 0;
    }
}

// ===============================================
// ASTC指令执行
// ===============================================

int astc_execute_instruction(ASTCVirtualMachine* vm) {
    if (vm->pc >= vm->code_size || !vm->running) {
        vm->running = false;
        return 0;
    }
    
    uint8_t opcode = vm->code[vm->pc++];
    
    switch (opcode) {
        case 0x01: // HALT
            vm->running = false;
            break;
            
        case 0x10: // CONST_I32
            {
                uint32_t value = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                astc_vm_push(vm, value);
            }
            break;
            
        case 0x12: // CONST_STRING
            {
                uint32_t length = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                
                // 将字符串地址压入栈
                astc_vm_push(vm, (uint32_t)(vm->code + vm->pc));
                vm->pc += length;
            }
            break;
            
        case 0x20: // ADD
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, a + b);
            }
            break;
            
        case 0x21: // SUB
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, a - b);
            }
            break;
            
        case 0x22: // MUL
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, a * b);
            }
            break;

        case 0x60: // LOAD_LOCAL - 加载局部变量
            {
                uint32_t index = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                if (index < 512) {
                    astc_vm_push(vm, vm->locals[index]);
                }
            }
            break;

        case 0x61: // STORE_LOCAL - 存储局部变量
            {
                uint32_t index = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                if (index < 512) {
                    vm->locals[index] = astc_vm_pop(vm);
                }
            }
            break;
            
        case 0xF0: // LIBC_CALL - 使用完整的libc转发系统
            {
                uint16_t func_id = astc_vm_pop(vm);
                uint16_t arg_count = astc_vm_pop(vm);
                
                printf("DEBUG: LIBC_CALL func_id=0x%04X, arg_count=%d\n", func_id, arg_count);
                
                LibcCall call;
                call.func_id = func_id;
                call.arg_count = arg_count;
                
                // 从栈中获取参数
                for (int i = arg_count - 1; i >= 0; i--) {
                    call.args[i] = astc_vm_pop(vm);
                }
                
                // 执行libc转发 - 这里使用core_libc.c的完整实现
                int result = libc_forward_call(&call);
                if (result == 0) {
                    astc_vm_push(vm, (uint32_t)call.return_value);
                    printf("DEBUG: LIBC_CALL successful, return_value=%u\n", (uint32_t)call.return_value);
                } else {
                    astc_vm_push(vm, 0); // 错误时返回0
                    printf("DEBUG: LIBC_CALL failed\n");
                }
            }
            break;
            
        case 0xF1: // USER_CALL
            {
                uint32_t func_hash = astc_vm_pop(vm);
                uint32_t arg_count = astc_vm_pop(vm);
                
                printf("DEBUG: USER_CALL func_hash=0x%08X, arg_count=%d\n", func_hash, arg_count);
                
                // 简化实现：暂时返回固定值
                astc_vm_push(vm, 0);
            }
            break;
            
        default:
            printf("DEBUG: Unknown opcode 0x%02X at pc=%zu\n", opcode, vm->pc - 1);
            vm->running = false;
            break;
    }
    
    return 0;
}

// ===============================================
// Runtime主入口点
// ===============================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <program.astc>\n", argv[0]);
        return 1;
    }
    
    // 初始化libc转发系统
    printf("Initializing libc forwarding system...\n");
    if (libc_forward_init() != 0) {
        fprintf(stderr, "Error: Failed to initialize libc forwarding\n");
        return 1;
    }
    
    // 加载ASTC程序
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
        libc_forward_cleanup();
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uint8_t* program_data = malloc(file_size);
    if (!program_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        libc_forward_cleanup();
        return 1;
    }
    
    fread(program_data, 1, file_size, file);
    fclose(file);
    
    printf("Loaded ASTC program: %zu bytes\n", file_size);
    
    // 检查ASTC格式
    if (file_size >= 16 && memcmp(program_data, "ASTC", 4) == 0) {
        // 解析ASTC头部
        uint32_t* header = (uint32_t*)program_data;
        uint32_t version = header[1];
        uint32_t data_size = header[2];
        uint32_t entry_point = header[3];
        
        printf("ASTC Header: version=%u, data_size=%u, entry_point=%u\n", 
               version, data_size, entry_point);
        
        // 获取ASTC代码段
        uint8_t* astc_code = program_data + 16;
        size_t astc_code_size = file_size - 16;
        
        // 初始化ASTC虚拟机
        ASTCVirtualMachine vm;
        astc_vm_init(&vm, astc_code, astc_code_size);
        
        // 执行ASTC程序
        printf("Executing ASTC program...\n");
        int result = 0;
        int instruction_count = 0;
        while (vm.running && instruction_count < 100000) {
            result = astc_execute_instruction(&vm);
            instruction_count++;
        }
        
        printf("Execution completed: %d instructions executed\n", instruction_count);
        
        // 获取返回值
        int return_value = (vm.stack_top >= 0) ? vm.stack[vm.stack_top] : 0;
        printf("Program return value: %d\n", return_value);
        
        // 清理
        free(program_data);
        libc_forward_cleanup();
        
        return return_value;
    } else {
        printf("Error: Invalid ASTC format\n");
        free(program_data);
        libc_forward_cleanup();
        return 1;
    }
}
