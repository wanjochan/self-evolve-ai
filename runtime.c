/**
 * runtime.c - ASTC虚拟机运行时实现
 * 
 * 该文件实现了ASTC虚拟机的基本功能，用于执行ASTC格式的程序。
 * Runtime模块是连接Loader和Program的关键组件。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "astc.h"

// ===============================================
// 内部函数声明
// ===============================================

// 设置错误信息
static void runtime_set_error(RuntimeVM* vm, const char* format, ...);

// 执行函数
static int runtime_execute_function(RuntimeVM* vm, struct ASTNode* func, RuntimeValue* args, size_t arg_count);

// 执行语句
static bool runtime_execute_statement(RuntimeVM* vm, struct ASTNode* stmt);

// 执行表达式
static RuntimeValue runtime_evaluate_expression(RuntimeVM* vm, struct ASTNode* expr);

// 查找函数
static RuntimeFunctionEntry* runtime_find_function(RuntimeVM* vm, const char* name);

// 查找全局变量
static RuntimeGlobalEntry* runtime_find_global(RuntimeVM* vm, const char* name);

// 创建调用帧
static RuntimeCallFrame* runtime_create_call_frame(RuntimeVM* vm, struct ASTNode* func, RuntimeValue* args, size_t arg_count);

// 销毁调用帧
static void runtime_destroy_call_frame(RuntimeVM* vm, RuntimeCallFrame* frame);

// ===============================================
// 公共函数实现
// ===============================================

bool runtime_init(RuntimeVM* vm) {
    if (!vm) return false;
    
    // 初始化内存
    vm->memory.stack_size = RUNTIME_STACK_SIZE;
    vm->memory.stack = (uint8_t*)malloc(vm->memory.stack_size);
    if (!vm->memory.stack) {
        runtime_set_error(vm, "无法分配栈内存");
        return false;
    }
    vm->memory.stack_pointer = 0;
    
    vm->memory.heap_size = RUNTIME_HEAP_INITIAL_SIZE;
    vm->memory.heap = (uint8_t*)malloc(vm->memory.heap_size);
    if (!vm->memory.heap) {
        free(vm->memory.stack);
        runtime_set_error(vm, "无法分配堆内存");
        return false;
    }
    vm->memory.heap_used = 0;
    
    // 初始化函数表
    vm->functions.capacity = 16;
    vm->functions.count = 0;
    vm->functions.entries = (RuntimeFunctionEntry*)malloc(vm->functions.capacity * sizeof(RuntimeFunctionEntry));
    if (!vm->functions.entries) {
        free(vm->memory.stack);
        free(vm->memory.heap);
        runtime_set_error(vm, "无法分配函数表");
        return false;
    }
    
    // 初始化全局变量表
    vm->globals.capacity = 16;
    vm->globals.count = 0;
    vm->globals.entries = (RuntimeGlobalEntry*)malloc(vm->globals.capacity * sizeof(RuntimeGlobalEntry));
    if (!vm->globals.entries) {
        free(vm->memory.stack);
        free(vm->memory.heap);
        free(vm->functions.entries);
        runtime_set_error(vm, "无法分配全局变量表");
        return false;
    }
    
    // 初始化其他字段
    vm->current_frame = NULL;
    vm->exit_code = 0;
    vm->running = false;
    vm->error_message[0] = '\0';
    
    return true;
}

void runtime_destroy(RuntimeVM* vm) {
    if (!vm) return;
    
    // 释放内存
    if (vm->memory.stack) {
        free(vm->memory.stack);
        vm->memory.stack = NULL;
    }
    
    if (vm->memory.heap) {
        free(vm->memory.heap);
        vm->memory.heap = NULL;
    }
    
    // 释放函数表
    if (vm->functions.entries) {
        // 释放函数名
        for (size_t i = 0; i < vm->functions.count; i++) {
            if (vm->functions.entries[i].name) {
                free((void*)vm->functions.entries[i].name);
            }
        }
        free(vm->functions.entries);
        vm->functions.entries = NULL;
    }
    
    // 释放全局变量表
    if (vm->globals.entries) {
        // 释放变量名
        for (size_t i = 0; i < vm->globals.count; i++) {
            if (vm->globals.entries[i].name) {
                free((void*)vm->globals.entries[i].name);
            }
        }
        free(vm->globals.entries);
        vm->globals.entries = NULL;
    }
    
    // 释放调用帧
    while (vm->current_frame) {
        RuntimeCallFrame* frame = vm->current_frame;
        vm->current_frame = frame->prev;
        runtime_destroy_call_frame(vm, frame);
    }
}

bool runtime_load_program(RuntimeVM* vm, struct ASTNode* root) {
    if (!vm || !root) {
        runtime_set_error(vm, "无效的参数");
        return false;
    }
    
    // 检查根节点类型
    if (root->type != ASTC_TRANSLATION_UNIT) {
        runtime_set_error(vm, "无效的ASTC根节点类型");
        return false;
    }
    
    // 遍历声明列表
    for (int i = 0; i < root->data.translation_unit.declaration_count; i++) {
        struct ASTNode* decl = root->data.translation_unit.declarations[i];
        
        // 处理函数声明
        if (decl->type == ASTC_FUNC_DECL) {
            // 检查函数表容量
            if (vm->functions.count >= vm->functions.capacity) {
                size_t new_capacity = vm->functions.capacity * 2;
                RuntimeFunctionEntry* new_entries = (RuntimeFunctionEntry*)realloc(
                    vm->functions.entries, 
                    new_capacity * sizeof(RuntimeFunctionEntry)
                );
                
                if (!new_entries) {
                    runtime_set_error(vm, "无法扩展函数表");
                    return false;
                }
                
                vm->functions.entries = new_entries;
                vm->functions.capacity = new_capacity;
            }
            
            // 添加函数到函数表
            RuntimeFunctionEntry* entry = &vm->functions.entries[vm->functions.count++];
            entry->name = strdup(decl->data.func_decl.name);
            entry->node = decl;
            entry->is_native = false;
            entry->native_func = NULL;
        }
        
        // 处理全局变量声明
        else if (decl->type == ASTC_VAR_DECL) {
            // 检查全局变量表容量
            if (vm->globals.count >= vm->globals.capacity) {
                size_t new_capacity = vm->globals.capacity * 2;
                RuntimeGlobalEntry* new_entries = (RuntimeGlobalEntry*)realloc(
                    vm->globals.entries, 
                    new_capacity * sizeof(RuntimeGlobalEntry)
                );
                
                if (!new_entries) {
                    runtime_set_error(vm, "无法扩展全局变量表");
                    return false;
                }
                
                vm->globals.entries = new_entries;
                vm->globals.capacity = new_capacity;
            }
            
            // 添加全局变量到全局变量表
            RuntimeGlobalEntry* entry = &vm->globals.entries[vm->globals.count++];
            entry->name = strdup(decl->data.var_decl.name);
            entry->is_mutable = true;  // 默认可变
            
            // 初始化全局变量
            if (decl->data.var_decl.initializer) {
                entry->value = runtime_evaluate_expression(vm, decl->data.var_decl.initializer);
            } else {
                // 默认初始化为0
                entry->value = runtime_value_i32(0);
            }
        }
    }
    
    return true;
}

int runtime_execute(RuntimeVM* vm, const char* entry_point) {
    if (!vm || !entry_point) {
        runtime_set_error(vm, "无效的参数");
        return -1;
    }
    
    // 查找入口函数
    RuntimeFunctionEntry* entry = runtime_find_function(vm, entry_point);
    if (!entry) {
        runtime_set_error(vm, "找不到入口函数: %s", entry_point);
        return -1;
    }
    
    // 执行入口函数
    vm->running = true;
    vm->exit_code = runtime_execute_function(vm, entry->node, NULL, 0);
    vm->running = false;
    
    return vm->exit_code;
}

bool runtime_register_native_function(RuntimeVM* vm, const char* name, void* func) {
    if (!vm || !name || !func) {
        runtime_set_error(vm, "无效的参数");
        return false;
    }
    
    // 检查函数表容量
    if (vm->functions.count >= vm->functions.capacity) {
        size_t new_capacity = vm->functions.capacity * 2;
        RuntimeFunctionEntry* new_entries = (RuntimeFunctionEntry*)realloc(
            vm->functions.entries, 
            new_capacity * sizeof(RuntimeFunctionEntry)
        );
        
        if (!new_entries) {
            runtime_set_error(vm, "无法扩展函数表");
            return false;
        }
        
        vm->functions.entries = new_entries;
        vm->functions.capacity = new_capacity;
    }
    
    // 添加原生函数到函数表
    RuntimeFunctionEntry* entry = &vm->functions.entries[vm->functions.count++];
    entry->name = strdup(name);
    entry->node = NULL;
    entry->is_native = true;
    entry->native_func = func;
    
    return true;
}

const char* runtime_get_error(RuntimeVM* vm) {
    if (!vm) return "无效的虚拟机实例";
    return vm->error_message;
}

void* runtime_allocate(RuntimeVM* vm, size_t size) {
    if (!vm || size == 0) return NULL;
    
    // 检查剩余堆空间
    if (vm->memory.heap_used + size > vm->memory.heap_size) {
        // 扩展堆
        size_t new_size = vm->memory.heap_size * 2;
        while (vm->memory.heap_used + size > new_size) {
            new_size *= 2;
        }
        
        uint8_t* new_heap = (uint8_t*)realloc(vm->memory.heap, new_size);
        if (!new_heap) {
            runtime_set_error(vm, "内存分配失败");
            return NULL;
        }
        
        vm->memory.heap = new_heap;
        vm->memory.heap_size = new_size;
    }
    
    // 分配内存
    void* ptr = vm->memory.heap + vm->memory.heap_used;
    vm->memory.heap_used += size;
    
    // 初始化为0
    memset(ptr, 0, size);
    
    return ptr;
}

void runtime_free(RuntimeVM* vm, void* ptr) {
    // 简单实现，不实际释放内存
    // 真实实现需要使用垃圾回收或引用计数
}

// 创建RuntimeValue
RuntimeValue runtime_value_i32(int32_t value) {
    RuntimeValue val;
    val.type = RT_VAL_I32;
    val.value.i32 = value;
    return val;
}

RuntimeValue runtime_value_i64(int64_t value) {
    RuntimeValue val;
    val.type = RT_VAL_I64;
    val.value.i64 = value;
    return val;
}

RuntimeValue runtime_value_f32(float value) {
    RuntimeValue val;
    val.type = RT_VAL_F32;
    val.value.f32 = value;
    return val;
}

RuntimeValue runtime_value_f64(double value) {
    RuntimeValue val;
    val.type = RT_VAL_F64;
    val.value.f64 = value;
    return val;
}

RuntimeValue runtime_value_ptr(void* value) {
    RuntimeValue val;
    val.type = RT_VAL_PTR;
    val.value.ptr = value;
    return val;
}

RuntimeValue runtime_value_func_ref(struct ASTNode* value) {
    RuntimeValue val;
    val.type = RT_VAL_FUNC_REF;
    val.value.func_ref = value;
    return val;
}

// ===============================================
// 内部函数实现
// ===============================================

// 设置错误信息
static void runtime_set_error(RuntimeVM* vm, const char* format, ...) {
    if (!vm) return;
    
    va_list args;
    va_start(args, format);
    vsnprintf(vm->error_message, sizeof(vm->error_message), format, args);
    va_end(args);
}

// 查找函数
static RuntimeFunctionEntry* runtime_find_function(RuntimeVM* vm, const char* name) {
    if (!vm || !name) return NULL;
    
    for (size_t i = 0; i < vm->functions.count; i++) {
        if (strcmp(vm->functions.entries[i].name, name) == 0) {
            return &vm->functions.entries[i];
        }
    }
    
    return NULL;
}

// 查找全局变量
static RuntimeGlobalEntry* runtime_find_global(RuntimeVM* vm, const char* name) {
    if (!vm || !name) return NULL;
    
    for (size_t i = 0; i < vm->globals.count; i++) {
        if (strcmp(vm->globals.entries[i].name, name) == 0) {
            return &vm->globals.entries[i];
        }
    }
    
    return NULL;
}

// 创建调用帧
static RuntimeCallFrame* runtime_create_call_frame(RuntimeVM* vm, struct ASTNode* func, RuntimeValue* args, size_t arg_count) {
    if (!vm || !func) return NULL;
    
    // 分配调用帧
    RuntimeCallFrame* frame = (RuntimeCallFrame*)malloc(sizeof(RuntimeCallFrame));
    if (!frame) {
        runtime_set_error(vm, "无法分配调用帧");
        return NULL;
    }
    
    // 初始化调用帧
    frame->func = func;
    frame->bp = vm->memory.stack_pointer;
    frame->ip = 0;
    frame->prev = vm->current_frame;
    
    // 分配局部变量
    size_t local_count = func->data.func_decl.param_count;
    frame->locals = (RuntimeValue*)malloc(local_count * sizeof(RuntimeValue));
    if (!frame->locals) {
        free(frame);
        runtime_set_error(vm, "无法分配局部变量");
        return NULL;
    }
    frame->local_count = local_count;
    
    // 初始化参数
    for (size_t i = 0; i < arg_count && i < local_count; i++) {
        frame->locals[i] = args[i];
    }
    
    // 默认初始化其余局部变量
    for (size_t i = arg_count; i < local_count; i++) {
        frame->locals[i] = runtime_value_i32(0);
    }
    
    return frame;
}

// 销毁调用帧
static void runtime_destroy_call_frame(RuntimeVM* vm, RuntimeCallFrame* frame) {
    if (!vm || !frame) return;
    
    // 释放局部变量
    if (frame->locals) {
        free(frame->locals);
    }
    
    free(frame);
}

// 执行函数
static int runtime_execute_function(RuntimeVM* vm, struct ASTNode* func, RuntimeValue* args, size_t arg_count) {
    if (!vm || !func) return -1;
    
    // 检查函数类型
    if (func->type != ASTC_FUNC_DECL) {
        runtime_set_error(vm, "无效的函数节点类型");
        return -1;
    }
    
    // 检查函数体
    if (!func->data.func_decl.has_body) {
        runtime_set_error(vm, "函数没有实现");
        return -1;
    }
    
    // 创建调用帧
    RuntimeCallFrame* frame = runtime_create_call_frame(vm, func, args, arg_count);
    if (!frame) {
        return -1;
    }
    
    // 设置当前帧
    vm->current_frame = frame;
    
    // 执行函数体
    bool result = runtime_execute_statement(vm, func->data.func_decl.body);
    
    // 恢复前一帧
    vm->current_frame = frame->prev;
    
    // 获取返回值
    int return_value = 0;
    if (result && frame->locals && frame->local_count > 0) {
        // 使用第一个局部变量作为返回值
        if (frame->locals[0].type == RT_VAL_I32) {
            return_value = frame->locals[0].value.i32;
        } else if (frame->locals[0].type == RT_VAL_I64) {
            return_value = (int)frame->locals[0].value.i64;
        }
    }
    
    // 销毁调用帧
    runtime_destroy_call_frame(vm, frame);
    
    return return_value;
}

// 执行语句
static bool runtime_execute_statement(RuntimeVM* vm, struct ASTNode* stmt) {
    if (!vm || !stmt) return false;
    
    switch (stmt->type) {
        case ASTC_COMPOUND_STMT: {
            // 执行复合语句
            for (int i = 0; i < stmt->data.compound_stmt.statement_count; i++) {
                if (!runtime_execute_statement(vm, stmt->data.compound_stmt.statements[i])) {
                    return false;
                }
            }
            return true;
        }
        
        case ASTC_EXPR_STMT: {
            // 执行表达式语句
            runtime_evaluate_expression(vm, stmt->data.expr_stmt.expr);
            return true;
        }
        
        case ASTC_RETURN_STMT: {
            // 执行return语句
            if (stmt->data.return_stmt.value) {
                RuntimeValue result = runtime_evaluate_expression(vm, stmt->data.return_stmt.value);
                if (vm->current_frame && vm->current_frame->locals && vm->current_frame->local_count > 0) {
                    // 将返回值存储在第一个局部变量中
                    vm->current_frame->locals[0] = result;
                }
            }
            return true;
        }
        
        case ASTC_IF_STMT: {
            // 执行if语句
            RuntimeValue condition = runtime_evaluate_expression(vm, stmt->data.if_stmt.condition);
            bool cond_result = false;
            
            // 检查条件
            if (condition.type == RT_VAL_I32) {
                cond_result = condition.value.i32 != 0;
            } else if (condition.type == RT_VAL_I64) {
                cond_result = condition.value.i64 != 0;
            } else if (condition.type == RT_VAL_PTR) {
                cond_result = condition.value.ptr != NULL;
            }
            
            // 执行分支
            if (cond_result) {
                return runtime_execute_statement(vm, stmt->data.if_stmt.then_branch);
            } else if (stmt->data.if_stmt.else_branch) {
                return runtime_execute_statement(vm, stmt->data.if_stmt.else_branch);
            }
            
            return true;
        }
        
        case ASTC_WHILE_STMT: {
            // 执行while语句
            while (true) {
                // 检查条件
                RuntimeValue condition = runtime_evaluate_expression(vm, stmt->data.while_stmt.condition);
                bool cond_result = false;
                
                if (condition.type == RT_VAL_I32) {
                    cond_result = condition.value.i32 != 0;
                } else if (condition.type == RT_VAL_I64) {
                    cond_result = condition.value.i64 != 0;
                } else if (condition.type == RT_VAL_PTR) {
                    cond_result = condition.value.ptr != NULL;
                }
                
                if (!cond_result) {
                    break;
                }
                
                // 执行循环体
                if (!runtime_execute_statement(vm, stmt->data.while_stmt.body)) {
                    return false;
                }
            }
            
            return true;
        }
        
        // 其他语句类型...
        
        default:
            runtime_set_error(vm, "不支持的语句类型: %d", stmt->type);
            return false;
    }
}

// 执行表达式
static RuntimeValue runtime_evaluate_expression(RuntimeVM* vm, struct ASTNode* expr) {
    if (!vm || !expr) {
        return runtime_value_i32(0);
    }
    
    switch (expr->type) {
        case ASTC_EXPR_IDENTIFIER: {
            // 查找局部变量
            if (vm->current_frame && vm->current_frame->locals) {
                // TODO: 实现局部变量查找
            }
            
            // 查找全局变量
            RuntimeGlobalEntry* global = runtime_find_global(vm, expr->data.identifier.name);
            if (global) {
                return global->value;
            }
            
            runtime_set_error(vm, "未定义的标识符: %s", expr->data.identifier.name);
            return runtime_value_i32(0);
        }
        
        case ASTC_EXPR_CONSTANT: {
            // 返回常量值
            if (expr->data.constant.type == ASTC_TYPE_INT) {
                return runtime_value_i32((int32_t)expr->data.constant.int_val);
            } else if (expr->data.constant.type == ASTC_TYPE_FLOAT) {
                return runtime_value_f32((float)expr->data.constant.float_val);
            }
            
            return runtime_value_i32(0);
        }
        
        case ASTC_BINARY_OP: {
            // 计算二元操作
            RuntimeValue left = runtime_evaluate_expression(vm, expr->data.binary_op.left);
            RuntimeValue right = runtime_evaluate_expression(vm, expr->data.binary_op.right);
            
            // 处理不同类型的二元操作
            switch (expr->data.binary_op.op) {
                case ASTC_OP_ADD:
                    if (left.type == RT_VAL_I32 && right.type == RT_VAL_I32) {
                        return runtime_value_i32(left.value.i32 + right.value.i32);
                    }
                    break;
                case ASTC_OP_SUB:
                    if (left.type == RT_VAL_I32 && right.type == RT_VAL_I32) {
                        return runtime_value_i32(left.value.i32 - right.value.i32);
                    }
                    break;
                case ASTC_OP_MUL:
                    if (left.type == RT_VAL_I32 && right.type == RT_VAL_I32) {
                        return runtime_value_i32(left.value.i32 * right.value.i32);
                    }
                    break;
                case ASTC_OP_DIV:
                    if (left.type == RT_VAL_I32 && right.type == RT_VAL_I32) {
                        if (right.value.i32 == 0) {
                            runtime_set_error(vm, "除零错误");
                            return runtime_value_i32(0);
                        }
                        return runtime_value_i32(left.value.i32 / right.value.i32);
                    }
                    break;
                // 其他操作符...
            }
            
            runtime_set_error(vm, "不支持的二元操作");
            return runtime_value_i32(0);
        }
        
        case ASTC_CALL_EXPR: {
            // 函数调用
            if (expr->data.call_expr.callee->type != ASTC_EXPR_IDENTIFIER) {
                runtime_set_error(vm, "不支持的函数调用类型");
                return runtime_value_i32(0);
            }
            
            // 获取函数名
            const char* func_name = expr->data.call_expr.callee->data.identifier.name;
            
            // 查找函数
            RuntimeFunctionEntry* func_entry = runtime_find_function(vm, func_name);
            if (!func_entry) {
                runtime_set_error(vm, "未定义的函数: %s", func_name);
                return runtime_value_i32(0);
            }
            
            // 计算参数
            RuntimeValue* args = NULL;
            if (expr->data.call_expr.arg_count > 0) {
                args = (RuntimeValue*)malloc(expr->data.call_expr.arg_count * sizeof(RuntimeValue));
                if (!args) {
                    runtime_set_error(vm, "无法分配参数内存");
                    return runtime_value_i32(0);
                }
                
                for (int i = 0; i < expr->data.call_expr.arg_count; i++) {
                    args[i] = runtime_evaluate_expression(vm, expr->data.call_expr.args[i]);
                }
            }
            
            // 调用函数
            int result;
            if (func_entry->is_native) {
                // 调用原生函数
                // TODO: 实现原生函数调用
                result = 0;
            } else {
                // 调用ASTC函数
                result = runtime_execute_function(vm, func_entry->node, args, expr->data.call_expr.arg_count);
            }
            
            // 释放参数
            if (args) {
                free(args);
            }
            
            return runtime_value_i32(result);
        }
        
        // 其他表达式类型...
        
        default:
            runtime_set_error(vm, "不支持的表达式类型: %d", expr->type);
            return runtime_value_i32(0);
    }
} 