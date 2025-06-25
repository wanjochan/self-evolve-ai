/**
 * loader.c - ASTC程序加载器实现
 * 
 * 该文件实现了ASTC程序加载器的基本功能，用于加载和运行ASTC格式的程序。
 * Loader模块是连接操作系统和Runtime的桥梁。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "loader.h"
#include "astc.h"
#include "runtime.h"
#include "c2astc.h"

// ===============================================
// 内部函数声明
// ===============================================

// 设置错误信息
static void loader_set_error(Loader* loader, const char* format, ...);

// 注册标准库函数
static bool loader_register_stdlib_functions(Loader* loader);

// 标准库函数实现
static RuntimeValue stdlib_print(RuntimeVM* vm, RuntimeValue* args, size_t arg_count);
static RuntimeValue stdlib_println(RuntimeVM* vm, RuntimeValue* args, size_t arg_count);
static RuntimeValue stdlib_read_int(RuntimeVM* vm, RuntimeValue* args, size_t arg_count);
static RuntimeValue stdlib_read_string(RuntimeVM* vm, RuntimeValue* args, size_t arg_count);
static RuntimeValue stdlib_malloc(RuntimeVM* vm, RuntimeValue* args, size_t arg_count);
static RuntimeValue stdlib_free(RuntimeVM* vm, RuntimeValue* args, size_t arg_count);

// ===============================================
// 公共函数实现
// ===============================================

bool loader_init(Loader* loader, const LoaderConfig* config) {
    if (!loader || !config) return false;
    
    // 初始化加载器
    memset(loader, 0, sizeof(Loader));
    
    // 复制配置
    loader->config = *config;
    
    // 如果提供了程序路径，则复制一份
    if (config->program_path) {
        loader->config.program_path = strdup(config->program_path);
    }
    
    // 如果提供了入口点，则复制一份
    if (config->entry_point) {
        loader->config.entry_point = strdup(config->entry_point);
    } else {
        // 默认入口点为main
        loader->config.entry_point = strdup("main");
    }
    
    // 初始化虚拟机
    if (!runtime_init(&loader->vm)) {
        loader_set_error(loader, "无法初始化虚拟机: %s", runtime_get_error(&loader->vm));
        return false;
    }
    
    // 注册标准库函数
    if (!loader_register_stdlib(loader)) {
        return false;
    }
    
    return true;
}

void loader_destroy(Loader* loader) {
    if (!loader) return;
    
    // 销毁虚拟机
    runtime_destroy(&loader->vm);
    
    // 释放程序
    if (loader->program) {
        ast_free(loader->program);
        loader->program = NULL;
    }
    
    // 释放配置中的字符串
    if (loader->config.program_path) {
        free((void*)loader->config.program_path);
        loader->config.program_path = NULL;
    }
    
    if (loader->config.entry_point) {
        free((void*)loader->config.entry_point);
        loader->config.entry_point = NULL;
    }
}

bool loader_load_program(Loader* loader, const char* path) {
    if (!loader || !path) {
        loader_set_error(loader, "无效的参数");
        return false;
    }
    
    // 更新程序路径
    if (loader->config.program_path) {
        free((void*)loader->config.program_path);
    }
    loader->config.program_path = strdup(path);
    
    // 打开文件
    FILE* file = fopen(path, "rb");
    if (!file) {
        loader_set_error(loader, "无法打开文件: %s", path);
        return false;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        loader_set_error(loader, "文件为空或无法获取文件大小: %s", path);
        return false;
    }
    
    // 分配内存并读取文件内容
    uint8_t* data = (uint8_t*)malloc(file_size);
    if (!data) {
        fclose(file);
        loader_set_error(loader, "内存分配失败");
        return false;
    }
    
    size_t bytes_read = fread(data, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        free(data);
        loader_set_error(loader, "读取文件失败: %s", path);
        return false;
    }
    
    // 从内存加载程序
    bool result = loader_load_program_from_memory(loader, data, file_size);
    
    // 释放内存
    free(data);
    
    return result;
}

bool loader_load_program_from_memory(Loader* loader, const uint8_t* data, size_t size) {
    if (!loader || !data || size == 0) {
        loader_set_error(loader, "无效的参数");
        return false;
    }
    
    // 检查魔数
    if (size < 4 || memcmp(data, ASTC_MAGIC, 4) != 0) {
        loader_set_error(loader, "无效的ASTC格式");
        return false;
    }
    
    // 释放之前的程序
    if (loader->program) {
        ast_free(loader->program);
        loader->program = NULL;
    }
    
    // 反序列化ASTC程序
    loader->program = c2astc_deserialize(data, size);
    if (!loader->program) {
        loader_set_error(loader, "反序列化ASTC程序失败: %s", c2astc_get_error());
        return false;
    }
    
    // 加载程序到虚拟机
    if (!runtime_load_program(&loader->vm, loader->program)) {
        loader_set_error(loader, "加载程序到虚拟机失败: %s", runtime_get_error(&loader->vm));
        return false;
    }
    
    return true;
}

int loader_run(Loader* loader) {
    if (!loader) {
        return -1;
    }
    
    // 检查程序是否已加载
    if (!loader->program) {
        loader_set_error(loader, "未加载程序");
        return -1;
    }
    
    // 执行程序
    int result = runtime_execute(&loader->vm, loader->config.entry_point);
    
    // 检查执行结果
    if (result < 0) {
        loader_set_error(loader, "程序执行失败: %s", runtime_get_error(&loader->vm));
    }
    
    return result;
}

bool loader_register_stdlib(Loader* loader) {
    if (!loader) {
        return false;
    }
    
    // 注册标准库函数
    return loader_register_stdlib_functions(loader);
}

const char* loader_get_error(Loader* loader) {
    if (!loader) return "无效的加载器实例";
    return loader->error_message;
}

bool loader_set_args(Loader* loader, int argc, char** argv) {
    if (!loader) {
        return false;
    }
    
    loader->config.argc = argc;
    loader->config.argv = argv;
    
    return true;
}

// ===============================================
// 内部函数实现
// ===============================================

// 设置错误信息
static void loader_set_error(Loader* loader, const char* format, ...) {
    if (!loader) return;
    
    va_list args;
    va_start(args, format);
    vsnprintf(loader->error_message, sizeof(loader->error_message), format, args);
    va_end(args);
}

// 注册标准库函数
static bool loader_register_stdlib_functions(Loader* loader) {
    if (!loader) return false;
    
    // 注册标准输入输出函数
    if (!runtime_register_native_function(&loader->vm, "print", stdlib_print)) {
        loader_set_error(loader, "注册print函数失败: %s", runtime_get_error(&loader->vm));
        return false;
    }
    
    if (!runtime_register_native_function(&loader->vm, "println", stdlib_println)) {
        loader_set_error(loader, "注册println函数失败: %s", runtime_get_error(&loader->vm));
        return false;
    }
    
    if (!runtime_register_native_function(&loader->vm, "read_int", stdlib_read_int)) {
        loader_set_error(loader, "注册read_int函数失败: %s", runtime_get_error(&loader->vm));
        return false;
    }
    
    if (!runtime_register_native_function(&loader->vm, "read_string", stdlib_read_string)) {
        loader_set_error(loader, "注册read_string函数失败: %s", runtime_get_error(&loader->vm));
        return false;
    }
    
    // 注册内存管理函数
    if (!runtime_register_native_function(&loader->vm, "malloc", stdlib_malloc)) {
        loader_set_error(loader, "注册malloc函数失败: %s", runtime_get_error(&loader->vm));
        return false;
    }
    
    if (!runtime_register_native_function(&loader->vm, "free", stdlib_free)) {
        loader_set_error(loader, "注册free函数失败: %s", runtime_get_error(&loader->vm));
        return false;
    }
    
    return true;
}

// 标准库函数实现
static RuntimeValue stdlib_print(RuntimeVM* vm, RuntimeValue* args, size_t arg_count) {
    // 打印参数
    for (size_t i = 0; i < arg_count; i++) {
        switch (args[i].type) {
            case RT_VAL_I32:
                printf("%d", args[i].value.i32);
                break;
            case RT_VAL_I64:
                printf("%lld", (long long)args[i].value.i64);
                break;
            case RT_VAL_F32:
                printf("%f", args[i].value.f32);
                break;
            case RT_VAL_F64:
                printf("%lf", args[i].value.f64);
                break;
            case RT_VAL_PTR:
                if (args[i].value.ptr) {
                    printf("%s", (char*)args[i].value.ptr);
                } else {
                    printf("(null)");
                }
                break;
            default:
                printf("(unknown)");
                break;
        }
    }
    
    return runtime_value_i32(0);
}

static RuntimeValue stdlib_println(RuntimeVM* vm, RuntimeValue* args, size_t arg_count) {
    // 调用print
    stdlib_print(vm, args, arg_count);
    
    // 打印换行
    printf("\n");
    
    return runtime_value_i32(0);
}

static RuntimeValue stdlib_read_int(RuntimeVM* vm, RuntimeValue* args, size_t arg_count) {
    int value;
    if (scanf("%d", &value) != 1) {
        value = 0;
    }
    
    return runtime_value_i32(value);
}

static RuntimeValue stdlib_read_string(RuntimeVM* vm, RuntimeValue* args, size_t arg_count) {
    // 检查参数
    if (arg_count < 1 || args[0].type != RT_VAL_I32) {
        return runtime_value_ptr(NULL);
    }
    
    // 获取缓冲区大小
    int buffer_size = args[0].value.i32;
    if (buffer_size <= 0) {
        return runtime_value_ptr(NULL);
    }
    
    // 分配缓冲区
    char* buffer = (char*)runtime_allocate(vm, buffer_size);
    if (!buffer) {
        return runtime_value_ptr(NULL);
    }
    
    // 读取字符串
    if (fgets(buffer, buffer_size, stdin) == NULL) {
        buffer[0] = '\0';
    } else {
        // 移除换行符
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
    }
    
    return runtime_value_ptr(buffer);
}

static RuntimeValue stdlib_malloc(RuntimeVM* vm, RuntimeValue* args, size_t arg_count) {
    // 检查参数
    if (arg_count < 1 || args[0].type != RT_VAL_I32) {
        return runtime_value_ptr(NULL);
    }
    
    // 获取大小
    int size = args[0].value.i32;
    if (size <= 0) {
        return runtime_value_ptr(NULL);
    }
    
    // 分配内存
    void* ptr = runtime_allocate(vm, size);
    
    return runtime_value_ptr(ptr);
}

static RuntimeValue stdlib_free(RuntimeVM* vm, RuntimeValue* args, size_t arg_count) {
    // 检查参数
    if (arg_count < 1 || args[0].type != RT_VAL_PTR) {
        return runtime_value_i32(0);
    }
    
    // 释放内存
    runtime_free(vm, args[0].value.ptr);
    
    return runtime_value_i32(0);
} 