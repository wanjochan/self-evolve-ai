/*
 * Pattern Analyzer - Stage 2 AI模式识别系统核心组件
 * T1.1: 代码模式分析器
 * 
 * 功能: 分析Stage 1代码中的模式、反模式和优化机会
 * 特性: AST解析、模式匹配、复杂度分析、热点识别
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <regex.h>

// AI模式识别系统头文件
#include "pattern_analyzer.h"

// Stage 1代码分析目标
static const char* STAGE1_ANALYSIS_TARGETS[] = {
    "src/core/modules/pipeline_module.c",     // 优先级1: 编译流水线 (6965行)
    "src/core/modules/c99bin_module.c",       // 优先级1: 编译器核心 (2263行)
    "src/core/modules/compiler_module.c",     // 优先级2: JIT和FFI (1446行)
    "src/core/modules/libc_module.c",         // 优先级2: C99标准库 (1633行)
    "src/core/modules/module_module.c",       // 优先级2: 模块管理 (1194行)
    "src/layer1/simple_loader.c",             // Layer 1加载器
    "tools/c99bin.c",                         // C99编译器工具
    NULL
};

// 代码模式数据库
typedef struct CodePattern {
    const char* name;           // 模式名称
    const char* regex;          // 正则表达式
    const char* description;    // 模式描述
    int priority;              // 优先级 (1=高, 2=中, 3=低)
    const char* category;      // 模式类别
} CodePattern;

// 预定义的代码模式库
static CodePattern PATTERN_DATABASE[] = {
    // 性能相关模式
    {"memory_leak_risk", "malloc\\s*\\([^)]+\\)[^;]*;[^}]*}[^}]*$", 
     "潜在内存泄漏风险: malloc后缺少对应的free", 1, "performance"},
    
    {"nested_loops", "for\\s*\\([^}]*for\\s*\\([^}]*for\\s*\\(", 
     "深度嵌套循环: 可能的性能瓶颈", 1, "performance"},
    
    {"string_concat_loop", "for\\s*\\([^}]*strcat\\s*\\(", 
     "循环中字符串连接: 低效的字符串操作", 1, "performance"},
    
    // 设计模式相关
    {"factory_pattern", "create_\\w+\\s*\\([^)]*\\)\\s*{[^}]*switch", 
     "工厂模式: 对象创建的抽象", 2, "design_pattern"},
    
    {"singleton_pattern", "static\\s+\\w+\\s*\\*\\s*instance\\s*=\\s*NULL", 
     "单例模式: 全局唯一实例", 2, "design_pattern"},
    
    {"observer_pattern", "callback\\s*\\(|notify\\s*\\(", 
     "观察者模式: 事件通知机制", 2, "design_pattern"},
    
    // 代码质量模式
    {"magic_numbers", "\\b[0-9]{3,}\\b", 
     "魔法数字: 应使用常量定义", 2, "quality"},
    
    {"long_function", "^[^{]*{([^{}]*{[^{}]*})*[^{}]*}\\s*$", 
     "过长函数: 建议分解", 2, "quality"},
    
    {"deep_nesting", "if\\s*\\([^}]*if\\s*\\([^}]*if\\s*\\([^}]*if", 
     "深度嵌套: 建议重构", 2, "quality"},
    
    // 安全相关模式
    {"buffer_overflow_risk", "strcpy\\s*\\(|strcat\\s*\\(|sprintf\\s*\\(", 
     "缓冲区溢出风险: 使用不安全的字符串函数", 1, "security"},
    
    {"format_string_vuln", "printf\\s*\\(\\s*[a-zA-Z_]\\w*\\s*\\)", 
     "格式化字符串漏洞: 用户输入直接作为格式字符串", 1, "security"},
    
    {NULL, NULL, NULL, 0, NULL} // 结束标记
};

// 模式分析结果
typedef struct PatternMatch {
    CodePattern* pattern;
    char* file_path;
    int line_number;
    char* matched_text;
    int confidence;  // 匹配置信度 (0-100)
} PatternMatch;

// 分析统计 (使用头文件中的定义)

// 全局分析状态
static AnalysisStats g_stats = {0};
static PatternMatch* g_matches = NULL;
static int g_match_count = 0;
static int g_match_capacity = 0;

// 函数声明
static int analyze_file(const char* file_path);
static int match_patterns_in_content(const char* file_path, const char* content);
static int add_pattern_match(CodePattern* pattern, const char* file_path, 
                           int line_number, const char* matched_text, int confidence);
static void print_analysis_report(void);
static void print_pattern_summary(void);
static void cleanup_analysis_data(void);

// 主分析函数
int pattern_analyzer_run(void) {
    printf("🧠 AI Pattern Analyzer - Stage 2 模式识别引擎启动\n");
    printf("==================================================\n");
    
    // 初始化分析数据
    g_match_capacity = 1000;  // 初始容量
    g_matches = calloc(g_match_capacity, sizeof(PatternMatch));
    if (!g_matches) {
        fprintf(stderr, "Error: 无法分配内存用于模式匹配结果\n");
        return -1;
    }
    
    // 分析Stage 1目标文件
    printf("📊 开始分析Stage 1代码库...\n");
    for (int i = 0; STAGE1_ANALYSIS_TARGETS[i]; i++) {
        const char* target = STAGE1_ANALYSIS_TARGETS[i];
        printf("   分析: %s\n", target);
        
        if (analyze_file(target) < 0) {
            printf("   ⚠️  跳过: %s (文件不存在或无法读取)\n", target);
        } else {
            g_stats.total_files++;
        }
    }
    
    // 生成分析报告
    printf("\n📋 生成模式识别报告...\n");
    print_analysis_report();
    print_pattern_summary();
    
    // 清理资源
    cleanup_analysis_data();
    
    printf("\n🎯 模式分析完成！发现 %d 个潜在优化机会\n", g_match_count);
    return 0;
}

// 分析单个文件
static int analyze_file(const char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (!file) {
        return -1;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 读取文件内容
    char* content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return -1;
    }
    
    size_t read_size = fread(content, 1, file_size, file);
    content[read_size] = '\0';
    fclose(file);
    
    // 统计行数
    int line_count = 1;
    for (char* p = content; *p; p++) {
        if (*p == '\n') line_count++;
    }
    g_stats.total_lines += line_count;
    
    // 在内容中匹配模式
    int matches = match_patterns_in_content(file_path, content);
    
    free(content);
    return matches;
}

// 在内容中匹配所有模式
static int match_patterns_in_content(const char* file_path, const char* content) {
    int total_matches = 0;
    
    for (int i = 0; PATTERN_DATABASE[i].name; i++) {
        CodePattern* pattern = &PATTERN_DATABASE[i];
        regex_t regex;
        
        // 编译正则表达式
        int regex_result = regcomp(&regex, pattern->regex, REG_EXTENDED | REG_ICASE);
        if (regex_result != 0) {
            continue; // 跳过无效的正则表达式
        }
        
        // 查找匹配
        regmatch_t match;
        const char* search_start = content;
        int line_number = 1;
        
        while (regexec(&regex, search_start, 1, &match, 0) == 0) {
            // 计算行号
            for (const char* p = content; p < search_start + match.rm_so; p++) {
                if (*p == '\n') line_number++;
            }
            
            // 提取匹配的文本
            int match_len = match.rm_eo - match.rm_so;
            char* matched_text = strndup(search_start + match.rm_so, match_len);
            
            // 计算置信度 (简单的启发式方法)
            int confidence = 80; // 基础置信度
            if (pattern->priority == 1) confidence += 15;  // 高优先级模式增加置信度
            if (match_len > 10) confidence += 5;           // 较长匹配增加置信度
            
            // 添加到匹配结果
            add_pattern_match(pattern, file_path, line_number, matched_text, confidence);
            total_matches++;
            
            // 继续搜索下一个匹配
            search_start += match.rm_eo;
            if (*search_start == '\0') break;
            
            free(matched_text);
        }
        
        regfree(&regex);
    }
    
    return total_matches;
}

// 添加模式匹配结果
static int add_pattern_match(CodePattern* pattern, const char* file_path, 
                           int line_number, const char* matched_text, int confidence) {
    // 扩展数组容量如果需要
    if (g_match_count >= g_match_capacity) {
        g_match_capacity *= 2;
        g_matches = realloc(g_matches, g_match_capacity * sizeof(PatternMatch));
        if (!g_matches) {
            return -1;
        }
    }
    
    // 添加新的匹配
    PatternMatch* match = &g_matches[g_match_count];
    match->pattern = pattern;
    match->file_path = strdup(file_path);
    match->line_number = line_number;
    match->matched_text = strdup(matched_text);
    match->confidence = confidence;
    
    // 更新统计
    g_stats.total_patterns++;
    if (pattern->priority == 1) g_stats.high_priority_issues++;
    else if (pattern->priority == 2) g_stats.medium_priority_issues++;
    else g_stats.low_priority_issues++;
    
    g_match_count++;
    return 0;
}

// 打印分析报告
static void print_analysis_report(void) {
    printf("\n🔍 AI模式识别分析报告\n");
    printf("========================\n");
    printf("📁 分析文件数: %d\n", g_stats.total_files);
    printf("📄 总代码行数: %d\n", g_stats.total_lines);
    printf("🎯 发现模式数: %d\n", g_stats.total_patterns);
    printf("\n💡 问题优先级分布:\n");
    printf("   🔴 高优先级: %d 个问题\n", g_stats.high_priority_issues);
    printf("   🟡 中优先级: %d 个问题\n", g_stats.medium_priority_issues);
    printf("   🟢 低优先级: %d 个问题\n", g_stats.low_priority_issues);
    
    // 显示高优先级问题详情
    if (g_stats.high_priority_issues > 0) {
        printf("\n🚨 高优先级问题详情:\n");
        for (int i = 0; i < g_match_count; i++) {
            PatternMatch* match = &g_matches[i];
            if (match->pattern->priority == 1) {
                printf("   📍 %s:%d - %s\n", 
                       match->file_path, match->line_number, match->pattern->name);
                printf("      💬 %s\n", match->pattern->description);
                printf("      🎯 置信度: %d%%\n", match->confidence);
                printf("      📝 代码: %.50s%s\n", 
                       match->matched_text, strlen(match->matched_text) > 50 ? "..." : "");
                printf("\n");
            }
        }
    }
}

// 打印模式总结
static void print_pattern_summary(void) {
    printf("📊 模式类别统计:\n");
    printf("==================\n");
    
    // 统计各类别的模式数量
    struct {
        const char* category;
        int count;
    } category_stats[] = {
        {"performance", 0},
        {"design_pattern", 0},
        {"quality", 0},
        {"security", 0}
    };
    
    for (int i = 0; i < g_match_count; i++) {
        const char* category = g_matches[i].pattern->category;
        for (int j = 0; j < 4; j++) {
            if (strcmp(category, category_stats[j].category) == 0) {
                category_stats[j].count++;
                break;
            }
        }
    }
    
    printf("🚀 性能相关:     %d 个模式\n", category_stats[0].count);
    printf("🎨 设计模式:     %d 个模式\n", category_stats[1].count);
    printf("✨ 代码质量:     %d 个模式\n", category_stats[2].count);
    printf("🔒 安全相关:     %d 个模式\n", category_stats[3].count);
}

// 清理分析数据
static void cleanup_analysis_data(void) {
    if (g_matches) {
        for (int i = 0; i < g_match_count; i++) {
            free(g_matches[i].file_path);
            free(g_matches[i].matched_text);
        }
        free(g_matches);
        g_matches = NULL;
    }
    g_match_count = 0;
    g_match_capacity = 0;
}

// 获取分析统计
AnalysisStats* pattern_analyzer_get_stats(void) {
    return &g_stats;
}

// 导出分析结果到JSON格式
int pattern_analyzer_export_json(const char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"stage2_pattern_analysis\": {\n");
    fprintf(file, "    \"timestamp\": \"%ld\",\n", time(NULL));
    fprintf(file, "    \"statistics\": {\n");
    fprintf(file, "      \"total_files\": %d,\n", g_stats.total_files);
    fprintf(file, "      \"total_lines\": %d,\n", g_stats.total_lines);
    fprintf(file, "      \"total_patterns\": %d,\n", g_stats.total_patterns);
    fprintf(file, "      \"high_priority\": %d,\n", g_stats.high_priority_issues);
    fprintf(file, "      \"medium_priority\": %d,\n", g_stats.medium_priority_issues);
    fprintf(file, "      \"low_priority\": %d\n", g_stats.low_priority_issues);
    fprintf(file, "    },\n");
    fprintf(file, "    \"matches\": [\n");
    
    for (int i = 0; i < g_match_count; i++) {
        PatternMatch* match = &g_matches[i];
        fprintf(file, "      {\n");
        fprintf(file, "        \"pattern\": \"%s\",\n", match->pattern->name);
        fprintf(file, "        \"file\": \"%s\",\n", match->file_path);
        fprintf(file, "        \"line\": %d,\n", match->line_number);
        fprintf(file, "        \"confidence\": %d,\n", match->confidence);
        fprintf(file, "        \"category\": \"%s\",\n", match->pattern->category);
        fprintf(file, "        \"priority\": %d\n", match->pattern->priority);
        fprintf(file, "      }%s\n", (i < g_match_count - 1) ? "," : "");
    }
    
    fprintf(file, "    ]\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return 0;
}

// 命令行接口
int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("AI Pattern Analyzer - Stage 2 模式识别系统\n");
        printf("用法: %s [--export output.json]\n", argv[0]);
        printf("功能: 分析Stage 1代码中的模式和优化机会\n");
        return 0;
    }
    
    // 运行分析
    int result = pattern_analyzer_run();
    
    // 导出结果
    if (argc > 2 && strcmp(argv[1], "--export") == 0) {
        if (pattern_analyzer_export_json(argv[2]) == 0) {
            printf("📄 分析结果已导出到: %s\n", argv[2]);
        } else {
            printf("❌ 导出失败\n");
        }
    }
    
    return result;
}