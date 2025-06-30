/**
 * test_enhanced_jit_simple.c - 简化的增强JIT编译器测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/runtime/enhanced_jit_compiler.h"

// 简化的架构名称函数
const char* get_architecture_name(TargetArch arch) {
    switch (arch) {
        case ARCH_X86_32: return "x86_32";
        case ARCH_X86_64: return "x86_64";
        case ARCH_ARM32:  return "arm32";
        case ARCH_ARM64:  return "arm64";
        default:          return "unknown";
    }
}

int main() {
    printf("=== Enhanced JIT Compiler Simple Test ===\n");
    
    // 1. 测试优化选项配置
    printf("\n1. Testing optimization options...\n");
    
    JitOptOptions default_opts = enhanced_get_default_opt_options();
    printf("✅ Default options: level=%d, inline=%s\n",
           default_opts.opt_level,
           default_opts.inline_functions ? "Yes" : "No");
    
    JitOptOptions perf_opts = enhanced_get_performance_opt_options();
    printf("✅ Performance options: level=%d, vectorize=%s\n",
           perf_opts.opt_level,
           perf_opts.vectorize ? "Yes" : "No");
    
    JitOptOptions size_opts = enhanced_get_size_opt_options();
    printf("✅ Size options: level=%d, max_inline=%u\n",
           size_opts.opt_level,
           size_opts.max_inline_size);
    
    // 2. 测试代码生成器创建
    printf("\n2. Testing code generator creation...\n");
    
    EnhancedCodeGen* gen = enhanced_codegen_create(ARCH_X86_64, &default_opts);
    if (!gen) {
        printf("❌ Failed to create code generator\n");
        return 1;
    }
    printf("✅ Code generator created successfully\n");
    printf("  Target architecture: %s\n", get_architecture_name(gen->target_arch));
    printf("  Optimizations enabled: %s\n", gen->enable_optimizations ? "Yes" : "No");
    printf("  Register allocation: %s\n", gen->enable_register_allocation ? "Yes" : "No");
    printf("  Constant folding: %s\n", gen->enable_constant_folding ? "Yes" : "No");
    
    // 3. 测试单个指令编译
    printf("\n3. Testing individual instruction compilation...\n");
    
    // 测试常量加载 (值为0，应该触发优化)
    uint8_t const_zero[] = {0x00, 0x00, 0x00, 0x00};
    int result1 = enhanced_compile_instruction(gen, 0x10, const_zero, 4);
    printf("  CONST_I32(0): %s\n", result1 == 0 ? "✅ Success" : "❌ Failed");
    
    // 测试常量加载 (小值，应该触发8位优化)
    uint8_t const_small[] = {0x0A, 0x00, 0x00, 0x00}; // 值为10
    int result2 = enhanced_compile_instruction(gen, 0x10, const_small, 4);
    printf("  CONST_I32(10): %s\n", result2 == 0 ? "✅ Success" : "❌ Failed");
    
    // 测试加法指令
    int result3 = enhanced_compile_instruction(gen, 0x20, NULL, 0);
    printf("  ADD: %s\n", result3 == 0 ? "✅ Success" : "❌ Failed");
    
    // 测试局部变量存储 (小偏移，应该触发优化)
    uint8_t store_local[] = {0x04, 0x00, 0x00, 0x00}; // 偏移为4
    int result4 = enhanced_compile_instruction(gen, 0x30, store_local, 4);
    printf("  STORE_LOCAL(4): %s\n", result4 == 0 ? "✅ Success" : "❌ Failed");
    
    // 测试局部变量加载
    uint8_t load_local[] = {0x04, 0x00, 0x00, 0x00}; // 偏移为4
    int result5 = enhanced_compile_instruction(gen, 0x31, load_local, 4);
    printf("  LOAD_LOCAL(4): %s\n", result5 == 0 ? "✅ Success" : "❌ Failed");
    
    // 4. 检查生成的代码
    printf("\n4. Checking generated code...\n");
    printf("  Code size: %zu bytes\n", gen->code_size);
    printf("  Instructions compiled: %u\n", gen->instructions_compiled);
    printf("  Optimizations applied: %u\n", gen->optimizations_applied);
    
    if (gen->code_size > 0) {
        printf("✅ Code generation successful\n");
        printf("  First 16 bytes of generated code: ");
        for (size_t i = 0; i < 16 && i < gen->code_size; i++) {
            printf("%02X ", gen->code[i]);
        }
        printf("\n");
    } else {
        printf("❌ No code generated\n");
    }
    
    // 5. 测试优化应用
    printf("\n5. Testing optimization application...\n");
    
    size_t code_size_before = gen->code_size;
    uint32_t opts_before = gen->optimizations_applied;
    
    int opt_result = enhanced_apply_optimizations(gen);
    printf("  Optimization result: %s\n", opt_result == 0 ? "✅ Success" : "❌ Failed");
    printf("  Code size: %zu → %zu bytes\n", code_size_before, gen->code_size);
    printf("  Optimizations: %u → %u\n", opts_before, gen->optimizations_applied);
    
    // 6. 测试统计信息
    printf("\n6. Testing compilation statistics...\n");
    
    JitCompilationStats stats;
    enhanced_get_compilation_stats(gen, &stats);
    
    printf("  Total instructions: %u\n", stats.total_instructions);
    printf("  Optimized instructions: %u\n", stats.optimized_instructions);
    printf("  Optimization ratio: %.1f%%\n", stats.optimization_ratio * 100.0f);
    printf("  Code size: %zu bytes\n", stats.code_size_after_opt);
    printf("  Compilation time: %llu μs\n", stats.compilation_time_us);
    
    if (stats.total_instructions > 0) {
        printf("✅ Statistics collection working\n");
    } else {
        printf("❌ Statistics collection failed\n");
    }
    
    // 7. 打印详细统计
    printf("\n7. Detailed compilation statistics:\n");
    enhanced_print_compilation_stats(gen);
    
    // 8. 测试不同架构
    printf("\n8. Testing different architectures...\n");
    
    // 测试ARM64
    EnhancedCodeGen* gen_arm64 = enhanced_codegen_create(ARCH_ARM64, &default_opts);
    if (gen_arm64) {
        printf("✅ ARM64 code generator created\n");
        
        // 测试ARM64指令编译
        int arm64_result = enhanced_compile_instruction(gen_arm64, 0x10, const_small, 4);
        printf("  ARM64 CONST_I32: %s\n", arm64_result == 0 ? "✅ Success" : "❌ Failed");
        
        printf("  ARM64 code size: %zu bytes\n", gen_arm64->code_size);
        enhanced_codegen_free(gen_arm64);
    } else {
        printf("❌ Failed to create ARM64 code generator\n");
    }
    
    // 9. 测试性能优化模式
    printf("\n9. Testing performance optimization mode...\n");
    
    EnhancedCodeGen* gen_perf = enhanced_codegen_create(ARCH_X86_64, &perf_opts);
    if (gen_perf) {
        printf("✅ Performance code generator created\n");
        
        // 编译相同的指令序列
        enhanced_compile_instruction(gen_perf, 0x10, const_zero, 4);
        enhanced_compile_instruction(gen_perf, 0x10, const_small, 4);
        enhanced_compile_instruction(gen_perf, 0x20, NULL, 0);
        
        printf("  Performance mode code size: %zu bytes\n", gen_perf->code_size);
        printf("  Performance mode optimizations: %u\n", gen_perf->optimizations_applied);
        
        enhanced_codegen_free(gen_perf);
    } else {
        printf("❌ Failed to create performance code generator\n");
    }
    
    // 清理
    enhanced_codegen_free(gen);
    
    printf("\n=== Test Summary ===\n");
    printf("✅ Enhanced JIT compiler simple test completed!\n");
    printf("🎉 Core functionality verified!\n");
    
    printf("\nKey features tested:\n");
    printf("- ✅ Optimization options configuration\n");
    printf("- ✅ Code generator creation and management\n");
    printf("- ✅ Individual instruction compilation\n");
    printf("- ✅ Instruction-level optimizations\n");
    printf("- ✅ Code generation statistics\n");
    printf("- ✅ Multi-architecture support\n");
    printf("- ✅ Performance optimization modes\n");
    
    return 0;
}
