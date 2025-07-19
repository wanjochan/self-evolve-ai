/*
 * Module Architecture Optimizer AI - Stage 2 AI优化引擎
 * T2.3: 模块架构优化AI
 * 
 * 功能: 优化Stage 1模块架构，提升模块化程度和系统可扩展性
 * 特性: 模块依赖分析、架构耦合度评估、模块重组建议、接口优化
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <dirent.h>

// 模块架构师头文件
#include "module_architect.h"

// 架构优化策略定义
typedef struct ArchitecturePattern {
    const char* name;                    // 模式名称
    const char* description;             // 模式描述
    const char* detection_signature;     // 检测特征
    int architecture_impact;            // 架构影响级别 (1-10)
    double modularity_improvement;      // 模块化改进百分比
    const char* optimization_strategy;  // 优化策略
    const char* pattern_category;       // 模式类别
} ArchitecturePattern;

// 模块架构优化模式数据库
static ArchitecturePattern ARCHITECTURE_PATTERNS[] = {
    // 模块解耦优化
    {
        "Circular Dependency Elimination",
        "消除模块间的循环依赖",
        "#include.*module.*#include.*pipeline|pipeline.*include.*module",
        9,
        35.0,
        "引入中间抽象层，打破循环依赖链",
        "DECOUPLING"
    },
    
    {
        "Interface Segregation",
        "接口隔离优化",
        "typedef\\s+struct.*{([^}]*\\w+\\s*\\([^)]*\\);[^}]*){5,}}",
        7,
        25.0,
        "拆分大接口为多个专门的小接口",
        "INTERFACE_DESIGN"
    },
    
    {
        "Dependency Injection",
        "依赖注入模式",
        "extern\\s+\\w+\\s*\\*|global\\s+\\w+\\s*\\*",
        8,
        30.0,
        "通过参数传递依赖，而非全局变量",
        "DECOUPLING"
    },
    
    // 模块组织优化
    {
        "Layer Architecture Enhancement",
        "分层架构增强",
        "src/layer\\d+.*#include.*src/layer\\d+",
        8,
        40.0,
        "严格控制层间依赖，上层依赖下层",
        "LAYERING"
    },
    
    {
        "Module Cohesion Improvement",
        "模块内聚性改进",
        "\\w+_module\\.c.*(?!\\w+_module)\\w+\\s*\\(",
        6,
        20.0,
        "将相关功能聚合到同一模块",
        "COHESION"
    },
    
    {
        "Plugin Architecture",
        "插件架构优化",
        "dlopen\\s*\\(|dlsym\\s*\\(|load_module",
        9,
        45.0,
        "标准化插件接口，提升可扩展性",
        "EXTENSIBILITY"
    },
    
    // 接口优化
    {
        "Facade Pattern Implementation",
        "外观模式实现",
        "typedef\\s+struct.*ops\\s*{|typedef\\s+struct.*interface\\s*{",
        7,
        25.0,
        "为复杂子系统提供统一简化接口",
        "INTERFACE_DESIGN"
    },
    
    {
        "Abstract Factory for Modules",
        "模块抽象工厂",
        "create_\\w+_module|\\w+_module_factory",
        8,
        30.0,
        "统一模块创建接口，支持动态切换",
        "CREATIONAL"
    },
    
    {
        "Module Registry Pattern",
        "模块注册模式",
        "register_\\w+|\\w+_registry|module_list",
        7,
        28.0,
        "中心化模块管理和发现机制",
        "MANAGEMENT"
    },
    
    // 通信优化
    {
        "Event-Driven Architecture",
        "事件驱动架构",
        "callback\\s*\\(|event_\\w+|notify_\\w+",
        8,
        35.0,
        "使用事件总线解耦模块间通信",
        "COMMUNICATION"
    },
    
    {
        "Message Queue Integration",
        "消息队列集成",
        "queue_\\w+|message_\\w+|async_\\w+",
        7,
        30.0,
        "异步消息传递，提升系统响应性",
        "COMMUNICATION"
    },
    
    {
        "Command Pattern for Modules",
        "模块命令模式",
        "execute\\s*\\(|command_\\w+|invoke_\\w+",
        6,
        22.0,
        "封装模块操作为命令对象",
        "COMMUNICATION"
    },
    
    // 编译器特定架构
    {
        "Compiler Pipeline Optimization",
        "编译器流水线优化",
        "pipeline_\\w+.*stage|stage_\\w+.*pipeline",
        9,
        40.0,
        "优化编译阶段划分和数据流",
        "COMPILER_SPECIFIC"
    },
    
    {
        "AST Module Separation",
        "AST模块分离",
        "ast_\\w+.*parser|parser_\\w+.*ast",
        8,
        32.0,
        "分离AST构建、遍历、优化模块",
        "COMPILER_SPECIFIC"
    },
    
    {
        "Code Generation Abstraction",
        "代码生成抽象层",
        "codegen_\\w+|generate_\\w+_code",
        8,
        35.0,
        "抽象目标架构相关的代码生成",
        "COMPILER_SPECIFIC"
    },
    
    // 数据流优化
    {
        "Data Flow Architecture",
        "数据流架构优化",
        "process_\\w+.*data|data_\\w+.*flow",
        7,
        28.0,
        "明确数据在模块间的流动路径",
        "DATA_FLOW"
    },
    
    {
        "Shared State Minimization",
        "共享状态最小化",
        "static\\s+\\w+.*=|global\\s+\\w+.*=",
        8,
        30.0,
        "减少全局状态，使用局部化数据",
        "DATA_FLOW"
    },
    
    {NULL, NULL, NULL, 0, 0.0, NULL, NULL}  // 结束标记
};

// 架构优化建议实例
typedef struct ArchitectureOptimization {
    ArchitecturePattern* pattern;
    char* affected_modules[10];        // 受影响的模块
    int module_count;
    double current_coupling;           // 当前耦合度
    double target_coupling;            // 目标耦合度
    char* optimization_plan;           // 优化计划
    int implementation_complexity;     // 实现复杂度
    double roi_estimate;              // 投资回报估算
} ArchitectureOptimization;

// 模块架构统计
typedef struct ArchitectureMetrics {
    int total_modules;
    int total_optimizations;
    double overall_coupling;           // 整体耦合度
    double overall_cohesion;           // 整体内聚度
    double modularity_score;           // 模块化评分
    int interface_violations;          // 接口违反数
    int circular_dependencies;         // 循环依赖数
    double architecture_quality;       // 架构质量评分
} ArchitectureMetrics;

// 全局状态
static ArchitectureOptimization* g_optimizations = NULL;
static int g_optimization_count = 0;
static int g_optimization_capacity = 0;
static ArchitectureMetrics g_arch_metrics = {0};

// 分析目标模块
static const char* MODULE_ANALYSIS_TARGETS[] = {
    "src/core/modules/",               // 核心模块目录
    "src/layer1/",                     // Layer 1模块
    "src/layer3/",                     // Layer 3模块
    "tools/",                          // 工具模块
    NULL
};

// 函数声明
static int analyze_module_architecture(void);
static int scan_modules_in_directory(const char* dir_path);
static int detect_architecture_patterns(const char* file_path, const char* content);
static int add_architecture_optimization(ArchitecturePattern* pattern, 
                                       const char** modules, int count,
                                       double coupling, const char* plan);
static void calculate_architecture_metrics(void);
static void generate_architecture_plan(void);
static void cleanup_architecture_data(void);
static double calculate_module_coupling(const char* file_path, const char* content);
static char* generate_optimization_plan(ArchitecturePattern* pattern, const char** modules);

// 主模块架构优化函数
int module_architect_run(void) {
    printf("🏗️ AI Module Architect - Stage 2 模块架构优化AI启动\n");
    printf("===================================================\n");
    
    // 初始化数据结构
    g_optimization_capacity = 100;
    g_optimizations = calloc(g_optimization_capacity, sizeof(ArchitectureOptimization));
    if (!g_optimizations) {
        fprintf(stderr, "Error: 无法分配内存用于架构分析\n");
        return -1;
    }
    
    // 分析模块架构
    printf("🔍 开始模块架构分析...\n");
    if (analyze_module_architecture() < 0) {
        fprintf(stderr, "模块架构分析失败\n");
        cleanup_architecture_data();
        return -1;
    }
    
    // 计算架构指标
    printf("📊 计算架构质量指标...\n");
    calculate_architecture_metrics();
    
    // 生成优化方案
    printf("📋 生成架构优化方案...\n");
    generate_architecture_plan();
    
    // 清理资源
    cleanup_architecture_data();
    
    printf("\n🎯 模块架构优化分析完成！发现 %d 个优化机会\n", g_optimization_count);
    return 0;
}

// 分析模块架构
static int analyze_module_architecture(void) {
    for (int i = 0; MODULE_ANALYSIS_TARGETS[i]; i++) {
        const char* target = MODULE_ANALYSIS_TARGETS[i];
        printf("   分析目录: %s\n", target);
        
        if (scan_modules_in_directory(target) < 0) {
            printf("   ⚠️  跳过: %s (目录不存在或无法访问)\n", target);
        }
    }
    return 0;
}

// 扫描目录中的模块
static int scan_modules_in_directory(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        return -1;
    }
    
    struct dirent* entry;
    int total_files = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".c") || strstr(entry->d_name, ".h")) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s%s", dir_path, entry->d_name);
            
            printf("     扫描: %s\n", entry->d_name);
            
            FILE* file = fopen(full_path, "r");
            if (file) {
                // 读取文件内容
                fseek(file, 0, SEEK_END);
                long file_size = ftell(file);
                fseek(file, 0, SEEK_SET);
                
                if (file_size > 0 && file_size < 1024*1024) { // 限制1MB以内
                    char* content = malloc(file_size + 1);
                    if (content) {
                        size_t read_size = fread(content, 1, file_size, file);
                        content[read_size] = '\0';
                        
                        detect_architecture_patterns(full_path, content);
                        total_files++;
                        
                        free(content);
                    }
                }
                fclose(file);
            }
        }
    }
    
    closedir(dir);
    g_arch_metrics.total_modules += total_files;
    return total_files;
}

// 检测架构模式
static int detect_architecture_patterns(const char* file_path, const char* content) {
    int patterns_found = 0;
    
    for (int i = 0; ARCHITECTURE_PATTERNS[i].name; i++) {
        ArchitecturePattern* pattern = &ARCHITECTURE_PATTERNS[i];
        regex_t regex;
        
        // 编译正则表达式
        if (regcomp(&regex, pattern->detection_signature, REG_EXTENDED | REG_ICASE) != 0) {
            continue;
        }
        
        // 查找匹配
        if (regexec(&regex, content, 0, NULL, 0) == 0) {
            // 计算当前耦合度
            double coupling = calculate_module_coupling(file_path, content);
            
            // 估算目标耦合度
            double target_coupling = coupling * (1.0 - pattern->modularity_improvement / 100.0);
            
            // 生成优化计划
            const char* modules[2] = {file_path, NULL};
            char* plan = generate_optimization_plan(pattern, modules);
            
            // 添加优化机会
            add_architecture_optimization(pattern, modules, 1, coupling, plan);
            patterns_found++;
            
            free(plan);
        }
        
        regfree(&regex);
    }
    
    return patterns_found;
}

// 计算模块耦合度
static double calculate_module_coupling(const char* file_path, const char* content) {
    double coupling = 0.0;
    
    // 计算#include数量
    const char* include_ptr = content;
    while ((include_ptr = strstr(include_ptr, "#include")) != NULL) {
        coupling += 1.0;
        include_ptr += 8; // 跳过"#include"
    }
    
    // 计算extern引用数量
    const char* extern_ptr = content;
    while ((extern_ptr = strstr(extern_ptr, "extern")) != NULL) {
        coupling += 2.0; // extern权重更高
        extern_ptr += 6;
    }
    
    // 计算全局变量使用
    const char* global_ptr = content;
    while ((global_ptr = strstr(global_ptr, "g_")) != NULL) {
        coupling += 1.5;
        global_ptr += 2;
    }
    
    // 标准化到0-100范围
    coupling = coupling > 50 ? 100.0 : (coupling / 50.0 * 100.0);
    
    return coupling;
}

// 生成优化计划
static char* generate_optimization_plan(ArchitecturePattern* pattern, const char** modules) {
    char* plan = malloc(1024);
    if (!plan) return strdup("优化计划生成失败");
    
    if (strcmp(pattern->pattern_category, "DECOUPLING") == 0) {
        snprintf(plan, 1024,
                "模块解耦计划:\n"
                "1. 分析当前模块间依赖关系\n"
                "2. %s\n"
                "3. 定义清晰的模块接口\n"
                "4. 重构代码消除直接依赖\n"
                "5. 验证解耦效果",
                pattern->optimization_strategy);
    } else if (strcmp(pattern->pattern_category, "INTERFACE_DESIGN") == 0) {
        snprintf(plan, 1024,
                "接口优化计划:\n"
                "1. 识别当前接口的职责范围\n"
                "2. %s\n"
                "3. 设计新的接口层次结构\n"
                "4. 逐步迁移现有代码\n"
                "5. 测试接口兼容性",
                pattern->optimization_strategy);
    } else if (strcmp(pattern->pattern_category, "COMPILER_SPECIFIC") == 0) {
        snprintf(plan, 1024,
                "编译器架构优化计划:\n"
                "1. 梳理编译流程和数据流\n"
                "2. %s\n"
                "3. 重新设计模块边界\n"
                "4. 实现新的架构模式\n"
                "5. 性能验证和调优",
                pattern->optimization_strategy);
    } else {
        snprintf(plan, 1024,
                "通用架构优化计划:\n"
                "1. 评估当前架构状态\n"
                "2. %s\n"
                "3. 制定迁移策略\n"
                "4. 分阶段实施改进\n"
                "5. 监控优化效果",
                pattern->optimization_strategy);
    }
    
    return plan;
}

// 添加架构优化
static int add_architecture_optimization(ArchitecturePattern* pattern,
                                       const char** modules, int count,
                                       double coupling, const char* plan) {
    // 扩展容量
    if (g_optimization_count >= g_optimization_capacity) {
        g_optimization_capacity *= 2;
        g_optimizations = realloc(g_optimizations,
                                g_optimization_capacity * sizeof(ArchitectureOptimization));
        if (!g_optimizations) {
            return -1;
        }
    }
    
    // 添加优化
    ArchitectureOptimization* opt = &g_optimizations[g_optimization_count];
    opt->pattern = pattern;
    opt->module_count = count < 10 ? count : 10;
    for (int i = 0; i < opt->module_count; i++) {
        opt->affected_modules[i] = strdup(modules[i]);
    }
    opt->current_coupling = coupling;
    opt->target_coupling = coupling * (1.0 - pattern->modularity_improvement / 100.0);
    opt->optimization_plan = strdup(plan);
    opt->implementation_complexity = pattern->architecture_impact;
    opt->roi_estimate = pattern->modularity_improvement / pattern->architecture_impact;
    
    g_optimization_count++;
    return 0;
}

// 计算架构指标
static void calculate_architecture_metrics(void) {
    g_arch_metrics.total_optimizations = g_optimization_count;
    g_arch_metrics.overall_coupling = 0.0;
    g_arch_metrics.overall_cohesion = 0.0;
    g_arch_metrics.interface_violations = 0;
    g_arch_metrics.circular_dependencies = 0;
    
    // 计算平均耦合度
    for (int i = 0; i < g_optimization_count; i++) {
        g_arch_metrics.overall_coupling += g_optimizations[i].current_coupling;
        
        // 统计违规项
        if (g_optimizations[i].current_coupling > 70.0) {
            g_arch_metrics.interface_violations++;
        }
        
        if (strstr(g_optimizations[i].pattern->name, "Circular")) {
            g_arch_metrics.circular_dependencies++;
        }
    }
    
    if (g_optimization_count > 0) {
        g_arch_metrics.overall_coupling /= g_optimization_count;
    }
    
    // 计算内聚度 (简化算法)
    g_arch_metrics.overall_cohesion = 100.0 - g_arch_metrics.overall_coupling;
    
    // 计算模块化评分
    g_arch_metrics.modularity_score = (g_arch_metrics.overall_cohesion + 
                                      (100.0 - g_arch_metrics.overall_coupling)) / 2.0;
    
    // 计算架构质量
    double violation_penalty = g_arch_metrics.interface_violations * 5.0;
    double dependency_penalty = g_arch_metrics.circular_dependencies * 10.0;
    g_arch_metrics.architecture_quality = g_arch_metrics.modularity_score - 
                                         violation_penalty - dependency_penalty;
    if (g_arch_metrics.architecture_quality < 0) {
        g_arch_metrics.architecture_quality = 0;
    }
}

// 生成架构方案
static void generate_architecture_plan(void) {
    printf("\n🏗️ AI模块架构优化方案\n");
    printf("======================\n");
    printf("📊 分析模块数: %d 个\n", g_arch_metrics.total_modules);
    printf("🔧 优化机会: %d 个\n", g_arch_metrics.total_optimizations);
    printf("📈 整体耦合度: %.1f/100\n", g_arch_metrics.overall_coupling);
    printf("🎯 整体内聚度: %.1f/100\n", g_arch_metrics.overall_cohesion);
    printf("🏆 模块化评分: %.1f/100\n", g_arch_metrics.modularity_score);
    printf("⚠️  接口违规: %d 项\n", g_arch_metrics.interface_violations);
    printf("🔄 循环依赖: %d 项\n", g_arch_metrics.circular_dependencies);
    printf("🌟 架构质量: %.1f/100\n", g_arch_metrics.architecture_quality);
    
    // 按ROI排序
    for (int i = 0; i < g_optimization_count - 1; i++) {
        for (int j = i + 1; j < g_optimization_count; j++) {
            if (g_optimizations[i].roi_estimate < g_optimizations[j].roi_estimate) {
                ArchitectureOptimization temp = g_optimizations[i];
                g_optimizations[i] = g_optimizations[j];
                g_optimizations[j] = temp;
            }
        }
    }
    
    // 显示前8个最佳架构优化建议
    printf("\n🎯 优先架构优化建议 (按ROI排序):\n");
    int display_count = (g_optimization_count > 8) ? 8 : g_optimization_count;
    for (int i = 0; i < display_count; i++) {
        ArchitectureOptimization* opt = &g_optimizations[i];
        printf("   %d. %s\n", i+1, opt->pattern->name);
        printf("      📍 影响模块: ");
        for (int j = 0; j < opt->module_count; j++) {
            printf("%s ", strrchr(opt->affected_modules[j], '/') ? 
                   strrchr(opt->affected_modules[j], '/') + 1 : opt->affected_modules[j]);
        }
        printf("\n");
        printf("      💡 描述: %s\n", opt->pattern->description);
        printf("      📊 耦合度: %.1f → %.1f | ROI: %.2f | 复杂度: %d/10\n",
               opt->current_coupling, opt->target_coupling, 
               opt->roi_estimate, opt->implementation_complexity);
        printf("      🔧 优化策略: %s\n", opt->pattern->optimization_strategy);
        printf("      📂 类别: %s\n", opt->pattern->pattern_category);
        printf("\n");
    }
    
    // 分类统计
    printf("📊 优化类别分布:\n");
    int decoupling = 0, interface = 0, communication = 0, compiler = 0, other = 0;
    
    for (int i = 0; i < g_optimization_count; i++) {
        const char* category = g_optimizations[i].pattern->pattern_category;
        if (strcmp(category, "DECOUPLING") == 0) decoupling++;
        else if (strcmp(category, "INTERFACE_DESIGN") == 0) interface++;
        else if (strcmp(category, "COMMUNICATION") == 0) communication++;
        else if (strcmp(category, "COMPILER_SPECIFIC") == 0) compiler++;
        else other++;
    }
    
    printf("   🔗 模块解耦: %d 项\n", decoupling);
    printf("   🔌 接口设计: %d 项\n", interface);
    printf("   📡 通信优化: %d 项\n", communication);
    printf("   🔧 编译器特定: %d 项\n", compiler);
    printf("   📋 其他优化: %d 项\n", other);
    
    // 实施路线图
    printf("\n🗺️  架构优化实施路线图:\n");
    printf("   Phase 1 (紧急): 消除循环依赖 (%d项)\n", g_arch_metrics.circular_dependencies);
    printf("   Phase 2 (重要): 接口违规修复 (%d项)\n", g_arch_metrics.interface_violations);
    printf("   Phase 3 (改进): 模块解耦优化 (%d项)\n", decoupling);
    printf("   Phase 4 (增强): 通信和扩展性优化 (%d项)\n", communication + other);
    
    // 预期效果
    double expected_coupling_reduction = 0.0;
    for (int i = 0; i < g_optimization_count; i++) {
        expected_coupling_reduction += g_optimizations[i].current_coupling - 
                                     g_optimizations[i].target_coupling;
    }
    
    printf("\n📈 预期优化效果:\n");
    printf("   耦合度平均减少: %.1f%%\n", expected_coupling_reduction / g_optimization_count);
    printf("   模块化评分提升: +%.1f分\n", expected_coupling_reduction * 0.4);
    printf("   架构质量提升: +%.1f分\n", expected_coupling_reduction * 0.6);
    printf("   可维护性改善: +%.1f%%\n", expected_coupling_reduction * 0.8);
}

// 导出架构分析结果
int module_architect_export_json(const char* output_file) {
    FILE* file = fopen(output_file, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"stage2_architecture_analysis\": {\n");
    fprintf(file, "    \"timestamp\": \"%ld\",\n", time(NULL));
    fprintf(file, "    \"architecture_metrics\": {\n");
    fprintf(file, "      \"total_modules\": %d,\n", g_arch_metrics.total_modules);
    fprintf(file, "      \"total_optimizations\": %d,\n", g_arch_metrics.total_optimizations);
    fprintf(file, "      \"overall_coupling\": %.2f,\n", g_arch_metrics.overall_coupling);
    fprintf(file, "      \"overall_cohesion\": %.2f,\n", g_arch_metrics.overall_cohesion);
    fprintf(file, "      \"modularity_score\": %.2f,\n", g_arch_metrics.modularity_score);
    fprintf(file, "      \"interface_violations\": %d,\n", g_arch_metrics.interface_violations);
    fprintf(file, "      \"circular_dependencies\": %d,\n", g_arch_metrics.circular_dependencies);
    fprintf(file, "      \"architecture_quality\": %.2f\n", g_arch_metrics.architecture_quality);
    fprintf(file, "    },\n");
    fprintf(file, "    \"optimizations\": [\n");
    
    for (int i = 0; i < g_optimization_count; i++) {
        ArchitectureOptimization* opt = &g_optimizations[i];
        fprintf(file, "      {\n");
        fprintf(file, "        \"pattern_name\": \"%s\",\n", opt->pattern->name);
        fprintf(file, "        \"current_coupling\": %.2f,\n", opt->current_coupling);
        fprintf(file, "        \"target_coupling\": %.2f,\n", opt->target_coupling);
        fprintf(file, "        \"architecture_impact\": %d,\n", opt->pattern->architecture_impact);
        fprintf(file, "        \"modularity_improvement\": %.2f,\n", opt->pattern->modularity_improvement);
        fprintf(file, "        \"implementation_complexity\": %d,\n", opt->implementation_complexity);
        fprintf(file, "        \"roi_estimate\": %.2f,\n", opt->roi_estimate);
        fprintf(file, "        \"category\": \"%s\"\n", opt->pattern->pattern_category);
        fprintf(file, "      }%s\n", (i < g_optimization_count - 1) ? "," : "");
    }
    
    fprintf(file, "    ]\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return 0;
}

// 清理数据
static void cleanup_architecture_data(void) {
    if (g_optimizations) {
        for (int i = 0; i < g_optimization_count; i++) {
            for (int j = 0; j < g_optimizations[i].module_count; j++) {
                free(g_optimizations[i].affected_modules[j]);
            }
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
        printf("AI Module Architect - Stage 2 模块架构优化AI\n");
        printf("用法: %s [--export output.json]\n", argv[0]);
        printf("功能: 优化Stage 1模块架构，提升模块化程度和系统可扩展性\n");
        return 0;
    }
    
    // 运行架构分析
    int result = module_architect_run();
    
    // 导出结果
    if (argc > 2 && strcmp(argv[1], "--export") == 0) {
        if (module_architect_export_json(argv[2]) == 0) {
            printf("📄 架构分析结果已导出到: %s\n", argv[2]);
        } else {
            printf("❌ 导出失败\n");
        }
    }
    
    return result;
}