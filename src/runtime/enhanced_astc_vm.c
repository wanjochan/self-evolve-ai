/**
 * enhanced_astc_vm.c - 增强的ASTC虚拟机实现
 * 
 * 集成libc转发系统的完整ASTC虚拟机
 * 目标：支持完整的C语言程序执行，为脱离TinyCC做准备
 */

#include "enhanced_astc_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===============================================
// 虚拟机初始化和清理
// ===============================================

int enhanced_astc_vm_init(EnhancedASTCVM* vm, uint8_t* code, size_t code_size, 
                         uint8_t* data, size_t data_size) {
    if (!vm || !code) {
        return -1;
    }
    
    // 清零虚拟机状态
    memset(vm, 0, sizeof(EnhancedASTCVM));
    
    // 设置代码和数据
    vm->code = code;
    vm->code_size = code_size;
    vm->data = data;
    vm->data_size = data_size;
    
    // 初始化执行状态
    vm->pc = 0;
    vm->running = true;
    vm->exit_code = 0;
    vm->stack_top = -1;
    vm->call_stack_top = -1;
    vm->instruction_count = 0;
    vm->debug_mode = false;
    
    // 初始化libc转发系统
    libc_forward_init();
    
    return 0;
}

void enhanced_astc_vm_cleanup(EnhancedASTCVM* vm) {
    if (!vm) return;
    
    libc_forward_cleanup();
    memset(vm, 0, sizeof(EnhancedASTCVM));
}

void enhanced_astc_vm_reset(EnhancedASTCVM* vm) {
    if (!vm) return;
    
    vm->pc = 0;
    vm->running = true;
    vm->exit_code = 0;
    vm->stack_top = -1;
    vm->call_stack_top = -1;
    vm->instruction_count = 0;
    
    // 清空栈和变量
    memset(vm->stack, 0, sizeof(vm->stack));
    memset(vm->locals, 0, sizeof(vm->locals));
    memset(vm->call_stack, 0, sizeof(vm->call_stack));
}

// ===============================================
// 栈操作
// ===============================================

int astc_vm_push(EnhancedASTCVM* vm, ASTCValue value) {
    if (vm->stack_top >= ASTC_VM_STACK_SIZE - 1) {
        return -1; // 栈溢出
    }
    
    vm->stack[++vm->stack_top] = value;
    return 0;
}

ASTCValue astc_vm_pop(EnhancedASTCVM* vm) {
    ASTCValue empty = {0, 0};
    
    if (vm->stack_top < 0) {
        return empty; // 栈下溢
    }
    
    return vm->stack[vm->stack_top--];
}

ASTCValue astc_vm_peek(EnhancedASTCVM* vm) {
    ASTCValue empty = {0, 0};
    
    if (vm->stack_top < 0) {
        return empty;
    }
    
    return vm->stack[vm->stack_top];
}

// ===============================================
// 指令执行
// ===============================================

int enhanced_astc_vm_step(EnhancedASTCVM* vm) {
    if (!vm || !vm->running || vm->pc >= vm->code_size) {
        return 1; // 停机
    }
    
    uint8_t opcode = vm->code[vm->pc++];
    vm->instruction_count++;
    
    if (vm->debug_mode) {
        printf("PC=%u, OP=0x%02X, Stack=%d\n", vm->pc-1, opcode, vm->stack_top+1);
    }
    
    switch (opcode) {
        case ASTC_NOP:
            // 空操作
            break;
            
        case ASTC_HALT:
            vm->running = false;
            return 1;
            
        case ASTC_CONST_I32: {
            if (vm->pc + 4 > vm->code_size) return -1;
            uint32_t value = *(uint32_t*)(vm->code + vm->pc);
            vm->pc += 4;
            
            ASTCValue val = {value, 0}; // type 0 = int32
            astc_vm_push(vm, val);
            break;
        }
        
        case ASTC_CONST_I64: {
            if (vm->pc + 8 > vm->code_size) return -1;
            uint64_t value = *(uint64_t*)(vm->code + vm->pc);
            vm->pc += 8;
            
            ASTCValue val = {value, 1}; // type 1 = int64
            astc_vm_push(vm, val);
            break;
        }
        
        case ASTC_ADD_I32: {
            ASTCValue b = astc_vm_pop(vm);
            ASTCValue a = astc_vm_pop(vm);
            ASTCValue result = {a.value + b.value, 0};
            astc_vm_push(vm, result);
            break;
        }
        
        case ASTC_SUB_I32: {
            ASTCValue b = astc_vm_pop(vm);
            ASTCValue a = astc_vm_pop(vm);
            ASTCValue result = {a.value - b.value, 0};
            astc_vm_push(vm, result);
            break;
        }
        
        case ASTC_MUL_I32: {
            ASTCValue b = astc_vm_pop(vm);
            ASTCValue a = astc_vm_pop(vm);
            ASTCValue result = {a.value * b.value, 0};
            astc_vm_push(vm, result);
            break;
        }
        
        case ASTC_LIBC_CALL: {
            // libc函数调用
            if (vm->pc + 4 > vm->code_size) return -1;
            uint16_t func_id = *(uint16_t*)(vm->code + vm->pc);
            uint16_t arg_count = *(uint16_t*)(vm->code + vm->pc + 2);
            vm->pc += 4;
            
            // 准备libc调用
            LibcCall call;
            call.func_id = func_id;
            call.arg_count = arg_count;
            
            // 从栈中获取参数（逆序）
            for (int i = arg_count - 1; i >= 0; i--) {
                ASTCValue arg = astc_vm_pop(vm);
                call.args[i] = arg.value;
            }
            
            // 执行libc调用
            int result = libc_forward_call(&call);
            if (result == 0) {
                // 将返回值压入栈
                ASTCValue ret_val = {call.return_value, 0};
                astc_vm_push(vm, ret_val);
            } else {
                // 错误处理
                ASTCValue error_val = {(uint64_t)call.error_code, 0};
                astc_vm_push(vm, error_val);
            }
            break;
        }
        
        case ASTC_JUMP: {
            if (vm->pc + 4 > vm->code_size) return -1;
            uint32_t target = *(uint32_t*)(vm->code + vm->pc);
            vm->pc = target;
            break;
        }
        
        case ASTC_JUMP_IF: {
            if (vm->pc + 4 > vm->code_size) return -1;
            uint32_t target = *(uint32_t*)(vm->code + vm->pc);
            vm->pc += 4;
            
            ASTCValue condition = astc_vm_pop(vm);
            if (condition.value != 0) {
                vm->pc = target;
            }
            break;
        }
        
        case ASTC_RETURN: {
            if (vm->call_stack_top < 0) {
                // 主函数返回，程序结束
                vm->running = false;
                ASTCValue ret_val = astc_vm_pop(vm);
                vm->exit_code = (int32_t)ret_val.value;
                return 1;
            } else {
                // 函数返回
                CallFrame frame = vm->call_stack[vm->call_stack_top--];
                vm->pc = frame.pc;
            }
            break;
        }
        
        case ASTC_DEBUG_PRINT: {
            ASTCValue val = astc_vm_pop(vm);
            printf("[DEBUG] Value: %llu (type: %u)\n", val.value, val.type);
            break;
        }
        
        default:
            printf("Unknown opcode: 0x%02X at PC=%u\n", opcode, vm->pc-1);
            return -1; // 未知指令
    }
    
    return 0; // 继续执行
}

int enhanced_astc_vm_run(EnhancedASTCVM* vm) {
    if (!vm) return -1;
    
    while (vm->running) {
        int result = enhanced_astc_vm_step(vm);
        if (result != 0) {
            break;
        }
    }
    
    return vm->exit_code;
}

// ===============================================
// 调试和状态
// ===============================================

void enhanced_astc_vm_set_debug(EnhancedASTCVM* vm, bool debug) {
    if (vm) {
        vm->debug_mode = debug;
    }
}

void enhanced_astc_vm_print_status(EnhancedASTCVM* vm) {
    if (!vm) return;
    
    printf("=== ASTC虚拟机状态 ===\n");
    printf("PC: %u / %zu\n", vm->pc, vm->code_size);
    printf("运行状态: %s\n", vm->running ? "运行中" : "已停止");
    printf("退出码: %d\n", vm->exit_code);
    printf("栈深度: %d / %d\n", vm->stack_top + 1, ASTC_VM_STACK_SIZE);
    printf("调用栈深度: %d / %d\n", vm->call_stack_top + 1, ASTC_VM_CALL_STACK_SIZE);
    printf("执行指令数: %llu\n", vm->instruction_count);
    
    // 显示libc统计
    libc_get_stats(&vm->libc_stats);
    printf("libc调用统计:\n");
    printf("  总调用: %llu\n", vm->libc_stats.total_calls);
    printf("  内存分配: %llu\n", vm->libc_stats.malloc_calls);
    printf("  文件操作: %llu\n", vm->libc_stats.file_operations);
}
