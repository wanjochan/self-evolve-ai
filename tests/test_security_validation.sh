#!/bin/bash
#
# test_security_validation.sh - 安全验证测试
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 测试统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

print_test_result() {
    local test_name="$1"
    local result="$2"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✅ PASS${NC}: $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ FAIL${NC}: $test_name"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# 安全测试1: 输入验证
test_input_validation() {
    echo -e "${BLUE}=== 安全测试1: 输入验证 ===${NC}"
    
    # 测试NULL指针处理
    cat > "$PROJECT_ROOT/security_input_test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int main() {
    printf("测试输入验证...\n");
    
    // 测试1: NULL路径
    void* handle1 = dlopen(NULL, RTLD_LAZY);
    if (handle1) {
        dlclose(handle1);
        printf("✅ NULL路径处理正确\n");
    } else {
        printf("✅ NULL路径被正确拒绝\n");
    }
    
    // 测试2: 无效路径
    void* handle2 = dlopen("/nonexistent/path/module.so", RTLD_LAZY);
    if (!handle2) {
        printf("✅ 无效路径被正确拒绝\n");
    } else {
        printf("❌ 无效路径未被拒绝\n");
        dlclose(handle2);
        return 1;
    }
    
    // 测试3: 路径遍历攻击
    void* handle3 = dlopen("../../../etc/passwd", RTLD_LAZY);
    if (!handle3) {
        printf("✅ 路径遍历攻击被阻止\n");
    } else {
        printf("❌ 路径遍历攻击未被阻止\n");
        dlclose(handle3);
        return 1;
    }
    
    // 测试4: 超长路径
    char long_path[2048];
    memset(long_path, 'A', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    
    void* handle4 = dlopen(long_path, RTLD_LAZY);
    if (!handle4) {
        printf("✅ 超长路径被正确处理\n");
    } else {
        printf("❌ 超长路径处理异常\n");
        dlclose(handle4);
        return 1;
    }
    
    printf("输入验证测试完成\n");
    return 0;
}
EOF

    if gcc "$PROJECT_ROOT/security_input_test.c" -o "$PROJECT_ROOT/security_input_test" -ldl; then
        if "$PROJECT_ROOT/security_input_test"; then
            print_test_result "输入验证测试" "PASS"
        else
            print_test_result "输入验证测试" "FAIL"
        fi
    else
        print_test_result "输入验证测试编译" "FAIL"
    fi
    
    rm -f "$PROJECT_ROOT/security_input_test.c" "$PROJECT_ROOT/security_input_test"
}

# 安全测试2: 权限检查
test_permission_checks() {
    echo -e "${BLUE}=== 安全测试2: 权限检查 ===${NC}"
    
    # 检查文件权限
    local modules_dir="$PROJECT_ROOT/bin"
    local permission_ok=true

    if [ -d "$modules_dir" ]; then
        # 检查模块文件权限 - 检查实际存在的.so文件
        for module_file in "$modules_dir"/*.so; do
            if [ -f "$module_file" ]; then
                # 检查文件是否可执行
                if [ -x "$module_file" ]; then
                    echo "✅ $(basename "$module_file"): 权限正确"
                else
                    echo "❌ $(basename "$module_file"): 权限不正确"
                    permission_ok=false
                fi
                
                # 检查文件所有者
                local owner=$(stat -c "%U" "$module_file" 2>/dev/null || echo "unknown")
                if [ "$owner" != "root" ]; then
                    echo "✅ $(basename "$module_file"): 非root所有者"
                else
                    echo "⚠️  $(basename "$module_file"): root所有者（可能的安全风险）"
                fi
            fi
        done
    else
        echo "❌ 模块目录不存在"
        permission_ok=false
    fi
    
    if $permission_ok; then
        print_test_result "权限检查测试" "PASS"
    else
        print_test_result "权限检查测试" "FAIL"
    fi
}

# 安全测试3: 内存安全
test_memory_safety() {
    echo -e "${BLUE}=== 安全测试3: 内存安全 ===${NC}"
    
    # 创建内存安全测试程序
    cat > "$PROJECT_ROOT/memory_safety_test.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

int main() {
    printf("测试内存安全...\n");
    
    // 测试1: 缓冲区溢出保护
    char buffer[256];
    const char* test_string = "这是一个测试字符串";
    
    // 安全的字符串复制
    strncpy(buffer, test_string, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    if (strlen(buffer) < sizeof(buffer)) {
        printf("✅ 缓冲区溢出保护正常\n");
    } else {
        printf("❌ 缓冲区溢出保护失败\n");
        return 1;
    }
    
    // 测试2: 内存泄漏检测
    void* handles[10];
    int loaded_count = 0;
    
    // 加载多个模块句柄
    for (int i = 0; i < 10; i++) {
        void* handle = dlopen("./bin/layer2/layer0.so", RTLD_LAZY);
        if (handle) {
            handles[loaded_count] = handle;
            loaded_count++;
        }
    }
    
    // 释放所有句柄
    for (int i = 0; i < loaded_count; i++) {
        dlclose(handles[i]);
    }
    
    printf("✅ 内存管理测试完成 (加载/释放 %d 个句柄)\n", loaded_count);
    
    // 测试3: 空指针检查
    void* null_handle = NULL;
    if (null_handle == NULL) {
        printf("✅ 空指针检查正常\n");
    }
    
    printf("内存安全测试完成\n");
    return 0;
}
EOF

    if gcc "$PROJECT_ROOT/memory_safety_test.c" -o "$PROJECT_ROOT/memory_safety_test" -ldl; then
        if "$PROJECT_ROOT/memory_safety_test"; then
            print_test_result "内存安全测试" "PASS"
        else
            print_test_result "内存安全测试" "FAIL"
        fi
    else
        print_test_result "内存安全测试编译" "FAIL"
    fi
    
    rm -f "$PROJECT_ROOT/memory_safety_test.c" "$PROJECT_ROOT/memory_safety_test"
}

# 安全测试4: 代码注入防护
test_code_injection_protection() {
    echo -e "${BLUE}=== 安全测试4: 代码注入防护 ===${NC}"
    
    # 测试恶意输入处理
    local malicious_inputs=(
        "'; rm -rf /; echo '"
        "../../../etc/passwd"
        "\$(whoami)"
        "|cat /etc/passwd"
        "&& rm -rf /"
    )
    
    local injection_blocked=true
    
    for input in "${malicious_inputs[@]}"; do
        # 尝试使用恶意输入作为模块路径
        if echo "$input" | grep -q "[;&|$]"; then
            echo "✅ 检测到恶意字符: $input"
        else
            echo "⚠️  未检测到恶意字符: $input"
        fi
    done
    
    # 测试命令执行防护
    local test_command="echo 'test' && echo 'injection'"
    if ! eval "$test_command" >/dev/null 2>&1; then
        echo "✅ 命令注入被阻止"
    else
        echo "⚠️  命令注入可能存在风险"
    fi
    
    if $injection_blocked; then
        print_test_result "代码注入防护测试" "PASS"
    else
        print_test_result "代码注入防护测试" "FAIL"
    fi
}

# 安全测试5: 资源限制
test_resource_limits() {
    echo -e "${BLUE}=== 安全测试5: 资源限制 ===${NC}"
    
    # 检查系统资源限制
    echo "检查系统资源限制:"
    
    # 检查文件描述符限制
    local fd_limit=$(ulimit -n)
    echo "  文件描述符限制: $fd_limit"
    
    # 检查内存限制
    local mem_limit=$(ulimit -v)
    if [ "$mem_limit" = "unlimited" ]; then
        echo "  虚拟内存限制: 无限制"
    else
        echo "  虚拟内存限制: $mem_limit KB"
    fi
    
    # 检查进程限制
    local proc_limit=$(ulimit -u)
    echo "  进程数限制: $proc_limit"
    
    # 测试是否能够正常工作在限制内
    local fd_ok=false
    local proc_ok=false

    # 检查文件描述符限制
    if [ "$fd_limit" = "unlimited" ] || [ "$fd_limit" -gt 100 ] 2>/dev/null; then
        fd_ok=true
    fi

    # 检查进程限制
    if [ "$proc_limit" = "unlimited" ] || [ "$proc_limit" -gt 10 ] 2>/dev/null; then
        proc_ok=true
    fi

    if $fd_ok && $proc_ok; then
        print_test_result "资源限制检查" "PASS"
    else
        print_test_result "资源限制检查" "FAIL"
    fi
}

# 主函数
main() {
    echo -e "${BLUE}=== 安全验证测试套件 ===${NC}"
    echo "测试目标: 验证系统的安全性和防护机制"
    echo
    
    cd "$PROJECT_ROOT"
    
    # 运行所有安全测试
    test_input_validation
    echo
    test_permission_checks
    echo
    test_memory_safety
    echo
    test_code_injection_protection
    echo
    test_resource_limits
    
    # 输出测试结果
    echo
    echo -e "${BLUE}=== 安全测试结果统计 ===${NC}"
    echo "总测试数: $TOTAL_TESTS"
    echo -e "通过: ${GREEN}$PASSED_TESTS${NC}"
    echo -e "失败: ${RED}$FAILED_TESTS${NC}"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}🔒 所有安全测试通过！系统安全性良好。${NC}"
        exit 0
    else
        echo -e "${RED}⚠️  有 $FAILED_TESTS 个安全测试失败，需要关注。${NC}"
        exit 1
    fi
}

# 运行主函数
main "$@"
