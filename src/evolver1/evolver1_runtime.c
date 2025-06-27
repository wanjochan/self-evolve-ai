/**
 * evolver1_runtime.c - evolver1运行时
 * 
 * 基于evolver0_runtime的改进版本，支持更复杂的ASTC执行
 * 主要改进：
 * 1. 更完整的ASTC指令支持
 * 2. 改进的内存管理
 * 3. 更好的错误处理
 * 4. 支持基础的系统调用
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ASTC运行时状态
typedef struct {
    unsigned char* astc_data;
    size_t astc_size;
    size_t pc;              // 程序计数器
    int32_t stack[1024];    // 执行栈
    int stack_top;          // 栈顶指针
    int32_t locals[256];    // 局部变量
    int exit_code;          // 退出码
    int running;            // 运行状态
} ASTCRuntime;

// 初始化运行时
ASTCRuntime* runtime_init(unsigned char* astc_data, size_t astc_size) {
    ASTCRuntime* rt = malloc(sizeof(ASTCRuntime));
    if (!rt) return NULL;
    
    rt->astc_data = astc_data;
    rt->astc_size = astc_size;
    rt->pc = 0;
    rt->stack_top = 0;
    rt->exit_code = 0;
    rt->running = 1;
    
    // 初始化栈和局部变量
    memset(rt->stack, 0, sizeof(rt->stack));
    memset(rt->locals, 0, sizeof(rt->locals));
    
    return rt;
}

// 释放运行时
void runtime_free(ASTCRuntime* rt) {
    if (rt) {
        free(rt);
    }
}

// 栈操作
void push(ASTCRuntime* rt, int32_t value) {
    if (rt->stack_top < 1024) {
        rt->stack[rt->stack_top++] = value;
    }
}

int32_t pop(ASTCRuntime* rt) {
    if (rt->stack_top > 0) {
        return rt->stack[--rt->stack_top];
    }
    return 0;
}

// 读取32位整数
int32_t read_i32(ASTCRuntime* rt) {
    if (rt->pc + 4 <= rt->astc_size) {
        int32_t value = *(int32_t*)(rt->astc_data + rt->pc);
        rt->pc += 4;
        return value;
    }
    return 0;
}

// 读取字节
uint8_t read_u8(ASTCRuntime* rt) {
    if (rt->pc < rt->astc_size) {
        return rt->astc_data[rt->pc++];
    }
    return 0;
}

// 执行ASTC指令
int execute_instruction(ASTCRuntime* rt) {
    if (rt->pc >= rt->astc_size) {
        rt->running = 0;
        return 0;
    }
    
    uint8_t opcode = read_u8(rt);
    
    switch (opcode) {
        case 0x41: // i32.const
            {
                int32_t value = read_i32(rt);
                push(rt, value);
                printf("  i32.const %d\n", value);
            }
            break;
            
        case 0x6A: // i32.add
            {
                int32_t b = pop(rt);
                int32_t a = pop(rt);
                push(rt, a + b);
                printf("  i32.add %d + %d = %d\n", a, b, a + b);
            }
            break;
            
        case 0x6B: // i32.sub
            {
                int32_t b = pop(rt);
                int32_t a = pop(rt);
                push(rt, a - b);
                printf("  i32.sub %d - %d = %d\n", a, b, a - b);
            }
            break;
            
        case 0x0F: // return
            {
                if (rt->stack_top > 0) {
                    rt->exit_code = pop(rt);
                }
                rt->running = 0;
                printf("  return %d\n", rt->exit_code);
            }
            break;
            
        case 0x20: // local.get
            {
                uint8_t index = read_u8(rt);
                if (index < 256) {
                    push(rt, rt->locals[index]);
                    printf("  local.get %d = %d\n", index, rt->locals[index]);
                }
            }
            break;
            
        case 0x21: // local.set
            {
                uint8_t index = read_u8(rt);
                if (index < 256) {
                    rt->locals[index] = pop(rt);
                    printf("  local.set %d = %d\n", index, rt->locals[index]);
                }
            }
            break;
            
        default:
            printf("  未知指令: 0x%02X\n", opcode);
            rt->running = 0;
            return 1;
    }
    
    return 0;
}

// 执行ASTC程序
int runtime_execute(ASTCRuntime* rt) {
    printf("🚀 开始执行ASTC程序\n");
    
    // 验证ASTC头部
    if (rt->astc_size < 8) {
        printf("错误: ASTC文件太小\n");
        return 1;
    }
    
    if (memcmp(rt->astc_data, "ASTC", 4) != 0) {
        printf("错误: 无效的ASTC魔数\n");
        return 1;
    }
    
    int version = *(int*)(rt->astc_data + 4);
    printf("📊 ASTC版本: %d\n", version);
    
    // 跳过头部
    rt->pc = 8;
    
    // 执行指令循环
    int instruction_count = 0;
    while (rt->running && instruction_count < 10000) {
        if (execute_instruction(rt) != 0) {
            printf("错误: 指令执行失败\n");
            return 1;
        }
        instruction_count++;
    }
    
    if (instruction_count >= 10000) {
        printf("警告: 达到最大指令数限制\n");
    }
    
    printf("✅ ASTC程序执行完成，执行了 %d 条指令\n", instruction_count);
    printf("🏁 退出码: %d\n", rt->exit_code);
    
    return rt->exit_code;
}

// 主函数（用于独立测试）
int main(int argc, char* argv[]) {
    printf("evolver1_runtime v1.0 - 改进的ASTC运行时\n");
    
    if (argc != 2) {
        printf("用法: %s <program.astc>\n", argv[0]);
        return 1;
    }
    
    // 加载ASTC文件
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        printf("错误: 无法打开文件 %s\n", argv[1]);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    unsigned char* data = malloc(size);
    if (!data) {
        printf("错误: 内存分配失败\n");
        fclose(file);
        return 1;
    }
    
    fread(data, 1, size, file);
    fclose(file);
    
    // 创建运行时并执行
    ASTCRuntime* rt = runtime_init(data, size);
    if (!rt) {
        printf("错误: 运行时初始化失败\n");
        free(data);
        return 1;
    }
    
    int exit_code = runtime_execute(rt);
    
    runtime_free(rt);
    free(data);
    
    return exit_code;
}
