#!/bin/bash

# TCC交叉编译构建脚本 (修复版)
# 使用两阶段构建解决交叉编译工具程序问题

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$TCC_ROOT/src"
BUILD_DIR="$TCC_ROOT/build"
HOST_BUILD_DIR="$BUILD_DIR/host"

echo "=== TCC交叉编译构建开始 (修复版) ==="
echo "TCC根目录: $TCC_ROOT"
echo "源码目录: $SRC_DIR"
echo "构建目录: $BUILD_DIR"

# 检查源码目录
if [ ! -d "$SRC_DIR" ] || [ ! -f "$SRC_DIR/Makefile" ]; then
    echo "错误: TCC源码目录不存在或不完整"
    exit 1
fi

# 创建构建目录结构
mkdir -p "$BUILD_DIR"/{x86_64,x86_32,arm64,arm32}/{linux,windows,macos}
mkdir -p "$HOST_BUILD_DIR"

# 定义目标架构配置
declare -A TARGETS=(
    # x86_64架构
    ["x86_64-linux"]="CC=gcc ARCH=x86_64"
    ["x86_64-windows"]="CC=x86_64-w64-mingw32-gcc ARCH=x86_64"
    
    # i686架构 (32位x86) - 使用本地编译器但指定32位
    ["i686-linux"]="CC='gcc -m32' ARCH=i386"
    ["i686-windows"]="CC=i686-w64-mingw32-gcc ARCH=i386"
    
    # ARM64架构
    ["aarch64-linux"]="CC=aarch64-linux-gnu-gcc ARCH=arm64"
    
    # ARM32架构  
    ["arm-linux"]="CC=arm-linux-gnueabi-gcc ARCH=arm"
)

# 第一阶段：构建主机版本TCC
build_host_tcc() {
    echo ""
    echo "=== 第一阶段：构建主机版本TCC ==="
    
    if [ -f "$HOST_BUILD_DIR/bin/tcc" ]; then
        echo "主机版本TCC已存在，跳过构建"
        return 0
    fi
    
    # 创建主机构建目录
    rm -rf "$HOST_BUILD_DIR"
    mkdir -p "$HOST_BUILD_DIR"
    
    # 复制源码
    cp -r "$SRC_DIR"/* "$HOST_BUILD_DIR/"
    cd "$HOST_BUILD_DIR"
    
    # 清理
    make distclean 2>/dev/null || true
    
    # 配置和构建主机版本
    echo "配置主机版本TCC..."
    ./configure --prefix="$HOST_BUILD_DIR"
    
    echo "编译主机版本TCC..."
    make
    
    echo "安装主机版本TCC..."
    make install
    
    if [ -f "$HOST_BUILD_DIR/bin/tcc" ]; then
        echo "✓ 主机版本TCC构建成功"
        cd "$TCC_ROOT"
        return 0
    else
        echo "✗ 主机版本TCC构建失败"
        cd "$TCC_ROOT"
        return 1
    fi
}

# 第二阶段：使用主机TCC构建交叉编译版本
build_cross_tcc() {
    local target_name=$1
    local config=$2
    
    echo ""
    echo "=== 构建 TCC for $target_name ==="
    
    # 提取架构和平台
    local arch=$(echo $target_name | cut -d'-' -f1)
    local platform=$(echo $target_name | cut -d'-' -f2)
    
    # 确定输出目录
    local output_dir
    case $arch in
        "x86_64") output_dir="$BUILD_DIR/x86_64/$platform" ;;
        "i686") output_dir="$BUILD_DIR/x86_32/$platform" ;;
        "aarch64") output_dir="$BUILD_DIR/arm64/$platform" ;;
        "arm") output_dir="$BUILD_DIR/arm32/$platform" ;;
    esac
    
    # 为x86_64-linux使用主机版本
    if [ "$target_name" = "x86_64-linux" ]; then
        echo "使用主机版本作为x86_64-linux版本"
        cp -r "$HOST_BUILD_DIR"/* "$output_dir/"
        if [ -f "$output_dir/bin/tcc" ]; then
            mv "$output_dir/bin/tcc" "$output_dir/bin/tcc-$target_name"
            echo "✓ $target_name 构建完成"
            return 0
        fi
    fi
    
    # 创建临时构建目录
    local tmp_build_dir="$BUILD_DIR/tmp_$target_name"
    rm -rf "$tmp_build_dir"
    mkdir -p "$tmp_build_dir"
    
    # 复制源码到临时目录
    echo "复制源码到临时目录..."
    cp -r "$SRC_DIR"/* "$tmp_build_dir/"
    cd "$tmp_build_dir"
    
    # 清理之前的构建
    make distclean 2>/dev/null || true
    
    echo "配置构建环境: $config"
    
    # 特殊处理：修改Makefile以避免运行目标架构程序
    if [[ "$target_name" == *"windows"* ]] || [[ "$target_name" == *"arm"* ]] || [[ "$target_name" == *"aarch64"* ]]; then
        echo "应用交叉编译补丁..."
        
        # 创建一个使用主机工具的Makefile补丁
        cat > cross_compile.patch << 'EOF'
--- Makefile.orig
+++ Makefile
@@ -253,7 +253,7 @@
 	$(CC) -DC2STR $< -o c2str.exe && ./c2str.exe $@.in $@
 
 tccdefs_.h: include/tccdefs.h conftest.c
-	$(CC) -DC2STR conftest.c -o c2str.exe && ./c2str.exe include/tccdefs.h tccdefs_.h
+	gcc -DC2STR conftest.c -o c2str.exe && ./c2str.exe include/tccdefs.h tccdefs_.h
 
 tcc$(EXESUF): libtcc.a tcc.o
 	$(LINK) -o $@ tcc.o $(LIBS)
EOF
        
        # 应用补丁 (如果失败就手动修改)
        cp Makefile Makefile.orig
        sed -i 's/$(CC) -DC2STR conftest.c -o c2str.exe/gcc -DC2STR conftest.c -o c2str.exe/' Makefile
    fi
    
    # 配置构建
    eval "$config ./configure --prefix=$output_dir --disable-static"
    
    # 编译
    echo "开始编译..."
    if eval "$config make"; then
        echo "编译成功"
    else
        echo "编译失败，尝试简化构建..."
        # 如果失败，尝试只构建核心组件
        eval "$config make tcc"
    fi
    
    # 创建输出目录
    mkdir -p "$output_dir/bin"
    
    # 复制可执行文件
    if [ -f "tcc" ]; then
        cp "tcc" "$output_dir/bin/tcc-$target_name"
        echo "生成可执行文件: $output_dir/bin/tcc-$target_name"
    elif [ -f "tcc.exe" ]; then
        cp "tcc.exe" "$output_dir/bin/tcc-$target_name.exe"
        echo "生成可执行文件: $output_dir/bin/tcc-$target_name.exe"
    else
        echo "✗ 未找到可执行文件"
        cd "$TCC_ROOT"
        return 1
    fi
    
    # 清理临时目录
    cd "$TCC_ROOT"
    rm -rf "$tmp_build_dir"
    
    echo "✓ $target_name 构建完成"
    return 0
}

# 检查交叉编译器
check_cross_compilers() {
    echo "=== 检查交叉编译器 ==="
    
    local compilers=(
        "gcc"
        "x86_64-w64-mingw32-gcc" 
        "i686-w64-mingw32-gcc"
        "aarch64-linux-gnu-gcc"
        "arm-linux-gnueabi-gcc"
    )
    
    for compiler in "${compilers[@]}"; do
        if command -v "$compiler" >/dev/null 2>&1; then
            echo "✓ $compiler: $(which $compiler)"
        else
            echo "✗ $compiler: 未找到"
        fi
    done
}

# 生成构建报告
generate_report() {
    echo ""
    echo "=== 构建报告 ==="
    
    local report_file="$BUILD_DIR/build_report.txt"
    echo "TCC交叉编译构建报告 (修复版)" > "$report_file"
    echo "构建时间: $(date)" >> "$report_file"
    echo "" >> "$report_file"
    
    echo "生成的TCC可执行文件:" | tee -a "$report_file"
    
    find "$BUILD_DIR" -name "tcc-*" -type f | while read -r file; do
        if [ -x "$file" ] || [[ "$file" == *.exe ]]; then
            local size=$(ls -lh "$file" | awk '{print $5}')
            echo "  $file (大小: $size)" | tee -a "$report_file"
        fi
    done
    
    echo "" | tee -a "$report_file"
    echo "报告保存至: $report_file"
}

# 主函数
main() {
    # 检查交叉编译器
    check_cross_compilers
    
    # 第一阶段：构建主机版本
    if ! build_host_tcc; then
        echo "✗ 主机版本TCC构建失败，无法继续"
        exit 1
    fi
    
    # 第二阶段：构建交叉编译版本
    echo ""
    echo "=== 第二阶段：构建交叉编译版本 ==="
    
    local success_count=0
    local total_count=${#TARGETS[@]}
    
    for target in "${!TARGETS[@]}"; do
        if build_cross_tcc "$target" "${TARGETS[$target]}"; then
            ((success_count++))
        else
            echo "✗ $target 构建失败"
        fi
    done
    
    echo ""
    echo "=== 构建总结 ==="
    echo "成功: $success_count/$total_count"
    
    # 生成报告
    generate_report
    
    if [ $success_count -gt 0 ]; then
        echo "🎉 成功构建了 $success_count 个TCC版本！"
        return 0
    else
        echo "⚠️ 所有目标构建失败"
        return 1
    fi
}

# 如果直接运行此脚本
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi