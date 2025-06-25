/**
 * program.c - ASTC程序模块实现
 * 
 * 该文件实现了ASTC程序模块的基本功能，用于创建和管理ASTC格式的程序。
 * Program模块是平台无关的程序逻辑部分。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "program.h"
#include "astc.h"
#include "c2astc.h"

// ===============================================
// 内部函数声明
// ===============================================

// 设置错误信息
static void program_set_error(Program* program, const char* format, ...);

// ===============================================
// 公共函数实现
// ===============================================

bool program_init(Program* program, const ProgramConfig* config) {
    if (!program || !config) return false;
    
    // 初始化程序
    memset(program, 0, sizeof(Program));
    
    // 复制配置
    program->config.name = config->name ? strdup(config->name) : strdup("unnamed");
    program->config.version = config->version ? strdup(config->version) : strdup("0.1.0");
    program->config.author = config->author ? strdup(config->author) : strdup("unknown");
    program->config.description = config->description ? strdup(config->description) : strdup("");
    program->config.debug_mode = config->debug_mode;
    
    return true;
}

void program_destroy(Program* program) {
    if (!program) return;
    
    // 释放根节点
    if (program->root) {
        ast_free(program->root);
        program->root = NULL;
    }
    
    // 释放配置中的字符串
    if (program->config.name) {
        free((void*)program->config.name);
        program->config.name = NULL;
    }
    
    if (program->config.version) {
        free((void*)program->config.version);
        program->config.version = NULL;
    }
    
    if (program->config.author) {
        free((void*)program->config.author);
        program->config.author = NULL;
    }
    
    if (program->config.description) {
        free((void*)program->config.description);
        program->config.description = NULL;
    }
}

bool program_create_empty(Program* program) {
    if (!program) {
        return false;
    }
    
    // 释放之前的根节点
    if (program->root) {
        ast_free(program->root);
        program->root = NULL;
    }
    
    // 创建空的翻译单元
    program->root = ast_create_node(ASTC_TRANSLATION_UNIT, 1, 1);
    if (!program->root) {
        program_set_error(program, "无法创建翻译单元节点");
        return false;
    }
    
    // 初始化声明列表
    program->root->data.translation_unit.declarations = NULL;
    program->root->data.translation_unit.declaration_count = 0;
    
    return true;
}

bool program_create_from_c(Program* program, const char* source) {
    if (!program || !source) {
        program_set_error(program, "无效的参数");
        return false;
    }
    
    // 释放之前的根节点
    if (program->root) {
        ast_free(program->root);
        program->root = NULL;
    }
    
    // 使用c2astc库转换C源代码
    C2AstcOptions options = c2astc_default_options();
    options.emit_debug_info = program->config.debug_mode;
    
    program->root = c2astc_convert(source, &options);
    if (!program->root) {
        program_set_error(program, "转换C源代码失败: %s", c2astc_get_error());
        return false;
    }
    
    return true;
}

bool program_create_from_file(Program* program, const char* path) {
    if (!program || !path) {
        program_set_error(program, "无效的参数");
        return false;
    }
    
    // 释放之前的根节点
    if (program->root) {
        ast_free(program->root);
        program->root = NULL;
    }
    
    // 使用c2astc库转换C源文件
    C2AstcOptions options = c2astc_default_options();
    options.emit_debug_info = program->config.debug_mode;
    
    program->root = c2astc_convert_file(path, &options);
    if (!program->root) {
        program_set_error(program, "转换C源文件失败: %s", c2astc_get_error());
        return false;
    }
    
    return true;
}

bool program_add_function(Program* program, const char* name, struct ASTNode* return_type,
                          struct ASTNode** param_types, const char** param_names, int param_count,
                          struct ASTNode* body) {
    if (!program || !name || !return_type || !body) {
        program_set_error(program, "无效的参数");
        return false;
    }
    
    // 检查根节点
    if (!program->root || program->root->type != ASTC_TRANSLATION_UNIT) {
        program_set_error(program, "无效的程序根节点");
        return false;
    }
    
    // 创建函数声明节点
    struct ASTNode* func_decl = ast_create_node(ASTC_FUNC_DECL, 1, 1);
    if (!func_decl) {
        program_set_error(program, "无法创建函数声明节点");
        return false;
    }
    
    // 设置函数名
    func_decl->data.func_decl.name = strdup(name);
    if (!func_decl->data.func_decl.name) {
        ast_free(func_decl);
        program_set_error(program, "内存分配失败");
        return false;
    }
    
    // 设置返回类型
    func_decl->data.func_decl.return_type = return_type;
    
    // 设置参数
    func_decl->data.func_decl.param_count = param_count;
    if (param_count > 0) {
        func_decl->data.func_decl.params = (struct ASTNode**)malloc(param_count * sizeof(struct ASTNode*));
        if (!func_decl->data.func_decl.params) {
            ast_free(func_decl);
            program_set_error(program, "内存分配失败");
            return false;
        }
        
        // 创建参数节点
        for (int i = 0; i < param_count; i++) {
            struct ASTNode* param = ast_create_node(ASTC_VAR_DECL, 1, 1);
            if (!param) {
                // 释放已创建的参数
                for (int j = 0; j < i; j++) {
                    ast_free(func_decl->data.func_decl.params[j]);
                }
                free(func_decl->data.func_decl.params);
                ast_free(func_decl);
                program_set_error(program, "无法创建参数节点");
                return false;
            }
            
            // 设置参数名
            param->data.var_decl.name = strdup(param_names[i]);
            if (!param->data.var_decl.name) {
                // 释放已创建的参数
                for (int j = 0; j < i; j++) {
                    ast_free(func_decl->data.func_decl.params[j]);
                }
                ast_free(param);
                free(func_decl->data.func_decl.params);
                ast_free(func_decl);
                program_set_error(program, "内存分配失败");
                return false;
            }
            
            // 设置参数类型
            param->data.var_decl.type = param_types[i];
            param->data.var_decl.initializer = NULL;
            
            func_decl->data.func_decl.params[i] = param;
        }
    } else {
        func_decl->data.func_decl.params = NULL;
    }
    
    // 设置函数体
    func_decl->data.func_decl.has_body = true;
    func_decl->data.func_decl.body = body;
    
    // 扩展声明列表
    int decl_count = program->root->data.translation_unit.declaration_count;
    struct ASTNode** new_declarations = (struct ASTNode**)realloc(
        program->root->data.translation_unit.declarations,
        (decl_count + 1) * sizeof(struct ASTNode*)
    );
    
    if (!new_declarations) {
        ast_free(func_decl);
        program_set_error(program, "内存分配失败");
        return false;
    }
    
    program->root->data.translation_unit.declarations = new_declarations;
    program->root->data.translation_unit.declarations[decl_count] = func_decl;
    program->root->data.translation_unit.declaration_count = decl_count + 1;
    
    return true;
}

bool program_add_global(Program* program, const char* name, struct ASTNode* type,
                        struct ASTNode* initializer) {
    if (!program || !name || !type) {
        program_set_error(program, "无效的参数");
        return false;
    }
    
    // 检查根节点
    if (!program->root || program->root->type != ASTC_TRANSLATION_UNIT) {
        program_set_error(program, "无效的程序根节点");
        return false;
    }
    
    // 创建变量声明节点
    struct ASTNode* var_decl = ast_create_node(ASTC_VAR_DECL, 1, 1);
    if (!var_decl) {
        program_set_error(program, "无法创建变量声明节点");
        return false;
    }
    
    // 设置变量名
    var_decl->data.var_decl.name = strdup(name);
    if (!var_decl->data.var_decl.name) {
        ast_free(var_decl);
        program_set_error(program, "内存分配失败");
        return false;
    }
    
    // 设置变量类型
    var_decl->data.var_decl.type = type;
    
    // 设置初始化表达式
    var_decl->data.var_decl.initializer = initializer;
    
    // 扩展声明列表
    int decl_count = program->root->data.translation_unit.declaration_count;
    struct ASTNode** new_declarations = (struct ASTNode**)realloc(
        program->root->data.translation_unit.declarations,
        (decl_count + 1) * sizeof(struct ASTNode*)
    );
    
    if (!new_declarations) {
        ast_free(var_decl);
        program_set_error(program, "内存分配失败");
        return false;
    }
    
    program->root->data.translation_unit.declarations = new_declarations;
    program->root->data.translation_unit.declarations[decl_count] = var_decl;
    program->root->data.translation_unit.declaration_count = decl_count + 1;
    
    return true;
}

uint8_t* program_serialize(Program* program, size_t* out_size) {
    if (!program || !out_size) {
        if (program) {
            program_set_error(program, "无效的参数");
        }
        return NULL;
    }
    
    // 检查根节点
    if (!program->root) {
        program_set_error(program, "程序为空");
        return NULL;
    }
    
    // 使用c2astc库序列化程序
    uint8_t* data = c2astc_serialize(program->root, out_size);
    if (!data) {
        program_set_error(program, "序列化程序失败: %s", c2astc_get_error());
        return NULL;
    }
    
    return data;
}

bool program_save(Program* program, const char* path) {
    if (!program || !path) {
        program_set_error(program, "无效的参数");
        return false;
    }
    
    // 序列化程序
    size_t size;
    uint8_t* data = program_serialize(program, &size);
    if (!data) {
        return false;
    }
    
    // 打开文件
    FILE* file = fopen(path, "wb");
    if (!file) {
        free(data);
        program_set_error(program, "无法打开文件: %s", path);
        return false;
    }
    
    // 写入数据
    size_t bytes_written = fwrite(data, 1, size, file);
    fclose(file);
    free(data);
    
    if (bytes_written != size) {
        program_set_error(program, "写入文件失败: %s", path);
        return false;
    }
    
    return true;
}

const char* program_get_error(Program* program) {
    if (!program) return "无效的程序实例";
    return program->error_message;
}

struct ASTNode* program_create_type(const char* type_name) {
    if (!type_name) return NULL;
    
    // 创建类型节点
    struct ASTNode* type_node = ast_create_node(ASTC_TYPE_SPECIFIER, 1, 1);
    if (!type_node) return NULL;
    
    // 根据类型名设置类型
    if (strcmp(type_name, "void") == 0) {
        type_node->data.type_specifier.type = ASTC_TYPE_VOID;
    } else if (strcmp(type_name, "char") == 0) {
        type_node->data.type_specifier.type = ASTC_TYPE_CHAR;
    } else if (strcmp(type_name, "short") == 0) {
        type_node->data.type_specifier.type = ASTC_TYPE_SHORT;
    } else if (strcmp(type_name, "int") == 0) {
        type_node->data.type_specifier.type = ASTC_TYPE_INT;
    } else if (strcmp(type_name, "long") == 0) {
        type_node->data.type_specifier.type = ASTC_TYPE_LONG;
    } else if (strcmp(type_name, "float") == 0) {
        type_node->data.type_specifier.type = ASTC_TYPE_FLOAT;
    } else if (strcmp(type_name, "double") == 0) {
        type_node->data.type_specifier.type = ASTC_TYPE_DOUBLE;
    } else if (strcmp(type_name, "signed") == 0) {
        type_node->data.type_specifier.type = ASTC_TYPE_SIGNED;
    } else if (strcmp(type_name, "unsigned") == 0) {
        type_node->data.type_specifier.type = ASTC_TYPE_UNSIGNED;
    } else {
        // 未知类型
        ast_free(type_node);
        return NULL;
    }
    
    return type_node;
}

struct ASTNode* program_create_pointer_type(struct ASTNode* base_type) {
    if (!base_type) return NULL;
    
    // 创建指针类型节点
    struct ASTNode* ptr_type = ast_create_node(ASTC_POINTER_TYPE, base_type->line, base_type->column);
    if (!ptr_type) return NULL;
    
    ptr_type->data.pointer_type.base_type = base_type;
    ptr_type->data.pointer_type.pointer_level = 1;
    
    return ptr_type;
}

struct ASTNode* program_create_array_type(struct ASTNode* element_type, int size) {
    if (!element_type) return NULL;
    
    // 创建数组类型节点
    struct ASTNode* array_type = ast_create_node(ASTC_ARRAY_TYPE, element_type->line, element_type->column);
    if (!array_type) return NULL;
    
    array_type->data.array_type.element_type = element_type;
    array_type->data.array_type.dimensions = 1;
    
    // 创建大小表达式
    if (size > 0) {
        struct ASTNode* size_expr = ast_create_node(ASTC_EXPR_CONSTANT, element_type->line, element_type->column);
        if (!size_expr) {
            ast_free(array_type);
            return NULL;
        }
        
        size_expr->data.constant.type = ASTC_TYPE_INT;
        size_expr->data.constant.int_val = size;
        
        array_type->data.array_type.size_expr = size_expr;
    } else {
        array_type->data.array_type.size_expr = NULL;
    }
    
    return array_type;
}

// ===============================================
// 内部函数实现
// ===============================================

// 设置错误信息
static void program_set_error(Program* program, const char* format, ...) {
    if (!program) return;
    
    va_list args;
    va_start(args, format);
    vsnprintf(program->error_message, sizeof(program->error_message), format, args);
    va_end(args);
} 