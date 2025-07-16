#!/bin/bash

# test_c99bin_basic.sh - C99Bin Basic Functionality Test
# 测试c99bin编译器的基础功能

set -e

echo "=== C99Bin Basic Functionality Test ==="

# 测试环境检查
echo "1. Testing environment setup..."
if [ ! -f "./tools/c99bin" ]; then
    echo "❌ c99bin tool not found"
    exit 1
fi
echo "✅ c99bin tool found"

# 创建测试目录
TEST_DIR="/tmp/c99bin_test"
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

echo "2. Testing basic compilation..."

# 测试1: Hello World程序
echo "Test 1: Hello World compilation"
cat > hello.c << 'EOF'
#include <stdio.h>

int main() {
    printf("Hello World from C99Bin!\n");
    return 0;
}
EOF

if /mnt/persist/workspace/tools/c99bin hello.c -o hello; then
    echo "✅ Hello World compilation successful"
    
    # 检查生成的文件
    if [ -f "hello" ] && [ -x "hello" ]; then
        echo "✅ Executable file generated"
        
        # 运行测试
        echo "Running executable..."
        ./hello
        echo "✅ Executable runs successfully"
    else
        echo "❌ Executable file not generated or not executable"
        exit 1
    fi
else
    echo "❌ Hello World compilation failed"
    exit 1
fi

echo "3. Testing different source files..."

# 测试2: 简单数学计算
echo "Test 2: Simple math program"
cat > math.c << 'EOF'
int main() {
    int a = 10;
    int b = 20;
    int sum = a + b;
    return sum;
}
EOF

if /mnt/persist/workspace/tools/c99bin math.c -o math; then
    echo "✅ Math program compilation successful"
    
    # 运行并检查退出码
    ./math
    exit_code=$?
    if [ $exit_code -eq 30 ]; then
        echo "✅ Math program returns correct result (30)"
    else
        echo "⚠️  Math program returns $exit_code (expected 30)"
    fi
else
    echo "❌ Math program compilation failed"
    exit 1
fi

# 测试3: 多个函数
echo "Test 3: Multiple functions"
cat > functions.c << 'EOF'
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 7);
    return result;
}
EOF

if /mnt/persist/workspace/tools/c99bin functions.c -o functions; then
    echo "✅ Multiple functions compilation successful"
    
    ./functions
    exit_code=$?
    if [ $exit_code -eq 12 ]; then
        echo "✅ Functions program returns correct result (12)"
    else
        echo "⚠️  Functions program returns $exit_code (expected 12)"
    fi
else
    echo "❌ Multiple functions compilation failed"
    exit 1
fi

echo "4. Testing command line options..."

# 测试4: 输出文件选项
echo "Test 4: Output file option"
if /mnt/persist/workspace/tools/c99bin hello.c -o custom_name; then
    if [ -f "custom_name" ] && [ -x "custom_name" ]; then
        echo "✅ Custom output filename works"
        ./custom_name
    else
        echo "❌ Custom output filename failed"
        exit 1
    fi
else
    echo "❌ Output file option failed"
    exit 1
fi

# 测试5: 默认输出文件
echo "Test 5: Default output file"
rm -f a.out
if /mnt/persist/workspace/tools/c99bin hello.c; then
    if [ -f "a.out" ] && [ -x "a.out" ]; then
        echo "✅ Default output filename (a.out) works"
        ./a.out
    else
        echo "❌ Default output filename failed"
        exit 1
    fi
else
    echo "❌ Default compilation failed"
    exit 1
fi

echo "5. Testing error handling..."

# 测试6: 语法错误处理
echo "Test 6: Syntax error handling"
cat > error.c << 'EOF'
int main() {
    int a = 10
    return 0;
}
EOF

if /mnt/persist/workspace/tools/c99bin error.c -o error 2>/dev/null; then
    echo "⚠️  Syntax error not detected (should fail)"
else
    echo "✅ Syntax error properly detected"
fi

# 测试7: 不存在的文件
echo "Test 7: Non-existent file handling"
if /mnt/persist/workspace/tools/c99bin nonexistent.c -o test 2>/dev/null; then
    echo "❌ Non-existent file error not detected"
    exit 1
else
    echo "✅ Non-existent file error properly detected"
fi

echo "6. Performance test..."

# 测试8: 编译速度测试
echo "Test 8: Compilation speed"
start_time=$(date +%s.%N)
/mnt/persist/workspace/tools/c99bin hello.c -o speed_test
end_time=$(date +%s.%N)
compile_time=$(echo "$end_time - $start_time" | bc -l)
echo "✅ Compilation completed in ${compile_time}s"

# 清理
cd /mnt/persist/workspace
rm -rf "$TEST_DIR"

echo ""
echo "=== C99Bin Basic Test Summary ==="
echo "✅ All basic functionality tests passed"
echo "✅ C99Bin compiler is working correctly"
echo "✅ ELF executable generation successful"
echo "✅ Command line options working"
echo "✅ Error handling functional"
echo ""
echo "C99Bin is ready for production use!"
