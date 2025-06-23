#!/bin/bash
# 编译和运行C2ASTC测试程序

# 设置TCC路径，如果没有找到，尝试使用系统的TCC
TCC_PATH="../../tcc/tcc"
if [ ! -f "$TCC_PATH" ]; then
    TCC_PATH=$(which tcc)
    if [ -z "$TCC_PATH" ]; then
        echo "Error: TCC compiler not found"
        exit 1
    fi
fi

# 编译所有测试
echo "Compiling all tests..."

# 编译run_tests.c
echo "Compiling run_tests.c..."
$TCC_PATH -I../../. -o run_tests run_tests.c ../../c2astc.c
if [ $? -ne 0 ]; then
    echo "Compilation of run_tests.c failed!"
    exit 1
fi

# 编译test1.c
echo "Compiling test1.c..."
$TCC_PATH -I../../. -o test1 test1.c ../../c2astc.c
if [ $? -ne 0 ]; then
    echo "Compilation of test1.c failed!"
    exit 1
fi

# 编译test2.c
echo "Compiling test2.c..."
$TCC_PATH -I../../. -o test2 test2.c ../../c2astc.c
if [ $? -ne 0 ]; then
    echo "Compilation of test2.c failed!"
    exit 1
fi

# 编译test3.c
echo "Compiling test3.c..."
$TCC_PATH -I../../. -o test3 test3.c ../../c2astc.c
if [ $? -ne 0 ]; then
    echo "Compilation of test3.c failed!"
    exit 1
fi

# 编译test4.c
echo "Compiling test4.c..."
$TCC_PATH -I../../. -o test4 test4.c ../../c2astc.c
if [ $? -ne 0 ]; then
    echo "Compilation of test4.c failed!"
    exit 1
fi

echo "All tests compiled successfully!"

# 运行测试
echo ""
echo "Running tests..."
echo ""

# 运行主测试程序
echo "Running main test suite..."
./run_tests test1.c test2.c test3.c test4.c
if [ $? -ne 0 ]; then
    echo "Main test suite failed!"
    exit 1
fi

# 运行单独测试
echo ""
echo "Running individual tests..."
echo ""

echo "Running test1..."
./test1
if [ $? -ne 0 ]; then
    echo "Test1 failed!"
    exit 1
fi

echo ""
echo "Running test2..."
./test2
if [ $? -ne 0 ]; then
    echo "Test2 failed!"
    exit 1
fi

echo ""
echo "Running test3..."
./test3
if [ $? -ne 0 ]; then
    echo "Test3 failed!"
    exit 1
fi

echo ""
echo "Running test4..."
./test4
if [ $? -ne 0 ]; then
    echo "Test4 failed!"
    exit 1
fi

echo ""
echo "All tests passed successfully!"
exit 0 