#include "module_stability.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>

// 全局模块系统状态
static ModuleSystemState g_module_system = {0};

// 获取当前时间（秒）
static double get_current_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 日志函数实现
void module_log(ModuleLogLevel level, const char* format, ...) {
    const char* level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"};
    
    printf("[MODULE_%s] ", level_names[level]);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

// 查找缓存项
static ModuleCacheEntry* find_cache_entry(const char* module_name) {
    if (!module_name) return NULL;
    
    ModuleCacheEntry* entry = g_module_system.cache_head;
    while (entry) {
        if (strcmp(entry->module_name, module_name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

// 创建新的缓存项
static ModuleCacheEntry* create_cache_entry(const char* module_name) {
    ModuleCacheEntry* entry = malloc(sizeof(ModuleCacheEntry));
    if (!entry) return NULL;
    
    entry->module_name = strdup(module_name);
    entry->handle = NULL;
    entry->is_loaded = false;
    entry->next = NULL;
    
    // 初始化统计信息
    memset(&entry->stats, 0, sizeof(ModuleStats));
    entry->stats.health = MODULE_HEALTH_UNKNOWN;
    
    return entry;
}

// 添加到缓存
static int add_to_cache(ModuleCacheEntry* entry) {
    if (!entry) return MODULE_ERROR_INVALID_PARAM;
    
    entry->next = g_module_system.cache_head;
    g_module_system.cache_head = entry;
    
    return MODULE_SUCCESS;
}

// 初始化模块稳定性系统
int module_stability_init(const ModuleSystemConfig* config) {
    if (g_module_system.is_initialized) {
        module_log(MODULE_LOG_WARNING, "Module stability system already initialized");
        return MODULE_SUCCESS;
    }
    
    // 使用提供的配置或默认配置
    if (config) {
        g_module_system.config = *config;
    } else {
        g_module_system.config = DEFAULT_MODULE_CONFIG;
    }
    
    g_module_system.cache_head = NULL;
    g_module_system.total_modules_loaded = 0;
    g_module_system.total_errors = 0;
    g_module_system.is_initialized = true;
    
    module_log(MODULE_LOG_INFO, "Module stability system initialized");
    return MODULE_SUCCESS;
}

// 清理模块稳定性系统
void module_stability_cleanup(void) {
    if (!g_module_system.is_initialized) return;
    
    // 清理所有缓存项
    ModuleCacheEntry* entry = g_module_system.cache_head;
    while (entry) {
        ModuleCacheEntry* next = entry->next;
        
        if (entry->handle && entry->is_loaded) {
            dlclose(entry->handle);
        }
        
        free(entry->module_name);
        free(entry);
        entry = next;
    }
    
    g_module_system.cache_head = NULL;
    g_module_system.is_initialized = false;
    
    module_log(MODULE_LOG_INFO, "Module stability system cleaned up");
}

// 稳定的模块加载
void* stable_module_load(const char* module_name) {
    if (!g_module_system.is_initialized) {
        module_log(MODULE_LOG_ERROR, "Module system not initialized");
        return NULL;
    }
    
    if (!module_name) {
        module_log(MODULE_LOG_ERROR, "Invalid module name (NULL)");
        return NULL;
    }
    
    // 查找缓存
    ModuleCacheEntry* entry = find_cache_entry(module_name);
    if (entry && entry->is_loaded && entry->handle) {
        entry->stats.load_count++;
        module_log(MODULE_LOG_DEBUG, "Module %s loaded from cache", module_name);
        return entry->handle;
    }
    
    // 创建新的缓存项（如果不存在）
    if (!entry) {
        entry = create_cache_entry(module_name);
        if (!entry) {
            module_log(MODULE_LOG_ERROR, "Failed to create cache entry for %s", module_name);
            return NULL;
        }
        add_to_cache(entry);
    }
    
    // 构建模块路径 - 使用实际的模块文件位置
    char module_path[256];
    if (strcmp(module_name, "pipeline") == 0) {
        snprintf(module_path, sizeof(module_path), "./bin/pipeline_module.so");
    } else {
        snprintf(module_path, sizeof(module_path), "./bin/%s_x64_64.native", module_name);
    }
    
    // 尝试加载模块
    double start_time = get_current_time();
    
    for (int retry = 0; retry < g_module_system.config.max_load_retries; retry++) {
        entry->handle = dlopen(module_path, RTLD_LAZY);
        if (entry->handle) {
            entry->is_loaded = true;
            entry->stats.load_count++;
            entry->stats.last_load_time = get_current_time();
            entry->stats.health = MODULE_HEALTH_HEALTHY;
            
            g_module_system.total_modules_loaded++;
            
            double load_time = get_current_time() - start_time;
            module_log(MODULE_LOG_INFO, "Module %s loaded successfully (%.3fs, retry %d)", 
                      module_name, load_time, retry);
            
            return entry->handle;
        }
        
        module_log(MODULE_LOG_WARNING, "Module %s load attempt %d failed: %s", 
                  module_name, retry + 1, dlerror());
    }
    
    // 加载失败
    entry->stats.error_count++;
    entry->stats.health = MODULE_HEALTH_ERROR;
    g_module_system.total_errors++;
    
    module_log(MODULE_LOG_ERROR, "Failed to load module %s after %d retries", 
              module_name, g_module_system.config.max_load_retries);
    
    return NULL;
}

// 稳定的符号解析
void* stable_module_resolve(const char* module_name, const char* symbol_name) {
    if (!g_module_system.is_initialized) {
        module_log(MODULE_LOG_ERROR, "Module system not initialized");
        return NULL;
    }
    
    if (!module_name || !symbol_name) {
        module_log(MODULE_LOG_ERROR, "Invalid parameters for symbol resolution");
        return NULL;
    }
    
    ModuleCacheEntry* entry = find_cache_entry(module_name);
    if (!entry || !entry->is_loaded || !entry->handle) {
        module_log(MODULE_LOG_ERROR, "Module %s not loaded", module_name);
        return NULL;
    }
    
    void* symbol = dlsym(entry->handle, symbol_name);
    if (symbol) {
        entry->stats.symbol_resolve_count++;
        module_log(MODULE_LOG_DEBUG, "Symbol %s resolved in module %s", symbol_name, module_name);
    } else {
        entry->stats.error_count++;
        module_log(MODULE_LOG_WARNING, "Symbol %s not found in module %s: %s", 
                  symbol_name, module_name, dlerror());
    }
    
    return symbol;
}

// 获取模块健康状态
ModuleHealthStatus module_get_health(const char* module_name) {
    if (!module_name) return MODULE_HEALTH_UNKNOWN;

    ModuleCacheEntry* entry = find_cache_entry(module_name);
    if (!entry) {
        // 对于未知模块，尝试检查是否存在但未加载
        char module_path[256];
        if (strcmp(module_name, "pipeline") == 0) {
            snprintf(module_path, sizeof(module_path), "./bin/pipeline_module.so");
        } else {
            snprintf(module_path, sizeof(module_path), "./bin/%s_x64_64.native", module_name);
        }
        if (access(module_path, F_OK) == 0) {
            return MODULE_HEALTH_UNKNOWN;  // 文件存在但未加载
        } else {
            return MODULE_HEALTH_UNKNOWN;  // 文件不存在
        }
    }

    return entry->stats.health;
}

// 获取模块统计信息
ModuleStats* module_get_stats(const char* module_name) {
    if (!module_name) return NULL;

    ModuleCacheEntry* entry = find_cache_entry(module_name);
    if (!entry) {
        // 对于不存在的模块，返回NULL是正确的行为
        return NULL;
    }

    return &entry->stats;
}

// 打印系统统计信息
void module_print_system_stats(void) {
    printf("=== Module System Statistics ===\n");
    printf("Total modules loaded: %lu\n", g_module_system.total_modules_loaded);
    printf("Total errors: %lu\n", g_module_system.total_errors);
    printf("Max cached modules: %u\n", g_module_system.config.max_cached_modules);
    printf("Auto recovery: %s\n", g_module_system.config.enable_auto_recovery ? "enabled" : "disabled");
    
    // 统计缓存中的模块
    int cached_count = 0;
    int loaded_count = 0;
    ModuleCacheEntry* entry = g_module_system.cache_head;
    while (entry) {
        cached_count++;
        if (entry->is_loaded) loaded_count++;
        entry = entry->next;
    }
    
    printf("Cached modules: %d\n", cached_count);
    printf("Currently loaded: %d\n", loaded_count);
    printf("================================\n");
}

// 打印模块统计信息
void module_print_module_stats(const char* module_name) {
    if (!module_name) return;
    
    ModuleCacheEntry* entry = find_cache_entry(module_name);
    if (!entry) {
        printf("Module %s not found in cache\n", module_name);
        return;
    }
    
    const char* health_names[] = {"Unknown", "Healthy", "Warning", "Error", "Critical"};
    
    printf("=== Module %s Statistics ===\n", module_name);
    printf("Load count: %lu\n", entry->stats.load_count);
    printf("Unload count: %lu\n", entry->stats.unload_count);
    printf("Symbol resolve count: %lu\n", entry->stats.symbol_resolve_count);
    printf("Error count: %lu\n", entry->stats.error_count);
    printf("Health status: %s\n", health_names[entry->stats.health]);
    printf("Is loaded: %s\n", entry->is_loaded ? "yes" : "no");
    printf("Last load time: %.3f\n", entry->stats.last_load_time);
    printf("===============================\n");
}
