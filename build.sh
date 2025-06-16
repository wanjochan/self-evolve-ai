#!/bin/bash

# 自举编译器构建脚本
# 自动化编译和测试过程

# 配置
ASM=nasm
LD=ld
CC=gcc
CFLAGS="-m32 -nostdlib -fno-builtin -fno-stack-protector"
LDFLAGS="-m elf_i386"

# 清理之前的构建
clean() {
    echo "Cleaning build files..."
    rm -f *.o *.bin *.img
}

# 汇编源文件
assemble() {
    echo "Assembling $1..."
    $ASM -f elf32 -o ${1%.*}.o $1
    if [ $? -ne 0 ]; then
        echo "Assembly failed: $1"
        exit 1
    fi
}

# 链接目标文件
link() {
    echo "Linking $@..."
    $LD $LDFLAGS -o $1 ${@:2}
    if [ $? -ne 0 ]; then
        echo "Linking failed"
        exit 1
    fi
}

# 构建引导加载程序
build_bootloader() {
    echo "Building bootloader..."
    $ASM -f bin -o boot.bin boot.asm
    if [ $? -ne 0 ]; then
        echo "Bootloader assembly failed"
        exit 1
    fi
}

# 构建编译器
build_compiler() {
    # 1. 汇编所有源文件
    for src in *.tasm; do
        assemble "$src"
    done
    
    # 2. 链接目标文件
    link compiler bootstrapper.o vm_core.o tir_generator.o tir_optimizer.o bootstrap_compiler.o
    
    # 3. 生成可启动镜像
    if [ -f boot.bin ]; then
        dd if=/dev/zero of=compiler.img bs=512 count=2880
        dd if=boot.bin of=compiler.img conv=notrunc
        mcopy -i compiler.img compiler ::/
    fi
}

# 运行测试
run_tests() {
    echo "Running tests..."
    
    # 1. 单元测试
    for test in tests/*_test.tasm; do
        echo "Testing $test..."
        $ASM -f elf32 -o test.o "$test"
        $LD $LDFLAGS -o test test.o
        ./test
        if [ $? -ne 0 ]; then
            echo "Test failed: $test"
            exit 1
        fi
    done
    
    # 2. 集成测试
    if [ -f compiler ]; then
        echo "Running integration tests..."
        ./compiler --test
    fi
}

# 主构建流程
main() {
    case "$1" in
        clean)
            clean
            ;;
        bootloader)
            build_bootloader
            ;;
        test)
            run_tests
            ;;
        *)
            clean
            build_compiler
            run_tests
            ;;
    esac
}

# 执行主函数
main "$@"
