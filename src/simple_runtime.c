/**
 * simple_runtime.c - 简化版C99运行时
 * 
 * 只包含核心的printf支持，用于验证基本功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// 简化的虚拟机状态
typedef struct {
    uint8_t* code;
    size_t code_size;
    size_t pc;
    uint32_t stack[256];
    int stack_top;
    int running;
    uint32_t variables[64];  // 简单的变量存储
} SimpleVM;

// 虚拟机操作
void simple_vm_push(SimpleVM* vm, uint32_t value) {
    if (vm->stack_top < 255) {
        vm->stack[++vm->stack_top] = value;
    }
}

uint32_t simple_vm_pop(SimpleVM* vm) {
    if (vm->stack_top >= 0) {
        return vm->stack[vm->stack_top--];
    }
    return 0;
}

// 执行printf调用
void execute_simple_printf(SimpleVM* vm, uint32_t arg_count) {
    if (arg_count == 1) {
        // 获取字符串参数
        uint32_t str_ptr = simple_vm_pop(vm);
        if (str_ptr) {
            printf("%s", (char*)str_ptr);
        }
    }
    // 返回值（printf返回打印的字符数，简化为0）
    simple_vm_push(vm, 0);
}

// 简化的虚拟机执行
int simple_vm_execute(SimpleVM* vm) {
    vm->pc = 0;
    vm->stack_top = -1;
    vm->running = 1;

    // 初始化变量数组
    memset(vm->variables, 0, sizeof(vm->variables));

    printf("Simple Runtime: Starting execution\n");
    
    while (vm->running && vm->pc < vm->code_size) {
        uint8_t opcode = vm->code[vm->pc++];
        
        switch (opcode) {
            case 0x00: // NOP
                // No operation - just continue
                break;

            case 0x01: // HALT
                vm->running = 0;
                break;
                
            case 0x10: // CONST_I32
                if (vm->pc + 4 <= vm->code_size) {
                    uint32_t value = *(uint32_t*)(vm->code + vm->pc);
                    vm->pc += 4;
                    simple_vm_push(vm, value);
                }
                break;
                
            case 0x12: // CONST_STRING
                if (vm->pc + 4 <= vm->code_size) {
                    uint32_t string_len = *(uint32_t*)(vm->code + vm->pc);
                    vm->pc += 4;
                    
                    if (vm->pc + string_len <= vm->code_size) {
                        char* string_ptr = (char*)(vm->code + vm->pc);
                        vm->pc += string_len;
                        simple_vm_push(vm, (uint32_t)(uintptr_t)string_ptr);
                    }
                }
                break;
                
            case 0xF0: // LIBC_CALL
                {
                    uint32_t func_id = simple_vm_pop(vm);
                    uint32_t arg_count = simple_vm_pop(vm);
                    
                    if (func_id == 0x0030) { // printf
                        execute_simple_printf(vm, arg_count);
                    } else {
                        printf("Simple Runtime: Unknown libc function 0x%04X\n", func_id);
                        simple_vm_push(vm, 0);
                    }
                }
                break;

            // Variable operations
            case 0x20: // STORE_VAR
                if (vm->pc + 4 <= vm->code_size) {
                    uint32_t var_index = *(uint32_t*)(vm->code + vm->pc);
                    vm->pc += 4;
                    uint32_t value = simple_vm_pop(vm);
                    if (var_index < 64) {
                        vm->variables[var_index] = value;
                    }
                }
                break;

            case 0x21: // LOAD_VAR
                if (vm->pc + 4 <= vm->code_size) {
                    uint32_t var_index = *(uint32_t*)(vm->code + vm->pc);
                    vm->pc += 4;
                    if (var_index < 64) {
                        simple_vm_push(vm, vm->variables[var_index]);
                    } else {
                        simple_vm_push(vm, 0);
                    }
                }
                break;

            // Arithmetic operations
            case 0x60: // ADD
                {
                    uint32_t b = simple_vm_pop(vm);
                    uint32_t a = simple_vm_pop(vm);
                    simple_vm_push(vm, a + b);
                }
                break;

            case 0x61: // SUB
                {
                    uint32_t b = simple_vm_pop(vm);
                    uint32_t a = simple_vm_pop(vm);
                    simple_vm_push(vm, a - b);
                }
                break;

            case 0x62: // MUL
                {
                    uint32_t b = simple_vm_pop(vm);
                    uint32_t a = simple_vm_pop(vm);
                    simple_vm_push(vm, a * b);
                }
                break;

            case 0x63: // DIV
                {
                    uint32_t b = simple_vm_pop(vm);
                    uint32_t a = simple_vm_pop(vm);
                    if (b != 0) {
                        simple_vm_push(vm, a / b);
                    } else {
                        simple_vm_push(vm, 0); // Division by zero protection
                    }
                }
                break;



            // Control flow operations
            case 0x70: // BREAK
                printf("Simple Runtime: BREAK statement\n");
                // TODO: Implement proper break handling
                break;

            case 0x71: // CONTINUE
                printf("Simple Runtime: CONTINUE statement\n");
                // TODO: Implement proper continue handling
                break;

            // Memory access operations
            case 0x72: // ARRAY_ACCESS
                {
                    uint32_t index = simple_vm_pop(vm);
                    uint32_t array_ptr = simple_vm_pop(vm);
                    printf("Simple Runtime: ARRAY_ACCESS [%d][%d]\n", array_ptr, index);
                    // TODO: Implement proper array access
                    simple_vm_push(vm, 42); // Placeholder
                }
                break;

            case 0x73: // PTR_MEMBER_ACCESS
                {
                    uint32_t ptr = simple_vm_pop(vm);
                    printf("Simple Runtime: PTR_MEMBER_ACCESS ->member on %d\n", ptr);
                    // TODO: Implement proper pointer member access
                    simple_vm_push(vm, 42); // Placeholder
                }
                break;

            case 0x74: // MEMBER_ACCESS
                {
                    uint32_t obj = simple_vm_pop(vm);
                    printf("Simple Runtime: MEMBER_ACCESS .member on %d\n", obj);
                    // TODO: Implement proper member access
                    simple_vm_push(vm, 42); // Placeholder
                }
                break;

            // Additional opcodes commonly generated by c2astc
            case 0x06: // EXPRESSION_STMT (commonly generated)
                // Expression statement - usually just pops the result
                if (vm->stack_top > 0) {
                    simple_vm_pop(vm); // Discard expression result
                }
                break;

            case 0x09: // VAR_DECL (variable declaration)
                // Variable declaration - usually just allocates space
                break;

            case 0x34: // BINARY_OP (binary operation)
                // Binary operation - simplified handling
                if (vm->stack_top >= 2) {
                    uint32_t b = simple_vm_pop(vm);
                    uint32_t a = simple_vm_pop(vm);
                    simple_vm_push(vm, a + b); // Default to addition
                }
                break;

            case 0x30: // STORE_LOCAL
                if (vm->pc + 4 <= vm->code_size) {
                    uint32_t local_index = *(uint32_t*)(vm->code + vm->pc);
                    vm->pc += 4;
                    uint32_t value = simple_vm_pop(vm);
                    // Store to local variable (simplified)
                    if (local_index < 64) {
                        vm->variables[local_index] = value;
                    }
                }
                break;

            case 0x31: // LOAD_LOCAL
                if (vm->pc + 4 <= vm->code_size) {
                    uint32_t local_index = *(uint32_t*)(vm->code + vm->pc);
                    vm->pc += 4;
                    // Load from local variable (simplified)
                    if (local_index < 64) {
                        simple_vm_push(vm, vm->variables[local_index]);
                    } else {
                        simple_vm_push(vm, 0);
                    }
                }
                break;

            case 0x40: // JUMP
                if (vm->pc + 4 <= vm->code_size) {
                    uint32_t target = *(uint32_t*)(vm->code + vm->pc);
                    vm->pc = target;
                } else {
                    vm->pc += 4;
                }
                break;

            case 0x41: // JUMP_IF_FALSE
                if (vm->pc + 4 <= vm->code_size) {
                    uint32_t target = *(uint32_t*)(vm->code + vm->pc);
                    vm->pc += 4;
                    uint32_t condition = simple_vm_pop(vm);
                    if (!condition) {
                        vm->pc = target;
                    }
                }
                break;

            case 0x50: // CALL_USER
                if (vm->pc + 4 <= vm->code_size) {
                    uint32_t func_addr = *(uint32_t*)(vm->code + vm->pc);
                    vm->pc += 4;
                    // Simplified user function call - just return 0
                    simple_vm_push(vm, 0);
                }
                break;

            default:
                printf("Simple Runtime: Unknown opcode 0x%02X\n", opcode);
                vm->running = 0;
                break;
        }
    }
    
    // 获取返回值
    int result = 0;
    if (vm->stack_top >= 0) {
        result = (int)simple_vm_pop(vm);
    }
    
    printf("Simple Runtime: Execution completed, result = %d\n", result);
    return result;
}

// 解析ASTC头部
int parse_astc_header(uint8_t* data, size_t size, uint8_t** code_start, size_t* code_size) {
    if (size < 16) {
        printf("Error: ASTC file too small\n");
        return -1;
    }
    
    // 检查魔数
    if (memcmp(data, "ASTC", 4) != 0) {
        printf("Error: Invalid ASTC magic number\n");
        return -1;
    }
    
    // 读取头部信息
    uint32_t version = *(uint32_t*)(data + 4);
    uint32_t data_size = *(uint32_t*)(data + 8);
    uint32_t entry_point = *(uint32_t*)(data + 12);
    
    printf("ASTC version: %d, data size: %d, entry point: %d\n", 
           version, data_size, entry_point);
    
    *code_start = data + 16;
    *code_size = data_size;
    
    return 0;
}

// 主入口点
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <program.astc>\n", argv[0]);
        return 1;
    }
    
    // 读取ASTC文件
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        printf("Error: Cannot open file: %s\n", argv[1]);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uint8_t* file_data = malloc(file_size);
    if (!file_data) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    
    fread(file_data, 1, file_size, file);
    fclose(file);
    
    printf("Simple Runtime called with %zu bytes\n", file_size);
    
    // 解析ASTC头部
    uint8_t* code_start;
    size_t code_size;
    if (parse_astc_header(file_data, file_size, &code_start, &code_size) != 0) {
        free(file_data);
        return 1;
    }
    
    // 创建虚拟机并执行
    SimpleVM vm;
    vm.code = code_start;
    vm.code_size = code_size;
    
    int result = simple_vm_execute(&vm);
    
    free(file_data);
    return result;
}
