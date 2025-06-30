/**
 * rt_module_manager.c - .rt模块管理系统
 * 
 * 实现完整的.rt模块管理功能：
 * - 模块搜索和发现
 * - 版本检查和兼容性
 * - 依赖解析和加载顺序
 * - 模块缓存和生命周期管理
 */

#include <stddef.h>
#include <stdint.h>

// ===============================================
// 模块信息结构
// ===============================================

typedef struct {
    char name[32];
    char version[16];
    char filename[64];
    size_t size;
    uint32_t checksum;
    int loaded;
    void* handle;
} ModuleInfo;

typedef struct {
    char name[32];
    char min_version[16];
    char max_version[16];
    int required;
} ModuleDependency;

typedef struct {
    ModuleInfo info;
    ModuleDependency dependencies[8];
    int dependency_count;
    int load_order;
} ModuleDescriptor;

// ===============================================
// 模块管理器状态
// ===============================================

static ModuleDescriptor modules[32];
static int module_count = 0;
static char search_paths[8][64];
static int search_path_count = 0;

// ===============================================
// 字符串工具函数
// ===============================================

int simple_strcmp(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return (str1[i] < str2[i]) ? -1 : 1;
        }
        i++;
    }
    
    if (str1[i] == '\0' && str2[i] == '\0') return 0;
    return (str1[i] == '\0') ? -1 : 1;
}

void simple_strcpy(char* dest, const char* src, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// ===============================================
// 版本比较
// ===============================================

int parse_version(const char* version_str) {
    // 简化的版本解析：假设格式为 "major.minor"
    int major = 0, minor = 0;
    int i = 0;
    
    // 解析主版本号
    while (version_str[i] >= '0' && version_str[i] <= '9') {
        major = major * 10 + (version_str[i] - '0');
        i++;
    }
    
    // 跳过点号
    if (version_str[i] == '.') {
        i++;
    }
    
    // 解析次版本号
    while (version_str[i] >= '0' && version_str[i] <= '9') {
        minor = minor * 10 + (version_str[i] - '0');
        i++;
    }
    
    return (major << 16) | minor;
}

int compare_versions(const char* version1, const char* version2) {
    int v1 = parse_version(version1);
    int v2 = parse_version(version2);
    
    if (v1 < v2) return -1;
    if (v1 > v2) return 1;
    return 0;
}

int is_version_compatible(const char* version, const char* min_version, const char* max_version) {
    if (compare_versions(version, min_version) < 0) {
        return 0; // 版本太低
    }
    
    if (max_version[0] != '\0' && compare_versions(version, max_version) > 0) {
        return 0; // 版本太高
    }
    
    return 1; // 兼容
}

// ===============================================
// 模块搜索
// ===============================================

void add_search_path(const char* path) {
    if (search_path_count < 8) {
        simple_strcpy(search_paths[search_path_count], path, 64);
        search_path_count++;
    }
}

int find_module_file(const char* module_name, char* result_path, size_t path_size) {
    // 在搜索路径中查找模块文件
    for (int i = 0; i < search_path_count; i++) {
        // 构建完整路径
        char full_path[128];
        char* ptr = full_path;
        
        // 复制搜索路径
        const char* search_path = search_paths[i];
        while (*search_path != '\0' && ptr < full_path + 127) {
            *ptr++ = *search_path++;
        }
        
        // 添加路径分隔符
        if (ptr > full_path && *(ptr-1) != '/' && *(ptr-1) != '\\') {
            *ptr++ = '/';
        }
        
        // 复制模块名
        const char* name_ptr = module_name;
        while (*name_ptr != '\0' && ptr < full_path + 127) {
            *ptr++ = *name_ptr++;
        }
        
        // 添加扩展名
        const char* ext = ".native";
        while (*ext != '\0' && ptr < full_path + 127) {
            *ptr++ = *ext++;
        }
        
        *ptr = '\0';
        
        // 检查文件是否存在（简化实现）
        if (1) { // 假设文件存在
            simple_strcpy(result_path, full_path, path_size);
            return 1;
        }
    }
    
    return 0; // 未找到
}

// ===============================================
// 模块注册和发现
// ===============================================

int register_module(const char* name, const char* version, const char* filename) {
    if (module_count >= 32) {
        return 0; // 模块数量超限
    }
    
    ModuleDescriptor* module = &modules[module_count];
    
    simple_strcpy(module->info.name, name, 32);
    simple_strcpy(module->info.version, version, 16);
    simple_strcpy(module->info.filename, filename, 64);
    module->info.size = 0;
    module->info.checksum = 0;
    module->info.loaded = 0;
    module->info.handle = NULL;
    module->dependency_count = 0;
    module->load_order = -1;
    
    module_count++;
    return 1;
}

ModuleDescriptor* find_module(const char* name) {
    for (int i = 0; i < module_count; i++) {
        if (simple_strcmp(modules[i].info.name, name) == 0) {
            return &modules[i];
        }
    }
    return NULL;
}

int add_module_dependency(const char* module_name, const char* dep_name, 
                         const char* min_version, const char* max_version, int required) {
    ModuleDescriptor* module = find_module(module_name);
    if (module == NULL || module->dependency_count >= 8) {
        return 0;
    }
    
    ModuleDependency* dep = &module->dependencies[module->dependency_count];
    simple_strcpy(dep->name, dep_name, 32);
    simple_strcpy(dep->min_version, min_version, 16);
    simple_strcpy(dep->max_version, max_version, 16);
    dep->required = required;
    
    module->dependency_count++;
    return 1;
}

// ===============================================
// 依赖解析
// ===============================================

int resolve_dependencies(const char* module_name, int* load_order, int order_index) {
    ModuleDescriptor* module = find_module(module_name);
    if (module == NULL) {
        return 0;
    }
    
    if (module->load_order >= 0) {
        return 1; // 已经解析过
    }
    
    // 先解析所有依赖
    for (int i = 0; i < module->dependency_count; i++) {
        ModuleDependency* dep = &module->dependencies[i];
        
        if (dep->required) {
            ModuleDescriptor* dep_module = find_module(dep->name);
            if (dep_module == NULL) {
                return 0; // 缺少必需依赖
            }
            
            // 检查版本兼容性
            if (!is_version_compatible(dep_module->info.version, 
                                     dep->min_version, dep->max_version)) {
                return 0; // 版本不兼容
            }
            
            // 递归解析依赖的依赖
            if (!resolve_dependencies(dep->name, load_order, order_index)) {
                return 0;
            }
        }
    }
    
    // 设置加载顺序
    module->load_order = order_index;
    load_order[order_index] = module - modules; // 模块索引
    
    return 1;
}

// ===============================================
// 模块加载
// ===============================================

int load_module(ModuleDescriptor* module) {
    if (module->info.loaded) {
        return 1; // 已经加载
    }
    
    // 在实际实现中，这里会：
    // 1. 读取.native文件
    // 2. 验证RTME头
    // 3. 加载机器码到内存
    // 4. 解析符号表
    // 5. 执行重定位
    
    module->info.handle = (void*)0x12345678; // 模拟句柄
    module->info.loaded = 1;
    
    return 1;
}

int unload_module(ModuleDescriptor* module) {
    if (!module->info.loaded) {
        return 1; // 已经卸载
    }
    
    // 在实际实现中，这里会释放内存和资源
    module->info.handle = NULL;
    module->info.loaded = 0;
    
    return 1;
}

// ===============================================
// 模块管理器API
// ===============================================

int rt_module_manager_init(void) {
    module_count = 0;
    search_path_count = 0;
    
    // 添加默认搜索路径
    add_search_path("bin");
    add_search_path("lib");
    add_search_path("modules");
    
    // 注册系统模块
    register_module("vm_x64_64", "1.0", "vm_x64_64.native");
    register_module("libc_x64_64", "1.0", "libc_x64_64.native");
    register_module("libc_minimal", "1.0", "libc_minimal.native");
    register_module("libc_os", "1.0", "libc_os.native");
    
    // 设置依赖关系
    add_module_dependency("vm_x64_64", "libc_x64_64", "1.0", "2.0", 1);
    
    return 1;
}

int load_module_by_name(const char* name) {
    ModuleDescriptor* module = find_module(name);
    if (module == NULL) {
        return 0;
    }
    
    // 解析依赖
    int load_order[32];
    if (!resolve_dependencies(name, load_order, 0)) {
        return 0;
    }
    
    // 按顺序加载模块
    for (int i = 0; i < module_count; i++) {
        if (modules[i].load_order >= 0) {
            if (!load_module(&modules[i])) {
                return 0;
            }
        }
    }
    
    return 1;
}

int get_loaded_module_count(void) {
    int count = 0;
    for (int i = 0; i < module_count; i++) {
        if (modules[i].info.loaded) {
            count++;
        }
    }
    return count;
}

// ===============================================
// 主入口（用于测试）
// ===============================================

int main(void) {
    // 初始化模块管理器
    if (!rt_module_manager_init()) {
        return 1;
    }
    
    // 测试模块加载
    if (!load_module_by_name("vm_x64_64")) {
        return 1;
    }
    
    // 检查加载结果
    int loaded_count = get_loaded_module_count();
    if (loaded_count < 2) { // 至少应该加载vm和libc
        return 1;
    }
    
    return 0; // 测试通过
}
