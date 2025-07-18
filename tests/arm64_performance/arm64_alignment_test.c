#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// 测试内存对齐对性能的影响
void test_alignment_performance() {
    const int iterations = 10000000;
    
    // 16字节对齐的数据
    uint64_t *aligned_data = aligned_alloc(16, iterations * sizeof(uint64_t));
    
    // 未对齐的数据
    uint64_t *unaligned_data = malloc(iterations * sizeof(uint64_t) + 1);
    uint64_t *unaligned_ptr = (uint64_t*)((char*)unaligned_data + 1);
    
    // 测试对齐访问
    clock_t start = clock();
    uint64_t sum1 = 0;
    for (int i = 0; i < iterations; i++) {
        sum1 += aligned_data[i];
    }
    clock_t end = clock();
    double aligned_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // 测试未对齐访问
    start = clock();
    uint64_t sum2 = 0;
    for (int i = 0; i < iterations; i++) {
        sum2 += unaligned_ptr[i];
    }
    end = clock();
    double unaligned_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("ARM64内存对齐性能测试\n");
    printf("对齐访问时间: %f 秒\n", aligned_time);
    printf("未对齐访问时间: %f 秒\n", unaligned_time);
    printf("性能提升: %.2fx\n", unaligned_time / aligned_time);
    
    free(aligned_data);
    free(unaligned_data);
}

int main() {
    printf("=== ARM64内存对齐测试 ===\n");
    test_alignment_performance();
    return 0;
}
