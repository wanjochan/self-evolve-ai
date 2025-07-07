#include "core_test_framework.h"
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

// 全局测试框架状态
TestFramework g_test_framework = {0};

// 模拟函数数组
MockFunction g_mock_functions[MAX_MOCK_FUNCTIONS] = {0};
int g_mock_function_count = 0;

// 测试框架初始化
void test_framework_init(bool verbose) {
    memset(&g_test_framework, 0, sizeof(TestFramework));
    g_test_framework.verbose = verbose;
    
    // 重置模拟函数
    memset(g_mock_functions, 0, sizeof(g_mock_functions));
    g_mock_function_count = 0;
    
    printf(ANSI_COLOR_MAGENTA "=== Core Test Framework Initialized ===" ANSI_COLOR_RESET "\n");
    if (verbose) {
        printf(ANSI_COLOR_YELLOW "Verbose mode enabled" ANSI_COLOR_RESET "\n");
    }
    printf("\n");
}

// 测试框架清理
void test_framework_cleanup(void) {
    // 清理模拟函数
    memset(g_mock_functions, 0, sizeof(g_mock_functions));
    g_mock_function_count = 0;
    
    printf(ANSI_COLOR_MAGENTA "=== Core Test Framework Cleaned Up ===" ANSI_COLOR_RESET "\n");
}

// 打印测试总结
void test_framework_print_summary(void) {
    printf(ANSI_COLOR_MAGENTA "=== Test Summary ===" ANSI_COLOR_RESET "\n");
    printf("Total tests: %d\n", g_test_framework.total_tests);
    printf("Passed: " ANSI_COLOR_GREEN "%d" ANSI_COLOR_RESET "\n", g_test_framework.passed_tests);
    printf("Failed: " ANSI_COLOR_RED "%d" ANSI_COLOR_RESET "\n", g_test_framework.failed_tests);
    
    if (g_test_framework.failed_tests == 0) {
        printf(ANSI_COLOR_GREEN "All tests passed! ✓" ANSI_COLOR_RESET "\n");
    } else {
        printf(ANSI_COLOR_RED "Some tests failed! ✗" ANSI_COLOR_RESET "\n");
    }
    printf("\n");
}

// 检查所有测试是否通过
bool test_framework_all_passed(void) {
    return g_test_framework.failed_tests == 0;
}

// 测试用内存分配
void* test_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

// 测试用内存释放
void test_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// 测试用字符串复制
char* test_strdup(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* copy = test_malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

// 检查文件是否存在
bool test_file_exists(const char* path) {
    if (!path) return false;
    
    struct stat st;
    return stat(path, &st) == 0;
}

// 创建临时文件
bool test_create_temp_file(const char* content, char* temp_path, size_t path_size) {
    if (!content || !temp_path || path_size == 0) {
        return false;
    }
    
    // 生成临时文件名
    srand(time(NULL));
    snprintf(temp_path, path_size, "/tmp/core_test_%d_%d.tmp", getpid(), rand());
    
    // 创建文件
    FILE* file = fopen(temp_path, "w");
    if (!file) {
        return false;
    }
    
    // 写入内容
    size_t written = fwrite(content, 1, strlen(content), file);
    fclose(file);
    
    return written == strlen(content);
}

// 删除临时文件
void test_remove_temp_file(const char* path) {
    if (path && test_file_exists(path)) {
        unlink(path);
    }
}

// ===============================================
// ASTC函数的Stub实现 (用于测试)
// ===============================================

// AST节点管理函数
ASTNode* ast_create_node(ASTNodeType type, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node) {
        memset(node, 0, sizeof(ASTNode));
        node->type = type;
        node->line = line;
        node->column = column;
    }
    return node;
}

void ast_free(ASTNode* node) {
    if (node) {
        // 简化实现：只释放节点本身
        free(node);
    }
}

void ast_print(ASTNode* node, int indent) {
    if (node) {
        printf("%*sAST Node Type: %d (Line: %d, Column: %d)\n", 
               indent, "", (int)node->type, node->line, node->column);
    }
}

// 模块管理函数
int ast_module_add_declaration(ASTNode* module, ASTNode* declaration) {
    // Stub实现：总是返回成功
    return 0;
}

int ast_module_add_export(ASTNode* module, ASTNode* export_decl) {
    // Stub实现：总是返回成功
    return 0;
}

int ast_module_add_import(ASTNode* module, ASTNode* import_decl) {
    // Stub实现：总是返回成功
    return 0;
}

int ast_resolve_symbol_references(ASTNode* module) {
    // Stub实现：总是返回成功
    return 0;
}

// 序列化/反序列化函数
int ast_serialize_module(ASTNode* module, uint8_t** buffer, size_t* size) {
    if (!module || !buffer || !size) {
        return -1;
    }
    
    // Stub实现：创建一个简单的序列化数据
    *size = 16;
    *buffer = malloc(*size);
    if (*buffer) {
        memset(*buffer, 0, *size);
        // 写入简单的魔数
        (*buffer)[0] = 'A';
        (*buffer)[1] = 'S';
        (*buffer)[2] = 'T';
        (*buffer)[3] = 'C';
        return 0;
    }
    return -1;
}

ASTNode* ast_deserialize_module(const uint8_t* buffer, size_t size) {
    if (!buffer || size < 4) {
        return NULL;
    }
    
    // Stub实现：检查魔数并创建一个简单的模块节点
    if (buffer[0] == 'A' && buffer[1] == 'S' && buffer[2] == 'T' && buffer[3] == 'C') {
        return ast_create_node(ASTC_MODULE_DECL, 1, 1);
    }
    return NULL;
}

// 验证函数
int ast_validate_module(ASTNode* module) {
    // Stub实现：总是返回成功
    return module ? 0 : -1;
}

int ast_validate_export_declaration(ASTNode* export_decl) {
    // Stub实现：总是返回成功
    return export_decl ? 0 : -1;
}

int ast_validate_import_declaration(ASTNode* import_decl) {
    // Stub实现：总是返回成功
    return import_decl ? 0 : -1;
}

// ASTC程序管理函数
ASTCProgram* astc_load_program(const char* astc_file) {
    if (!astc_file) {
        return NULL;
    }
    
    // Stub实现：创建一个简单的程序结构
    ASTCProgram* program = malloc(sizeof(ASTCProgram));
    if (program) {
        memset(program, 0, sizeof(ASTCProgram));
        strncpy(program->program_name, "test_program", sizeof(program->program_name) - 1);
        program->version = 1;
        program->flags = 0;
        program->entry_point = 0;
    }
    return program;
}

void astc_free_program(ASTCProgram* program) {
    if (program) {
        if (program->source_code) {
            free(program->source_code);
        }
        if (program->bytecode) {
            free(program->bytecode);
        }
        if (program->compiler_context) {
            free(program->compiler_context);
        }
        free(program);
    }
}

int astc_validate_program(const ASTCProgram* program) {
    // Stub实现：基本验证
    if (!program) {
        return -1;
    }
    
    // 检查基本字段
    if (program->version == 0) {
        return -1;
    }
    
    return 0;
}

// 模块符号解析函数 (修复test_module_system.c中的警告)
void* module_sym(Module* module, const char* symbol_name) {
    if (!module || !symbol_name) {
        return NULL;
    }
    
    // 使用已有的module_resolve函数
    return module_resolve(module, symbol_name);
} 