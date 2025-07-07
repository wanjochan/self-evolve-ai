#!/bin/bash
#
# build_module.sh - 构建原生模块工具的脚本
# 

# 确保脚本在错误时退出
set -e

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 定义路径 - 使用新的模块架构
MODULE_C="$SCRIPT_DIR/src/core/modules/module_module.c"
LAYER0_MODULE_C="$SCRIPT_DIR/src/core/modules/layer0_module.c"
BUILD_NATIVE_MODULE_C="$SCRIPT_DIR/src/tools/build_native_module.c"
OUTPUT_BIN="$SCRIPT_DIR/bin/build_native_module"

# 检查文件是否存在
if [ ! -f "$MODULE_C" ]; then
    echo "错误: module_module.c 未找到: $MODULE_C"
    exit 1
fi

if [ ! -f "$LAYER0_MODULE_C" ]; then
    echo "错误: layer0_module.c 未找到: $LAYER0_MODULE_C"
    exit 1
fi

if [ ! -f "$BUILD_NATIVE_MODULE_C" ]; then
    echo "错误: build_native_module.c 未找到: $BUILD_NATIVE_MODULE_C"
    exit 1
fi

# 创建bin目录
mkdir -p "$SCRIPT_DIR/bin"

# 编译
echo "编译 build_native_module (使用新模块架构)..."
echo "使用 ./cc.sh 编译 $MODULE_C $LAYER0_MODULE_C $BUILD_NATIVE_MODULE_C -o $OUTPUT_BIN"
"$SCRIPT_DIR/cc.sh" -I"$SCRIPT_DIR/src" "$MODULE_C" "$LAYER0_MODULE_C" "$BUILD_NATIVE_MODULE_C" -o "$OUTPUT_BIN"

# 检查退出状态
exit_code=$?
if [ $exit_code -ne 0 ]; then
    echo "编译失败，退出码: $exit_code"
    exit $exit_code
fi

echo "成功编译: $OUTPUT_BIN"

# 创建一个简单的Layer 1加载器示例
echo "编译简单加载器示例..."
mkdir -p "$SCRIPT_DIR/src/layer1"
cat > "$SCRIPT_DIR/src/layer1/simple_loader.c" << 'EOF'
/**
 * simple_loader.c - 简单的Layer 1加载器示例
 * 
 * 这是一个简单的加载器示例，用于演示如何加载.native模块。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 检测架构
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_NAME "x86_64"
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define ARCH_NAME "arm64"
#elif defined(__i386__) || defined(_M_IX86)
    #define ARCH_NAME "x86_32"
#else
    #error "不支持的架构"
#endif

// 简单的mmap模拟
void* load_module(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "无法打开模块文件: %s\n", filename);
        return NULL;
    }
    
    // 读取文件大小
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 分配内存
    void* memory = malloc(size);
    if (!memory) {
        fprintf(stderr, "内存分配失败\n");
        fclose(file);
        return NULL;
    }
    
    // 读取文件内容
    if (fread(memory, 1, size, file) != size) {
        fprintf(stderr, "读取文件失败\n");
        free(memory);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    return memory;
}

// 主函数
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("用法: %s <程序.astc> [参数...]\n", argv[0]);
        return 1;
    }
    
    // 构建VM模块名称
    char vm_module_name[256];
    snprintf(vm_module_name, sizeof(vm_module_name), "vm_%s.native", ARCH_NAME);
    
    printf("加载VM模块: %s\n", vm_module_name);
    void* vm_module = load_module(vm_module_name);
    if (!vm_module) {
        return 1;
    }
    
    // 这里应该解析模块头部，查找main函数等
    // 为简化示例，我们只打印一条消息
    printf("VM模块已加载，将执行: %s\n", argv[1]);
    
    // 在实际实现中，我们会调用VM模块的main函数
    // int (*vm_main)(int, char**) = find_export(vm_module, "main");
    // return vm_main(argc - 1, argv + 1);
    
    // 清理
    free(vm_module);
    
    return 0;
}
EOF

"$SCRIPT_DIR/cc.sh" "$SCRIPT_DIR/src/layer1/simple_loader.c" -o "$SCRIPT_DIR/bin/simple_loader"

echo "简单加载器已构建: $SCRIPT_DIR/bin/simple_loader"

# 显示使用说明
echo ""
echo "使用方法:"
echo "1. 编译目标代码: $SCRIPT_DIR/cc.sh -c your_module.c -o your_module.o"
echo "2. 创建导出符号文件 exports.txt，格式为: 符号名 类型 偏移 大小"
echo "3. 生成.native模块: $SCRIPT_DIR/bin/build_native_module your_module.o your_module.native --arch=x86_64 --type=vm --exports=exports.txt"
echo "4. 使用加载器: $SCRIPT_DIR/bin/simple_loader your_program.astc" 