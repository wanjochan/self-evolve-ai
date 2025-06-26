/**
 * test_astc_limitations.c - 测试ASTC环境的限制
 * 
 * 这个测试验证在ASTC环境中哪些功能可用，哪些不可用
 */

#include <stdio.h>
#include <stdlib.h>

// 测试文件操作
int test_file_operations() {
    printf("测试文件操作...\n");
    
    // 尝试创建文件
    FILE* fp = fopen("test_output.txt", "w");
    if (!fp) {
        printf("❌ 无法创建文件\n");
        return 1;
    }
    
    fprintf(fp, "Hello from ASTC environment\n");
    fclose(fp);
    
    // 尝试读取文件
    fp = fopen("test_output.txt", "r");
    if (!fp) {
        printf("❌ 无法读取文件\n");
        return 1;
    }
    
    char buffer[100];
    if (fgets(buffer, sizeof(buffer), fp)) {
        printf("✅ 文件操作成功: %s", buffer);
    } else {
        printf("❌ 文件读取失败\n");
        fclose(fp);
        return 1;
    }
    
    fclose(fp);
    return 0;
}

// 测试内存操作
int test_memory_operations() {
    printf("测试内存操作...\n");
    
    // 尝试分配内存
    char* buffer = (char*)malloc(1024);
    if (!buffer) {
        printf("❌ 内存分配失败\n");
        return 1;
    }
    
    // 写入数据
    sprintf(buffer, "Memory allocation test successful");
    printf("✅ 内存操作成功: %s\n", buffer);
    
    free(buffer);
    return 0;
}

int main() {
    printf("=== ASTC环境限制测试 ===\n");
    
    int file_result = test_file_operations();
    int memory_result = test_memory_operations();
    
    if (file_result == 0 && memory_result == 0) {
        printf("✅ 基本操作在ASTC环境中可用\n");
        return 0;
    } else {
        printf("❌ 某些操作在ASTC环境中不可用\n");
        return 1;
    }
}
