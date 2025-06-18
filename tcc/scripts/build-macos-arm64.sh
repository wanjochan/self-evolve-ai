#!/bin/bash

# TCC macOS ARM64 构建脚本
# 在 macOS ARM64 (Apple Silicon) 上本地构建 TCC

set -e

# 超时处理函数
timeout_handler() {
    echo "⚠️ 操作超时，强制终止"
    kill -9 $1 >/dev/null 2>&1 || true
    return 1
}

# 带超时的命令执行函数
run_with_timeout() {
    local cmd="$1"
    local timeout_seconds="$2"
    local message="$3"
    
    echo "$message"
    
    # 启动命令并获取PID
    eval "$cmd" &
    local cmd_pid=$!
    
    # 监控超时
    local count=0
    while kill -0 $cmd_pid 2>/dev/null; do
        if [ $count -ge $timeout_seconds ]; then
            echo "⚠️ 命令执行超时 ($timeout_seconds 秒): $cmd"
            timeout_handler $cmd_pid
            return 1
        fi
        sleep 1
        ((count++))
        
        # 每30秒显示一次进度
        if [ $((count % 30)) -eq 0 ]; then
            echo "... 仍在执行 ($count 秒)"
        fi
    done
    
    # 等待进程结束并获取退出状态
    wait $cmd_pid
    return $?
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$TCC_ROOT/src"
BUILD_DIR="$TCC_ROOT/build"
MACOS_BUILD_DIR="$BUILD_DIR/macos-native"
MACOS_ARM64_DIR="$BUILD_DIR/arm64/macos"

echo "=== TCC macOS ARM64 构建开始 ==="
echo "TCC根目录: $TCC_ROOT"
echo "源码目录: $SRC_DIR"
echo "构建目录: $BUILD_DIR"
echo "macOS构建目录: $MACOS_BUILD_DIR"

# 检查系统架构
ARCH=$(uname -m)
if [ "$ARCH" != "arm64" ]; then
    echo "警告: 当前系统架构不是 arm64，而是 $ARCH"
    echo "这个脚本设计用于 Apple Silicon (M1/M2/M3) 机器"
    read -p "是否继续? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# 检查源码目录
if [ ! -d "$SRC_DIR" ] || [ ! -f "$SRC_DIR/Makefile" ]; then
    echo "错误: TCC源码目录不存在或不完整"
    exit 1
fi

# 创建构建目录
mkdir -p "$MACOS_BUILD_DIR"
mkdir -p "$MACOS_ARM64_DIR/bin"

# 构建 macOS 原生版本
build_macos_native() {
    echo ""
    echo "=== 构建 macOS 原生版本 TCC ==="
    
    # 创建构建目录
    rm -rf "$MACOS_BUILD_DIR"
    mkdir -p "$MACOS_BUILD_DIR"
    
    # 复制源码
    echo "复制源码到构建目录..."
    cp -r "$SRC_DIR"/* "$MACOS_BUILD_DIR/"
    cd "$MACOS_BUILD_DIR"
    
    # 清理
    echo "清理之前的构建..."
    make distclean 2>/dev/null || true
    
    # 配置和构建
    echo "配置 macOS 原生版本 TCC..."
    if ! run_with_timeout "./configure --prefix=\"$MACOS_BUILD_DIR\" --cpu=arm64" 120 "运行配置脚本 (最多等待 120 秒)..."; then
        echo "✗ 配置失败"
        cd "$TCC_ROOT"
        return 1
    fi
    
    echo "编译 macOS 原生版本 TCC..."
    if ! run_with_timeout "make -j$(sysctl -n hw.ncpu)" 300 "编译中 (最多等待 300 秒)..."; then
        echo "✗ 编译失败"
        cd "$TCC_ROOT"
        return 1
    fi
    
    echo "安装 macOS 原生版本 TCC..."
    if ! run_with_timeout "make install" 120 "安装中 (最多等待 120 秒)..."; then
        echo "✗ 安装失败"
        cd "$TCC_ROOT"
        return 1
    fi
    
    # 检查是否构建成功
    if [ -f "$MACOS_BUILD_DIR/tcc" ]; then
        echo "✓ macOS 原生版本 TCC 构建成功"
        
        # 复制到输出目录
        cp "$MACOS_BUILD_DIR/tcc" "$MACOS_ARM64_DIR/bin/tcc-macos-arm64"
        
        # 创建符号链接
        ln -sf "$MACOS_ARM64_DIR/bin/tcc-macos-arm64" "$MACOS_ARM64_DIR/bin/tcc"
        
        echo "✓ 可执行文件已复制到: $MACOS_ARM64_DIR/bin/tcc-macos-arm64"
        
        # 复制库文件和头文件
        mkdir -p "$MACOS_ARM64_DIR/lib"
        mkdir -p "$MACOS_ARM64_DIR/include"
        
        if [ -d "$MACOS_BUILD_DIR/lib" ]; then
            cp -r "$MACOS_BUILD_DIR/lib/"* "$MACOS_ARM64_DIR/lib/"
            echo "✓ 库文件已复制"
        fi
        
        if [ -d "$MACOS_BUILD_DIR/include" ]; then
            cp -r "$MACOS_BUILD_DIR/include/"* "$MACOS_ARM64_DIR/include/"
            echo "✓ 头文件已复制"
        fi
        
        cd "$TCC_ROOT"
        return 0
    else
        echo "✗ macOS 原生版本 TCC 构建失败"
        cd "$TCC_ROOT"
        return 1
    fi
}

# 测试构建的 TCC
test_tcc() {
    echo ""
    echo "=== 测试 macOS ARM64 TCC ==="
    
    local tcc_path="$MACOS_ARM64_DIR/bin/tcc-macos-arm64"
    
    if [ ! -f "$tcc_path" ]; then
        echo "✗ TCC 可执行文件不存在"
        return 1
    fi
    
    # 显示版本信息
    echo "TCC 版本信息:"
    "$tcc_path" -v
    
    # 创建测试程序
    mkdir -p "$BUILD_DIR/tests"
    cat > "$BUILD_DIR/tests/hello.c" << 'EOF'
#include <stdio.h>
int main() {
    printf("Hello from TCC on macOS ARM64!\n");
    #ifdef __APPLE__
    printf("Compiled on macOS\n");
    #endif
    #ifdef __aarch64__
    printf("Running on ARM64 architecture\n");
    #endif
    return 0;
}
EOF
    
    # 编译测试程序
    echo "编译测试程序..."
    "$tcc_path" -o "$BUILD_DIR/tests/hello" "$BUILD_DIR/tests/hello.c"
    
    # 运行测试程序
    if [ -f "$BUILD_DIR/tests/hello" ]; then
        echo "运行测试程序:"
        "$BUILD_DIR/tests/hello"
        
        # 检查文件类型
        echo "检查可执行文件类型:"
        file "$BUILD_DIR/tests/hello"
        
        return 0
    else
        echo "✗ 测试程序编译失败"
        return 1
    fi
}

# 生成构建报告
generate_report() {
    echo ""
    echo "=== 构建报告 ==="
    
    local report_file="$BUILD_DIR/macos_arm64_report.txt"
    echo "TCC macOS ARM64 构建报告" > "$report_file"
    echo "构建时间: $(date)" >> "$report_file"
    echo "系统信息: $(uname -a)" >> "$report_file"
    echo "" >> "$report_file"
    
    if [ -f "$MACOS_ARM64_DIR/bin/tcc-macos-arm64" ]; then
        local size=$(ls -lh "$MACOS_ARM64_DIR/bin/tcc-macos-arm64" | awk '{print $5}')
        echo "TCC macOS ARM64 可执行文件: $MACOS_ARM64_DIR/bin/tcc-macos-arm64 (大小: $size)" | tee -a "$report_file"
        echo "文件类型: $(file "$MACOS_ARM64_DIR/bin/tcc-macos-arm64")" | tee -a "$report_file"
    else
        echo "✗ 未找到 TCC macOS ARM64 可执行文件" | tee -a "$report_file"
    fi
    
    echo "" | tee -a "$report_file"
    echo "报告保存至: $report_file"
}

# 主函数
main() {
    # 设置整体超时（30分钟）
    SECONDS=0
    MAX_SECONDS=$((30 * 60))
    
    # 捕获中断信号
    trap 'echo "⚠️ 脚本被中断"; exit 1' INT TERM
    
    # 构建 macOS 原生版本
    if build_macos_native; then
        echo "✓ macOS ARM64 TCC 构建成功"
    else
        echo "✗ macOS ARM64 TCC 构建失败"
        exit 1
    fi
    
    # 检查整体超时
    if [ $SECONDS -gt $MAX_SECONDS ]; then
        echo "⚠️ 脚本执行超时 (${MAX_SECONDS}秒)"
        generate_report
        exit 1
    fi
    
    # 测试构建的 TCC
    if test_tcc; then
        echo "✓ macOS ARM64 TCC 测试通过"
    else
        echo "⚠️ macOS ARM64 TCC 测试失败"
    fi
    
    # 生成报告
    generate_report
    
    echo ""
    echo "🎉 TCC macOS ARM64 构建完成！"
    echo "可执行文件位置: $MACOS_ARM64_DIR/bin/tcc-macos-arm64"
    echo "总耗时: $SECONDS 秒"
}

# 运行主函数
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi 