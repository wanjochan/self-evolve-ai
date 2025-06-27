#!/bin/bash
# TCC 构建产物重新组织脚本
# 将现有的混乱构建产物重新整理为规范的目录结构

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$TCC_DIR/build"
NEW_BUILD_DIR="$TCC_DIR/build_organized"

echo "🔄 TCC 构建产物重新组织脚本"
echo "=================================================="

# 创建新的目录结构
create_organized_structure() {
    echo "📁 创建新的组织结构..."
    
    # 清理并创建新目录
    rm -rf "$NEW_BUILD_DIR"
    mkdir -p "$NEW_BUILD_DIR"/{host,cross}
    
    # 创建主机目录结构
    mkdir -p "$NEW_BUILD_DIR/host"/{bin,lib,include,share}
    
    echo "  ✅ 新目录结构已创建"
}

# 组织主机编译器
organize_host_compiler() {
    echo "🏠 组织主机编译器..."
    
    local host_source="$BUILD_DIR/host"
    local host_target="$NEW_BUILD_DIR/host"
    
    if [ -d "$host_source" ]; then
        echo "  📋 复制主机编译器..."
        cp -r "$host_source"/* "$host_target"/ 2>/dev/null || true
        echo "  ✅ 主机编译器已组织"
    else
        echo "  ⚠️  未找到主机编译器源目录"
    fi
}

# 组织交叉编译器
organize_cross_compilers() {
    echo "🎯 组织交叉编译器..."
    
    # 查找现有的 TCC 交叉编译器
    local found_compilers=0
    
    # 1. 处理 x86_64/linux 目录结构
    if [ -d "$BUILD_DIR/x86_64/linux" ]; then
        echo "  📦 处理 x86_64-linux..."
        local target_dir="$NEW_BUILD_DIR/cross/x86_64-linux"
        mkdir -p "$target_dir"/{bin,lib,include,share}
        
        # 复制编译器
        if [ -f "$BUILD_DIR/x86_64/linux/bin/tcc-x86_64-linux" ]; then
            cp "$BUILD_DIR/x86_64/linux/bin/tcc-x86_64-linux" "$target_dir/bin/"
            chmod +x "$target_dir/bin/tcc-x86_64-linux"
            found_compilers=$((found_compilers + 1))
            echo "    ✅ x86_64-linux 编译器已组织"
        fi
        
        # 复制库和头文件
        [ -d "$BUILD_DIR/x86_64/linux/lib" ] && cp -r "$BUILD_DIR/x86_64/linux/lib"/* "$target_dir/lib/" 2>/dev/null || true
        [ -d "$BUILD_DIR/x86_64/linux/include" ] && cp -r "$BUILD_DIR/x86_64/linux/include"/* "$target_dir/include/" 2>/dev/null || true
    fi
    
    # 2. 处理 tmp_* 临时目录
    for tmp_dir in "$BUILD_DIR"/tmp_*; do
        if [ -d "$tmp_dir" ]; then
            local arch_name=$(basename "$tmp_dir" | sed 's/tmp_//')
            echo "  📦 处理 $arch_name..."
            
            local target_dir="$NEW_BUILD_DIR/cross/$arch_name"
            mkdir -p "$target_dir"/{bin,lib,include,share}
            
            # 查找 TCC 可执行文件
            local tcc_files=($(find "$tmp_dir" -name "tcc" -type f 2>/dev/null))
            
            if [ ${#tcc_files[@]} -gt 0 ]; then
                cp "${tcc_files[0]}" "$target_dir/bin/tcc-$arch_name"
                chmod +x "$target_dir/bin/tcc-$arch_name"
                found_compilers=$((found_compilers + 1))
                echo "    ✅ $arch_name 编译器已组织"
            else
                echo "    ⚠️  未找到 $arch_name 编译器"
            fi
            
            # 复制库和头文件
            [ -d "$tmp_dir/lib" ] && cp -r "$tmp_dir/lib"/* "$target_dir/lib/" 2>/dev/null || true
            [ -d "$tmp_dir/include" ] && cp -r "$tmp_dir/include"/* "$target_dir/include/" 2>/dev/null || true
        fi
    done
    
    # 3. 查找其他可能的 TCC 文件
    echo "  🔍 查找其他 TCC 文件..."
    
    while IFS= read -r -d '' tcc_file; do
        if [ -x "$tcc_file" ] && [[ "$tcc_file" == *"tcc"* ]]; then
            local relative_path="${tcc_file#$BUILD_DIR/}"
            local dir_parts=($(echo "$relative_path" | tr '/' ' '))
            
            # 尝试推断架构名称
            local arch_name=""
            for part in "${dir_parts[@]}"; do
                if [[ "$part" == *"x86_64"* ]] || [[ "$part" == *"aarch64"* ]] || [[ "$part" == *"arm"* ]] || [[ "$part" == *"i386"* ]]; then
                    arch_name="$part"
                    break
                fi
            done
            
            if [ -n "$arch_name" ] && [ ! -f "$NEW_BUILD_DIR/cross/$arch_name/bin/tcc-$arch_name" ]; then
                echo "    📦 发现额外的编译器: $arch_name"
                local target_dir="$NEW_BUILD_DIR/cross/$arch_name"
                mkdir -p "$target_dir"/{bin,lib,include,share}
                
                cp "$tcc_file" "$target_dir/bin/tcc-$arch_name"
                chmod +x "$target_dir/bin/tcc-$arch_name"
                found_compilers=$((found_compilers + 1))
                echo "      ✅ 已组织"
            fi
        fi
    done < <(find "$BUILD_DIR" -name "*tcc*" -type f -print0 2>/dev/null)
    
    echo "📊 交叉编译器组织完成: $found_compilers 个"
}

# 创建使用说明
create_usage_guide() {
    echo "📖 创建使用说明..."
    
    cat > "$NEW_BUILD_DIR/README.md" << 'EOF'
# TCC 重新组织的构建产物

## 目录结构

```
build_organized/
├── host/                   # 主机版本 TCC
│   ├── bin/tcc            # 主机 TCC 编译器
│   ├── lib/               # 运行时库
│   ├── include/           # 头文件
│   └── share/             # 文档
├── cross/                 # 交叉编译器
│   ├── x86_64-linux/     # x86_64 Linux 目标
│   │   ├── bin/tcc-x86_64-linux
│   │   ├── lib/
│   │   └── include/
│   ├── aarch64-linux/    # ARM64 Linux 目标
│   ├── arm-linux/        # ARM Linux 目标
│   └── ...               # 其他架构
└── README.md             # 本文件
```

## 使用方法

### 编译 x86_64 Linux 程序
```bash
build_organized/cross/x86_64-linux/bin/tcc-x86_64-linux hello.c -o hello
```

### 编译 ARM64 程序
```bash
build_organized/cross/aarch64-linux/bin/tcc-aarch64-linux hello.c -o hello-arm64
```

### 使用主机编译器
```bash
build_organized/host/bin/tcc hello.c -o hello
```

## 验证编译器

运行测试脚本：
```bash
bash scripts/test-tcc-organized.sh
```

## 支持的架构

EOF
    
    # 自动生成架构列表
    for arch_dir in "$NEW_BUILD_DIR/cross"/*; do
        if [ -d "$arch_dir" ]; then
            local arch_name=$(basename "$arch_dir")
            local compiler_path="$arch_dir/bin/tcc-$arch_name"
            if [ -x "$compiler_path" ]; then
                echo "- $arch_name" >> "$NEW_BUILD_DIR/README.md"
            fi
        fi
    done
    
    echo "" >> "$NEW_BUILD_DIR/README.md"
    echo "重新组织时间: $(date)" >> "$NEW_BUILD_DIR/README.md"
    
    echo "  ✅ 使用说明已创建"
}

# 创建测试程序
create_test_program() {
    echo "🧪 创建测试程序..."
    
    cat > "$NEW_BUILD_DIR/test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    printf("Hello from TCC cross-compiler!\n");
    printf("Architecture: %s\n", 
#ifdef __x86_64__
        "x86_64"
#elif defined(__i386__)
        "i386"
#elif defined(__aarch64__)
        "aarch64"
#elif defined(__arm__)
        "arm"
#elif defined(__riscv) && __riscv_xlen == 64
        "riscv64"
#elif defined(__mips64)
        "mips64"
#elif defined(__mips__)
        "mips"
#elif defined(__powerpc64__)
        "powerpc64"
#elif defined(__powerpc__)
        "powerpc"
#elif defined(__s390x__)
        "s390x"
#else
        "unknown"
#endif
    );
    printf("Compiler: TCC\n");
    printf("Arguments: %d\n", argc);
    return 0;
}
EOF
    
    echo "  ✅ 测试程序已创建"
}

# 验证组织结果
verify_organization() {
    echo "✅ 验证组织结果..."
    
    local host_count=0
    local cross_count=0
    
    # 检查主机编译器
    if [ -x "$NEW_BUILD_DIR/host/bin/tcc" ]; then
        host_count=1
        echo "  ✅ 主机编译器: 可用"
    else
        echo "  ❌ 主机编译器: 不可用"
    fi
    
    # 检查交叉编译器
    for compiler in "$NEW_BUILD_DIR"/cross/*/bin/tcc-*; do
        if [ -x "$compiler" ]; then
            cross_count=$((cross_count + 1))
            local arch_name=$(basename "$(dirname "$(dirname "$compiler")")")
            echo "  ✅ 交叉编译器: $arch_name"
        fi
    done
    
    echo "📊 组织结果统计:"
    echo "  主机编译器: $host_count"
    echo "  交叉编译器: $cross_count"
    
    if [ $cross_count -gt 0 ] || [ $host_count -gt 0 ]; then
        echo "  🎉 重新组织成功！"
    else
        echo "  ❌ 没有找到可用的编译器"
        return 1
    fi
}

# 替换旧目录
replace_old_build() {
    echo "🔄 替换旧的构建目录..."
    
    # 备份旧目录
    if [ -d "$BUILD_DIR" ]; then
        local backup_dir="${BUILD_DIR}_backup_$(date +%Y%m%d_%H%M%S)"
        echo "  📦 备份旧目录到: $backup_dir"
        mv "$BUILD_DIR" "$backup_dir"
    fi
    
    # 移动新目录
    echo "  🔄 启用新的组织结构..."
    mv "$NEW_BUILD_DIR" "$BUILD_DIR"
    
    echo "  ✅ 目录替换完成"
}

# 主流程
main() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "❌ 构建目录不存在: $BUILD_DIR"
        exit 1
    fi
    
    echo "📁 源目录: $BUILD_DIR"
    echo "📁 目标目录: $NEW_BUILD_DIR"
    echo ""
    
    # 1. 创建新结构
    create_organized_structure
    
    # 2. 组织主机编译器
    organize_host_compiler
    
    # 3. 组织交叉编译器
    organize_cross_compilers
    
    # 4. 创建文档和测试程序
    create_usage_guide
    create_test_program
    
    # 5. 验证结果
    if verify_organization; then
        echo ""
        echo "🤔 是否替换旧的构建目录？ (y/N)"
        read -r response
        if [[ "$response" =~ ^[Yy]$ ]]; then
            replace_old_build
            echo ""
            echo "🎉 TCC 构建产物重新组织完成！"
            echo "📁 新的构建目录: $BUILD_DIR"
            echo "📖 使用说明: $BUILD_DIR/README.md"
            echo "🧪 测试脚本: bash scripts/test-tcc-organized.sh"
        else
            echo ""
            echo "✅ 重新组织完成，新目录: $NEW_BUILD_DIR"
            echo "📖 使用说明: $NEW_BUILD_DIR/README.md"
            echo ""
            echo "如需替换旧目录，请手动运行："
            echo "  mv $BUILD_DIR ${BUILD_DIR}_backup"
            echo "  mv $NEW_BUILD_DIR $BUILD_DIR"
        fi
    else
        echo "❌ 重新组织失败"
        exit 1
    fi
}

# 运行主流程
main "$@"