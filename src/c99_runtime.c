/**
 * c99_runtime.c - C99运行时实现
 * 
 * 提供完整的C99运行时支持：
 * 1. 完整的libc函数转发
 * 2. 标准库函数实现
 * 3. 内存管理
 * 4. 文件I/O
 * 5. 字符串处理
 * 6. 数学函数
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "runtime/core_libc.h"
#include "runtime/core_astc.h"

// ===============================================
// C99运行时配置
// ===============================================

typedef struct {
    size_t heap_size;           // 堆大小
    size_t stack_size;          // 栈大小
    bool debug_mode;            // 调试模式
    bool memory_tracking;       // 内存跟踪
    bool performance_monitoring; // 性能监控
} C99RuntimeConfig;

// ===============================================
// C99标准库函数转发表
// ===============================================

typedef struct {
    const char* name;           // 函数名
    uint16_t func_id;          // 函数ID
    int arg_count;             // 参数个数
    const char* signature;     // 函数签名
} LibcFunction;

// C99标准库函数映射表
static const LibcFunction c99_libc_functions[] = {
    // stdio.h
    {"printf", 0x0030, -1, "int printf(const char *format, ...)"},
    {"fprintf", 0x0031, -1, "int fprintf(FILE *stream, const char *format, ...)"},
    {"sprintf", 0x0032, -1, "int sprintf(char *str, const char *format, ...)"},
    {"scanf", 0x0033, -1, "int scanf(const char *format, ...)"},
    {"fscanf", 0x0034, -1, "int fscanf(FILE *stream, const char *format, ...)"},
    {"sscanf", 0x0035, -1, "int sscanf(const char *str, const char *format, ...)"},
    {"fopen", 0x0040, 2, "FILE *fopen(const char *filename, const char *mode)"},
    {"fclose", 0x0041, 1, "int fclose(FILE *stream)"},
    {"fread", 0x0042, 4, "size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)"},
    {"fwrite", 0x0043, 4, "size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)"},
    {"fseek", 0x0044, 3, "int fseek(FILE *stream, long offset, int whence)"},
    {"ftell", 0x0045, 1, "long ftell(FILE *stream)"},
    {"fgetc", 0x0046, 1, "int fgetc(FILE *stream)"},
    {"fputc", 0x0047, 2, "int fputc(int c, FILE *stream)"},
    {"fgets", 0x0048, 3, "char *fgets(char *str, int n, FILE *stream)"},
    {"fputs", 0x0049, 2, "int fputs(const char *str, FILE *stream)"},
    
    // stdlib.h
    {"malloc", 0x0050, 1, "void *malloc(size_t size)"},
    {"calloc", 0x0051, 2, "void *calloc(size_t nmemb, size_t size)"},
    {"realloc", 0x0052, 2, "void *realloc(void *ptr, size_t size)"},
    {"free", 0x0053, 1, "void free(void *ptr)"},
    {"exit", 0x0054, 1, "void exit(int status)"},
    {"abort", 0x0055, 0, "void abort(void)"},
    {"atoi", 0x0056, 1, "int atoi(const char *str)"},
    {"atol", 0x0057, 1, "long atol(const char *str)"},
    {"atof", 0x0058, 1, "double atof(const char *str)"},
    {"strtol", 0x0059, 3, "long strtol(const char *str, char **endptr, int base)"},
    {"strtod", 0x005A, 2, "double strtod(const char *str, char **endptr)"},
    {"rand", 0x005B, 0, "int rand(void)"},
    {"srand", 0x005C, 1, "void srand(unsigned int seed)"},
    
    // string.h
    {"strlen", 0x0060, 1, "size_t strlen(const char *str)"},
    {"strcpy", 0x0061, 2, "char *strcpy(char *dest, const char *src)"},
    {"strncpy", 0x0062, 3, "char *strncpy(char *dest, const char *src, size_t n)"},
    {"strcat", 0x0063, 2, "char *strcat(char *dest, const char *src)"},
    {"strncat", 0x0064, 3, "char *strncat(char *dest, const char *src, size_t n)"},
    {"strcmp", 0x0065, 2, "int strcmp(const char *str1, const char *str2)"},
    {"strncmp", 0x0066, 3, "int strncmp(const char *str1, const char *str2, size_t n)"},
    {"strchr", 0x0067, 2, "char *strchr(const char *str, int c)"},
    {"strrchr", 0x0068, 2, "char *strrchr(const char *str, int c)"},
    {"strstr", 0x0069, 2, "char *strstr(const char *haystack, const char *needle)"},
    {"memcpy", 0x006A, 3, "void *memcpy(void *dest, const void *src, size_t n)"},
    {"memmove", 0x006B, 3, "void *memmove(void *dest, const void *src, size_t n)"},
    {"memset", 0x006C, 3, "void *memset(void *ptr, int value, size_t n)"},
    {"memcmp", 0x006D, 3, "int memcmp(const void *ptr1, const void *ptr2, size_t n)"},
    
    // math.h
    {"sin", 0x0070, 1, "double sin(double x)"},
    {"cos", 0x0071, 1, "double cos(double x)"},
    {"tan", 0x0072, 1, "double tan(double x)"},
    {"asin", 0x0073, 1, "double asin(double x)"},
    {"acos", 0x0074, 1, "double acos(double x)"},
    {"atan", 0x0075, 1, "double atan(double x)"},
    {"atan2", 0x0076, 2, "double atan2(double y, double x)"},
    {"exp", 0x0077, 1, "double exp(double x)"},
    {"log", 0x0078, 1, "double log(double x)"},
    {"log10", 0x0079, 1, "double log10(double x)"},
    {"pow", 0x007A, 2, "double pow(double x, double y)"},
    {"sqrt", 0x007B, 1, "double sqrt(double x)"},
    {"ceil", 0x007C, 1, "double ceil(double x)"},
    {"floor", 0x007D, 1, "double floor(double x)"},
    {"fabs", 0x007E, 1, "double fabs(double x)"},
    
    // time.h
    {"time", 0x0080, 1, "time_t time(time_t *timer)"},
    {"clock", 0x0081, 0, "clock_t clock(void)"},
    {"difftime", 0x0082, 2, "double difftime(time_t time1, time_t time0)"},
    
    {NULL, 0, 0, NULL}  // 结束标记
};

// ===============================================
// C99运行时虚拟机
// ===============================================

typedef struct {
    uint8_t* code;              // ASTC字节码
    size_t code_size;           // 代码大小
    uint32_t pc;                // 程序计数器
    uint32_t stack[2048];       // 操作数栈 (增大到2048)
    int32_t stack_top;          // 栈顶指针
    uint32_t locals[512];       // 局部变量 (增大到512)
    uint32_t globals[1024];     // 全局变量
    bool running;               // 运行状态
    C99RuntimeConfig config;    // 运行时配置
    
    // 性能统计
    uint64_t instruction_count;
    uint64_t function_calls;
    clock_t start_time;
} C99VirtualMachine;

// ===============================================
// C99运行时核心功能
// ===============================================

void c99_vm_init(C99VirtualMachine* vm, uint8_t* code, size_t code_size) {
    vm->code = code;
    vm->code_size = code_size;
    vm->pc = 0;
    vm->stack_top = -1;
    vm->running = true;
    vm->instruction_count = 0;
    vm->function_calls = 0;
    vm->start_time = clock();
    
    // 初始化配置
    vm->config.heap_size = 1024 * 1024;  // 1MB堆
    vm->config.stack_size = 64 * 1024;   // 64KB栈
    vm->config.debug_mode = false;
    vm->config.memory_tracking = false;
    vm->config.performance_monitoring = false;
    
    memset(vm->stack, 0, sizeof(vm->stack));
    memset(vm->locals, 0, sizeof(vm->locals));
    memset(vm->globals, 0, sizeof(vm->globals));
}

void c99_vm_push(C99VirtualMachine* vm, uint32_t value) {
    if (vm->stack_top < 2047) {
        vm->stack[++vm->stack_top] = value;
    } else {
        fprintf(stderr, "Runtime Error: Stack overflow\n");
        vm->running = false;
    }
}

uint32_t c99_vm_pop(C99VirtualMachine* vm) {
    if (vm->stack_top >= 0) {
        return vm->stack[vm->stack_top--];
    } else {
        fprintf(stderr, "Runtime Error: Stack underflow\n");
        vm->running = false;
        return 0;
    }
}

// 查找libc函数
const LibcFunction* find_libc_function(uint16_t func_id) {
    for (int i = 0; c99_libc_functions[i].name != NULL; i++) {
        if (c99_libc_functions[i].func_id == func_id) {
            return &c99_libc_functions[i];
        }
    }
    return NULL;
}

// 执行libc函数调用
int execute_libc_call(C99VirtualMachine* vm, uint16_t func_id, uint16_t arg_count) {
    const LibcFunction* func = find_libc_function(func_id);
    if (!func) {
        fprintf(stderr, "Runtime Error: Unknown libc function ID: 0x%04X\n", func_id);
        return 1;
    }
    
    if (vm->config.debug_mode) {
        printf("Calling libc function: %s (ID: 0x%04X, args: %d)\n", 
               func->name, func_id, arg_count);
    }
    
    // 创建libc调用结构
    LibcCall call;
    call.func_id = func_id;
    call.arg_count = arg_count;
    
    // 从栈中获取参数
    for (int i = arg_count - 1; i >= 0; i--) {
        call.args[i] = c99_vm_pop(vm);
    }
    
    // 执行libc转发
    int result = libc_forward_call(&call);
    if (result == 0) {
        c99_vm_push(vm, (uint32_t)call.return_value);
        vm->function_calls++;
    } else {
        fprintf(stderr, "Runtime Error: libc function call failed: %s\n", func->name);
        c99_vm_push(vm, 0); // 错误时返回0
    }
    
    return result;
}

// 执行单条ASTC指令
int c99_execute_instruction(C99VirtualMachine* vm) {
    if (vm->pc >= vm->code_size || !vm->running) {
        vm->running = false;
        return 0;
    }
    
    uint8_t opcode = vm->code[vm->pc++];
    vm->instruction_count++;

    switch (opcode) {
        case 0x00: // NOP
            break;
            
        case 0x01: // HALT
            vm->running = false;
            return c99_vm_pop(vm);
            
        case 0x10: // CONST_I32
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t value = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                c99_vm_push(vm, value);
            }
            break;
            
        case 0x11: // CONST_F32
            if (vm->pc + 4 <= vm->code_size) {
                float value = *(float*)(vm->code + vm->pc);
                vm->pc += 4;
                c99_vm_push(vm, *(uint32_t*)&value);
            }
            break;

        case 0x12: // CONST_STRING
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t string_len = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;

                if (vm->pc + string_len <= vm->code_size) {
                    // 字符串数据直接嵌入在字节码中
                    char* string_ptr = (char*)(vm->code + vm->pc);
                    vm->pc += string_len;
                    c99_vm_push(vm, (uint32_t)(uintptr_t)string_ptr);
                }
            }
            break;
            
        case 0x20: // ADD
            {
                uint32_t b = c99_vm_pop(vm);
                uint32_t a = c99_vm_pop(vm);
                c99_vm_push(vm, a + b);
            }
            break;
            
        case 0x21: // SUB
            {
                uint32_t b = c99_vm_pop(vm);
                uint32_t a = c99_vm_pop(vm);
                c99_vm_push(vm, a - b);
            }
            break;
            
        case 0x22: // MUL
            {
                uint32_t b = c99_vm_pop(vm);
                uint32_t a = c99_vm_pop(vm);
                c99_vm_push(vm, a * b);
            }
            break;
            
        case 0x23: // DIV
            {
                uint32_t b = c99_vm_pop(vm);
                uint32_t a = c99_vm_pop(vm);
                if (b != 0) {
                    c99_vm_push(vm, a / b);
                } else {
                    fprintf(stderr, "Runtime Error: Division by zero\n");
                    vm->running = false;
                }
            }
            break;
            
        case 0x30: // LOAD_LOCAL
            if (vm->pc < vm->code_size) {
                uint8_t index = vm->code[vm->pc++];
                if (index < 512) {
                    c99_vm_push(vm, vm->locals[index]);
                }
            }
            break;
            
        case 0x31: // STORE_LOCAL
            if (vm->pc < vm->code_size) {
                uint8_t index = vm->code[vm->pc++];
                if (index < 512) {
                    vm->locals[index] = c99_vm_pop(vm);
                }
            }
            break;
            
        case 0x32: // LOAD_GLOBAL
            if (vm->pc + 2 <= vm->code_size) {
                uint16_t index = *(uint16_t*)(vm->code + vm->pc);
                vm->pc += 2;
                if (index < 1024) {
                    c99_vm_push(vm, vm->globals[index]);
                }
            }
            break;
            
        case 0x33: // STORE_GLOBAL
            if (vm->pc + 2 <= vm->code_size) {
                uint16_t index = *(uint16_t*)(vm->code + vm->pc);
                vm->pc += 2;
                if (index < 1024) {
                    vm->globals[index] = c99_vm_pop(vm);
                }
            }
            break;
            
        case 0xF0: // LIBC_CALL - C99标准库调用
            {
                printf("DEBUG: LIBC_CALL instruction executed\n");
                uint16_t func_id = c99_vm_pop(vm);
                uint16_t arg_count = c99_vm_pop(vm);
                printf("DEBUG: func_id=0x%04X, arg_count=%d\n", func_id, arg_count);
                execute_libc_call(vm, func_id, arg_count);
                printf("DEBUG: LIBC_CALL completed\n");
            }
            break;
            
        default:
            fprintf(stderr, "Runtime Error: Unknown opcode: 0x%02X at PC: %u\n", opcode, vm->pc - 1);
            vm->running = false;
            break;
    }
    
    return 0;
}

// ===============================================
// C99运行时主入口点
// ===============================================

/**
 * C99运行时主入口点，由Loader调用
 * 参数：ASTC程序数据和大小
 */
int c99_runtime_main(void* program_data, size_t program_size) {
    // 初始化libc转发系统
    libc_forward_init();

    // 简单的调试输出
    printf("C99 Runtime called with %zu bytes\n", program_size);
    
    if (!program_data || program_size == 0) {
        libc_forward_cleanup();
        return 1;
    }

    // 检查ASTC格式
    if (program_size >= 16 && memcmp(program_data, "ASTC", 4) == 0) {
        // 解析ASTC头部
        uint32_t* header = (uint32_t*)program_data;
        uint32_t version = header[1];
        uint32_t data_size = header[2];
        uint32_t entry_point = header[3];
        
        printf("C99 Runtime v1.0 - Starting execution\n");
        printf("ASTC version: %u, data size: %u, entry point: %u\n", 
               version, data_size, entry_point);
        
        // 获取ASTC代码段
        uint8_t* astc_code = (uint8_t*)program_data + 16;
        size_t astc_code_size = program_size - 16;
        
        // 初始化C99虚拟机
        C99VirtualMachine vm;
        c99_vm_init(&vm, astc_code, astc_code_size);
        
        // 执行ASTC程序
        int result = 0;
        while (vm.running && vm.instruction_count < 1000000) {
            result = c99_execute_instruction(&vm);
            if (result != 0) break;
        }
        
        // 输出性能统计
        clock_t end_time = clock();
        double execution_time = ((double)(end_time - vm.start_time)) / CLOCKS_PER_SEC;
        
        printf("\nC99 Runtime execution completed\n");
        printf("Instructions executed: %llu\n", vm.instruction_count);
        printf("Function calls: %llu\n", vm.function_calls);
        printf("Execution time: %.3f seconds\n", execution_time);
        
        // 清理libc转发系统
        libc_forward_cleanup();
        
        return result;
    } else {
        fprintf(stderr, "Error: Invalid ASTC format\n");
        libc_forward_cleanup();
        return 1;
    }
}

// ===============================================
// 无头二进制入口点
// ===============================================

/**
 * Runtime入口点 - 支持命令行调用和函数指针调用
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program.astc>\n", argv[0]);
        return 1;
    }

    // 读取ASTC文件
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file: %s\n", argv[1]);
        return 1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 读取文件内容
    void* program_data = malloc(file_size);
    if (!program_data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return 1;
    }

    size_t read_size = fread(program_data, 1, file_size, file);
    fclose(file);

    if (read_size != file_size) {
        fprintf(stderr, "Error: Failed to read file completely\n");
        free(program_data);
        return 1;
    }

    // 调用Runtime主函数
    int result = c99_runtime_main(program_data, file_size);

    // 清理
    free(program_data);
    return result;
}

/**
 * Runtime函数指针入口点 - 由Loader调用
 */
int runtime_entry(void* program_data, size_t program_size) {
    return c99_runtime_main(program_data, program_size);
}
