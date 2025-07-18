#include "build_system_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>

// 全局构建系统管理器实例
BuildSystemManager g_build_system = {0};

// 获取高精度时间
static double get_current_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 获取文件修改时间
time_t build_system_get_file_mtime(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

// 检查文件是否存在
bool build_system_file_exists(const char* path) {
    return access(path, F_OK) == 0;
}

// 创建目录
int build_system_create_directory(const char* path) {
    char tmp[1024];
    char* p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    
    return mkdir(tmp, 0755);
}

// 获取默认配置
BuildSystemConfig build_system_get_default_config(void) {
    BuildSystemConfig config = {0};
    
    config.target_platform = BUILD_PLATFORM_AUTO;
    config.configuration = BUILD_CONFIG_DEBUG;
    
    config.compiler_path = strdup("gcc");
    config.linker_path = strdup("gcc");
    config.archiver_path = strdup("ar");
    
    config.build_dir = strdup("build");
    config.output_dir = strdup("bin");
    config.temp_dir = strdup("build/temp");
    
    config.enable_incremental_build = true;
    config.enable_parallel_build = true;
    config.enable_ccache = false;
    config.enable_verbose_output = false;
    config.enable_warnings_as_errors = false;
    
    config.parallel_jobs = 4;
    config.optimization_level = 0; // -O0 for debug
    
    config.global_cflags = strdup("-std=c99 -Wall -Wextra");
    config.global_ldflags = strdup("");
    config.global_includes = strdup("");
    
    return config;
}

// 平台检测
BuildPlatform build_system_detect_platform(void) {
#ifdef __APPLE__
    #ifdef __aarch64__
        return BUILD_PLATFORM_MACOS_ARM64;
    #else
        return BUILD_PLATFORM_MACOS_X64;
    #endif
#elif defined(__linux__)
    #ifdef __aarch64__
        return BUILD_PLATFORM_LINUX_ARM64;
    #else
        return BUILD_PLATFORM_LINUX_X64;
    #endif
#elif defined(_WIN32) || defined(_WIN64)
    #ifdef _WIN64
        return BUILD_PLATFORM_WINDOWS_X64;
    #else
        return BUILD_PLATFORM_WINDOWS_ARM64; // 简化处理
    #endif
#else
    return BUILD_PLATFORM_LINUX_X64; // 默认
#endif
}

// 获取平台名称
const char* build_system_get_platform_name(BuildPlatform platform) {
    switch (platform) {
        case BUILD_PLATFORM_LINUX_X64: return "linux_x64";
        case BUILD_PLATFORM_LINUX_ARM64: return "linux_arm64";
        case BUILD_PLATFORM_MACOS_X64: return "macos_x64";
        case BUILD_PLATFORM_MACOS_ARM64: return "macos_arm64";
        case BUILD_PLATFORM_WINDOWS_X64: return "windows_x64";
        case BUILD_PLATFORM_WINDOWS_ARM64: return "windows_arm64";
        case BUILD_PLATFORM_AUTO: return "auto";
        default: return "unknown";
    }
}

// 初始化构建系统
int build_system_init(const char* project_root, const BuildSystemConfig* config) {
    if (g_build_system.is_initialized) {
        return 0; // 已经初始化
    }
    
    memset(&g_build_system, 0, sizeof(BuildSystemManager));
    
    // 设置项目根目录
    if (project_root) {
        g_build_system.project_root = strdup(project_root);
    } else {
        g_build_system.project_root = strdup(".");
    }
    
    // 使用默认配置或提供的配置
    if (config) {
        g_build_system.config = *config;
    } else {
        g_build_system.config = build_system_get_default_config();
    }
    
    // 自动检测平台
    if (g_build_system.config.target_platform == BUILD_PLATFORM_AUTO) {
        g_build_system.config.target_platform = build_system_detect_platform();
    }
    
    // 创建必要的目录
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", g_build_system.project_root, g_build_system.config.build_dir);
    build_system_create_directory(full_path);
    
    snprintf(full_path, sizeof(full_path), "%s/%s", g_build_system.project_root, g_build_system.config.output_dir);
    build_system_create_directory(full_path);
    
    snprintf(full_path, sizeof(full_path), "%s/%s", g_build_system.project_root, g_build_system.config.temp_dir);
    build_system_create_directory(full_path);
    
    // 设置依赖缓存文件路径
    snprintf(full_path, sizeof(full_path), "%s/%s/dependency_cache.dat", 
             g_build_system.project_root, g_build_system.config.build_dir);
    g_build_system.dependency_cache_file = strdup(full_path);
    
    // 初始化统计信息
    memset(&g_build_system.stats, 0, sizeof(BuildStatistics));
    g_build_system.stats.build_start_time = time(NULL);
    
    g_build_system.is_initialized = true;
    
    printf("Build System Manager: 初始化完成\n");
    printf("  项目根目录: %s\n", g_build_system.project_root);
    printf("  目标平台: %s\n", build_system_get_platform_name(g_build_system.config.target_platform));
    printf("  构建配置: %s\n", build_system_config_to_string(g_build_system.config.configuration));
    printf("  编译器: %s\n", g_build_system.config.compiler_path);
    printf("  增量构建: %s\n", g_build_system.config.enable_incremental_build ? "启用" : "禁用");
    printf("  并行构建: %s (%d jobs)\n", 
           g_build_system.config.enable_parallel_build ? "启用" : "禁用",
           g_build_system.config.parallel_jobs);
    
    return 0;
}

// 清理构建系统
void build_system_cleanup(void) {
    if (!g_build_system.is_initialized) {
        return;
    }
    
    // 清理目标链表
    BuildTarget* current = g_build_system.targets;
    while (current) {
        BuildTarget* next = current->next;
        
        // 清理依赖链表
        BuildDependency* dep = current->dependencies;
        while (dep) {
            BuildDependency* next_dep = dep->next;
            free(dep->name);
            free(dep->path);
            free(dep);
            dep = next_dep;
        }
        
        // 清理目标
        free(current->name);
        free(current->output_path);
        free(current->compiler_flags);
        free(current->linker_flags);
        free(current);
        
        current = next;
    }
    
    // 清理配置
    free(g_build_system.config.compiler_path);
    free(g_build_system.config.linker_path);
    free(g_build_system.config.archiver_path);
    free(g_build_system.config.build_dir);
    free(g_build_system.config.output_dir);
    free(g_build_system.config.temp_dir);
    free(g_build_system.config.global_cflags);
    free(g_build_system.config.global_ldflags);
    free(g_build_system.config.global_includes);
    
    // 清理其他资源
    free(g_build_system.project_root);
    free(g_build_system.build_file_path);
    free(g_build_system.dependency_cache_file);
    
    g_build_system.is_initialized = false;
    printf("Build System Manager: 清理完成\n");
}

// 检查是否已初始化
bool build_system_is_initialized(void) {
    return g_build_system.is_initialized;
}

// 创建构建目标
BuildTarget* build_system_create_target(const char* name, BuildTargetType type) {
    if (!name) return NULL;
    
    BuildTarget* target = malloc(sizeof(BuildTarget));
    if (!target) return NULL;
    
    memset(target, 0, sizeof(BuildTarget));
    
    target->name = strdup(name);
    target->type = type;
    target->source_count = 0;
    target->include_count = 0;
    target->library_count = 0;
    target->dependencies = NULL;
    target->last_build_time = 0;
    target->needs_rebuild = true;
    target->next = NULL;
    
    return target;
}

// 添加构建目标
int build_system_add_target(BuildTarget* target) {
    if (!g_build_system.is_initialized || !target) {
        return -1;
    }
    
    // 检查是否已存在同名目标
    if (build_system_find_target(target->name)) {
        return -2; // 目标已存在
    }
    
    // 添加到链表头部
    target->next = g_build_system.targets;
    g_build_system.targets = target;
    
    g_build_system.stats.total_targets++;
    
    return 0;
}

// 查找构建目标
BuildTarget* build_system_find_target(const char* name) {
    if (!g_build_system.is_initialized || !name) {
        return NULL;
    }
    
    BuildTarget* current = g_build_system.targets;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// 添加源文件
int build_system_add_source_file(BuildTarget* target, const char* source_file) {
    if (!target || !source_file || target->source_count >= 64) {
        return -1;
    }
    
    target->source_files[target->source_count] = strdup(source_file);
    target->source_count++;
    target->needs_rebuild = true;
    
    return 0;
}

// 添加包含目录
int build_system_add_include_dir(BuildTarget* target, const char* include_dir) {
    if (!target || !include_dir || target->include_count >= 32) {
        return -1;
    }
    
    target->include_dirs[target->include_count] = strdup(include_dir);
    target->include_count++;
    target->needs_rebuild = true;
    
    return 0;
}

// 添加库
int build_system_add_library(BuildTarget* target, const char* library) {
    if (!target || !library || target->library_count >= 32) {
        return -1;
    }
    
    target->libraries[target->library_count] = strdup(library);
    target->library_count++;
    target->needs_rebuild = true;
    
    return 0;
}

// 设置输出路径
int build_system_set_output_path(BuildTarget* target, const char* output_path) {
    if (!target || !output_path) {
        return -1;
    }
    
    if (target->output_path) {
        free(target->output_path);
    }
    
    target->output_path = strdup(output_path);
    target->needs_rebuild = true;

    return 0;
}

// 检查目标是否需要重新构建
bool build_system_needs_rebuild(BuildTarget* target) {
    if (!target) return false;

    // 如果输出文件不存在，需要重新构建
    if (!target->output_path || !build_system_file_exists(target->output_path)) {
        return true;
    }

    time_t output_mtime = build_system_get_file_mtime(target->output_path);

    // 检查源文件是否比输出文件新
    for (int i = 0; i < target->source_count; i++) {
        if (target->source_files[i]) {
            time_t source_mtime = build_system_get_file_mtime(target->source_files[i]);
            if (source_mtime > output_mtime) {
                return true;
            }
        }
    }

    // 检查依赖是否比输出文件新
    BuildDependency* dep = target->dependencies;
    while (dep) {
        if (dep->path) {
            time_t dep_mtime = build_system_get_file_mtime(dep->path);
            if (dep_mtime > output_mtime) {
                return true;
            }
        }
        dep = dep->next;
    }

    return false;
}

// 执行命令
int build_system_execute_command(const char* command, char* output, size_t output_size) {
    if (!command) return -1;

    if (g_build_system.config.enable_verbose_output) {
        printf("执行命令: %s\n", command);
    }

    FILE* pipe = popen(command, "r");
    if (!pipe) {
        return -1;
    }

    if (output && output_size > 0) {
        size_t bytes_read = fread(output, 1, output_size - 1, pipe);
        output[bytes_read] = '\0';
    }

    int result = pclose(pipe);
    return WEXITSTATUS(result);
}

// 编译源文件
int build_system_compile_source(const char* source_file, const char* object_file,
                               const char* include_dirs, const char* flags) {
    if (!source_file || !object_file) return -1;

    char command[4096];
    int pos = 0;

    // 构建编译命令
    pos += snprintf(command + pos, sizeof(command) - pos, "%s", g_build_system.config.compiler_path);

    // 添加全局标志
    if (g_build_system.config.global_cflags) {
        pos += snprintf(command + pos, sizeof(command) - pos, " %s", g_build_system.config.global_cflags);
    }

    // 添加优化级别
    pos += snprintf(command + pos, sizeof(command) - pos, " -O%d", g_build_system.config.optimization_level);

    // 添加包含目录
    if (include_dirs) {
        pos += snprintf(command + pos, sizeof(command) - pos, " %s", include_dirs);
    }

    // 添加额外标志
    if (flags) {
        pos += snprintf(command + pos, sizeof(command) - pos, " %s", flags);
    }

    // 添加源文件和输出文件
    pos += snprintf(command + pos, sizeof(command) - pos, " -c %s -o %s", source_file, object_file);

    return build_system_execute_command(command, NULL, 0);
}

// 链接目标文件
int build_system_link_objects(const char** object_files, int object_count,
                             const char* output_file, const char* libraries, const char* flags) {
    if (!object_files || object_count == 0 || !output_file) return -1;

    char command[4096];
    int pos = 0;

    // 构建链接命令
    pos += snprintf(command + pos, sizeof(command) - pos, "%s", g_build_system.config.linker_path);

    // 添加目标文件
    for (int i = 0; i < object_count; i++) {
        pos += snprintf(command + pos, sizeof(command) - pos, " %s", object_files[i]);
    }

    // 添加库
    if (libraries) {
        pos += snprintf(command + pos, sizeof(command) - pos, " %s", libraries);
    }

    // 添加全局链接标志
    if (g_build_system.config.global_ldflags) {
        pos += snprintf(command + pos, sizeof(command) - pos, " %s", g_build_system.config.global_ldflags);
    }

    // 添加额外标志
    if (flags) {
        pos += snprintf(command + pos, sizeof(command) - pos, " %s", flags);
    }

    // 添加输出文件
    pos += snprintf(command + pos, sizeof(command) - pos, " -o %s", output_file);

    return build_system_execute_command(command, NULL, 0);
}

// 构建单个目标
int build_system_build_target(const char* target_name) {
    if (!g_build_system.is_initialized || !target_name) {
        return -1;
    }

    BuildTarget* target = build_system_find_target(target_name);
    if (!target) {
        printf("错误: 找不到目标 '%s'\n", target_name);
        return -1;
    }

    // 检查是否需要重新构建
    if (g_build_system.config.enable_incremental_build && !build_system_needs_rebuild(target)) {
        printf("目标 '%s' 是最新的，跳过构建\n", target_name);
        g_build_system.stats.skipped_targets++;
        return 0;
    }

    printf("构建目标: %s (%s)\n", target->name, build_system_target_type_to_string(target->type));

    double start_time = get_current_time();

    // 创建输出目录
    if (target->output_path) {
        char* dir = strdup(target->output_path);
        char* last_slash = strrchr(dir, '/');
        if (last_slash) {
            *last_slash = '\0';
            build_system_create_directory(dir);
        }
        free(dir);
    }

    // 编译源文件
    char* object_files[64];
    int object_count = 0;

    for (int i = 0; i < target->source_count; i++) {
        if (!target->source_files[i]) continue;

        // 生成目标文件路径
        char object_path[1024];
        const char* source_name = strrchr(target->source_files[i], '/');
        source_name = source_name ? source_name + 1 : target->source_files[i];

        snprintf(object_path, sizeof(object_path), "%s/%s/%s.o",
                g_build_system.project_root, g_build_system.config.temp_dir, source_name);

        // 构建包含目录字符串
        char include_dirs[2048] = "";
        for (int j = 0; j < target->include_count; j++) {
            if (target->include_dirs[j]) {
                strcat(include_dirs, " -I");
                strcat(include_dirs, target->include_dirs[j]);
            }
        }

        // 编译源文件
        printf("  编译: %s\n", target->source_files[i]);
        if (build_system_compile_source(target->source_files[i], object_path,
                                       include_dirs, target->compiler_flags) != 0) {
            printf("❌ 编译失败: %s\n", target->source_files[i]);
            g_build_system.stats.failed_targets++;
            return -1;
        }

        object_files[object_count] = strdup(object_path);
        object_count++;
    }

    // 链接目标文件
    if (target->type != BUILD_TARGET_STATIC_LIBRARY && object_count > 0) {
        // 构建库字符串
        char libraries[2048] = "";
        for (int i = 0; i < target->library_count; i++) {
            if (target->libraries[i]) {
                strcat(libraries, " -l");
                strcat(libraries, target->libraries[i]);
            }
        }

        printf("  链接: %s\n", target->output_path);
        if (build_system_link_objects((const char**)object_files, object_count,
                                     target->output_path, libraries, target->linker_flags) != 0) {
            printf("❌ 链接失败: %s\n", target->output_path);
            g_build_system.stats.failed_targets++;

            // 清理目标文件
            for (int i = 0; i < object_count; i++) {
                free(object_files[i]);
            }
            return -1;
        }
    }

    // 清理目标文件
    for (int i = 0; i < object_count; i++) {
        free(object_files[i]);
    }

    double build_time = get_current_time() - start_time;
    target->last_build_time = time(NULL);
    target->needs_rebuild = false;

    g_build_system.stats.built_targets++;
    g_build_system.stats.total_build_time += build_time;

    printf("✅ 目标 '%s' 构建成功 (%.3f 秒)\n", target_name, build_time);

    return 0;
}

// 构建所有目标
int build_system_build_all(void) {
    if (!g_build_system.is_initialized) {
        return -1;
    }

    printf("开始构建所有目标...\n");

    int total_built = 0;
    int total_failed = 0;

    BuildTarget* current = g_build_system.targets;
    while (current) {
        if (build_system_build_target(current->name) == 0) {
            total_built++;
        } else {
            total_failed++;
        }
        current = current->next;
    }

    printf("\n构建完成: %d 成功, %d 失败\n", total_built, total_failed);

    return total_failed == 0 ? 0 : -1;
}

// 清理目标
int build_system_clean_target(const char* target_name) {
    if (!g_build_system.is_initialized || !target_name) {
        return -1;
    }

    BuildTarget* target = build_system_find_target(target_name);
    if (!target) {
        printf("错误: 找不到目标 '%s'\n", target_name);
        return -1;
    }

    printf("清理目标: %s\n", target_name);

    // 删除输出文件
    if (target->output_path && build_system_file_exists(target->output_path)) {
        if (unlink(target->output_path) == 0) {
            printf("  删除: %s\n", target->output_path);
        }
    }

    // 删除临时目标文件
    for (int i = 0; i < target->source_count; i++) {
        if (target->source_files[i]) {
            char object_path[1024];
            const char* source_name = strrchr(target->source_files[i], '/');
            source_name = source_name ? source_name + 1 : target->source_files[i];

            snprintf(object_path, sizeof(object_path), "%s/%s/%s.o",
                    g_build_system.project_root, g_build_system.config.temp_dir, source_name);

            if (build_system_file_exists(object_path)) {
                if (unlink(object_path) == 0) {
                    printf("  删除: %s\n", object_path);
                }
            }
        }
    }

    target->needs_rebuild = true;

    return 0;
}

// 清理所有目标
int build_system_clean_all(void) {
    if (!g_build_system.is_initialized) {
        return -1;
    }

    printf("清理所有目标...\n");

    BuildTarget* current = g_build_system.targets;
    while (current) {
        build_system_clean_target(current->name);
        current = current->next;
    }

    printf("清理完成\n");

    return 0;
}

// 目标类型转字符串
const char* build_system_target_type_to_string(BuildTargetType type) {
    switch (type) {
        case BUILD_TARGET_EXECUTABLE: return "可执行文件";
        case BUILD_TARGET_SHARED_LIBRARY: return "共享库";
        case BUILD_TARGET_STATIC_LIBRARY: return "静态库";
        case BUILD_TARGET_MODULE: return "模块";
        case BUILD_TARGET_TOOL: return "工具";
        case BUILD_TARGET_TEST: return "测试";
        default: return "未知";
    }
}

// 配置转字符串
const char* build_system_config_to_string(BuildConfiguration config) {
    switch (config) {
        case BUILD_CONFIG_DEBUG: return "调试";
        case BUILD_CONFIG_RELEASE: return "发布";
        case BUILD_CONFIG_PROFILE: return "性能分析";
        case BUILD_CONFIG_TEST: return "测试";
        default: return "未知";
    }
}

// 获取统计信息
BuildStatistics build_system_get_statistics(void) {
    if (!g_build_system.is_initialized) {
        BuildStatistics empty_stats = {0};
        return empty_stats;
    }

    g_build_system.stats.build_end_time = time(NULL);
    return g_build_system.stats;
}

// 打印统计信息
void build_system_print_statistics(void) {
    if (!g_build_system.is_initialized) {
        printf("Build System Manager: 未初始化\n");
        return;
    }

    BuildStatistics stats = build_system_get_statistics();

    printf("=== 构建系统统计信息 ===\n");
    printf("总目标数: %u\n", stats.total_targets);
    printf("已构建: %u\n", stats.built_targets);
    printf("已跳过: %u\n", stats.skipped_targets);
    printf("构建失败: %u\n", stats.failed_targets);
    printf("增量构建: %u\n", stats.incremental_builds);
    printf("完整构建: %u\n", stats.full_builds);
    printf("总构建时间: %.3f 秒\n", stats.total_build_time);
    printf("平均构建时间: %.3f 秒\n",
           stats.built_targets > 0 ? stats.total_build_time / stats.built_targets : 0.0);
    printf("构建成功率: %.1f%%\n",
           stats.total_targets > 0 ? (double)stats.built_targets / stats.total_targets * 100 : 0.0);
    printf("=============================\n");
}

// 添加标准目标
int build_system_add_standard_targets(void) {
    if (!g_build_system.is_initialized) {
        return -1;
    }

    // 添加核心模块目标
    BuildTarget* core_target = build_system_create_target("core", BUILD_TARGET_STATIC_LIBRARY);
    if (core_target) {
        build_system_add_source_file(core_target, "src/core/astc.c");
        build_system_add_source_file(core_target, "src/core/modules/module_module.c");
        build_system_add_include_dir(core_target, "src/core");
        build_system_set_output_path(core_target, "bin/libcore.a");
        build_system_add_target(core_target);
    }

    // 添加工具目标
    const char* tools[] = {"c2astc", "c2native", "simple_loader"};
    for (int i = 0; i < 3; i++) {
        char target_name[64];
        char source_path[256];
        char output_path[256];

        snprintf(target_name, sizeof(target_name), "%s", tools[i]);
        snprintf(source_path, sizeof(source_path), "tools/%s.c", tools[i]);
        snprintf(output_path, sizeof(output_path), "bin/%s", tools[i]);

        BuildTarget* tool_target = build_system_create_target(target_name, BUILD_TARGET_TOOL);
        if (tool_target) {
            build_system_add_source_file(tool_target, source_path);
            build_system_add_include_dir(tool_target, "src/core");
            build_system_add_include_dir(tool_target, "tools");
            build_system_set_output_path(tool_target, output_path);
            build_system_add_target(tool_target);
        }
    }

    printf("已添加标准构建目标\n");

    return 0;
}
