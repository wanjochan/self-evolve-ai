#!/bin/bash

# TCC 交叉编译环境设置脚本
# 从 macOS ARM64 环境交叉编译到 Linux 和 Windows

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="$(dirname "$SCRIPT_DIR")"
DIST_DIR="$TCC_ROOT/dist"
CROSS_DIR="$TCC_ROOT/cross"
TCC_BIN="$DIST_DIR/bin/tcc-macos-arm64"

echo "=== TCC 交叉编译环境设置 ==="
echo "TCC根目录: $TCC_ROOT"
echo "TCC可执行文件: $TCC_BIN"
echo "交叉编译目录: $CROSS_DIR"

# 检查 TCC 是否存在
if [ ! -f "$TCC_BIN" ]; then
    echo "错误: TCC 可执行文件不存在，请先构建 macOS ARM64 版本的 TCC"
    exit 1
fi

# 创建交叉编译目录结构
mkdir -p "$CROSS_DIR"/{linux,windows}/{x86_64,i386}/{include,lib}

# 下载必要的头文件
download_headers() {
    echo "=== 下载必要的头文件 ==="
    
    # 创建临时目录
    TEMP_DIR="$CROSS_DIR/temp"
    mkdir -p "$TEMP_DIR"
    
    # 下载 Linux 头文件
    echo "下载 Linux 头文件..."
    curl -L -o "$TEMP_DIR/linux-headers.tar.gz" "https://mirrors.edge.kernel.org/pub/linux/kernel/v5.x/linux-headers-5.15.tar.gz" || {
        echo "注意: 无法下载 Linux 头文件，将使用本地提供的最小头文件集"
    }
    
    # 下载 MinGW 头文件 (Windows)
    echo "下载 MinGW 头文件..."
    curl -L -o "$TEMP_DIR/mingw-headers.tar.gz" "https://sourceforge.net/projects/mingw-w64/files/mingw-w64/mingw-w64-release/mingw-w64-v9.0.0.tar.bz2" || {
        echo "注意: 无法下载 MinGW 头文件，将使用本地提供的最小头文件集"
    }
    
    # 清理临时目录
    # rm -rf "$TEMP_DIR"
}

# 创建最小头文件集
create_minimal_headers() {
    echo "=== 创建最小头文件集 ==="
    
    # Linux x86_64 最小头文件
    mkdir -p "$CROSS_DIR/linux/x86_64/include"
    cat > "$CROSS_DIR/linux/x86_64/include/stddef.h" << 'EOF'
#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef __PTRDIFF_TYPE__
#define __PTRDIFF_TYPE__ long int
#endif
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned long
#endif
typedef __SIZE_TYPE__ size_t;

#ifndef __WCHAR_TYPE__
#define __WCHAR_TYPE__ int
#endif
typedef __WCHAR_TYPE__ wchar_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#endif /* _STDDEF_H */
EOF

    cat > "$CROSS_DIR/linux/x86_64/include/stdio.h" << 'EOF'
#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>

typedef struct _FILE FILE;
#define EOF (-1)

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int fflush(FILE *stream);
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int puts(const char *s);
int putchar(int c);
int getchar(void);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif /* _STDIO_H */
EOF

    # 复制到 i386 目录
    cp -r "$CROSS_DIR/linux/x86_64/include" "$CROSS_DIR/linux/i386/"
    
    # Windows x86_64 最小头文件
    mkdir -p "$CROSS_DIR/windows/x86_64/include"
    cat > "$CROSS_DIR/windows/x86_64/include/windows.h" << 'EOF'
#ifndef _WINDOWS_H
#define _WINDOWS_H

#include <stddef.h>

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int BOOL;
typedef void *HANDLE;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef wchar_t *LPWSTR;
typedef const wchar_t *LPCWSTR;
typedef void *LPVOID;

#define TRUE 1
#define FALSE 0
#define NULL ((void *)0)

typedef struct _STARTUPINFOA {
    DWORD cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    BYTE *lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

#endif /* _WINDOWS_H */
EOF

    cat > "$CROSS_DIR/windows/x86_64/include/stdio.h" << 'EOF'
#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>

typedef struct _FILE FILE;
#define EOF (-1)

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int fflush(FILE *stream);
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int puts(const char *s);
int putchar(int c);
int getchar(void);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif /* _STDIO_H */
EOF

    # 复制到 i386 目录
    cp -r "$CROSS_DIR/windows/x86_64/include" "$CROSS_DIR/windows/i386/"
}

# 创建配置文件
create_config_files() {
    echo "=== 创建交叉编译配置文件 ==="
    
    # Linux x86_64 配置
    cat > "$CROSS_DIR/linux-x86_64.cfg" << EOF
# TCC Linux x86_64 交叉编译配置
-nostdinc
-I$CROSS_DIR/linux/x86_64/include
-L$CROSS_DIR/linux/x86_64/lib
-isystem $CROSS_DIR/linux/x86_64/include
-DTCC_TARGET_X86_64
-DTCC_TARGET_LINUX
-DTCC_USING_CROSS_COMPILER
-DTCC_TARGET_PE=0
-m64
EOF

    # Linux i386 配置
    cat > "$CROSS_DIR/linux-i386.cfg" << EOF
# TCC Linux i386 交叉编译配置
-nostdinc
-I$CROSS_DIR/linux/i386/include
-L$CROSS_DIR/linux/i386/lib
-isystem $CROSS_DIR/linux/i386/include
-DTCC_TARGET_I386
-DTCC_TARGET_LINUX
-DTCC_USING_CROSS_COMPILER
-DTCC_TARGET_PE=0
-m32
EOF

    # Windows x86_64 配置
    cat > "$CROSS_DIR/windows-x86_64.cfg" << EOF
# TCC Windows x86_64 交叉编译配置
-nostdinc
-I$CROSS_DIR/windows/x86_64/include
-L$CROSS_DIR/windows/x86_64/lib
-isystem $CROSS_DIR/windows/x86_64/include
-DTCC_TARGET_X86_64
-DTCC_TARGET_PE
-DTCC_USING_CROSS_COMPILER
-DTCC_TARGET_PE=1
-m64
EOF

    # Windows i386 配置
    cat > "$CROSS_DIR/windows-i386.cfg" << EOF
# TCC Windows i386 交叉编译配置
-nostdinc
-I$CROSS_DIR/windows/i386/include
-L$CROSS_DIR/windows/i386/lib
-isystem $CROSS_DIR/windows/i386/include
-DTCC_TARGET_I386
-DTCC_TARGET_PE
-DTCC_USING_CROSS_COMPILER
-DTCC_TARGET_PE=1
-m32
EOF
}

# 创建包装脚本
create_wrapper_scripts() {
    echo "=== 创建包装脚本 ==="
    
    # Linux x86_64 包装脚本
    cat > "$DIST_DIR/bin/tcc-linux-x86_64" << EOF
#!/bin/bash
SCRIPT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="\$(dirname "\$(dirname "\$SCRIPT_DIR")")"
"\$SCRIPT_DIR/tcc-macos-arm64" -B"\$TCC_ROOT/cross/linux/x86_64" @"\$TCC_ROOT/cross/linux-x86_64.cfg" "\$@" -o "\${@: -1}.elf"
EOF
    chmod +x "$DIST_DIR/bin/tcc-linux-x86_64"
    
    # Linux i386 包装脚本
    cat > "$DIST_DIR/bin/tcc-linux-i386" << EOF
#!/bin/bash
SCRIPT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="\$(dirname "\$(dirname "\$SCRIPT_DIR")")"
"\$SCRIPT_DIR/tcc-macos-arm64" -B"\$TCC_ROOT/cross/linux/i386" @"\$TCC_ROOT/cross/linux-i386.cfg" "\$@" -o "\${@: -1}.elf"
EOF
    chmod +x "$DIST_DIR/bin/tcc-linux-i386"
    
    # Windows x86_64 包装脚本
    cat > "$DIST_DIR/bin/tcc-windows-x86_64" << EOF
#!/bin/bash
SCRIPT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="\$(dirname "\$(dirname "\$SCRIPT_DIR")")"
"\$SCRIPT_DIR/tcc-macos-arm64" -B"\$TCC_ROOT/cross/windows/x86_64" @"\$TCC_ROOT/cross/windows-x86_64.cfg" "\$@" -o "\${@: -1}.exe"
EOF
    chmod +x "$DIST_DIR/bin/tcc-windows-x86_64"
    
    # Windows i386 包装脚本
    cat > "$DIST_DIR/bin/tcc-windows-i386" << EOF
#!/bin/bash
SCRIPT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
TCC_ROOT="\$(dirname "\$(dirname "\$SCRIPT_DIR")")"
"\$SCRIPT_DIR/tcc-macos-arm64" -B"\$TCC_ROOT/cross/windows/i386" @"\$TCC_ROOT/cross/windows-i386.cfg" "\$@" -o "\${@: -1}.exe"
EOF
    chmod +x "$DIST_DIR/bin/tcc-windows-i386"
}

# 创建测试程序
create_test_programs() {
    echo "=== 创建测试程序 ==="
    
    # 创建测试目录
    TEST_DIR="$CROSS_DIR/tests"
    mkdir -p "$TEST_DIR"
    
    # 创建简单的 Hello World 程序
    cat > "$TEST_DIR/hello.c" << 'EOF'
#include <stdio.h>

int main() {
    printf("Hello from Cross-Compiled TCC!\n");
    
    #ifdef __i386__
    printf("Architecture: x86 (32-bit)\n");
    #elif defined(__x86_64__)
    printf("Architecture: x86_64 (64-bit)\n");
    #endif
    
    #ifdef _WIN32
    printf("Platform: Windows\n");
    #elif defined(__linux__)
    printf("Platform: Linux\n");
    #endif
    
    return 0;
}
EOF
}

# 运行测试
run_tests() {
    echo "=== 运行交叉编译测试 ==="
    
    TEST_DIR="$CROSS_DIR/tests"
    
    echo "测试 Linux x86_64 交叉编译..."
    "$DIST_DIR/bin/tcc-linux-x86_64" -o "$TEST_DIR/hello-linux-x86_64" "$TEST_DIR/hello.c"
    
    echo "测试 Linux i386 交叉编译..."
    "$DIST_DIR/bin/tcc-linux-i386" -o "$TEST_DIR/hello-linux-i386" "$TEST_DIR/hello.c"
    
    echo "测试 Windows x86_64 交叉编译..."
    "$DIST_DIR/bin/tcc-windows-x86_64" -o "$TEST_DIR/hello-windows-x86_64" "$TEST_DIR/hello.c"
    
    echo "测试 Windows i386 交叉编译..."
    "$DIST_DIR/bin/tcc-windows-i386" -o "$TEST_DIR/hello-windows-i386" "$TEST_DIR/hello.c"
    
    echo "检查生成的文件..."
    ls -la "$TEST_DIR"
    
    echo "检查文件类型..."
    file "$TEST_DIR/hello-linux-x86_64" || echo "无法检查 Linux x86_64 文件类型"
    file "$TEST_DIR/hello-linux-i386" || echo "无法检查 Linux i386 文件类型"
    file "$TEST_DIR/hello-windows-x86_64.exe" || echo "无法检查 Windows x86_64 文件类型"
    file "$TEST_DIR/hello-windows-i386.exe" || echo "无法检查 Windows i386 文件类型"
}

# 显示使用说明
show_usage() {
    echo "=== TCC 交叉编译环境使用说明 ==="
    echo ""
    echo "可用的交叉编译器:"
    echo "  $DIST_DIR/bin/tcc-linux-x86_64   - 编译到 Linux x86_64 (64位)"
    echo "  $DIST_DIR/bin/tcc-linux-i386     - 编译到 Linux i386 (32位)"
    echo "  $DIST_DIR/bin/tcc-windows-x86_64 - 编译到 Windows x86_64 (64位)"
    echo "  $DIST_DIR/bin/tcc-windows-i386   - 编译到 Windows i386 (32位)"
    echo ""
    echo "使用示例:"
    echo "  $DIST_DIR/bin/tcc-linux-x86_64 -o output program.c"
    echo "  $DIST_DIR/bin/tcc-windows-x86_64 -o output program.c"
    echo ""
    echo "注意: 这是一个基本的交叉编译环境，只支持最小的标准库。"
    echo "      对于更复杂的程序，可能需要提供更多的头文件和库。"
}

# 主函数
main() {
    # 下载头文件
    download_headers
    
    # 创建最小头文件集
    create_minimal_headers
    
    # 创建配置文件
    create_config_files
    
    # 创建包装脚本
    create_wrapper_scripts
    
    # 创建测试程序
    create_test_programs
    
    # 运行测试
    # run_tests
    
    # 显示使用说明
    show_usage
    
    echo ""
    echo "🎉 TCC 交叉编译环境设置完成！"
}

# 运行主函数
main "$@" 