/*
 * Memory Management Optimizer AI - Stage 2 AI优化引擎
 * T2.2: 内存管理优化AI
 * 
 * 功能: 优化Stage 1内存使用，减少内存泄漏和提升内存效率
 * 特性: 内存泄漏检测、内存池设计、智能垃圾回收、内存使用优化
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

// 内存优化AI头文件
#include "memory_optimizer.h"

// 内存优化策略定义
typedef struct MemoryOptimization {
    const char* name;                 // 优化名称
    const char* description;          // 优化描述
    const char* detection_pattern;    // 检测模式
    int priority;                    // 优先级 (1-10)
    double memory_savings;           // 预期内存节省百分比
    const char* implementation;      // 实现方法
    const char* optimization_type;   // 优化类型
} MemoryOptimization;

// 内存优化策略数据库
static MemoryOptimization MEMORY_OPTIMIZATIONS[] = {
    // 内存泄漏修复
    {
        "Memory Leak Fix",
        "修复潜在的内存泄漏",
        "malloc\\s*\\([^}]*(?!.*free\\s*\\()",
        9,
        15.0,
        "为每个malloc添加对应的free调用，使用RAII模式",
        "LEAK_PREVENTION"
    },
    
    {
        "Buffer Overflow Prevention",
        "防止缓冲区溢出",
        "strcpy\\s*\\(|strcat\\s*\\(|sprintf\\s*\\(",
        8,
        0.0,
        "替换为安全版本: strncpy, strncat, snprintf",
        "SECURITY_FIX"
    },
    
    {
        "Double Free Prevention",
        "防止双重释放",
        "free\\s*\\([^}]*free\\s*\\(",
        9,
        0.0,
        "添加NULL检查和指针置空",
        "SECURITY_FIX"
    },
    
    // 内存池优化
    {
        "Memory Pool for Small Objects",
        "小对象内存池优化",
        "malloc\\s*\\(\\s*sizeof\\s*\\([^)]*\\)\\s*\\)",
        7,
        30.0,
        "实现固定大小对象的内存池",
        "MEMORY_POOL"
    },
    
    {
        "Arena Allocator for Temporary Objects",
        "临时对象的Arena分配器",
        "malloc\\s*\\([^}]*free\\s*\\([^}]*\\)",
        6,
        25.0,
        "使用Arena分配器管理短生命周期对象",
        "MEMORY_POOL"
    },
    
    {
        "String Pool for Compiler",
        "编译器字符串池",
        "malloc\\s*\\([^}]*strlen\\s*\\(|strdup\\s*\\(",
        8,
        40.0,
        "实现字符串去重和池化管理",
        "MEMORY_POOL"
    },
    
    // 内存碎片优化
    {
        "Large Buffer Pre-allocation",
        "大缓冲区预分配",
        "realloc\\s*\\([^}]*\\+|malloc\\s*\\([^}]*\\*",
        6,
        20.0,
        "预分配足够大的缓冲区，避免频繁扩容",
        "FRAGMENTATION_REDUCTION"
    },
    
    {
        "Aligned Memory Allocation",
        "对齐内存分配",
        "malloc\\s*\\([^}]*sizeof\\s*\\([^)]*\\)\\s*\\*",
        5,
        10.0,
        "使用posix_memalign确保内存对齐",
        "CACHE_OPTIMIZATION"
    },
    
    // 智能内存管理
    {
        "Reference Counting",
        "引用计数内存管理",
        "\\w+\\s*\\*.*=.*malloc|struct.*\\*.*=.*malloc",
        7,
        35.0,
        "实现自动引用计数和智能指针",
        "SMART_MEMORY"
    },
    
    {
        "Copy-on-Write Optimization",
        "写时复制优化",
        "memcpy\\s*\\([^}]*sizeof|strcpy\\s*\\([^}]*",
        6,
        25.0,
        "实现写时复制减少不必要的内存复制",
        "COPY_OPTIMIZATION"
    },
    
    // 编译器特定优化
    {
        "AST Node Pool",
        "AST节点内存池",
        "create_\\w*node|new_\\w*node|alloc.*node",
        8,
        45.0,
        "专用AST节点内存池，批量分配和释放",
        "COMPILER_SPECIFIC"
    },
    
    {
        "Symbol Table Optimization",
        "符号表内存优化",
        "symbol.*malloc|hash.*malloc|table.*malloc",
        7,
        30.0,
        "优化符号表内存布局和访问模式",
        "COMPILER_SPECIFIC"
    },
    
    {
        "Compile Cache Memory",
        "编译缓存内存管理",
        "cache.*malloc|compile.*cache.*alloc",
        6,
        20.0,
        "智能编译缓存内存管理和LRU策略",
        "COMPILER_SPECIFIC"
    },
    
    // 垃圾回收优化
    {
        "Mark-and-Sweep GC",
        "标记清除垃圾回收",
        "malloc\\s*\\([^}]*(?=.*complex.*allocation)",
        8,
        50.0,
        "为复杂对象实现标记清除垃圾回收",
        "GARBAGE_COLLECTION"
    },
    
    {
        "Generational GC for Temps",
        "临时对象分代回收",
        "malloc\\s*\\([^}]*(?=.*temp|tmp)",
        7,
        40.0,
        "临时对象的分代垃圾回收机制",
        "GARBAGE_COLLECTION"
    },
    
    {NULL, NULL, NULL, 0, 0.0, NULL, NULL}  // 结束标记
};

// 内存优化实例
typedef struct MemoryOptimizationInstance {
    MemoryOptimization* optimization;
    char* file_path;
    int line_number;
    char* function_name;
    char* code_context;
    double estimated_savings;
    int implementation_complexity;
    char* optimization_plan;
} MemoryOptimizationInstance;

// 内存优化统计
typedef struct MemoryStats {
    int total_optimizations;
    int high_priority_optimizations;
    double total_memory_savings;
    int leak_fixes;
    int security_fixes;
    int pool_optimizations;
    int smart_memory_improvements;
} MemoryStats;

// 全局状态
static MemoryOptimizationInstance* g_optimizations = NULL;
static int g_optimization_count = 0;
static int g_optimization_capacity = 0;
static MemoryStats g_memory_stats = {0};

// 分析目标
static const char* MEMORY_ANALYSIS_TARGETS[] = {
    "src/core/modules/pipeline_module.c",     // 编译流水线内存密集
    "src/core/modules/c99bin_module.c",       // 编译器核心内存管理
    "src/core/modules/compiler_module.c",     // JIT编译器内存
    "src/core/modules/libc_module.c",         // 标准库内存操作
    "src/core/modules/module_module.c",       // 模块加载内存
    "src/layer1/simple_loader.c",             // 加载器内存
    NULL
};

// 函数声明
static int analyze_memory_optimizations(void);
static int scan_file_for_memory_issues(const char* file_path);
static int detect_memory_optimization_opportunities(const char* file_path, const char* content);
static int add_memory_optimization(MemoryOptimization* opt, const char* file_path,
                                 int line_number, const char* function_name,
                                 const char* context, double savings, int complexity);
static void calculate_memory_statistics(void);
static void generate_memory_optimization_plan(void);
static void cleanup_memory_data(void);
static char* extract_context_info(const char* content, const char* position);
static double calculate_memory_impact(MemoryOptimization* opt, const char* context);

// 主内存优化函数
int memory_optimizer_run(void) {
    printf("🧠 AI Memory Optimizer - Stage 2 内存管理优化AI启动\n");
    printf("===================================================\n");
    
    // 初始化数据结构
    g_optimization_capacity = 300;
    g_optimizations = calloc(g_optimization_capacity, sizeof(MemoryOptimizationInstance));
    if (!g_optimizations) {
        fprintf(stderr, "Error: 无法分配内存用于内存优化分析\n");
        return -1;
    }
    
    // 分析内存优化机会
    printf("🔍 开始内存优化机会分析...\n");
    if (analyze_memory_optimizations() < 0) {
        fprintf(stderr, "内存优化分析失败\n");
        cleanup_memory_data();
        return -1;
    }
    
    // 计算内存统计
    printf("📊 计算内存优化统计...\n");
    calculate_memory_statistics();
    
    // 生成优化方案
    printf("📋 生成内存优化方案...\n");
    generate_memory_optimization_plan();
    
    // 清理资源
    cleanup_memory_data();
    
    printf("\n🎯 内存优化分析完成！发现 %d 个优化机会\n", g_optimization_count);
    return 0;
}

// 分析内存优化机会
static int analyze_memory_optimizations(void) {
    for (int i = 0; MEMORY_ANALYSIS_TARGETS[i]; i++) {
        const char* target = MEMORY_ANALYSIS_TARGETS[i];
        printf("   分析: %s\n", target);
        
        if (scan_file_for_memory_issues(target) < 0) {
            printf("   ⚠️  跳过: %s (文件不存在或无法读取)\n", target);
        }
    }
    return 0;
}

// 扫描文件寻找内存问题
static int scan_file_for_memory_issues(const char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return -1;
    }
    
    // 读取文件内容
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return -1;
    }
    
    size_t read_size = fread(content, 1, file_size, file);
    content[read_size] = '\0';
    fclose(file);
    
    // 检测内存优化机会
    int optimizations = detect_memory_optimization_opportunities(file_path, content);
    
    free(content);
    return optimizations;
}

// 检测内存优化机会
static int detect_memory_optimization_opportunities(const char* file_path, const char* content) {
    int total_optimizations = 0;
    
    for (int i = 0; MEMORY_OPTIMIZATIONS[i].name; i++) {
        MemoryOptimization* opt = &MEMORY_OPTIMIZATIONS[i];
        regex_t regex;
        
        // 编译正则表达式
        if (regcomp(&regex, opt->detection_pattern, REG_EXTENDED | REG_ICASE) != 0) {
            continue;
        }
        
        // 查找匹配
        regmatch_t match;
        const char* search_start = content;
        
        while (regexec(&regex, search_start, 1, &match, 0) == 0) {
            // 计算行号
            int line_number = 1;
            for (const char* p = content; p < search_start + match.rm_so; p++) {
                if (*p == '\n') line_number++;
            }
            
            // 提取函数名 (简化版本)
            char function_name[256] = "unknown";
            const char* line_start = search_start + match.rm_so;
            while (line_start > content && *(line_start-1) != '\n') line_start--;
            
            for (const char* p = line_start; p >= content - 500 && p >= content; p--) {
                if (sscanf(p, "%*s %255[a-zA-Z_][a-zA-Z0-9_]*\\s*\\(", function_name) == 1) {
                    break;
                }
            }
            
            // 提取上下文信息
            char* context = extract_context_info(content, search_start + match.rm_so);
            
            // 计算内存影响
            double savings = calculate_memory_impact(opt, context);
            
            // 计算实现复杂度
            int complexity = opt->priority; // 优先级越高，实现相对简单
            
            // 添加优化机会
            add_memory_optimization(opt, file_path, line_number, function_name,
                                   context, savings, complexity);
            total_optimizations++;
            
            // 继续搜索
            search_start += match.rm_eo;
            if (*search_start == '\0') break;
            
            free(context);
        }
        
        regfree(&regex);
    }
    
    return total_optimizations;
}

// 提取上下文信息
static char* extract_context_info(const char* content, const char* position) {
    // 提取前后各100字符作为上下文
    int context_start = (position - content > 100) ? (position - content - 100) : 0;
    int context_end = (position - content + 100 < strlen(content)) ? 
                     (position - content + 100) : strlen(content);
    
    return strndup(content + context_start, context_end - context_start);
}

// 计算内存影响
static double calculate_memory_impact(MemoryOptimization* opt, const char* context) {
    double base_savings = opt->memory_savings;
    
    // 根据上下文调整影响
    if (strstr(context, "loop") || strstr(context, "for") || strstr(context, "while")) {
        base_savings *= 2.0; // 循环中的优化影响更大
    }
    
    if (strstr(context, "recursive")) {
        base_savings *= 1.5; // 递归函数中的优化
    }
    
    if (strstr(context, "cache") || strstr(context, "pool")) {
        base_savings *= 1.3; // 缓存或池相关的优化
    }
    
    // 基于代码复杂度调整
    int complexity_indicators = 0;
    if (strstr(context, "malloc")) complexity_indicators++;
    if (strstr(context, "free")) complexity_indicators++;
    if (strstr(context, "realloc")) complexity_indicators++;
    
    base_savings *= (1.0 + complexity_indicators * 0.1);
    
    return base_savings;
}

// 添加内存优化
static int add_memory_optimization(MemoryOptimization* opt, const char* file_path,
                                 int line_number, const char* function_name,
                                 const char* context, double savings, int complexity) {
    // 扩展容量
    if (g_optimization_count >= g_optimization_capacity) {
        g_optimization_capacity *= 2;
        g_optimizations = realloc(g_optimizations, 
                                g_optimization_capacity * sizeof(MemoryOptimizationInstance));
        if (!g_optimizations) {
            return -1;
        }
    }
    
    // 添加优化实例
    MemoryOptimizationInstance* instance = &g_optimizations[g_optimization_count];
    instance->optimization = opt;
    instance->file_path = strdup(file_path);
    instance->line_number = line_number;
    instance->function_name = strdup(function_name);
    instance->code_context = strdup(context);
    instance->estimated_savings = savings;
    instance->implementation_complexity = complexity;
    instance->optimization_plan = strdup(opt->implementation);
    
    g_optimization_count++;
    return 0;
}

// 计算内存统计
static void calculate_memory_statistics(void) {
    g_memory_stats.total_optimizations = g_optimization_count;
    g_memory_stats.high_priority_optimizations = 0;
    g_memory_stats.total_memory_savings = 0.0;
    g_memory_stats.leak_fixes = 0;
    g_memory_stats.security_fixes = 0;
    g_memory_stats.pool_optimizations = 0;
    g_memory_stats.smart_memory_improvements = 0;
    
    for (int i = 0; i < g_optimization_count; i++) {
        MemoryOptimizationInstance* instance = &g_optimizations[i];
        
        // 统计高优先级优化
        if (instance->optimization->priority >= 7) {
            g_memory_stats.high_priority_optimizations++;
        }
        
        // 累计内存节省
        g_memory_stats.total_memory_savings += instance->estimated_savings;
        
        // 分类统计
        const char* type = instance->optimization->optimization_type;
        if (strcmp(type, "LEAK_PREVENTION") == 0) {
            g_memory_stats.leak_fixes++;
        } else if (strcmp(type, "SECURITY_FIX") == 0) {
            g_memory_stats.security_fixes++;
        } else if (strcmp(type, "MEMORY_POOL") == 0) {
            g_memory_stats.pool_optimizations++;
        } else if (strcmp(type, "SMART_MEMORY") == 0) {
            g_memory_stats.smart_memory_improvements++;
        }
    }
}

// 生成内存优化方案
static void generate_memory_optimization_plan(void) {
    printf("\n🧠 AI内存管理优化方案\n");
    printf("======================\n");
    printf("📊 发现优化机会: %d 个\n", g_memory_stats.total_optimizations);
    printf("🔥 高优先级优化: %d 个\n", g_memory_stats.high_priority_optimizations);
    printf("💾 预期内存节省: %.1f%%\n", g_memory_stats.total_memory_savings);
    
    // 分类统计
    printf("\n📊 优化类别分布:\n");
    printf("   🔒 内存泄漏修复: %d 项\n", g_memory_stats.leak_fixes);
    printf("   🛡️  安全漏洞修复: %d 项\n", g_memory_stats.security_fixes);
    printf("   🏊 内存池优化: %d 项\n", g_memory_stats.pool_optimizations);
    printf("   🧠 智能内存管理: %d 项\n", g_memory_stats.smart_memory_improvements);
    
    // 按节省效果排序
    for (int i = 0; i < g_optimization_count - 1; i++) {
        for (int j = i + 1; j < g_optimization_count; j++) {
            if (g_optimizations[i].estimated_savings < g_optimizations[j].estimated_savings) {
                MemoryOptimizationInstance temp = g_optimizations[i];
                g_optimizations[i] = g_optimizations[j];
                g_optimizations[j] = temp;
            }
        }
    }
    
    // 显示前10个最佳优化建议
    printf("\n🎯 高效内存优化建议 (按效果排序):\n");
    int display_count = (g_optimization_count > 10) ? 10 : g_optimization_count;
    for (int i = 0; i < display_count; i++) {
        MemoryOptimizationInstance* instance = &g_optimizations[i];
        printf("   %d. %s\n", i+1, instance->optimization->name);
        printf("      📍 位置: %s:%d (%s)\n", 
               instance->file_path, instance->line_number, instance->function_name);
        printf("      💡 描述: %s\n", instance->optimization->description);
        printf("      💾 预期节省: %.1f%% | 优先级: %d/10 | 复杂度: %d/10\n",
               instance->estimated_savings, instance->optimization->priority, 
               instance->implementation_complexity);
        printf("      🔧 实施方案: %s\n", instance->optimization_plan);
        printf("      📂 类型: %s\n", instance->optimization->optimization_type);
        printf("\n");
    }
    
    // 实施路线图
    printf("🗺️  内存优化实施路线图:\n");
    printf("   Phase 1 (立即): 修复内存泄漏和安全漏洞 (%d项)\n", 
           g_memory_stats.leak_fixes + g_memory_stats.security_fixes);
    printf("   Phase 2 (短期): 实施内存池优化 (%d项)\n", 
           g_memory_stats.pool_optimizations);
    printf("   Phase 3 (中期): 智能内存管理升级 (%d项)\n", 
           g_memory_stats.smart_memory_improvements);
    
    // ROI分析
    printf("\n📈 投资回报分析:\n");
    printf("   预期内存使用减少: %.1f%%\n", g_memory_stats.total_memory_savings);
    printf("   性能提升预期: %.1f%%\n", g_memory_stats.total_memory_savings * 0.3);
    printf("   开发工作量估算: %d 人天\n", g_optimization_count * 2);
    printf("   投资回报比: %.2f\n", g_memory_stats.total_memory_savings / (g_optimization_count * 0.1));
}

// 导出内存优化分析结果
int memory_optimizer_export_json(const char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"stage2_memory_optimization\": {\n");
    fprintf(file, "    \"timestamp\": \"%ld\",\n", time(NULL));
    fprintf(file, "    \"memory_statistics\": {\n");
    fprintf(file, "      \"total_optimizations\": %d,\n", g_memory_stats.total_optimizations);
    fprintf(file, "      \"high_priority_optimizations\": %d,\n", g_memory_stats.high_priority_optimizations);
    fprintf(file, "      \"total_memory_savings\": %.2f,\n", g_memory_stats.total_memory_savings);
    fprintf(file, "      \"leak_fixes\": %d,\n", g_memory_stats.leak_fixes);
    fprintf(file, "      \"security_fixes\": %d,\n", g_memory_stats.security_fixes);
    fprintf(file, "      \"pool_optimizations\": %d,\n", g_memory_stats.pool_optimizations);
    fprintf(file, "      \"smart_memory_improvements\": %d\n", g_memory_stats.smart_memory_improvements);
    fprintf(file, "    },\n");
    fprintf(file, "    \"optimizations\": [\n");
    
    for (int i = 0; i < g_optimization_count; i++) {
        MemoryOptimizationInstance* instance = &g_optimizations[i];
        fprintf(file, "      {\n");
        fprintf(file, "        \"name\": \"%s\",\n", instance->optimization->name);
        fprintf(file, "        \"file\": \"%s\",\n", instance->file_path);
        fprintf(file, "        \"line\": %d,\n", instance->line_number);
        fprintf(file, "        \"function\": \"%s\",\n", instance->function_name);
        fprintf(file, "        \"priority\": %d,\n", instance->optimization->priority);
        fprintf(file, "        \"savings\": %.2f,\n", instance->estimated_savings);
        fprintf(file, "        \"complexity\": %d,\n", instance->implementation_complexity);
        fprintf(file, "        \"type\": \"%s\"\n", instance->optimization->optimization_type);
        fprintf(file, "      }%s\n", (i < g_optimization_count - 1) ? "," : "");
    }
    
    fprintf(file, "    ]\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return 0;
}

// 清理数据
static void cleanup_memory_data(void) {
    if (g_optimizations) {
        for (int i = 0; i < g_optimization_count; i++) {
            free(g_optimizations[i].file_path);
            free(g_optimizations[i].function_name);
            free(g_optimizations[i].code_context);
            free(g_optimizations[i].optimization_plan);
        }
        free(g_optimizations);
        g_optimizations = NULL;
    }
    
    g_optimization_count = 0;
    g_optimization_capacity = 0;
}

// 命令行接口
int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("AI Memory Optimizer - Stage 2 内存管理优化AI\n");
        printf("用法: %s [--export output.json]\n", argv[0]);
        printf("功能: 优化Stage 1内存使用，减少内存泄漏和提升内存效率\n");
        return 0;
    }
    
    // 运行内存优化分析
    int result = memory_optimizer_run();
    
    // 导出结果
    if (argc > 2 && strcmp(argv[1], "--export") == 0) {
        if (memory_optimizer_export_json(argv[2]) == 0) {
            printf("📄 内存优化分析结果已导出到: %s\n", argv[2]);
        } else {
            printf("❌ 导出失败\n");
        }
    }
    
    return result;
}