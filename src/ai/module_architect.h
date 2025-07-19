/*
 * Module Architect Header - Stage 2 AI模块架构优化系统
 * T2.3: 模块架构优化AI接口定义
 */

#ifndef MODULE_ARCHITECT_H
#define MODULE_ARCHITECT_H

#include <time.h>

// 模块架构统计结果
typedef struct ModuleArchitectureMetrics {
    int total_modules;
    int total_optimizations;
    double overall_coupling;           // 整体耦合度
    double overall_cohesion;           // 整体内聚度
    double modularity_score;           // 模块化评分
    int interface_violations;          // 接口违反数
    int circular_dependencies;         // 循环依赖数
    double architecture_quality;       // 架构质量评分
} ModuleArchitectureMetrics;

// 主要接口函数
int module_architect_run(void);                           // 运行模块架构分析
int module_architect_export_json(const char* output_file); // 导出JSON报告

// 架构模式类别常量
#define ARCH_DECOUPLING          "DECOUPLING"
#define ARCH_INTERFACE_DESIGN    "INTERFACE_DESIGN"
#define ARCH_LAYERING            "LAYERING"
#define ARCH_COHESION            "COHESION"
#define ARCH_EXTENSIBILITY       "EXTENSIBILITY"
#define ARCH_CREATIONAL          "CREATIONAL"
#define ARCH_MANAGEMENT          "MANAGEMENT"
#define ARCH_COMMUNICATION       "COMMUNICATION"
#define ARCH_COMPILER_SPECIFIC   "COMPILER_SPECIFIC"
#define ARCH_DATA_FLOW           "DATA_FLOW"

// 架构影响级别常量
#define ARCH_IMPACT_CRITICAL     9
#define ARCH_IMPACT_HIGH         7
#define ARCH_IMPACT_MEDIUM       5
#define ARCH_IMPACT_LOW          3

#endif // MODULE_ARCHITECT_H