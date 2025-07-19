/*
 * Refactoring Advisor - Stage 2 AI模式识别系统
 * T1.4: 重构机会识别器
 * 
 * 功能: 识别Stage 1代码中的重构机会和代码质量改进点
 * 特性: 代码异味检测、重构建议生成、质量度量分析、技术债务评估
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <math.h>

// 重构顾问头文件
#include "refactoring_advisor.h"

// 代码异味定义
typedef struct CodeSmell {
    const char* name;                 // 异味名称
    const char* description;          // 异味描述
    const char* detection_pattern;    // 检测模式
    int severity;                    // 严重程度 (1-10)
    const char* refactoring_method;   // 重构方法
    double complexity_reduction;     // 复杂度减少百分比
    const char* category;            // 异味类别
} CodeSmell;

// 代码异味数据库
static CodeSmell CODE_SMELLS[] = {
    // 函数级异味
    {
        "Long Method",
        "函数过长，超过50行",
        "\\w+\\s*\\([^)]*\\)\\s*{([^{}]*{[^{}]*}[^{}]*|[^{}]){50,}",
        7,
        "提取方法(Extract Method)，分解为多个小函数",
        25.0,
        "FUNCTION_LEVEL"
    },
    
    {
        "Too Many Parameters",
        "函数参数过多，超过5个",
        "\\w+\\s*\\([^)]*,[^)]*,[^)]*,[^)]*,[^)]*,[^)]*\\)",
        6,
        "引入参数对象(Introduce Parameter Object)",
        20.0,
        "FUNCTION_LEVEL"
    },
    
    {
        "Large Class/Structure",
        "结构体或类过大",
        "struct\\s+\\w+\\s*{([^{}]*;[^{}]*){20,}}|typedef\\s+struct[^}]{500,}",
        8,
        "提取类(Extract Class)，分解数据结构",
        30.0,
        "CLASS_LEVEL"
    },
    
    {
        "God Function",
        "上帝函数，控制过多逻辑",
        "\\w+\\s*\\([^)]*\\)\\s*{([^{}]*if[^{}]*){5,}",
        9,
        "分解函数职责，提取专门的处理函数",
        40.0,
        "FUNCTION_LEVEL"
    },
    
    // 重复代码异味
    {
        "Duplicate Code",
        "重复代码块",
        "for\\s*\\([^}]*\\)\\s*{[^}]*printf[^}]*}.*for\\s*\\([^}]*\\)\\s*{[^}]*printf[^}]*}",
        8,
        "提取方法(Extract Method)，消除重复",
        35.0,
        "DUPLICATION"
    },
    
    {
        "Similar Functions",
        "相似函数结构",
        "(\\w+_init[^}]*}[^}]*\\w+_init|\\w+_create[^}]*}[^}]*\\w+_create)",
        6,
        "提取公共部分，使用模板方法模式",
        25.0,
        "DUPLICATION"
    },
    
    {
        "Copy-Paste Programming",
        "复制粘贴编程",
        "//\\s*copy|//\\s*duplicate|//\\s*same\\s+as",
        7,
        "重构为可复用的函数或宏",
        30.0,
        "DUPLICATION"
    },
    
    // 命名异味
    {
        "Meaningless Names",
        "无意义的变量名",
        "\\b(tmp|temp|data|info|value|val|x|y|z|i|j|k)\\b(?![a-zA-Z])",
        5,
        "重命名变量(Rename Variable)，使用有意义的名称",
        15.0,
        "NAMING"
    },
    
    {
        "Hungarian Notation Abuse",
        "匈牙利命名法滥用",
        "\\b(str|int|ptr|bool|char)\\w+\\b",
        4,
        "使用描述性命名，避免类型前缀",
        10.0,
        "NAMING"
    },
    
    {
        "Inconsistent Naming",
        "命名不一致",
        "(\\w*_init[^}]*\\w*Init|\\w*_create[^}]*\\w*Create)",
        6,
        "统一命名约定，保持一致性",
        20.0,
        "NAMING"
    },
    
    // 注释异味
    {
        "Commented Out Code",
        "被注释的代码",
        "//\\s*\\w+\\s*\\([^)]*\\)|/\\*[^*]*\\w+\\s*\\([^)]*\\)[^*]*\\*/",
        6,
        "删除注释代码，使用版本控制",
        15.0,
        "COMMENTS"
    },
    
    {
        "Lack of Comments",
        "缺乏注释的复杂逻辑",
        "if\\s*\\([^)]{20,}\\)\\s*{[^}]{50,}}",
        5,
        "添加解释性注释，提高可读性",
        10.0,
        "COMMENTS"
    },
    
    {
        "Obvious Comments",
        "显而易见的注释",
        "//\\s*increment\\s*i|//\\s*return\\s*\\w+|//\\s*set\\s*\\w+",
        3,
        "删除冗余注释，保留有价值的说明",
        5.0,
        "COMMENTS"
    },
    
    // 错误处理异味
    {
        "Ignored Return Values",
        "忽略返回值",
        "malloc\\s*\\([^)]*\\);|fopen\\s*\\([^)]*\\);|printf\\s*\\([^)]*\\);",
        8,
        "检查返回值，添加错误处理",
        25.0,
        "ERROR_HANDLING"
    },
    
    {
        "Magic Numbers",
        "魔法数字",
        "\\b(\\d{2,}|0x[0-9A-Fa-f]{3,})\\b(?!\\s*[;})])",
        6,
        "定义常量(Define Constants)，提高可读性",
        15.0,
        "ERROR_HANDLING"
    },
    
    {
        "Resource Leaks",
        "资源泄漏风险",
        "fopen\\s*\\([^}]*(?!fclose)|malloc\\s*\\([^}]*(?!free)",
        9,
        "确保资源释放，使用RAII模式",
        30.0,
        "ERROR_HANDLING"
    },
    
    // 编译器特定异味
    {
        "Deep Nesting",
        "过深的嵌套层次",
        "if\\s*\\([^{]*{[^{}]*if\\s*\\([^{]*{[^{}]*if\\s*\\([^{]*{",
        7,
        "早期返回(Early Return)，减少嵌套",
        25.0,
        "COMPILER_SPECIFIC"
    },
    
    {
        "Switch Statement Smell",
        "Switch语句异味",
        "switch\\s*\\([^{]*{([^}]*case[^}]*){8,}}",
        6,
        "使用多态或查找表替代",
        20.0,
        "COMPILER_SPECIFIC"
    },
    
    {
        "Global Variable Abuse",
        "全局变量滥用",
        "^\\s*(static\\s+)?\\w+\\s+\\w+\\s*=.*?;.*^\\s*(static\\s+)?\\w+\\s+\\w+\\s*=.*?;",
        8,
        "封装到结构体或使用依赖注入",
        30.0,
        "COMPILER_SPECIFIC"
    },
    
    {NULL, NULL, NULL, 0, NULL, 0.0, NULL}  // 结束标记
};

// 重构建议实例
typedef struct RefactoringOpportunity {
    CodeSmell* smell;
    char* file_path;
    int line_number;
    char* function_name;
    char* code_snippet;
    double urgency_score;
    char* specific_recommendation;
    int estimated_effort_hours;
} RefactoringOpportunity;

// 代码质量统计
typedef struct QualityMetrics {
    int total_smells;
    int high_severity_smells;
    int function_level_issues;
    int duplication_issues;
    int naming_issues;
    int comment_issues;
    int error_handling_issues;
    double overall_quality_score;
    double technical_debt_hours;
} QualityMetrics;

// 全局状态
static RefactoringOpportunity* g_opportunities = NULL;
static int g_opportunity_count = 0;
static int g_opportunity_capacity = 0;
static QualityMetrics g_quality_metrics = {0};

// 分析目标
static const char* REFACTORING_ANALYSIS_TARGETS[] = {
    "src/core/modules/pipeline_module.c",     // 编译流水线 - 复杂度高
    "src/core/modules/c99bin_module.c",       // 编译器核心 - 功能密集
    "src/core/modules/compiler_module.c",     // JIT编译器 - 算法复杂
    "src/core/modules/libc_module.c",         // 标准库 - 接口多样
    "src/core/modules/module_module.c",       // 模块管理 - 逻辑复杂
    "src/layer1/simple_loader.c",             // 加载器 - 关键路径
    "tools/c99bin.c",                         // 编译器工具
    NULL
};

// 函数声明
static int analyze_refactoring_opportunities(void);
static int scan_file_for_code_smells(const char* file_path);
static int detect_code_smells(const char* file_path, const char* content);
static int add_refactoring_opportunity(CodeSmell* smell, const char* file_path,
                                     int line_number, const char* function_name,
                                     const char* code_snippet, double urgency);
static void calculate_quality_metrics(void);
static void generate_refactoring_plan(void);
static void cleanup_refactoring_data(void);
static double calculate_urgency_score(CodeSmell* smell, const char* context);
static char* generate_specific_recommendation(CodeSmell* smell, const char* context);

// 主重构分析函数
int refactoring_advisor_run(void) {
    printf("🔧 AI Refactoring Advisor - Stage 2 重构机会识别器启动\n");
    printf("======================================================\n");
    
    // 初始化数据结构
    g_opportunity_capacity = 200;
    g_opportunities = calloc(g_opportunity_capacity, sizeof(RefactoringOpportunity));
    if (!g_opportunities) {
        fprintf(stderr, "Error: 无法分配内存用于重构分析\n");
        return -1;
    }
    
    // 分析重构机会
    printf("🔍 开始代码异味检测和重构机会分析...\n");
    if (analyze_refactoring_opportunities() < 0) {
        fprintf(stderr, "重构机会分析失败\n");
        cleanup_refactoring_data();
        return -1;
    }
    
    // 计算质量指标
    printf("📊 计算代码质量指标...\n");
    calculate_quality_metrics();
    
    // 生成重构计划
    printf("📋 生成重构改进计划...\n");
    generate_refactoring_plan();
    
    // 清理资源
    cleanup_refactoring_data();
    
    printf("\n🎯 重构机会识别完成！发现 %d 个改进机会\n", g_opportunity_count);
    return 0;
}

// 分析重构机会
static int analyze_refactoring_opportunities(void) {
    for (int i = 0; REFACTORING_ANALYSIS_TARGETS[i]; i++) {
        const char* target = REFACTORING_ANALYSIS_TARGETS[i];
        printf("   分析: %s\n", target);
        
        if (scan_file_for_code_smells(target) < 0) {
            printf("   ⚠️  跳过: %s (文件不存在或无法读取)\n", target);
        }
    }
    return 0;
}

// 扫描文件寻找代码异味
static int scan_file_for_code_smells(const char* file_path) {
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
    
    // 检测代码异味
    int smells = detect_code_smells(file_path, content);
    
    free(content);
    return smells;
}

// 检测代码异味
static int detect_code_smells(const char* file_path, const char* content) {
    int total_smells = 0;
    
    for (int i = 0; CODE_SMELLS[i].name; i++) {
        CodeSmell* smell = &CODE_SMELLS[i];
        regex_t regex;
        
        // 编译正则表达式
        if (regcomp(&regex, smell->detection_pattern, REG_EXTENDED | REG_ICASE) != 0) {
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
            char function_name[256] = "unknown";
            const char* line_start = search_start + match.rm_so;
            while (line_start > content && *(line_start-1) != '\n') line_start--;
            
            for (const char* p = line_start; p >= content - 500 && p >= content; p--) {
                if (sscanf(p, "%*s %255[a-zA-Z_][a-zA-Z0-9_]*\\s*\\(", function_name) == 1) {
                    break;
                }
            }
            
            // 提取代码片段
            int snippet_start = (search_start + match.rm_so - content > 80) ? 
                               (search_start + match.rm_so - content - 80) : 0;
            int snippet_end = (search_start + match.rm_eo - content + 80 < strlen(content)) ?
                             (search_start + match.rm_eo - content + 80) : strlen(content);
            char* code_snippet = strndup(content + snippet_start, snippet_end - snippet_start);
            
            // 计算紧急度评分
            double urgency = calculate_urgency_score(smell, code_snippet);
            
            // 添加重构机会
            add_refactoring_opportunity(smell, file_path, line_number, function_name,
                                       code_snippet, urgency);
            total_smells++;
            
            // 继续搜索
            search_start += match.rm_eo;
            if (*search_start == '\0') break;
            
            free(code_snippet);
        }
        
        regfree(&regex);
    }
    
    return total_smells;
}

// 计算紧急度评分
static double calculate_urgency_score(CodeSmell* smell, const char* context) {
    double base_urgency = smell->severity * 10.0;
    
    // 根据上下文调整紧急度
    if (strstr(context, "critical") || strstr(context, "important")) {
        base_urgency *= 1.5;
    }
    
    if (strstr(context, "TODO") || strstr(context, "FIXME") || strstr(context, "HACK")) {
        base_urgency *= 1.3;
    }
    
    if (strstr(context, "main") || strstr(context, "init") || strstr(context, "load")) {
        base_urgency *= 1.2; // 关键函数的问题更紧急
    }
    
    // 基于复杂度调整
    int complexity_indicators = 0;
    if (strstr(context, "if")) complexity_indicators++;
    if (strstr(context, "for")) complexity_indicators++;
    if (strstr(context, "while")) complexity_indicators++;
    if (strstr(context, "switch")) complexity_indicators++;
    
    base_urgency *= (1.0 + complexity_indicators * 0.1);
    
    return base_urgency;
}

// 生成具体建议
static char* generate_specific_recommendation(CodeSmell* smell, const char* context) {
    char* recommendation = malloc(512);
    if (!recommendation) return strdup(smell->refactoring_method);
    
    // 基于上下文生成更具体的建议
    if (strcmp(smell->name, "Long Method") == 0) {
        snprintf(recommendation, 512, 
                "%s。建议拆分为3-4个职责单一的小函数，每个不超过15行。", 
                smell->refactoring_method);
    } else if (strcmp(smell->name, "Magic Numbers") == 0) {
        snprintf(recommendation, 512,
                "%s。定义具有描述性名称的常量，如#define MAX_BUFFER_SIZE 1024。",
                smell->refactoring_method);
    } else if (strcmp(smell->name, "Duplicate Code") == 0) {
        snprintf(recommendation, 512,
                "%s。考虑创建通用函数或使用宏来消除重复逻辑。",
                smell->refactoring_method);
    } else {
        snprintf(recommendation, 512, "%s", smell->refactoring_method);
    }
    
    return recommendation;
}

// 添加重构机会
static int add_refactoring_opportunity(CodeSmell* smell, const char* file_path,
                                     int line_number, const char* function_name,
                                     const char* code_snippet, double urgency) {
    // 扩展容量
    if (g_opportunity_count >= g_opportunity_capacity) {
        g_opportunity_capacity *= 2;
        g_opportunities = realloc(g_opportunities, 
                                g_opportunity_capacity * sizeof(RefactoringOpportunity));
        if (!g_opportunities) {
            return -1;
        }
    }
    
    // 添加机会
    RefactoringOpportunity* opportunity = &g_opportunities[g_opportunity_count];
    opportunity->smell = smell;
    opportunity->file_path = strdup(file_path);
    opportunity->line_number = line_number;
    opportunity->function_name = strdup(function_name);
    opportunity->code_snippet = strdup(code_snippet);
    opportunity->urgency_score = urgency;
    opportunity->specific_recommendation = generate_specific_recommendation(smell, code_snippet);
    opportunity->estimated_effort_hours = smell->severity * 2; // 简单估算
    
    g_opportunity_count++;
    return 0;
}

// 计算质量指标
static void calculate_quality_metrics(void) {
    g_quality_metrics.total_smells = g_opportunity_count;
    g_quality_metrics.high_severity_smells = 0;
    g_quality_metrics.function_level_issues = 0;
    g_quality_metrics.duplication_issues = 0;
    g_quality_metrics.naming_issues = 0;
    g_quality_metrics.comment_issues = 0;
    g_quality_metrics.error_handling_issues = 0;
    g_quality_metrics.technical_debt_hours = 0.0;
    
    for (int i = 0; i < g_opportunity_count; i++) {
        RefactoringOpportunity* opportunity = &g_opportunities[i];
        
        // 统计高严重度问题
        if (opportunity->smell->severity >= 7) {
            g_quality_metrics.high_severity_smells++;
        }
        
        // 累计技术债务工时
        g_quality_metrics.technical_debt_hours += opportunity->estimated_effort_hours;
        
        // 分类统计
        const char* category = opportunity->smell->category;
        if (strcmp(category, "FUNCTION_LEVEL") == 0) {
            g_quality_metrics.function_level_issues++;
        } else if (strcmp(category, "DUPLICATION") == 0) {
            g_quality_metrics.duplication_issues++;
        } else if (strcmp(category, "NAMING") == 0) {
            g_quality_metrics.naming_issues++;
        } else if (strcmp(category, "COMMENTS") == 0) {
            g_quality_metrics.comment_issues++;
        } else if (strcmp(category, "ERROR_HANDLING") == 0) {
            g_quality_metrics.error_handling_issues++;
        }
    }
    
    // 计算整体质量评分 (基于异味密度和严重程度)
    double severity_impact = g_quality_metrics.high_severity_smells * 10.0;
    double density_impact = g_quality_metrics.total_smells * 2.0;
    g_quality_metrics.overall_quality_score = 100.0 - (severity_impact + density_impact);
    if (g_quality_metrics.overall_quality_score < 0) {
        g_quality_metrics.overall_quality_score = 0;
    }
}

// 生成重构计划
static void generate_refactoring_plan(void) {
    printf("\n🔧 AI代码重构改进计划\n");
    printf("======================\n");
    printf("📊 发现代码异味: %d 个\n", g_quality_metrics.total_smells);
    printf("🔥 高严重度问题: %d 个\n", g_quality_metrics.high_severity_smells);
    printf("📈 整体代码质量: %.1f/100\n", g_quality_metrics.overall_quality_score);
    printf("⏱️  技术债务估算: %.1f 小时\n", g_quality_metrics.technical_debt_hours);
    
    // 分类统计
    printf("\n📊 问题类别分布:\n");
    printf("   🏗️  函数级问题: %d 项\n", g_quality_metrics.function_level_issues);
    printf("   📋 重复代码: %d 项\n", g_quality_metrics.duplication_issues);
    printf("   🏷️  命名问题: %d 项\n", g_quality_metrics.naming_issues);
    printf("   📝 注释问题: %d 项\n", g_quality_metrics.comment_issues);
    printf("   ⚠️  错误处理: %d 项\n", g_quality_metrics.error_handling_issues);
    
    // 按紧急度排序
    for (int i = 0; i < g_opportunity_count - 1; i++) {
        for (int j = i + 1; j < g_opportunity_count; j++) {
            if (g_opportunities[i].urgency_score < g_opportunities[j].urgency_score) {
                RefactoringOpportunity temp = g_opportunities[i];
                g_opportunities[i] = g_opportunities[j];
                g_opportunities[j] = temp;
            }
        }
    }
    
    // 显示前10个最紧急的重构建议
    printf("\n🎯 优先重构建议 (按紧急度排序):\n");
    int display_count = (g_opportunity_count > 10) ? 10 : g_opportunity_count;
    for (int i = 0; i < display_count; i++) {
        RefactoringOpportunity* opportunity = &g_opportunities[i];
        printf("   %d. %s\n", i+1, opportunity->smell->name);
        printf("      📍 位置: %s:%d (%s)\n", 
               opportunity->file_path, opportunity->line_number, opportunity->function_name);
        printf("      💡 描述: %s\n", opportunity->smell->description);
        printf("      🎯 紧急度: %.1f | 严重度: %d/10 | 预估工时: %d小时\n",
               opportunity->urgency_score, opportunity->smell->severity, 
               opportunity->estimated_effort_hours);
        printf("      🔧 重构建议: %s\n", opportunity->specific_recommendation);
        printf("      📂 类别: %s\n", opportunity->smell->category);
        printf("\n");
    }
    
    // 重构路线图
    printf("🗺️  重构实施路线图:\n");
    printf("   Phase 1 (紧急): 修复高严重度问题 (%d项, %.1f小时)\n", 
           g_quality_metrics.high_severity_smells,
           g_quality_metrics.high_severity_smells * 10.0);
    printf("   Phase 2 (重要): 消除重复代码 (%d项)\n", 
           g_quality_metrics.duplication_issues);
    printf("   Phase 3 (改进): 函数级重构 (%d项)\n", 
           g_quality_metrics.function_level_issues);
    printf("   Phase 4 (优化): 命名和注释改进 (%d项)\n", 
           g_quality_metrics.naming_issues + g_quality_metrics.comment_issues);
    
    // 质量改进预期
    double expected_improvement = 0.0;
    for (int i = 0; i < g_opportunity_count; i++) {
        expected_improvement += g_opportunities[i].smell->complexity_reduction;
    }
    printf("\n📈 预期改进效果:\n");
    printf("   代码复杂度减少: %.1f%%\n", expected_improvement);
    printf("   可维护性提升: %.1f%%\n", expected_improvement * 0.8);
    printf("   代码质量提升: +%.1f分\n", expected_improvement * 0.3);
}

// 导出重构分析结果
int refactoring_advisor_export_json(const char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"stage2_refactoring_analysis\": {\n");
    fprintf(file, "    \"timestamp\": \"%ld\",\n", time(NULL));
    fprintf(file, "    \"quality_metrics\": {\n");
    fprintf(file, "      \"total_smells\": %d,\n", g_quality_metrics.total_smells);
    fprintf(file, "      \"high_severity_smells\": %d,\n", g_quality_metrics.high_severity_smells);
    fprintf(file, "      \"overall_quality_score\": %.2f,\n", g_quality_metrics.overall_quality_score);
    fprintf(file, "      \"technical_debt_hours\": %.2f,\n", g_quality_metrics.technical_debt_hours);
    fprintf(file, "      \"function_level_issues\": %d,\n", g_quality_metrics.function_level_issues);
    fprintf(file, "      \"duplication_issues\": %d,\n", g_quality_metrics.duplication_issues);
    fprintf(file, "      \"naming_issues\": %d,\n", g_quality_metrics.naming_issues);
    fprintf(file, "      \"comment_issues\": %d,\n", g_quality_metrics.comment_issues);
    fprintf(file, "      \"error_handling_issues\": %d\n", g_quality_metrics.error_handling_issues);
    fprintf(file, "    },\n");
    fprintf(file, "    \"refactoring_opportunities\": [\n");
    
    for (int i = 0; i < g_opportunity_count; i++) {
        RefactoringOpportunity* opportunity = &g_opportunities[i];
        fprintf(file, "      {\n");
        fprintf(file, "        \"smell_name\": \"%s\",\n", opportunity->smell->name);
        fprintf(file, "        \"file\": \"%s\",\n", opportunity->file_path);
        fprintf(file, "        \"line\": %d,\n", opportunity->line_number);
        fprintf(file, "        \"function\": \"%s\",\n", opportunity->function_name);
        fprintf(file, "        \"severity\": %d,\n", opportunity->smell->severity);
        fprintf(file, "        \"urgency_score\": %.2f,\n", opportunity->urgency_score);
        fprintf(file, "        \"estimated_effort_hours\": %d,\n", opportunity->estimated_effort_hours);
        fprintf(file, "        \"category\": \"%s\"\n", opportunity->smell->category);
        fprintf(file, "      }%s\n", (i < g_opportunity_count - 1) ? "," : "");
    }
    
    fprintf(file, "    ]\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return 0;
}

// 清理数据
static void cleanup_refactoring_data(void) {
    if (g_opportunities) {
        for (int i = 0; i < g_opportunity_count; i++) {
            free(g_opportunities[i].file_path);
            free(g_opportunities[i].function_name);
            free(g_opportunities[i].code_snippet);
            free(g_opportunities[i].specific_recommendation);
        }
        free(g_opportunities);
        g_opportunities = NULL;
    }
    
    g_opportunity_count = 0;
    g_opportunity_capacity = 0;
}

// 命令行接口
int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("AI Refactoring Advisor - Stage 2 重构机会识别系统\n");
        printf("用法: %s [--export output.json]\n", argv[0]);
        printf("功能: 识别Stage 1代码中的重构机会和代码质量改进点\n");
        return 0;
    }
    
    // 运行重构分析
    int result = refactoring_advisor_run();
    
    // 导出结果
    if (argc > 2 && strcmp(argv[1], "--export") == 0) {
        if (refactoring_advisor_export_json(argv[2]) == 0) {
            printf("📄 重构分析结果已导出到: %s\n", argv[2]);
        } else {
            printf("❌ 导出失败\n");
        }
    }
    
    return result;
}