#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/core/module.h"

// 模块系统接口
typedef struct Module {
    const char* name;
    void* handle;
    int state;
    char error[256];
} Module;

// 模块系统函数声明
int module_init();
Module* module_load(const char* name);
void* module_resolve(Module* module, const char* symbol);
void module_unload(Module* module);
void module_cleanup();

int main(int argc, char *argv[]) {
    printf("=== 模块加载测试 ===\n");
    
    // 初始化模块系统
    printf("初始化模块系统...\n");
    if (module_init() != 0) {
        printf("错误：模块系统初始化失败\n");
        return 1;
    }
    
    // 尝试加载内存模块
    printf("尝试加载内存模块...\n");
    Module *memory_module = module_load("memory");
    if (memory_module == NULL) {
        printf("注意：无法加载内存模块，这可能是因为模块文件不存在\n");
    } else {
        printf("成功：内存模块已加载\n");
        module_unload(memory_module);
    }
    
    // 清理模块系统
    printf("清理模块系统...\n");
    module_cleanup();
    
    printf("=== 测试完成 ===\n");
    return 0;
}
