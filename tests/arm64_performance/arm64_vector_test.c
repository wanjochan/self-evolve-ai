#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arm_neon.h>

// ARM64 NEON向量化测试
void test_neon_performance() {
    const int size = 1000000;
    float *a = malloc(size * sizeof(float));
    float *b = malloc(size * sizeof(float));
    float *c = malloc(size * sizeof(float));
    
    // 初始化数据
    for (int i = 0; i < size; i++) {
        a[i] = (float)i;
        b[i] = (float)(i + 1);
    }
    
    clock_t start = clock();
    
    // NEON向量化加法
    for (int i = 0; i < size; i += 4) {
        float32x4_t va = vld1q_f32(&a[i]);
        float32x4_t vb = vld1q_f32(&b[i]);
        float32x4_t vc = vaddq_f32(va, vb);
        vst1q_f32(&c[i], vc);
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("ARM64 NEON向量化测试完成\n");
    printf("处理 %d 个元素，耗时: %f 秒\n", size, time_taken);
    printf("性能: %.2f MFLOPS\n", (size / time_taken) / 1000000.0);
    
    free(a);
    free(b);
    free(c);
}

int main() {
    printf("=== ARM64性能测试 ===\n");
    test_neon_performance();
    return 0;
}
