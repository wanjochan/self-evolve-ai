#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 简单的布尔类型定义，避免使用stdbool.h */
typedef int bool;
#define true 1
#define false 0

/* 模块结构定义 */
typedef enum {
    MODULE_UNLOADED = 0,
    MODULE_LOADING,
    MODULE_READY,
    MODULE_ERROR
} ModuleState;

typedef struct Module {
    const char* name;
    void* handle;
    ModuleState state;
    const char* error;
    
    int (*load)(void);
    void (*unload)(void);
    void* (*resolve)(const char* symbol);
    
    void (*on_init)(void);
    void (*on_exit)(void);
    void (*on_error)(const char* msg);
} Module;

/* 模块系统函数声明 */
extern int module_init(void);
extern void module_cleanup(void);
extern Module* module_load(const char* name);
extern void module_unload(Module* module);
extern void* module_resolve(Module* module, const char* symbol);
extern Module* module_get(const char* name);
extern int module_register(Module* module);

/* 测试函数 */
bool test_module_init() {
    printf("测试模块系统初始化...\n");
    
    if (module_init() != 0) {
        printf("模块系统初始化失败\n");
        return false;
    }
    
    printf("模块系统初始化成功\n");
    return true;
}

/* 内存模块测试 */
bool test_memory_module() {
    printf("测试内存模块...\n");
    
    /* 创建一个简单的内存模块 */
    static Module memory_module = {
        .name = "memory",
        .state = MODULE_UNLOADED,
        .error = NULL,
        .load = NULL,
        .unload = NULL,
        .resolve = NULL
    };
    
    /* 注册模块 */
    if (module_register(&memory_module) != 0) {
        printf("注册内存模块失败\n");
        return false;
    }
    
    /* 获取模块 */
    Module* memory = module_get("memory");
    if (!memory) {
        printf("获取内存模块失败\n");
        return false;
    }
    
    printf("内存模块注册成功\n");
    return true;
}

/* 主函数 */
int main() {
    printf("开始模块系统测试...\n");
    
    bool success = test_module_init();
    if (success) {
        success = test_memory_module();
    }
    
    /* 清理模块系统 */
    module_cleanup();
    
    if (success) {
        printf("所有测试通过!\n");
        return 0;
    } else {
        printf("测试失败!\n");
        return 1;
    }
}
