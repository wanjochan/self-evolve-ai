#!/bin/bash

# TCC 构建整合脚本
# 整合所有交叉编译的 TCC 可执行文件到一个统一的目录结构

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
BUILD_DIR="$TCC_ROOT/build"
CROSS_DIR="$BUILD_DIR/cross"
OUTPUT_DIR="$TCC_ROOT/dist"

echo "=== TCC 构建整合开始 ==="
echo "TCC根目录: $TCC_ROOT"
echo "构建目录: $BUILD_DIR"
echo "输出目录: $OUTPUT_DIR"

# 清理输出目录
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR/bin"
mkdir -p "$OUTPUT_DIR/lib"
mkdir -p "$OUTPUT_DIR/include"

# 查找所有 TCC 可执行文件
find_tcc_executables() {
    echo "查找所有 TCC 可执行文件..."
    
    # 查找所有 tcc-* 可执行文件
    find "$BUILD_DIR" -name "tcc-*" -type f | while read -r file; do
        if [ -x "$file" ] || [[ "$file" == *.exe ]]; then
            local size=$(ls -lh "$file" | awk '{print $5}')
            local name=$(basename "$file")
            echo "找到: $file (大小: $size)"
            
            # 复制到输出目录
            cp "$file" "$OUTPUT_DIR/bin/"
            echo "  ✓ 已复制到 $OUTPUT_DIR/bin/$name"
        fi
    done
    
    # 查找 macOS 原生版本
    if [ -f "$BUILD_DIR/arm64/macos/bin/tcc-macos-arm64" ]; then
        echo "找到 macOS ARM64 版本"
        cp "$BUILD_DIR/arm64/macos/bin/tcc-macos-arm64" "$OUTPUT_DIR/bin/"
    fi
    
    # 查找 x86_64 Linux 版本
    if [ -f "$BUILD_DIR/host/bin/tcc" ]; then
        echo "找到主机版本 TCC"
        cp "$BUILD_DIR/host/bin/tcc" "$OUTPUT_DIR/bin/tcc-host"
    fi
}

# 复制库文件和头文件
copy_libs_and_headers() {
    echo "复制库文件和头文件..."
    
    # 复制 macOS ARM64 的库文件
    if [ -d "$BUILD_DIR/arm64/macos/lib" ]; then
        mkdir -p "$OUTPUT_DIR/lib/macos-arm64"
        if ! run_with_timeout "cp -r \"$BUILD_DIR/arm64/macos/lib/\"* \"$OUTPUT_DIR/lib/macos-arm64/\"" 60 "复制 macOS ARM64 库文件 (最多等待 60 秒)..."; then
            echo "⚠️ 复制 macOS ARM64 库文件超时，跳过"
        else
            echo "  ✓ 已复制 macOS ARM64 库文件"
        fi
    fi
    
    # 复制 macOS ARM64 的头文件
    if [ -d "$BUILD_DIR/arm64/macos/include" ]; then
        mkdir -p "$OUTPUT_DIR/include/macos-arm64"
        if ! run_with_timeout "cp -r \"$BUILD_DIR/arm64/macos/include/\"* \"$OUTPUT_DIR/include/macos-arm64/\"" 60 "复制 macOS ARM64 头文件 (最多等待 60 秒)..."; then
            echo "⚠️ 复制 macOS ARM64 头文件超时，跳过"
        else
            echo "  ✓ 已复制 macOS ARM64 头文件"
        fi
    fi
    
    # 复制 x86_64 Linux 的库文件
    if [ -d "$BUILD_DIR/host/lib" ]; then
        mkdir -p "$OUTPUT_DIR/lib/host"
        if ! run_with_timeout "cp -r \"$BUILD_DIR/host/lib/\"* \"$OUTPUT_DIR/lib/host/\"" 60 "复制主机版本库文件 (最多等待 60 秒)..."; then
            echo "⚠️ 复制主机版本库文件超时，跳过"
        else
            echo "  ✓ 已复制主机版本库文件"
        fi
    fi
    
    # 复制 x86_64 Linux 的头文件
    if [ -d "$BUILD_DIR/host/include" ]; then
        mkdir -p "$OUTPUT_DIR/include/host"
        if ! run_with_timeout "cp -r \"$BUILD_DIR/host/include/\"* \"$OUTPUT_DIR/include/host/\"" 60 "复制主机版本头文件 (最多等待 60 秒)..."; then
            echo "⚠️ 复制主机版本头文件超时，跳过"
        else
            echo "  ✓ 已复制主机版本头文件"
        fi
    fi
}

# 创建便捷脚本
create_convenience_scripts() {
    echo "创建便捷脚本..."
    
    # 创建 tcc-macos-arm64 便捷脚本
    if [ -f "$OUTPUT_DIR/bin/tcc-macos-arm64" ]; then
        cat > "$OUTPUT_DIR/bin/tcc-macos" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$SCRIPT_DIR/tcc-macos-arm64" "$@"
EOF
        chmod +x "$OUTPUT_DIR/bin/tcc-macos"
        echo "  ✓ 已创建 tcc-macos 便捷脚本"
    fi
    
    # 创建 tcc-linux-x86_64 便捷脚本
    if [ -f "$OUTPUT_DIR/bin/tcc-x86_64-linux" ]; then
        cat > "$OUTPUT_DIR/bin/tcc-linux" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$SCRIPT_DIR/tcc-x86_64-linux" "$@"
EOF
        chmod +x "$OUTPUT_DIR/bin/tcc-linux"
        echo "  ✓ 已创建 tcc-linux 便捷脚本"
    fi
    
    # 创建默认 tcc 脚本，根据系统架构自动选择
    cat > "$OUTPUT_DIR/bin/tcc" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 检测系统类型和架构
OS=$(uname -s)
ARCH=$(uname -m)

if [ "$OS" = "Darwin" ]; then
    if [ "$ARCH" = "arm64" ]; then
        # macOS ARM64
        if [ -f "$SCRIPT_DIR/tcc-macos-arm64" ]; then
            exec "$SCRIPT_DIR/tcc-macos-arm64" "$@"
        fi
    elif [ "$ARCH" = "x86_64" ]; then
        # macOS x86_64
        if [ -f "$SCRIPT_DIR/tcc-macos-x86_64" ]; then
            exec "$SCRIPT_DIR/tcc-macos-x86_64" "$@"
        fi
    fi
elif [ "$OS" = "Linux" ]; then
    if [ "$ARCH" = "x86_64" ]; then
        # Linux x86_64
        if [ -f "$SCRIPT_DIR/tcc-x86_64-linux" ]; then
            exec "$SCRIPT_DIR/tcc-x86_64-linux" "$@"
        fi
    elif [ "$ARCH" = "aarch64" ]; then
        # Linux ARM64
        if [ -f "$SCRIPT_DIR/tcc-aarch64-linux" ]; then
            exec "$SCRIPT_DIR/tcc-aarch64-linux" "$@"
        fi
    fi
fi

# 如果没有找到匹配的版本，尝试使用主机版本
if [ -f "$SCRIPT_DIR/tcc-host" ]; then
    exec "$SCRIPT_DIR/tcc-host" "$@"
fi

echo "错误: 没有找到适合当前系统 ($OS $ARCH) 的 TCC 版本"
exit 1
EOF
    chmod +x "$OUTPUT_DIR/bin/tcc"
    echo "  ✓ 已创建智能 tcc 脚本"
}

# 生成报告
generate_report() {
    echo ""
    echo "=== 整合报告 ==="
    
    local report_file="$OUTPUT_DIR/tcc_builds_report.txt"
    echo "TCC 构建整合报告" > "$report_file"
    echo "生成时间: $(date)" >> "$report_file"
    echo "系统信息: $(uname -a)" >> "$report_file"
    echo "" >> "$report_file"
    
    echo "可用的 TCC 版本:" | tee -a "$report_file"
    
    # 使用兼容 macOS 的方式查找可执行文件
    find "$OUTPUT_DIR/bin" -type f | while read -r file; do
        if [ -x "$file" ]; then
            local size=$(ls -lh "$file" | awk '{print $5}')
            local name=$(basename "$file")
            local type=$(file "$file" | cut -d':' -f2-)
            echo "  $name (大小: $size): $type" | tee -a "$report_file"
        fi
    done
    
    echo "" | tee -a "$report_file"
    echo "报告保存至: $report_file"
}

# 主函数
main() {
    # 设置整体超时（10分钟）
    SECONDS=0
    MAX_SECONDS=$((10 * 60))
    
    # 捕获中断信号
    trap 'echo "⚠️ 脚本被中断"; exit 1' INT TERM
    
    # 查找和复制可执行文件
    find_tcc_executables
    
    # 检查整体超时
    if [ $SECONDS -gt $MAX_SECONDS ]; then
        echo "⚠️ 脚本执行超时 (${MAX_SECONDS}秒)"
        generate_report
        exit 1
    fi
    
    # 复制库文件和头文件
    copy_libs_and_headers
    
    # 检查整体超时
    if [ $SECONDS -gt $MAX_SECONDS ]; then
        echo "⚠️ 脚本执行超时 (${MAX_SECONDS}秒)"
        generate_report
        exit 1
    fi
    
    # 创建便捷脚本
    create_convenience_scripts
    
    # 生成报告
    generate_report
    
    echo ""
    echo "🎉 TCC 构建整合完成！"
    echo "所有文件已整合到: $OUTPUT_DIR"
    echo ""
    echo "使用方法:"
    echo "  $OUTPUT_DIR/bin/tcc            # 自动选择适合当前系统的版本"
    echo "  $OUTPUT_DIR/bin/tcc-macos      # 使用 macOS 版本"
    echo "  $OUTPUT_DIR/bin/tcc-linux      # 使用 Linux 版本"
    echo ""
    echo "将 $OUTPUT_DIR/bin 添加到 PATH 环境变量以便全局使用"
    echo "总耗时: $SECONDS 秒"
}

# 运行主函数
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi 