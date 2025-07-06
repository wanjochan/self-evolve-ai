#!/bin/bash

# build_all_tools.sh - 完整的三层架构构建流程
# 
# 这个脚本构建所有必要的工具，实现从C源码到三层架构执行的完整链路

echo "=========================================="
echo "Self-Evolve AI 三层架构构建系统"
echo "=========================================="
echo "基于 TinyCC 的完整构建流程"
echo ""

# 检查cc.sh是否存在
if [ ! -x "./cc.sh" ]; then
    echo "❌ 错误: cc.sh 不存在或不可执行"
    echo "请确保 TinyCC 已正确安装并且 cc.sh 在当前目录"
    exit 1
fi

echo "✅ TinyCC 编译器检查通过"

# 创建必要的目录
echo ""
echo "创建目录结构..."
mkdir -p bin
mkdir -p examples
mkdir -p tests

echo "✅ 目录结构创建完成"

# 构建步骤1: build_native_module工具
echo ""
echo "=========================================="
echo "步骤1: 构建 build_native_module 工具"
echo "=========================================="

if ./build_native_module_tool.sh; then
    echo "✅ build_native_module 工具构建成功"
else
    echo "❌ build_native_module 工具构建失败"
    exit 1
fi

# 构建步骤2: c2astc工具
echo ""
echo "=========================================="
echo "步骤2: 构建 c2astc 工具"
echo "=========================================="

if ./build_c2astc_tool.sh; then
    echo "✅ c2astc 工具构建成功"
else
    echo "❌ c2astc 工具构建失败"
    exit 1
fi

# 构建步骤3: simple_loader
echo ""
echo "=========================================="
echo "步骤3: 构建 simple_loader"
echo "=========================================="

if ./build_simple_loader.sh; then
    echo "✅ simple_loader 构建成功"
else
    echo "❌ simple_loader 构建失败"
    exit 1
fi

# 测试步骤4: 创建ASTC程序示例
echo ""
echo "=========================================="
echo "步骤4: 创建 ASTC 程序示例"
echo "=========================================="

echo "创建 hello_world.astc..."
if ./bin/c2astc examples/hello_world.c examples/hello_world.astc; then
    echo "✅ hello_world.astc 创建成功"
else
    echo "❌ hello_world.astc 创建失败"
    exit 1
fi

echo "创建 test_program.astc..."
if ./bin/c2astc examples/test_program.c examples/test_program.astc; then
    echo "✅ test_program.astc 创建成功"
else
    echo "❌ test_program.astc 创建失败"
    exit 1
fi

# 验证步骤5: 检查构建结果
echo ""
echo "=========================================="
echo "步骤5: 验证构建结果"
echo "=========================================="

echo "检查构建的工具:"
echo ""

if [ -x "bin/build_native_module" ]; then
    echo "✅ bin/build_native_module ($(ls -lh bin/build_native_module | awk '{print $5}'))"
else
    echo "❌ bin/build_native_module 缺失"
fi

if [ -x "bin/c2astc" ]; then
    echo "✅ bin/c2astc ($(ls -lh bin/c2astc | awk '{print $5}'))"
else
    echo "❌ bin/c2astc 缺失"
fi

if [ -x "bin/simple_loader" ]; then
    echo "✅ bin/simple_loader ($(ls -lh bin/simple_loader | awk '{print $5}'))"
else
    echo "❌ bin/simple_loader 缺失"
fi

echo ""
echo "检查ASTC程序:"
echo ""

if [ -f "examples/hello_world.astc" ]; then
    echo "✅ examples/hello_world.astc ($(ls -lh examples/hello_world.astc | awk '{print $5}'))"
else
    echo "❌ examples/hello_world.astc 缺失"
fi

if [ -f "examples/test_program.astc" ]; then
    echo "✅ examples/test_program.astc ($(ls -lh examples/test_program.astc | awk '{print $5}'))"
else
    echo "❌ examples/test_program.astc 缺失"
fi

# 总结
echo ""
echo "=========================================="
echo "构建完成总结"
echo "=========================================="
echo ""
echo "🎉 三层架构工具链构建完成！"
echo ""
echo "Layer 1 (Loader):"
echo "  📁 bin/simple_loader - 架构特定加载器"
echo ""
echo "Layer 2 (Runtime) 工具:"
echo "  📁 bin/build_native_module - 创建.native模块"
echo ""
echo "Layer 3 (Program) 工具:"
echo "  📁 bin/c2astc - C源码到ASTC转换器"
echo ""
echo "示例程序:"
echo "  📁 examples/hello_world.astc - Hello World程序"
echo "  📁 examples/test_program.astc - 测试程序"
echo ""
echo "使用方法:"
echo "  1. 编译C程序为ASTC: ./bin/c2astc source.c program.astc"
echo "  2. 运行ASTC程序: ./bin/simple_loader program.astc"
echo ""
echo "注意: 当前缺少VM模块(.native文件)，simple_loader会报告找不到VM模块"
echo "这是正常的，因为我们还没有创建对应架构的VM模块文件"
