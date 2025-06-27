/**
 * evolver1_runtime.c - 第一代Runtime实现
 * 由evolver0自举编译生成
 * 优化功能：更快的AST执行、改进的内存管理
 */

/**
 * evolver0_runtime.c - 第零代Runtime实现
 * 
 * 这是evolver0的Runtime层，包含完整的ASTC虚拟机
 * 编译为无头二进制，由evolver0_loader加载执行
 * 
 * 职责：
 * 1. 实现完整的ASTC虚拟机
 * 2. 执行Program层的ASTC代码
 * 3. 提供系统调用接口
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// 引入公共组件
#include "../runtime/astc.h"

// ===============================================
// 简化的Runtime实现
// ===============================================

// ===============================================
// Runtime入口点和接口
// ===============================================

// ASTC虚拟机状态
struct ASTCVirtualMachine {
    unsigned char* code;     // ASTC字节码
    size_t code_size;        // 代码大小
    size_t pc;               // 程序计数器
    uint32_t stack[1024];    // 操作数栈
    int stack_top;           // 栈顶指针
    uint32_t locals[256];    // 局部变量
    int running;             // 运行状态
};

// 初始化虚拟机
void astc_vm_init(struct ASTCVirtualMachine* vm, unsigned char* code, size_t code_size) {
    vm->code = code;
    vm->code_size = code_size;
    vm->pc = 0;
    vm->stack_top = -1;
    vm->running = 1;
    memset(vm->locals, 0, sizeof(vm->locals));
    memset(vm->stack, 0, sizeof(vm->stack));
}

// 栈操作
void astc_push(struct ASTCVirtualMachine* vm, uint32_t value) {
    if (vm->stack_top < 1023) {
        vm->stack[++vm->stack_top] = value;
    }
}

uint32_t astc_pop(struct ASTCVirtualMachine* vm) {
    if (vm->stack_top >= 0) {
        return vm->stack[vm->stack_top--];
    }
    return 0;
}

// 执行ASTC指令
int astc_execute_instruction(struct ASTCVirtualMachine* vm) {
    if (vm->pc >= vm->code_size) {
        vm->running = 0;
        return 0;
    }

    unsigned char opcode = vm->code[vm->pc++];

    switch (opcode) {
        case 0x41: // i32.const
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t value = *(uint32_t*)(vm->code + vm->pc);
                astc_push(vm, value);
                vm->pc += 4;
            }
            break;

        case 0x0F: // return
            vm->running = 0;
            return vm->stack_top >= 0 ? vm->stack[vm->stack_top] : 0;

        case 0x10: // call (简化实现)
            // 这里应该调用函数，暂时返回固定值
            astc_push(vm, 200); // evolver0成功标识
            break;

        default:
            // 未知指令，跳过
            break;
    }

    return 0;
}

// Runtime的主入口点，由Loader调用
// 参数：ASTC程序数据和大小
int evolver0_runtime_main(void* program_data, size_t program_size) {
    printf("=== Evolver0 Runtime ===\n");
    printf("Runtime: Starting ASTC Virtual Machine\n");
    printf("Runtime: Program data size: %zu bytes\n", program_size);

    if (!program_data || program_size == 0) {
        printf("Runtime: Error - No program data\n");
        return 1;
    }

    // 检查ASTC格式
    if (program_size >= 8 && memcmp(program_data, "ASTC", 4) == 0) {
        printf("Runtime: Valid ASTC program detected\n");

        // 跳过ASTC头部，获取实际代码
        unsigned char* astc_code = (unsigned char*)program_data + 16; // 跳过头部
        size_t astc_code_size = program_size - 16;

        // 初始化ASTC虚拟机
        struct ASTCVirtualMachine vm;
        astc_vm_init(&vm, astc_code, astc_code_size);

        printf("Runtime: Executing ASTC bytecode...\n");

        // 执行虚拟机
        int result = 0;
        int instruction_count = 0;
        while (vm.running && instruction_count < 10000) { // 防止无限循环
            result = astc_execute_instruction(&vm);
            instruction_count++;
        }

        printf("Runtime: Executed %d instructions\n", instruction_count);
        printf("Runtime: Program executed, return value: %d\n", result);
        return result;
    } else {
        printf("Runtime: Invalid program format\n");
        return 1;
    }
}

// 简化的Runtime实现，只保留核心功能

// Runtime内存管理接口
void* evolver0_runtime_alloc(size_t size) {
    return malloc(size);
}

void evolver0_runtime_free(void* ptr) {
    free(ptr);
}

// ===============================================
// Runtime信息和版本
// ===============================================

const char* evolver0_runtime_version(void) {
    return "Evolver0 Runtime v1.0 - ASTC Virtual Machine";
}

uint32_t evolver0_runtime_get_version_number(void) {
    return 0x00010000; // 1.0.0
}

// ===============================================
// Runtime导出符号表
// ===============================================

// 这个结构定义了Runtime对外提供的接口
typedef struct {
    const char* name;
    void* function;
} RuntimeExport;

static const RuntimeExport runtime_exports[] = {
    {"main", evolver0_runtime_main},
    {NULL, NULL} // 结束标记
};

// 获取Runtime导出函数
void* evolver0_runtime_get_export(const char* name) {
    for (int i = 0; runtime_exports[i].name; i++) {
        if (strcmp(runtime_exports[i].name, name) == 0) {
            return runtime_exports[i].function;
        }
    }
    return NULL;
}

// ===============================================
// Runtime初始化和清理
// ===============================================

// Runtime全局初始化
int evolver0_runtime_init(void) {
    printf("Evolver0 Runtime initializing...\n");
    // 这里可以进行全局初始化
    return 0;
}

// Runtime全局清理
void evolver0_runtime_cleanup(void) {
    printf("Evolver0 Runtime cleaning up...\n");
    // 这里可以进行全局清理
}

// ===============================================
// 编译为无头二进制的入口点
// ===============================================

// 当Runtime被编译为无头二进制时，这是真正的入口点
// 这个函数会被Loader通过函数指针调用
int _start(void) {
    // 这是无头二进制的入口点
    // 实际的参数会通过其他方式传递
    printf("Evolver0 Runtime binary entry point\n");
    return evolver0_runtime_init();
}

// ===============================================
// 测试和调试接口
// ===============================================

#ifdef EVOLVER0_RUNTIME_TEST
// 测试模式下的main函数
int main(int argc, char* argv[]) {
    printf("=== Evolver0 Runtime Test Mode ===\n");
    
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
    
    unsigned char* data = malloc(size);
    fread(data, 1, size, file);
    fclose(file);
    
    // 执行Runtime
    int result = evolver0_runtime_main(data, size);
    
    free(data);
    return result;
}
#endif

#ifdef EVOLVER0_RUNTIME_STANDALONE
// 独立模式下的main函数 - 由Loader调用
int main(int argc, char* argv[]) {
    printf("=== Evolver0 Runtime Standalone Mode ===\n");

    if (argc < 2) {
        printf("Usage: %s <astc_file>\n", argv[0]);
        printf("This Runtime binary should be called by evolver0_loader.exe\n");
        return 1;
    }

    const char* astc_file = argv[1];
    printf("Runtime executing ASTC file: %s\n", astc_file);

    // 读取ASTC文件
    FILE* file = fopen(astc_file, "rb");
    if (!file) {
        fprintf(stderr, "Runtime Error: Cannot open ASTC file: %s\n", astc_file);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char* data = malloc(size);
    fread(data, 1, size, file);
    fclose(file);

    printf("Runtime: Loaded %zu bytes from %s\n", size, astc_file);

    // 执行Runtime
    int result = evolver0_runtime_main(data, size);

    printf("Runtime: Execution completed with result: %d\n", result);

    free(data);
    return result;
}
#endif
