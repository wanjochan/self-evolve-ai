#ifndef BUILD_SYSTEM_MANAGER_H
#define BUILD_SYSTEM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

// T4.3 构建系统改进
// 目标: 构建过程简化，支持增量构建

#ifdef __cplusplus
extern "C" {
#endif

// 构建目标类型
typedef enum {
    BUILD_TARGET_EXECUTABLE,
    BUILD_TARGET_SHARED_LIBRARY,
    BUILD_TARGET_STATIC_LIBRARY,
    BUILD_TARGET_MODULE,
    BUILD_TARGET_TOOL,
    BUILD_TARGET_TEST
} BuildTargetType;

// 构建平台
typedef enum {
    BUILD_PLATFORM_LINUX_X64,
    BUILD_PLATFORM_LINUX_ARM64,
    BUILD_PLATFORM_MACOS_X64,
    BUILD_PLATFORM_MACOS_ARM64,
    BUILD_PLATFORM_WINDOWS_X64,
    BUILD_PLATFORM_WINDOWS_ARM64,
    BUILD_PLATFORM_AUTO
} BuildPlatform;

// 构建配置
typedef enum {
    BUILD_CONFIG_DEBUG,
    BUILD_CONFIG_RELEASE,
    BUILD_CONFIG_PROFILE,
    BUILD_CONFIG_TEST
} BuildConfiguration;

// 依赖关系
typedef struct BuildDependency {
    char* name;
    char* path;
    time_t last_modified;
    bool is_system_lib;
    struct BuildDependency* next;
} BuildDependency;

// 构建目标
typedef struct BuildTarget {
    char* name;
    BuildTargetType type;
    char* source_files[64];
    int source_count;
    char* include_dirs[32];
    int include_count;
    char* libraries[32];
    int library_count;
    char* output_path;
    char* compiler_flags;
    char* linker_flags;
    
    BuildDependency* dependencies;
    time_t last_build_time;
    bool needs_rebuild;
    
    struct BuildTarget* next;
} BuildTarget;

// 构建统计
typedef struct {
    uint32_t total_targets;
    uint32_t built_targets;
    uint32_t skipped_targets;
    uint32_t failed_targets;
    uint32_t incremental_builds;
    uint32_t full_builds;
    
    double total_build_time;
    double compilation_time;
    double linking_time;
    
    time_t build_start_time;
    time_t build_end_time;
} BuildStatistics;

// 构建系统配置
typedef struct {
    BuildPlatform target_platform;
    BuildConfiguration configuration;
    
    char* compiler_path;
    char* linker_path;
    char* archiver_path;
    
    char* build_dir;
    char* output_dir;
    char* temp_dir;
    
    bool enable_incremental_build;
    bool enable_parallel_build;
    bool enable_ccache;
    bool enable_verbose_output;
    bool enable_warnings_as_errors;
    
    int parallel_jobs;
    int optimization_level;
    
    char* global_cflags;
    char* global_ldflags;
    char* global_includes;
} BuildSystemConfig;

// 构建系统管理器
typedef struct {
    BuildSystemConfig config;
    BuildTarget* targets;
    BuildStatistics stats;
    
    bool is_initialized;
    char* project_root;
    char* build_file_path;
    
    // 缓存
    char* dependency_cache_file;
    time_t cache_last_update;
    
    // 回调函数
    void (*progress_callback)(const char* message, int progress, void* user_data);
    void (*error_callback)(const char* error, void* user_data);
    void* callback_user_data;
} BuildSystemManager;

// 全局构建系统管理器实例
extern BuildSystemManager g_build_system;

// 初始化和清理
int build_system_init(const char* project_root, const BuildSystemConfig* config);
void build_system_cleanup(void);
bool build_system_is_initialized(void);

// 配置管理
BuildSystemConfig build_system_get_default_config(void);
int build_system_set_config(const BuildSystemConfig* config);
BuildSystemConfig build_system_get_config(void);

// 目标管理
BuildTarget* build_system_create_target(const char* name, BuildTargetType type);
int build_system_add_target(BuildTarget* target);
BuildTarget* build_system_find_target(const char* name);
int build_system_remove_target(const char* name);

// 目标配置
int build_system_add_source_file(BuildTarget* target, const char* source_file);
int build_system_add_include_dir(BuildTarget* target, const char* include_dir);
int build_system_add_library(BuildTarget* target, const char* library);
int build_system_set_output_path(BuildTarget* target, const char* output_path);
int build_system_set_compiler_flags(BuildTarget* target, const char* flags);
int build_system_set_linker_flags(BuildTarget* target, const char* flags);

// 依赖管理
int build_system_add_dependency(BuildTarget* target, const char* dep_name, const char* dep_path);
int build_system_check_dependencies(BuildTarget* target);
bool build_system_needs_rebuild(BuildTarget* target);

// 增量构建
int build_system_update_dependency_cache(void);
int build_system_load_dependency_cache(void);
int build_system_save_dependency_cache(void);

// 构建执行
int build_system_build_target(const char* target_name);
int build_system_build_all(void);
int build_system_clean_target(const char* target_name);
int build_system_clean_all(void);

// 并行构建
int build_system_build_parallel(const char** target_names, int target_count);
int build_system_get_build_order(BuildTarget** ordered_targets, int* count);

// 平台检测和配置
BuildPlatform build_system_detect_platform(void);
const char* build_system_get_platform_name(BuildPlatform platform);
int build_system_configure_for_platform(BuildPlatform platform);

// 编译器检测
int build_system_detect_compiler(char* compiler_path, size_t path_size);
int build_system_get_compiler_version(const char* compiler_path, char* version, size_t version_size);
bool build_system_compiler_supports_feature(const char* compiler_path, const char* feature);

// 构建脚本生成
int build_system_generate_makefile(const char* output_path);
int build_system_generate_cmake(const char* output_path);
int build_system_generate_ninja(const char* output_path);

// 统计和报告
BuildStatistics build_system_get_statistics(void);
void build_system_reset_statistics(void);
void build_system_print_statistics(void);
int build_system_export_build_report(const char* filename);

// 回调管理
void build_system_set_progress_callback(void (*callback)(const char*, int, void*), void* user_data);
void build_system_set_error_callback(void (*callback)(const char*, void*), void* user_data);

// 实用工具
const char* build_system_target_type_to_string(BuildTargetType type);
const char* build_system_config_to_string(BuildConfiguration config);
bool build_system_file_exists(const char* path);
time_t build_system_get_file_mtime(const char* path);
int build_system_create_directory(const char* path);

// 构建命令执行
int build_system_execute_command(const char* command, char* output, size_t output_size);
int build_system_compile_source(const char* source_file, const char* object_file, 
                               const char* include_dirs, const char* flags);
int build_system_link_objects(const char** object_files, int object_count,
                             const char* output_file, const char* libraries, const char* flags);

// 构建文件解析
int build_system_load_build_file(const char* build_file_path);
int build_system_save_build_file(const char* build_file_path);
int build_system_parse_build_script(const char* script_content);

// 预定义构建目标
int build_system_add_standard_targets(void);
int build_system_add_test_targets(void);
int build_system_add_tool_targets(void);

// 构建宏
#define BUILD_TARGET(name, type) \
    build_system_create_target(name, type)

#define BUILD_ADD_SOURCE(target, source) \
    build_system_add_source_file(target, source)

#define BUILD_ADD_INCLUDE(target, include) \
    build_system_add_include_dir(target, include)

#define BUILD_ADD_LIBRARY(target, library) \
    build_system_add_library(target, library)

#define BUILD_SET_OUTPUT(target, output) \
    build_system_set_output_path(target, output)

#ifdef __cplusplus
}
#endif

#endif // BUILD_SYSTEM_MANAGER_H
