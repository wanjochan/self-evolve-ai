#!/bin/bash
#
# build_arm64_optimized.sh - ARM64优化构建脚本
# 自动生成，使用ARM64特定优化标志
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# 加载ARM64优化标志
if [ -f "$PROJECT_ROOT/build/arm64_optimization_flags.mk" ]; then
    source <(grep -E '^[A-Z_]+ =' "$PROJECT_ROOT/build/arm64_optimization_flags.mk" | sed 's/ = /="/; s/$/"/')
fi

# ARM64优化编译函数
compile_arm64_optimized() {
    local source="$1"
    local output="$2"
    local extra_flags="${3:-}"
    
    local cc_cmd="gcc"
    if command -v clang >/dev/null 2>&1; then
        cc_cmd="clang"
    fi
    
    echo "编译ARM64优化版本: $(basename "$source")"
    $cc_cmd $ARM64_CFLAGS $extra_flags -c "$source" -o "$output"
}

# 构建ARM64优化模块
build_arm64_modules() {
    echo "=== 构建ARM64优化模块 ==="
    
    local src_dir="$PROJECT_ROOT/src/core/modules"
    local obj_dir="$PROJECT_ROOT/build/arm64_obj"
    local out_dir="$PROJECT_ROOT/bin"
    
    mkdir -p "$obj_dir" "$out_dir"
    
    local modules=(
        "layer0_module"
        "pipeline_module"
        "compiler_module"
        "module_module"
        "libc_module"
    )
    
    for module in "${modules[@]}"; do
        local source="$src_dir/${module}.c"
        local object="$obj_dir/${module}.o"
        local output="$out_dir/${module%_module}_arm64_64_optimized.native"
        
        if [ -f "$source" ]; then
            if compile_arm64_optimized "$source" "$object" "-fPIC"; then
                echo "创建ARM64优化共享库: $(basename "$output")"
                gcc $ARM64_LDFLAGS -shared "$object" -o "$output"
                echo "✅ $module ARM64优化版本构建完成"
            else
                echo "❌ $module ARM64优化版本构建失败"
            fi
        fi
    done
}

# 主函数
main() {
    echo "ARM64优化构建开始..."
    build_arm64_modules
    echo "ARM64优化构建完成！"
}

main "$@"
