/**
 * pipeline_vm.c - Pipeline Virtual Machine Module
 * 
 * 虚拟机模块，负责ASTC字节码执行：
 * - ASTC字节码解释执行
 * - 栈管理和寄存器管理
 * - 运行时错误处理
 */

#include "pipeline_common.h"

// ===============================================
// 虚拟机上下文管理
// ===============================================

VMContext* vm_create_context(void) {
    VMContext* ctx = malloc(sizeof(VMContext));
    if (!ctx) return NULL;

    ctx->state = VM_STATE_READY;
    ctx->astc_program = NULL;
    ctx->bytecode = NULL;
    ctx->bytecode_size = 0;
    ctx->program_counter = 0;
    
    // 初始化栈
    ctx->stack_size = 1024;
    ctx->stack = malloc(ctx->stack_size * sizeof(uint64_t));
    ctx->stack_pointer = 0;
    
    // 初始化寄存器
    memset(ctx->registers, 0, sizeof(ctx->registers));
    
    ctx->error_message[0] = '\0';

    if (!ctx->stack) {
        free(ctx);
        return NULL;
    }

    return ctx;
}

void vm_free_context(VMContext* ctx) {
    if (!ctx) return;
    
    if (ctx->stack) free(ctx->stack);
    if (ctx->bytecode) free(ctx->bytecode);
    
    free(ctx);
}

// ===============================================
// 栈操作
// ===============================================

static bool vm_push(VMContext* ctx, uint64_t value) {
    if (!ctx || ctx->stack_pointer >= ctx->stack_size) {
        if (ctx) {
            set_pipeline_error(ctx->error_message, sizeof(ctx->error_message), 
                             "Stack overflow");
        }
        return false;
    }
    
    ctx->stack[ctx->stack_pointer++] = value;
    return true;
}

static bool vm_pop(VMContext* ctx, uint64_t* value) {
    if (!ctx || !value || ctx->stack_pointer == 0) {
        if (ctx) {
            set_pipeline_error(ctx->error_message, sizeof(ctx->error_message), 
                             "Stack underflow");
        }
        return false;
    }
    
    *value = ctx->stack[--ctx->stack_pointer];
    return true;
}

static uint64_t vm_peek(VMContext* ctx) {
    if (!ctx || ctx->stack_pointer == 0) return 0;
    return ctx->stack[ctx->stack_pointer - 1];
}

// ===============================================
// 指令执行
// ===============================================

static bool execute_i32_const(VMContext* ctx, int32_t value) {
    return vm_push(ctx, (uint64_t)value);
}

static bool execute_i64_const(VMContext* ctx, int64_t value) {
    return vm_push(ctx, (uint64_t)value);
}

static bool execute_i32_add(VMContext* ctx) {
    uint64_t b, a;
    if (!vm_pop(ctx, &b) || !vm_pop(ctx, &a)) return false;
    
    int32_t result = (int32_t)a + (int32_t)b;
    return vm_push(ctx, (uint64_t)result);
}

static bool execute_i32_sub(VMContext* ctx) {
    uint64_t b, a;
    if (!vm_pop(ctx, &b) || !vm_pop(ctx, &a)) return false;
    
    int32_t result = (int32_t)a - (int32_t)b;
    return vm_push(ctx, (uint64_t)result);
}

static bool execute_i32_mul(VMContext* ctx) {
    uint64_t b, a;
    if (!vm_pop(ctx, &b) || !vm_pop(ctx, &a)) return false;
    
    int32_t result = (int32_t)a * (int32_t)b;
    return vm_push(ctx, (uint64_t)result);
}

static bool execute_i32_div_s(VMContext* ctx) {
    uint64_t b, a;
    if (!vm_pop(ctx, &b) || !vm_pop(ctx, &a)) return false;
    
    int32_t divisor = (int32_t)b;
    if (divisor == 0) {
        set_pipeline_error(ctx->error_message, sizeof(ctx->error_message), 
                         "Division by zero");
        return false;
    }
    
    int32_t result = (int32_t)a / divisor;
    return vm_push(ctx, (uint64_t)result);
}

static bool execute_local_get(VMContext* ctx, uint32_t local_index) {
    if (local_index >= 16) {
        set_pipeline_error(ctx->error_message, sizeof(ctx->error_message), 
                         "Invalid local index: %u", local_index);
        return false;
    }
    
    return vm_push(ctx, ctx->registers[local_index]);
}

static bool execute_local_set(VMContext* ctx, uint32_t local_index) {
    if (local_index >= 16) {
        set_pipeline_error(ctx->error_message, sizeof(ctx->error_message), 
                         "Invalid local index: %u", local_index);
        return false;
    }
    
    uint64_t value;
    if (!vm_pop(ctx, &value)) return false;
    
    ctx->registers[local_index] = value;
    return true;
}

static bool execute_return(VMContext* ctx) {
    ctx->state = VM_STATE_STOPPED;
    return true;
}

static bool execute_drop(VMContext* ctx) {
    uint64_t value;
    return vm_pop(ctx, &value);
}

// ===============================================
// 指令分发
// ===============================================

static bool execute_instruction(VMContext* ctx, const ASTCInstruction* instr) {
    if (!ctx || !instr) return false;

    switch (instr->opcode) {
        case AST_I32_CONST:
            return execute_i32_const(ctx, instr->operand.i32);
        
        case AST_I64_CONST:
            return execute_i64_const(ctx, instr->operand.i64);
        
        case AST_I32_ADD:
            return execute_i32_add(ctx);
        
        case AST_I32_SUB:
            return execute_i32_sub(ctx);
        
        case AST_I32_MUL:
            return execute_i32_mul(ctx);
        
        case AST_I32_DIV_S:
            return execute_i32_div_s(ctx);
        
        case AST_LOCAL_GET:
            return execute_local_get(ctx, instr->operand.index);
        
        case AST_LOCAL_SET:
            return execute_local_set(ctx, instr->operand.index);
        
        case AST_RETURN:
            return execute_return(ctx);
        
        case AST_DROP:
            return execute_drop(ctx);
        
        case AST_NOP:
            return true; // 空操作
        
        default:
            set_pipeline_error(ctx->error_message, sizeof(ctx->error_message), 
                             "Unsupported instruction: %d", instr->opcode);
            return false;
    }
}

// ===============================================
// 程序加载和执行
// ===============================================

bool vm_load_program(VMContext* ctx, ASTCBytecodeProgram* program) {
    if (!ctx || !program) return false;

    // 验证程序格式
    if (memcmp(program->magic, "ASTC", 4) != 0) {
        set_pipeline_error(ctx->error_message, sizeof(ctx->error_message), 
                         "Invalid ASTC magic number");
        return false;
    }

    if (program->version != 1) {
        set_pipeline_error(ctx->error_message, sizeof(ctx->error_message), 
                         "Unsupported ASTC version: %u", program->version);
        return false;
    }

    ctx->astc_program = program;
    ctx->program_counter = 0;
    ctx->state = VM_STATE_READY;

    return true;
}

bool vm_execute(VMContext* ctx) {
    if (!ctx || !ctx->astc_program) {
        if (ctx) {
            set_pipeline_error(ctx->error_message, sizeof(ctx->error_message), 
                             "No program loaded");
        }
        return false;
    }

    ctx->state = VM_STATE_RUNNING;

    while (ctx->state == VM_STATE_RUNNING && 
           ctx->program_counter < ctx->astc_program->instruction_count) {
        
        ASTCInstruction* instr = &ctx->astc_program->instructions[ctx->program_counter];
        
        if (!execute_instruction(ctx, instr)) {
            ctx->state = VM_STATE_ERROR;
            return false;
        }
        
        ctx->program_counter++;
    }

    // 如果正常结束，确保状态为停止
    if (ctx->state == VM_STATE_RUNNING) {
        ctx->state = VM_STATE_STOPPED;
    }

    return ctx->state == VM_STATE_STOPPED;
}

// ===============================================
// 调试和诊断
// ===============================================

static void vm_print_stack(VMContext* ctx) {
    if (!ctx) return;
    
    printf("Stack (SP=%zu):\n", ctx->stack_pointer);
    for (size_t i = 0; i < ctx->stack_pointer; i++) {
        printf("  [%zu]: %llu (0x%llx)\n", i, 
               (unsigned long long)ctx->stack[i],
               (unsigned long long)ctx->stack[i]);
    }
}

static void vm_print_registers(VMContext* ctx) {
    if (!ctx) return;
    
    printf("Registers:\n");
    for (int i = 0; i < 16; i++) {
        if (ctx->registers[i] != 0) {
            printf("  R%d: %llu (0x%llx)\n", i, 
                   (unsigned long long)ctx->registers[i],
                   (unsigned long long)ctx->registers[i]);
        }
    }
}

void vm_print_state(VMContext* ctx) {
    if (!ctx) return;
    
    printf("VM State: ");
    switch (ctx->state) {
        case VM_STATE_READY: printf("READY"); break;
        case VM_STATE_RUNNING: printf("RUNNING"); break;
        case VM_STATE_STOPPED: printf("STOPPED"); break;
        case VM_STATE_ERROR: printf("ERROR"); break;
    }
    printf("\n");
    
    printf("PC: %zu\n", ctx->program_counter);
    
    if (ctx->error_message[0] != '\0') {
        printf("Error: %s\n", ctx->error_message);
    }
    
    vm_print_stack(ctx);
    vm_print_registers(ctx);
}

// ===============================================
// 简单的字节码执行接口
// ===============================================

bool vm_execute_bytecode(const uint8_t* bytecode, size_t size) {
    if (!bytecode || size < sizeof(ASTCBytecodeProgram)) return false;

    // 反序列化字节码
    ASTCBytecodeProgram* program = (ASTCBytecodeProgram*)bytecode;
    
    // 验证魔数
    if (memcmp(program->magic, "ASTC", 4) != 0) return false;

    VMContext* ctx = vm_create_context();
    if (!ctx) return false;

    bool result = false;
    if (vm_load_program(ctx, program)) {
        result = vm_execute(ctx);
    }

    vm_free_context(ctx);
    return result;
}

// ===============================================
// 内存管理辅助函数
// ===============================================

static void vm_reset(VMContext* ctx) {
    if (!ctx) return;
    
    ctx->program_counter = 0;
    ctx->stack_pointer = 0;
    memset(ctx->registers, 0, sizeof(ctx->registers));
    ctx->error_message[0] = '\0';
    ctx->state = VM_STATE_READY;
}

bool vm_restart(VMContext* ctx) {
    if (!ctx || !ctx->astc_program) return false;
    
    vm_reset(ctx);
    return true;
}

uint64_t vm_get_result(VMContext* ctx) {
    if (!ctx || ctx->stack_pointer == 0) return 0;
    return vm_peek(ctx);
}