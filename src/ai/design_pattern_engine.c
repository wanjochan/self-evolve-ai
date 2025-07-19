/*
 * Design Pattern Recognition Engine - Stage 2 AI模式识别系统
 * T1.2: 设计模式识别引擎
 * 
 * 功能: 识别Stage 1代码中的设计模式和架构优化机会
 * 特性: 模式数据库、架构分析、重构建议生成
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <dirent.h>

// 设计模式识别引擎头文件
#include "design_pattern_engine.h"

// 设计模式定义
typedef struct DesignPattern {
    const char* name;              // 模式名称
    const char* description;       // 模式描述
    const char* signature_regex;   // 特征正则表达式
    const char* context_regex;     // 上下文匹配
    int complexity_score;         // 复杂度评分 (1-10)
    const char* benefits;         // 模式优势
    const char* recommendations;   // 重构建议
} DesignPattern;

// 设计模式数据库
static DesignPattern DESIGN_PATTERNS[] = {
    // 创建型模式
    {
        "Factory Pattern",
        "工厂模式: 创建对象的抽象接口",
        "create_\\w+\\s*\\([^)]*\\)\\s*{[^}]*switch|case.*return.*new",
        "typedef.*\\*.*create|\\w+_create_\\w+",
        7,
        "解耦对象创建，易于扩展新类型",
        "可以进一步抽象为抽象工厂模式"
    },
    
    {
        "Singleton Pattern", 
        "单例模式: 确保类只有一个实例",
        "static\\s+\\w+\\s*\\*\\s*instance\\s*=\\s*NULL|if\\s*\\(.*instance.*==.*NULL\\)",
        "get_instance|getInstance|static.*instance",
        5,
        "全局访问点，节省资源",
        "考虑线程安全和lazy initialization"
    },
    
    {
        "Builder Pattern",
        "建造者模式: 分步构建复杂对象", 
        "\\w+_builder.*{|build_\\w+|set_\\w+.*return.*this",
        "builder|Builder.*struct|chain.*call",
        8,
        "分离构建过程，支持复杂配置",
        "Stage 1模块初始化可以采用此模式"
    },
    
    // 结构型模式
    {
        "Module Pattern",
        "模块模式: 代码组织和封装",
        "typedef\\s+struct.*Module|\\w+_module\\s*{|load_module|module_\\w+",
        "module.*interface|module.*vtable|module.*ops",
        9,
        "Stage 1已采用，模块化架构的核心",
        "可以增强模块依赖管理和版本控制"
    },
    
    {
        "Adapter Pattern",
        "适配器模式: 接口转换和兼容",
        "\\w+_adapter|adapt_\\w+|wrapper_\\w+|convert_\\w+_to_\\w+",
        "interface.*conversion|compatibility.*layer",
        6,
        "接口兼容，系统集成",
        "编译器后端可以用适配器统一不同架构"
    },
    
    {
        "Facade Pattern",
        "外观模式: 简化复杂子系统接口",
        "\\w+_facade|simple_\\w+|unified_\\w+|\\w+_interface",
        "high.*level.*interface|simplified.*access",
        7,
        "简化使用，隐藏复杂性",
        "c99bin编译器接口是很好的外观模式例子"
    },
    
    // 行为型模式
    {
        "Strategy Pattern",
        "策略模式: 算法族的封装和互换",
        "typedef.*\\*.*strategy|\\w+_strategy|switch.*algorithm|select_\\w+",
        "algorithm.*selection|runtime.*choice|configurable",
        8,
        "算法灵活切换，易于扩展",
        "代码生成器已使用，可扩展到优化策略"
    },
    
    {
        "Observer Pattern", 
        "观察者模式: 事件通知机制",
        "callback\\s*\\(|notify\\s*\\(|register_\\w+|subscribe_\\w+|event_\\w+",
        "event.*handler|notification|listener|observer",
        7,
        "松耦合的事件处理",
        "错误处理系统可以增强观察者功能"
    },
    
    {
        "State Pattern",
        "状态模式: 状态相关行为的封装",
        "state_\\w+|\\w+_state|typedef.*State|switch.*state",
        "state.*machine|current.*state|transition",
        8,
        "状态管理清晰，易于维护",
        "编译器状态机可以更正式地采用此模式"
    },
    
    {
        "Command Pattern",
        "命令模式: 请求的封装和参数化",
        "execute\\s*\\(|command_\\w+|\\w+_command|typedef.*Command",
        "undo|redo|queue.*command|batch.*operation",
        7,
        "操作封装，支持撤销和批处理",
        "编译流水线可以增强命令模式支持"
    },
    
    // Stage 1特有模式
    {
        "Pipeline Pattern",
        "管道模式: 数据流处理链",
        "pipeline_\\w+|\\w+_pipeline|process_\\w+.*next|chain.*process",
        "stage.*process|filter.*chain|data.*flow",
        9,
        "Stage 1核心架构，处理流清晰",
        "可以增加异步处理和并行管道"
    },
    
    {
        "Plugin Pattern", 
        "插件模式: 动态功能扩展",
        "plugin_\\w+|load_\\w+\\.so|dlopen|dlsym|register_\\w+",
        "dynamic.*loading|runtime.*extension|modular",
        8,
        "动态扩展，模块热插拔",
        "模块系统已实现，可以增强插件注册机制"
    },
    
    {NULL, NULL, NULL, NULL, 0, NULL, NULL}  // 结束标记
};

// 模式匹配结果
typedef struct PatternMatch {
    DesignPattern* pattern;
    char* file_path;
    int line_number;
    char* matched_code;
    int confidence;
    char* context;
    int architecture_impact;  // 架构影响评分 1-10
} PatternMatch;

// 架构分析结果
typedef struct ArchitectureAnalysis {
    int total_patterns;
    int design_quality_score;    // 设计质量评分 1-100
    int maintainability_score;   // 可维护性评分 1-100
    int extensibility_score;     // 可扩展性评分 1-100
    char* recommendations[10];   // 架构建议
    int recommendation_count;
} ArchitectureAnalysis;

// 全局状态
static PatternMatch* g_pattern_matches = NULL;
static int g_match_count = 0;
static int g_match_capacity = 0;
static ArchitectureAnalysis g_architecture = {0};

// Stage 1分析目标 (与pattern_analyzer保持一致)
static const char* ANALYSIS_TARGETS[] = {
    "src/core/modules/pipeline_module.c",
    "src/core/modules/c99bin_module.c", 
    "src/core/modules/compiler_module.c",
    "src/core/modules/libc_module.c",
    "src/core/modules/module_module.c",
    "src/layer1/simple_loader.c",
    "tools/c99bin.c",
    NULL
};

// 函数声明
static int analyze_design_patterns(void);
static int scan_file_for_patterns(const char* file_path);
static int match_pattern_in_content(const char* file_path, const char* content, DesignPattern* pattern);
static int add_pattern_match(DesignPattern* pattern, const char* file_path, int line_number, 
                           const char* matched_code, int confidence, const char* context);
static void analyze_architecture_quality(void);
static void generate_recommendations(void);
static void print_design_analysis_report(void);
static void cleanup_design_data(void);

// 主分析函数
int design_pattern_engine_run(void) {
    printf("🎨 AI Design Pattern Engine - Stage 2 设计模式识别引擎启动\n");
    printf("=======================================================\n");
    
    // 初始化数据结构
    g_match_capacity = 500;
    g_pattern_matches = calloc(g_match_capacity, sizeof(PatternMatch));
    if (!g_pattern_matches) {
        fprintf(stderr, "Error: 无法分配内存用于模式匹配\n");
        return -1;
    }
    
    // 分析设计模式
    printf("🔍 开始设计模式识别...\n");
    if (analyze_design_patterns() < 0) {
        fprintf(stderr, "设计模式分析失败\n");
        cleanup_design_data();
        return -1;
    }
    
    // 架构质量分析
    printf("📐 进行架构质量分析...\n");
    analyze_architecture_quality();
    
    // 生成改进建议
    printf("💡 生成架构改进建议...\n");
    generate_recommendations();
    
    // 输出分析报告
    print_design_analysis_report();
    
    // 清理资源
    cleanup_design_data();
    
    printf("\n🎯 设计模式识别完成！发现 %d 个设计模式\n", g_match_count);
    return 0;
}

// 分析设计模式
static int analyze_design_patterns(void) {
    for (int i = 0; ANALYSIS_TARGETS[i]; i++) {
        const char* target = ANALYSIS_TARGETS[i];
        printf("   扫描: %s\n", target);
        
        if (scan_file_for_patterns(target) < 0) {
            printf("   ⚠️  跳过: %s (文件不存在或无法读取)\n", target);
        }
    }
    return 0;
}

// 扫描单个文件的设计模式
static int scan_file_for_patterns(const char* file_path) {
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
    
    // 匹配所有设计模式
    int matches = 0;
    for (int i = 0; DESIGN_PATTERNS[i].name; i++) {
        matches += match_pattern_in_content(file_path, content, &DESIGN_PATTERNS[i]);
    }
    
    free(content);
    return matches;
}

// 在内容中匹配特定设计模式
static int match_pattern_in_content(const char* file_path, const char* content, DesignPattern* pattern) {
    regex_t signature_regex, context_regex;
    int matches = 0;
    
    // 编译签名正则表达式
    if (regcomp(&signature_regex, pattern->signature_regex, REG_EXTENDED | REG_ICASE) != 0) {
        return 0;
    }
    
    // 编译上下文正则表达式 (可选)
    int has_context = pattern->context_regex && 
                     regcomp(&context_regex, pattern->context_regex, REG_EXTENDED | REG_ICASE) == 0;
    
    // 查找签名匹配
    regmatch_t match;
    const char* search_start = content;
    
    while (regexec(&signature_regex, search_start, 1, &match, 0) == 0) {
        // 计算行号
        int line_number = 1;
        for (const char* p = content; p < search_start + match.rm_so; p++) {
            if (*p == '\n') line_number++;
        }
        
        // 提取匹配的代码
        int match_len = match.rm_eo - match.rm_so;
        char* matched_code = strndup(search_start + match.rm_so, match_len);
        
        // 计算置信度
        int confidence = 70; // 基础置信度
        
        // 检查上下文匹配
        char* context = "";
        if (has_context) {
            // 获取匹配点周围的上下文 (前后500字符)
            int context_start = (search_start + match.rm_so - content > 500) ? 
                               (search_start + match.rm_so - content - 500) : 0;
            int context_end = (search_start + match.rm_eo - content + 500 < strlen(content)) ?
                             (search_start + match.rm_eo - content + 500) : strlen(content);
            
            context = strndup(content + context_start, context_end - context_start);
            
            // 检查上下文是否匹配
            regmatch_t context_match;
            if (regexec(&context_regex, context, 1, &context_match, 0) == 0) {
                confidence += 20; // 上下文匹配增加置信度
            }
        }
        
        // 根据模式复杂度调整置信度
        confidence += pattern->complexity_score;
        if (confidence > 100) confidence = 100;
        
        // 添加到匹配结果
        add_pattern_match(pattern, file_path, line_number, matched_code, confidence, context);
        matches++;
        
        // 继续搜索
        search_start += match.rm_eo;
        if (*search_start == '\0') break;
        
        free(matched_code);
        if (has_context && context != "") free(context);
    }
    
    regfree(&signature_regex);
    if (has_context) regfree(&context_regex);
    
    return matches;
}

// 添加模式匹配结果
static int add_pattern_match(DesignPattern* pattern, const char* file_path, int line_number,
                           const char* matched_code, int confidence, const char* context) {
    // 扩展容量
    if (g_match_count >= g_match_capacity) {
        g_match_capacity *= 2;
        g_pattern_matches = realloc(g_pattern_matches, g_match_capacity * sizeof(PatternMatch));
        if (!g_pattern_matches) {
            return -1;
        }
    }
    
    // 添加匹配
    PatternMatch* match = &g_pattern_matches[g_match_count];
    match->pattern = pattern;
    match->file_path = strdup(file_path);
    match->line_number = line_number;
    match->matched_code = strdup(matched_code);
    match->confidence = confidence;
    match->context = strdup(context ? context : "");
    match->architecture_impact = pattern->complexity_score; // 架构影响等于复杂度
    
    g_match_count++;
    return 0;
}

// 分析架构质量
static void analyze_architecture_quality(void) {
    g_architecture.total_patterns = g_match_count;
    
    // 计算设计质量评分
    int total_complexity = 0;
    int high_quality_patterns = 0;
    
    for (int i = 0; i < g_match_count; i++) {
        PatternMatch* match = &g_pattern_matches[i];
        total_complexity += match->pattern->complexity_score;
        
        if (match->confidence > 80 && match->pattern->complexity_score >= 7) {
            high_quality_patterns++;
        }
    }
    
    // 设计质量评分 (基于高质量模式比例和平均复杂度)
    g_architecture.design_quality_score = (g_match_count > 0) ? 
        (high_quality_patterns * 100 / g_match_count + total_complexity * 10 / g_match_count) / 2 : 0;
    
    // 可维护性评分 (基于模式种类多样性)
    int unique_patterns = 0;
    for (int i = 0; DESIGN_PATTERNS[i].name; i++) {
        for (int j = 0; j < g_match_count; j++) {
            if (g_pattern_matches[j].pattern == &DESIGN_PATTERNS[i]) {
                unique_patterns++;
                break;
            }
        }
    }
    g_architecture.maintainability_score = unique_patterns * 8; // 每种模式+8分
    
    // 可扩展性评分 (基于关键扩展性模式的存在)
    int extensibility_indicators = 0;
    const char* extensible_patterns[] = {"Factory Pattern", "Strategy Pattern", "Module Pattern", 
                                       "Plugin Pattern", "Observer Pattern", NULL};
    
    for (int i = 0; extensible_patterns[i]; i++) {
        for (int j = 0; j < g_match_count; j++) {
            if (strcmp(g_pattern_matches[j].pattern->name, extensible_patterns[i]) == 0) {
                extensibility_indicators++;
                break;
            }
        }
    }
    g_architecture.extensibility_score = extensibility_indicators * 20; // 每个关键模式+20分
}

// 生成架构改进建议
static void generate_recommendations(void) {
    g_architecture.recommendation_count = 0;
    
    // 基于分析结果生成建议
    if (g_architecture.design_quality_score < 60) {
        g_architecture.recommendations[g_architecture.recommendation_count++] = 
            strdup("建议增加更多高级设计模式，提升代码架构质量");
    }
    
    if (g_architecture.maintainability_score < 40) {
        g_architecture.recommendations[g_architecture.recommendation_count++] = 
            strdup("建议采用更多结构型模式，提升代码可维护性");
    }
    
    if (g_architecture.extensibility_score < 60) {
        g_architecture.recommendations[g_architecture.recommendation_count++] = 
            strdup("建议增强Factory和Strategy模式，提升系统可扩展性");
    }
    
    // 基于发现的模式生成具体建议
    for (int i = 0; i < g_match_count; i++) {
        PatternMatch* match = &g_pattern_matches[i];
        if (match->confidence > 85 && match->pattern->recommendations && 
            g_architecture.recommendation_count < 10) {
            g_architecture.recommendations[g_architecture.recommendation_count++] = 
                strdup(match->pattern->recommendations);
        }
    }
}

// 打印设计分析报告
static void print_design_analysis_report(void) {
    printf("\n🎨 AI设计模式识别报告\n");
    printf("========================\n");
    printf("📊 发现设计模式: %d 个\n", g_architecture.total_patterns);
    printf("📈 设计质量评分: %d/100\n", g_architecture.design_quality_score);
    printf("🔧 可维护性评分: %d/100\n", g_architecture.maintainability_score);
    printf("🚀 可扩展性评分: %d/100\n", g_architecture.extensibility_score);
    
    // 显示高置信度的设计模式
    printf("\n🎯 识别到的设计模式:\n");
    for (int i = 0; i < g_match_count; i++) {
        PatternMatch* match = &g_pattern_matches[i];
        if (match->confidence > 75) {
            printf("   📍 %s:%d - %s\n", 
                   match->file_path, match->line_number, match->pattern->name);
            printf("      💬 %s\n", match->pattern->description);
            printf("      🎯 置信度: %d%% | 架构影响: %d/10\n", 
                   match->confidence, match->architecture_impact);
            printf("      ✨ 优势: %s\n", match->pattern->benefits);
            printf("\n");
        }
    }
    
    // 显示改进建议
    if (g_architecture.recommendation_count > 0) {
        printf("💡 架构改进建议:\n");
        for (int i = 0; i < g_architecture.recommendation_count; i++) {
            printf("   %d. %s\n", i+1, g_architecture.recommendations[i]);
        }
    }
}

// 导出设计分析结果
int design_pattern_engine_export_json(const char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"stage2_design_analysis\": {\n");
    fprintf(file, "    \"timestamp\": \"%ld\",\n", time(NULL));
    fprintf(file, "    \"architecture_quality\": {\n");
    fprintf(file, "      \"total_patterns\": %d,\n", g_architecture.total_patterns);
    fprintf(file, "      \"design_quality_score\": %d,\n", g_architecture.design_quality_score);
    fprintf(file, "      \"maintainability_score\": %d,\n", g_architecture.maintainability_score);
    fprintf(file, "      \"extensibility_score\": %d\n", g_architecture.extensibility_score);
    fprintf(file, "    },\n");
    fprintf(file, "    \"patterns\": [\n");
    
    for (int i = 0; i < g_match_count; i++) {
        PatternMatch* match = &g_pattern_matches[i];
        fprintf(file, "      {\n");
        fprintf(file, "        \"name\": \"%s\",\n", match->pattern->name);
        fprintf(file, "        \"file\": \"%s\",\n", match->file_path);
        fprintf(file, "        \"line\": %d,\n", match->line_number);
        fprintf(file, "        \"confidence\": %d,\n", match->confidence);
        fprintf(file, "        \"architecture_impact\": %d\n", match->architecture_impact);
        fprintf(file, "      }%s\n", (i < g_match_count - 1) ? "," : "");
    }
    
    fprintf(file, "    ],\n");
    fprintf(file, "    \"recommendations\": [\n");
    for (int i = 0; i < g_architecture.recommendation_count; i++) {
        fprintf(file, "      \"%s\"%s\n", 
                g_architecture.recommendations[i], 
                (i < g_architecture.recommendation_count - 1) ? "," : "");
    }
    fprintf(file, "    ]\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return 0;
}

// 清理数据
static void cleanup_design_data(void) {
    if (g_pattern_matches) {
        for (int i = 0; i < g_match_count; i++) {
            free(g_pattern_matches[i].file_path);
            free(g_pattern_matches[i].matched_code);
            free(g_pattern_matches[i].context);
        }
        free(g_pattern_matches);
        g_pattern_matches = NULL;
    }
    
    for (int i = 0; i < g_architecture.recommendation_count; i++) {
        free(g_architecture.recommendations[i]);
    }
    
    g_match_count = 0;
    g_match_capacity = 0;
}

// 命令行接口
int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("AI Design Pattern Engine - Stage 2 设计模式识别系统\n");
        printf("用法: %s [--export output.json]\n", argv[0]);
        printf("功能: 识别Stage 1代码中的设计模式和架构优化机会\n");
        return 0;
    }
    
    // 运行设计模式分析
    int result = design_pattern_engine_run();
    
    // 导出结果
    if (argc > 2 && strcmp(argv[1], "--export") == 0) {
        if (design_pattern_engine_export_json(argv[2]) == 0) {
            printf("📄 设计分析结果已导出到: %s\n", argv[2]);
        } else {
            printf("❌ 导出失败\n");
        }
    }
    
    return result;
}