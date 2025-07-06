#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/core/module.h"

// 测试模块系统的基本功能
int main(int argc, char *argv[]) {
    printf("=== 模块系统测试 ===\n");
    
    // 初始化模块系统
    printf("初始化模块系统...\n");
    if (module_init() != 0) {
        printf("错误：模块系统初始化失败\n");
        return 1;
    }
    
    // 加载内存模块
    printf("加载内存模块...\n");
    Module *memory_module = module_load("memory");
    if (memory_module == NULL) {
        printf("错误：加载内存模块失败\n");
        return 1;
    }
    
    // 解析内存分配函数
    printf("解析内存分配函数...\n");
    void* (*memory_alloc)(size_t size) = module_resolve(memory_module, "memory_alloc");
    if (memory_alloc == NULL) {
        printf("错误：解析memory_alloc函数失败\n");
        return 1;
    }
    
    // 测试内存分配
    printf("测试内存分配...\n");
    void *ptr = memory_alloc(100);
    if (ptr == NULL) {
        printf("错误：内存分配失败\n");
        return 1;
    }
    
    printf("内存分配成功：%p\n", ptr);
    
    // 解析内存释放函数
    printf("解析内存释放函数...\n");
    void (*memory_free)(void *ptr) = module_resolve(memory_module, "memory_free");
    if (memory_free == NULL) {
        printf("错误：解析memory_free函数失败\n");
        return 1;
    }
    
    // 测试内存释放
    printf("测试内存释放...\n");
    memory_free(ptr);
    
    // 加载ASTC模块
    printf("加载ASTC模块...\n");
    Module *astc_module = module_load("astc");
    if (astc_module == NULL) {
        printf("错误：加载ASTC模块失败\n");
        return 1;
    }
    
    // 卸载模块
    printf("卸载模块...\n");
    module_unload(memory_module);
    module_unload(astc_module);
    
    // 清理模块系统
    printf("清理模块系统...\n");
    module_cleanup();
    
    printf("=== 测试完成 ===\n");
    return 0;
} 