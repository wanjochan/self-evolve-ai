#!/bin/bash
#
# build_modules_gcc.sh - 使用GCC构建模块系统
# 
# 这个脚本专门用于构建需要真正目标文件和动态链接的模块系统
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src/core"
MODULES_DIR="$SRC_DIR/modules"
BIN_DIR="$SCRIPT_DIR/bin"

echo "=== 使用GCC构建模块系统 ==="

# 创建输出目录
mkdir -p "$BIN_DIR/layer2"

# 检测架构
if [[ "$(uname -m)" == "arm64" ]]; then
    ARCH="arm64"
    BITS="64"
elif [[ "$(uname -m)" == "x86_64" ]]; then
    ARCH="x64"
    BITS="64"
elif [[ "$(uname -m)" == "i386" ]] || [[ "$(uname -m)" == "i686" ]]; then
    ARCH="x86"
    BITS="32"
else
    echo "警告: 未知架构 $(uname -m)，使用默认 x64_64"
    ARCH="x64"
    BITS="64"
fi

echo "检测到架构: ${ARCH}_${BITS}"

# 编译核心模块
echo ""
echo "1. 使用GCC编译核心模块..."

# 编译layer0模块
echo "编译 layer0_module.c..."
gcc -c "$MODULES_DIR/layer0_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/layer0_module.o" -fPIC -std=c99 -D_GNU_SOURCE

# 编译pipeline模块
echo "编译 pipeline_module.c..."
gcc -c "$MODULES_DIR/pipeline_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/pipeline_module.o" -fPIC -std=c99 -D_GNU_SOURCE -Wno-format

# 编译compiler模块
echo "编译 compiler_module.c..."
gcc -c "$MODULES_DIR/compiler_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/compiler_module.o" -fPIC -std=c99 -D_GNU_SOURCE

# 编译module模块
echo "编译 module_module.c..."
gcc -c "$MODULES_DIR/module_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/module_module.o" -fPIC -std=c99 -D_GNU_SOURCE

# 编译libc模块
echo "编译 libc_module.c..."
gcc -c "$MODULES_DIR/libc_module.c" -I "$SRC_DIR" -o "$MODULES_DIR/libc_module.o" -fPIC -std=c99 -D_GNU_SOURCE

echo "所有模块编译完成"

# 生成共享库文件
echo ""
echo "2. 生成共享库文件..."

# 生成各个模块的共享库
echo "生成 layer0.so..."
gcc -shared "$MODULES_DIR/layer0_module.o" -o "$BIN_DIR/layer2/layer0.so"

echo "生成 pipeline.so..."
gcc -shared "$MODULES_DIR/pipeline_module.o" -o "$BIN_DIR/layer2/pipeline.so"

echo "生成 compiler.so..."
gcc -shared "$MODULES_DIR/compiler_module.o" -o "$BIN_DIR/layer2/compiler.so"

echo "生成 module.so..."
gcc -shared "$MODULES_DIR/module_module.o" -o "$BIN_DIR/layer2/module.so" -ldl

echo "生成 libc.so..."
gcc -shared "$MODULES_DIR/libc_module.o" -o "$BIN_DIR/layer2/libc.so"

# 为了兼容性，创建.native符号链接
ln -sf layer0.so "$BIN_DIR/layer2/layer0.native"
ln -sf pipeline.so "$BIN_DIR/layer2/pipeline.native"
ln -sf compiler.so "$BIN_DIR/layer2/compiler.native"
ln -sf module.so "$BIN_DIR/layer2/module.native"
ln -sf libc.so "$BIN_DIR/layer2/libc.native"

echo "所有共享库文件生成完成"

# 构建测试程序
echo ""
echo "3. 构建模块系统测试程序..."

cat > "$BIN_DIR/test_module_system.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int main() {
    printf("=== 模块系统测试 ===\n");
    
    // 测试动态库加载
    void* handle = dlopen("./layer2/layer0.native", RTLD_LAZY);
    if (!handle) {
        printf("无法加载模块: %s\n", dlerror());
        return 1;
    }
    
    printf("✅ 模块加载测试通过\n");
    
    dlclose(handle);
    printf("✅ 模块卸载测试通过\n");
    
    printf("=== 模块系统构建完成 ===\n");
    return 0;
}
EOF

gcc "$BIN_DIR/test_module_system.c" -o "$BIN_DIR/test_module_system" -ldl

echo "测试程序已构建: $BIN_DIR/test_module_system"

# 运行测试
echo ""
echo "4. 运行模块系统测试..."
cd "$BIN_DIR"
if ./test_module_system; then
    echo "✅ 模块系统构建和测试成功"
else
    echo "❌ 模块系统测试失败"
    exit 1
fi

# 清理临时文件
rm -f create_native.c create_native test_module_system.c

echo ""
echo "=== 模块系统构建完成 ==="
echo "模块文件位置: $BIN_DIR/layer2/"
echo "可用模块: layer0.native, pipeline.native, compiler.native, module.native, libc.native"
