/*
 * Performance Bottleneck Detector - Stage 2 AI模式识别系统
 * T1.3: 性能瓶颈检测器
 * 
 * 功能: 深度分析Stage 1代码中的性能热点和瓶颈
 * 特性: 热点分析、算法复杂度检测、内存泄漏识别、并发瓶颈检测
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <math.h>

// 性能瓶颈检测器头文件
#include "performance_detector.h"

// 性能瓶颈类型定义
typedef struct PerformancePattern {
    const char* name;                 // 瓶颈名称
    const char* description;          // 瓶颈描述
    const char* detection_regex;      // 检测正则表达式
    int severity_level;              // 严重程度 (1-10)
    double performance_impact;       // 性能影响百分比
    const char* optimization_hint;   // 优化提示
    const char* category;            // 瓶颈类别
} PerformancePattern;

// 性能瓶颈模式数据库
static PerformancePattern PERFORMANCE_PATTERNS[] = {
    // 循环性能瓶颈
    {
        "Nested Loop O(n²)",
        "嵌套循环导致二次时间复杂度",
        "for\\s*\\([^}]*for\\s*\\([^}]*\\)|while\\s*\\([^}]*while\\s*\\([^}]*\\)",
        9,
        60.0,
        "考虑使用哈希表或其他O(n log n)算法替代",
        "ALGORITHM_COMPLEXITY"
    },
    
    {
        "String Concat in Loop",
        "循环中的字符串连接性能问题",
        "for\\s*\\([^}]*strcat\\s*\\(|while\\s*\\([^}]*strcat\\s*\\(",
        8,
        45.0,
        "使用预分配缓冲区或StringBuilder模式",
        "STRING_PROCESSING"
    },
    
    {
        "Malloc in Tight Loop",
        "紧密循环中的内存分配",
        "for\\s*\\([^}]*malloc\\s*\\(|while\\s*\\([^}]*malloc\\s*\\(",
        9,
        70.0,
        "循环外预分配内存或使用内存池",
        "MEMORY_MANAGEMENT"
    },
    
    {
        "Recursive Call Without Memoization",
        "未优化的递归调用",
        "\\w+\\s*\\([^)]*\\)\\s*{[^}]*return.*\\w+\\s*\\(",
        7,
        40.0,
        "添加记忆化或转换为迭代实现",
        "ALGORITHM_COMPLEXITY"
    },
    
    // I/O性能瓶颈
    {
        "Unbuffered File Operations",
        "无缓冲的文件操作",
        "fopen\\s*\\([^)]*\\)[^}]*fread\\s*\\([^)]*1\\s*,",
        6,
        25.0,
        "使用缓冲I/O或批量读取",
        "IO_OPERATIONS"
    },
    
    {
        "Frequent Small Writes",
        "频繁的小块写入",
        "for\\s*\\([^}]*fwrite\\s*\\([^)]*1\\s*,|while\\s*\\([^}]*fwrite\\s*\\([^)]*1\\s*,",
        7,
        35.0,
        "批量写入或使用更大的缓冲区",
        "IO_OPERATIONS"
    },
    
    {
        "Sync After Every Write",
        "每次写入后同步",
        "fwrite\\s*\\([^}]*fsync\\s*\\(|write\\s*\\([^}]*fsync\\s*\\(",
        8,
        50.0,
        "批量同步或使用异步I/O",
        "IO_OPERATIONS"
    },
    
    // 内存性能瓶颈
    {
        "Memory Leak Potential",
        "潜在的内存泄漏",
        "malloc\\s*\\([^}]*(?!.*free\\s*\\()",
        8,
        30.0,
        "确保每个malloc都有对应的free调用",
        "MEMORY_MANAGEMENT"
    },
    
    {
        "Double Free Risk",
        "双重释放风险",
        "free\\s*\\([^}]*free\\s*\\(",
        9,
        0.0,
        "设置指针为NULL或使用智能指针模式",
        "MEMORY_MANAGEMENT"
    },
    
    {
        "Large Stack Allocation",
        "大型栈分配",
        "char\\s+\\w+\\[\\s*[0-9]{4,}\\s*\\]|int\\s+\\w+\\[\\s*[0-9]{3,}\\s*\\]",
        6,
        20.0,
        "考虑使用堆分配避免栈溢出",
        "MEMORY_MANAGEMENT"
    },
    
    // 缓存性能瓶颈
    {
        "Cache Unfriendly Access",
        "缓存不友好的内存访问",
        "\\[\\s*j\\s*\\]\\[\\s*i\\s*\\]|\\[\\s*\\w+\\s*\\+\\s*\\w+\\s*\\*\\s*\\w+\\s*\\]",
        7,
        30.0,
        "优化内存访问模式，提高缓存局部性",
        "CACHE_PERFORMANCE"
    },
    
    {
        "Random Memory Access",
        "随机内存访问模式",
        "\\[\\s*rand\\(\\)|\\[\\s*random\\(\\)|\\[\\s*\\w+\\s*%\\s*\\w+\\s*\\]",
        6,
        25.0,
        "重新组织数据结构以提高访问局部性",
        "CACHE_PERFORMANCE"
    },
    
    // 并发性能瓶颈
    {
        "Lock Contention",
        "锁争用瓶颈",
        "pthread_mutex_lock\\s*\\([^}]*pthread_mutex_lock\\s*\\(",
        8,
        40.0,
        "减少锁粒度或使用无锁数据结构",
        "CONCURRENCY"
    },
    
    {
        "Busy Wait Loop",
        "忙等待循环",
        "while\\s*\\([^}]*\\)\\s*;|for\\s*\\([^}]*\\)\\s*;",
        7,
        35.0,
        "使用条件变量或信号量替代忙等待",
        "CONCURRENCY"
    },
    
    // 编译器特定瓶颈
    {
        "Linear Symbol Lookup",
        "线性符号表查找",
        "for\\s*\\([^}]*strcmp\\s*\\(|while\\s*\\([^}]*strcmp\\s*\\(",
        8,
        45.0,
        "使用哈希表或二叉搜索树优化查找",
        "COMPILER_SPECIFIC"
    },
    
    {
        "Inefficient AST Traversal",
        "低效的AST遍历",
        "recursive.*visit|visit.*recursive|traverse.*node.*traverse",
        7,
        30.0,
        "使用迭代器模式或栈-based遍历",
        "COMPILER_SPECIFIC"
    },
    
    {
        "Redundant Type Checking",
        "冗余的类型检查",
        "check_type\\s*\\([^}]*check_type\\s*\\(",
        6,
        20.0,
        "缓存类型信息或延迟类型检查",
        "COMPILER_SPECIFIC"
    },
    
    {NULL, NULL, NULL, 0, 0.0, NULL, NULL}  // 结束标记
};

// 性能瓶颈实例
typedef struct BottleneckInstance {
    PerformancePattern* pattern;
    char* file_path;
    int line_number;
    char* function_name;
    char* code_snippet;
    double estimated_impact;
    int confidence_score;
    char* context_info;
} BottleneckInstance;

// 性能热点统计
typedef struct HotspotStats {
    int total_bottlenecks;
    int critical_bottlenecks;     // 严重程度 >= 8
    int high_bottlenecks;         // 严重程度 >= 6
    double total_impact;          // 总性能影响
    char* worst_file;             // 最差文件
    int worst_file_issues;        // 最差文件问题数
} HotspotStats;

// 全局状态
static BottleneckInstance* g_bottlenecks = NULL;
static int g_bottleneck_count = 0;
static int g_bottleneck_capacity = 0;
static HotspotStats g_hotspot_stats = {0};

// 分析目标
static const char* PERFORMANCE_ANALYSIS_TARGETS[] = {
    "src/core/modules/pipeline_module.c",     // 编译流水线 - 性能关键
    "src/core/modules/c99bin_module.c",       // 编译器核心 - 计算密集
    "src/core/modules/compiler_module.c",     // JIT编译器 - 热点代码
    "src/core/modules/module_module.c",       // 模块管理 - 查找密集
    "src/layer1/simple_loader.c",             // 加载器 - I/O密集
    "tools/c99bin.c",                         // 编译器工具
    NULL
};

// 函数声明
static int analyze_performance_bottlenecks(void);
static int scan_file_for_bottlenecks(const char* file_path);
static int detect_bottleneck_patterns(const char* file_path, const char* content);
static int add_bottleneck_instance(PerformancePattern* pattern, const char* file_path,
                                 int line_number, const char* function_name,
                                 const char* code_snippet, double impact, int confidence);
static void calculate_hotspot_statistics(void);
static void generate_performance_report(void);
static void cleanup_performance_data(void);
static char* extract_function_name(const char* content, const char* position);
static double calculate_performance_impact(PerformancePattern* pattern, const char* context);

// 主性能检测函数
int performance_detector_run(void) {
    printf("🔍 AI Performance Detector - Stage 2 性能瓶颈检测器启动\n");
    printf("========================================================\n");
    
    // 初始化数据结构
    g_bottleneck_capacity = 500;
    g_bottlenecks = calloc(g_bottleneck_capacity, sizeof(BottleneckInstance));
    if (!g_bottlenecks) {
        fprintf(stderr, "Error: 无法分配内存用于性能瓶颈检测\n");
        return -1;
    }
    
    // 分析性能瓶颈
    printf("🎯 开始性能瓶颈深度分析...\n");
    if (analyze_performance_bottlenecks() < 0) {
        fprintf(stderr, "性能瓶颈分析失败\n");
        cleanup_performance_data();
        return -1;
    }
    
    // 计算热点统计
    printf("📊 计算性能热点统计...\n");
    calculate_hotspot_statistics();
    
    // 生成性能报告
    generate_performance_report();
    
    // 清理资源
    cleanup_performance_data();
    
    printf("\n🎯 性能瓶颈检测完成！发现 %d 个性能问题\n", g_bottleneck_count);
    return 0;
}

// 分析性能瓶颈
static int analyze_performance_bottlenecks(void) {
    for (int i = 0; PERFORMANCE_ANALYSIS_TARGETS[i]; i++) {
        const char* target = PERFORMANCE_ANALYSIS_TARGETS[i];
        printf("   分析: %s\n", target);
        
        if (scan_file_for_bottlenecks(target) < 0) {
            printf("   ⚠️  跳过: %s (文件不存在或无法读取)\n", target);
        }
    }
    return 0;
}

// 扫描文件寻找性能瓶颈
static int scan_file_for_bottlenecks(const char* file_path) {
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
    
    // 检测性能瓶颈模式
    int bottlenecks = detect_bottleneck_patterns(file_path, content);
    
    free(content);
    return bottlenecks;
}

// 检测瓶颈模式
static int detect_bottleneck_patterns(const char* file_path, const char* content) {
    int total_bottlenecks = 0;
    
    for (int i = 0; PERFORMANCE_PATTERNS[i].name; i++) {
        PerformancePattern* pattern = &PERFORMANCE_PATTERNS[i];
        regex_t regex;
        
        // 编译正则表达式
        if (regcomp(&regex, pattern->detection_regex, REG_EXTENDED | REG_ICASE) != 0) {
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
            
            // 提取函数名
            char* function_name = extract_function_name(content, search_start + match.rm_so);
            
            // 提取代码片段
            int snippet_start = (search_start + match.rm_so - content > 50) ? 
                               (search_start + match.rm_so - content - 50) : 0;
            int snippet_end = (search_start + match.rm_eo - content + 50 < strlen(content)) ?
                             (search_start + match.rm_eo - content + 50) : strlen(content);
            char* code_snippet = strndup(content + snippet_start, snippet_end - snippet_start);
            
            // 计算性能影响
            double impact = calculate_performance_impact(pattern, code_snippet);
            
            // 计算置信度 (基于上下文匹配)
            int confidence = 70 + pattern->severity_level * 3;
            if (confidence > 100) confidence = 100;
            
            // 添加瓶颈实例
            add_bottleneck_instance(pattern, file_path, line_number, function_name,
                                   code_snippet, impact, confidence);
            total_bottlenecks++;
            
            // 继续搜索
            search_start += match.rm_eo;
            if (*search_start == '\0') break;
            
            free(function_name);
            free(code_snippet);
        }
        
        regfree(&regex);
    }
    
    return total_bottlenecks;
}

// 提取函数名
static char* extract_function_name(const char* content, const char* position) {
    // 向前搜索函数定义
    const char* search_start = position;
    while (search_start > content && *(search_start-1) != '\n') {
        search_start--;
    }
    
    // 在前1000字符内搜索函数定义
    char function_name[256] = "unknown";
    for (const char* p = search_start; p >= content - 1000 && p >= content; p--) {
        // 匹配C函数定义模式
        if (sscanf(p, "%*s %255[a-zA-Z_][a-zA-Z0-9_]*\\s*\\(", function_name) == 1) {
            break;
        }
        // 匹配简单函数定义
        if (sscanf(p, "%255[a-zA-Z_][a-zA-Z0-9_]*\\s*\\(", function_name) == 1) {
            break;
        }
    }
    
    return strdup(function_name);
}

// 计算性能影响
static double calculate_performance_impact(PerformancePattern* pattern, const char* context) {
    double base_impact = pattern->performance_impact;
    
    // 根据上下文调整影响
    if (strstr(context, "while") || strstr(context, "for")) {
        base_impact *= 1.5; // 循环中的问题影响更大
    }
    
    if (strstr(context, "malloc") && strstr(context, "free")) {
        base_impact *= 0.8; // 有配对的malloc/free影响较小
    }
    
    // 基于代码复杂度调整
    int complexity_indicators = 0;
    if (strstr(context, "nested")) complexity_indicators++;
    if (strstr(context, "recursive")) complexity_indicators++;
    if (strstr(context, "switch")) complexity_indicators++;
    
    base_impact *= (1.0 + complexity_indicators * 0.2);
    
    return base_impact;
}

// 添加瓶颈实例
static int add_bottleneck_instance(PerformancePattern* pattern, const char* file_path,
                                 int line_number, const char* function_name,
                                 const char* code_snippet, double impact, int confidence) {
    // 扩展容量
    if (g_bottleneck_count >= g_bottleneck_capacity) {
        g_bottleneck_capacity *= 2;
        g_bottlenecks = realloc(g_bottlenecks, g_bottleneck_capacity * sizeof(BottleneckInstance));
        if (!g_bottlenecks) {
            return -1;
        }
    }
    
    // 添加瓶颈实例
    BottleneckInstance* instance = &g_bottlenecks[g_bottleneck_count];
    instance->pattern = pattern;
    instance->file_path = strdup(file_path);
    instance->line_number = line_number;
    instance->function_name = strdup(function_name);
    instance->code_snippet = strdup(code_snippet);
    instance->estimated_impact = impact;
    instance->confidence_score = confidence;
    instance->context_info = strdup(pattern->category);
    
    g_bottleneck_count++;
    return 0;
}

// 计算热点统计
static void calculate_hotspot_statistics(void) {
    g_hotspot_stats.total_bottlenecks = g_bottleneck_count;
    g_hotspot_stats.critical_bottlenecks = 0;
    g_hotspot_stats.high_bottlenecks = 0;
    g_hotspot_stats.total_impact = 0.0;
    
    // 文件问题统计
    const char* files[10];
    int file_counts[10] = {0};
    int unique_files = 0;
    
    for (int i = 0; i < g_bottleneck_count; i++) {
        BottleneckInstance* instance = &g_bottlenecks[i];
        
        // 统计严重程度
        if (instance->pattern->severity_level >= 8) {
            g_hotspot_stats.critical_bottlenecks++;
        }
        if (instance->pattern->severity_level >= 6) {
            g_hotspot_stats.high_bottlenecks++;
        }
        
        // 累计性能影响
        g_hotspot_stats.total_impact += instance->estimated_impact;
        
        // 统计文件问题数
        int file_found = 0;
        for (int j = 0; j < unique_files; j++) {
            if (strcmp(files[j], instance->file_path) == 0) {
                file_counts[j]++;
                file_found = 1;
                break;
            }
        }
        if (!file_found && unique_files < 10) {
            files[unique_files] = instance->file_path;
            file_counts[unique_files] = 1;
            unique_files++;
        }
    }
    
    // 找出最差文件
    int max_issues = 0;
    const char* worst_file = "unknown";
    for (int i = 0; i < unique_files; i++) {
        if (file_counts[i] > max_issues) {
            max_issues = file_counts[i];
            worst_file = files[i];
        }
    }
    g_hotspot_stats.worst_file = strdup(worst_file);
    g_hotspot_stats.worst_file_issues = max_issues;
}

// 生成性能报告
static void generate_performance_report(void) {
    printf("\n🔍 AI性能瓶颈检测报告\n");
    printf("======================\n");
    printf("📊 发现性能瓶颈: %d 个\n", g_hotspot_stats.total_bottlenecks);
    printf("🔥 严重瓶颈 (8-10级): %d 个\n", g_hotspot_stats.critical_bottlenecks);
    printf("⚠️  高风险瓶颈 (6-7级): %d 个\n", g_hotspot_stats.high_bottlenecks);
    printf("📈 总体性能影响: %.1f%%\n", g_hotspot_stats.total_impact);
    printf("📁 最差文件: %s (%d 个问题)\n", 
           g_hotspot_stats.worst_file, g_hotspot_stats.worst_file_issues);
    
    // 显示严重瓶颈
    printf("\n🔥 严重性能瓶颈 (按影响排序):\n");
    
    // 简单排序 (按影响降序)
    for (int i = 0; i < g_bottleneck_count - 1; i++) {
        for (int j = i + 1; j < g_bottleneck_count; j++) {
            if (g_bottlenecks[i].estimated_impact < g_bottlenecks[j].estimated_impact) {
                BottleneckInstance temp = g_bottlenecks[i];
                g_bottlenecks[i] = g_bottlenecks[j];
                g_bottlenecks[j] = temp;
            }
        }
    }
    
    // 显示前15个最严重的瓶颈
    int display_count = (g_bottleneck_count > 15) ? 15 : g_bottleneck_count;
    for (int i = 0; i < display_count; i++) {
        BottleneckInstance* instance = &g_bottlenecks[i];
        if (instance->pattern->severity_level >= 6) {
            printf("   %d. %s\n", i+1, instance->pattern->name);
            printf("      📍 位置: %s:%d (%s)\n", 
                   instance->file_path, instance->line_number, instance->function_name);
            printf("      💡 描述: %s\n", instance->pattern->description);
            printf("      📊 影响: %.1f%% | 严重度: %d/10 | 置信度: %d%%\n",
                   instance->estimated_impact, instance->pattern->severity_level, 
                   instance->confidence_score);
            printf("      🔧 优化建议: %s\n", instance->pattern->optimization_hint);
            printf("      📂 类别: %s\n", instance->context_info);
            printf("\n");
        }
    }
    
    // 分类统计
    printf("📊 瓶颈类别分布:\n");
    int algo_count = 0, memory_count = 0, io_count = 0, cache_count = 0, 
        concurrency_count = 0, compiler_count = 0, string_count = 0;
    
    for (int i = 0; i < g_bottleneck_count; i++) {
        const char* category = g_bottlenecks[i].context_info;
        if (strcmp(category, "ALGORITHM_COMPLEXITY") == 0) algo_count++;
        else if (strcmp(category, "MEMORY_MANAGEMENT") == 0) memory_count++;
        else if (strcmp(category, "IO_OPERATIONS") == 0) io_count++;
        else if (strcmp(category, "CACHE_PERFORMANCE") == 0) cache_count++;
        else if (strcmp(category, "CONCURRENCY") == 0) concurrency_count++;
        else if (strcmp(category, "COMPILER_SPECIFIC") == 0) compiler_count++;
        else if (strcmp(category, "STRING_PROCESSING") == 0) string_count++;
    }
    
    printf("   🧮 算法复杂度: %d 项\n", algo_count);
    printf("   🧠 内存管理: %d 项\n", memory_count);
    printf("   📁 I/O操作: %d 项\n", io_count);
    printf("   ⚡ 缓存性能: %d 项\n", cache_count);
    printf("   🔀 并发处理: %d 项\n", concurrency_count);
    printf("   🔧 编译器特定: %d 项\n", compiler_count);
    printf("   📝 字符串处理: %d 项\n", string_count);
}

// 导出性能分析结果
int performance_detector_export_json(const char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"stage2_performance_analysis\": {\n");
    fprintf(file, "    \"timestamp\": \"%ld\",\n", time(NULL));
    fprintf(file, "    \"hotspot_statistics\": {\n");
    fprintf(file, "      \"total_bottlenecks\": %d,\n", g_hotspot_stats.total_bottlenecks);
    fprintf(file, "      \"critical_bottlenecks\": %d,\n", g_hotspot_stats.critical_bottlenecks);
    fprintf(file, "      \"high_bottlenecks\": %d,\n", g_hotspot_stats.high_bottlenecks);
    fprintf(file, "      \"total_impact\": %.2f,\n", g_hotspot_stats.total_impact);
    fprintf(file, "      \"worst_file\": \"%s\",\n", g_hotspot_stats.worst_file);
    fprintf(file, "      \"worst_file_issues\": %d\n", g_hotspot_stats.worst_file_issues);
    fprintf(file, "    },\n");
    fprintf(file, "    \"bottlenecks\": [\n");
    
    for (int i = 0; i < g_bottleneck_count; i++) {
        BottleneckInstance* instance = &g_bottlenecks[i];
        fprintf(file, "      {\n");
        fprintf(file, "        \"name\": \"%s\",\n", instance->pattern->name);
        fprintf(file, "        \"file\": \"%s\",\n", instance->file_path);
        fprintf(file, "        \"line\": %d,\n", instance->line_number);
        fprintf(file, "        \"function\": \"%s\",\n", instance->function_name);
        fprintf(file, "        \"severity\": %d,\n", instance->pattern->severity_level);
        fprintf(file, "        \"impact\": %.2f,\n", instance->estimated_impact);
        fprintf(file, "        \"confidence\": %d,\n", instance->confidence_score);
        fprintf(file, "        \"category\": \"%s\"\n", instance->context_info);
        fprintf(file, "      }%s\n", (i < g_bottleneck_count - 1) ? "," : "");
    }
    
    fprintf(file, "    ]\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return 0;
}

// 清理数据
static void cleanup_performance_data(void) {
    if (g_bottlenecks) {
        for (int i = 0; i < g_bottleneck_count; i++) {
            free(g_bottlenecks[i].file_path);
            free(g_bottlenecks[i].function_name);
            free(g_bottlenecks[i].code_snippet);
            free(g_bottlenecks[i].context_info);
        }
        free(g_bottlenecks);
        g_bottlenecks = NULL;
    }
    
    if (g_hotspot_stats.worst_file) {
        free(g_hotspot_stats.worst_file);
    }
    
    g_bottleneck_count = 0;
    g_bottleneck_capacity = 0;
}

// 命令行接口
int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("AI Performance Detector - Stage 2 性能瓶颈检测系统\n");
        printf("用法: %s [--export output.json]\n", argv[0]);
        printf("功能: 深度分析Stage 1代码中的性能热点和瓶颈\n");
        return 0;
    }
    
    // 运行性能瓶颈检测
    int result = performance_detector_run();
    
    // 导出结果
    if (argc > 2 && strcmp(argv[1], "--export") == 0) {
        if (performance_detector_export_json(argv[2]) == 0) {
            printf("📄 性能分析结果已导出到: %s\n", argv[2]);
        } else {
            printf("❌ 导出失败\n");
        }
    }
    
    return result;
}