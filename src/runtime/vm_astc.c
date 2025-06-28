/**
 * runtime.c - ASTC虚拟机运行时实现
 * 
 * 该文件实现了ASTC虚拟机的基本功能，用于执行ASTC格式的程序。
 * Runtime模块是连接Loader和Program的关键组件。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "runtime.h"
#include "astc.h"
#include "platform.h"

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

// 调用原生函数
static int runtime_call_native_function(RuntimeVM* vm, const char* func_name, RuntimeValue* args, size_t arg_count);

// 添加局部变量到映射表
static bool runtime_add_local_variable(RuntimeVM* vm, const char* name, RuntimeValue value);

// 查找局部变量
static RuntimeValue* runtime_find_local_variable(RuntimeVM* vm, const char* name);

// 性能统计初始化
static void runtime_perf_init(RuntimeVM* vm);

// 性能统计更新
static void runtime_perf_update(RuntimeVM* vm, const char* event);

// 性能统计报告
static void runtime_perf_report(RuntimeVM* vm);

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
    vm->debug_mode = false;
    vm->instruction_count = 0;
    vm->function_call_count = 0;

    // 性能统计初始化
    runtime_perf_init(vm);

    // 注册标准库原生函数
    runtime_register_native_function(vm, "printf", NULL);
    runtime_register_native_function(vm, "fopen", NULL);
    runtime_register_native_function(vm, "fwrite", NULL);
    runtime_register_native_function(vm, "fclose", NULL);
    runtime_register_native_function(vm, "malloc", NULL);
    runtime_register_native_function(vm, "free", NULL);

    return true;
}

void runtime_destroy(RuntimeVM* vm) {
    if (!vm) return;
    
    // 生成性能报告（如果启用了调试模式）
    if (vm->debug_mode) {
        runtime_perf_report(vm);
    }
    
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
    
    // 释放性能统计数据
    if (vm->perf_events) {
        free(vm->perf_events);
        vm->perf_events = NULL;
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
    
    if (vm->debug_mode) {
        printf("ASTC虚拟机启动, 入口点: %s\n", entry_point);
        runtime_perf_update(vm, "VM启动");
    }
    
    // 查找入口函数
    RuntimeFunctionEntry* entry_func = runtime_find_function(vm, entry_point);
    if (!entry_func) {
        runtime_set_error(vm, "找不到入口函数: %s", entry_point);
        return -1;
    }
    
    if (vm->debug_mode) {
        printf("找到入口函数: %s\n", entry_point);
    }
    
    // 设置运行状态
    vm->running = true;
    vm->exit_code = 0;
    
    // 执行入口函数
    RuntimeValue args[0]; // 空参数数组
    int result = runtime_execute_function(vm, entry_func->node, args, 0);
    
    // 更新退出码
    vm->exit_code = result;
    vm->running = false;
    
    if (vm->debug_mode) {
        printf("程序执行完成，返回值: %d\n", result);
        runtime_perf_update(vm, "VM关闭");
    }
    
    return result;
}

bool runtime_register_native_function(RuntimeVM* vm, const char* name, void* func) {
    if (!vm || !name) {
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

    // 初始化局部变量映射
    frame->local_map_capacity = 16;
    frame->local_map_count = 0;
    frame->local_map = (RuntimeLocalEntry*)malloc(frame->local_map_capacity * sizeof(RuntimeLocalEntry));
    if (!frame->local_map) {
        free(frame->locals);
        free(frame);
        runtime_set_error(vm, "无法分配局部变量映射");
        return NULL;
    }
    
    // 分配局部变量（至少分配一个用于返回值）
    size_t local_count = func->data.func_decl.param_count;
    if (local_count == 0) local_count = 1;  // 至少分配一个用于返回值
    frame->locals = (RuntimeValue*)malloc(local_count * sizeof(RuntimeValue));
    if (!frame->locals) {
        free(frame);
        runtime_set_error(vm, "无法分配局部变量");
        return NULL;
    }
    frame->local_count = local_count;
    
    // 初始化参数
    size_t param_count = func->data.func_decl.param_count;
    for (size_t i = 0; i < arg_count && i < param_count; i++) {
        frame->locals[i] = args[i];
    }

    // 默认初始化其余局部变量（包括返回值槽）
    for (size_t i = param_count; i < local_count; i++) {
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

    // 释放局部变量映射
    if (frame->local_map) {
        for (size_t i = 0; i < frame->local_map_count; i++) {
            if (frame->local_map[i].name) {
                free((void*)frame->local_map[i].name);
            }
        }
        free(frame->local_map);
    }

    free(frame);
}

// 添加局部变量到映射表
static bool runtime_add_local_variable(RuntimeVM* vm, const char* name, RuntimeValue value) {
    if (!vm || !vm->current_frame || !name) return false;

    RuntimeCallFrame* frame = vm->current_frame;

    // 检查映射表容量
    if (frame->local_map_count >= frame->local_map_capacity) {
        size_t new_capacity = frame->local_map_capacity * 2;
        RuntimeLocalEntry* new_map = (RuntimeLocalEntry*)realloc(
            frame->local_map,
            new_capacity * sizeof(RuntimeLocalEntry)
        );
        if (!new_map) {
            runtime_set_error(vm, "无法扩展局部变量映射表");
            return false;
        }
        frame->local_map = new_map;
        frame->local_map_capacity = new_capacity;
    }

    // 扩展局部变量数组
    size_t new_count = frame->local_count + 1;
    RuntimeValue* new_locals = (RuntimeValue*)realloc(frame->locals,
                                                      new_count * sizeof(RuntimeValue));
    if (!new_locals) {
        runtime_set_error(vm, "无法分配局部变量内存");
        return false;
    }

    frame->locals = new_locals;
    frame->locals[frame->local_count] = value;

    // 添加到映射表
    RuntimeLocalEntry* entry = &frame->local_map[frame->local_map_count];
    entry->name = strdup(name);
    entry->index = frame->local_count;

    frame->local_count = new_count;
    frame->local_map_count++;

    return true;
}

// 查找局部变量
static RuntimeValue* runtime_find_local_variable(RuntimeVM* vm, const char* name) {
    if (!vm || !vm->current_frame || !name) return NULL;

    RuntimeCallFrame* frame = vm->current_frame;

    // 在映射表中查找变量名
    for (size_t i = 0; i < frame->local_map_count; i++) {
        if (frame->local_map[i].name && strcmp(frame->local_map[i].name, name) == 0) {
            size_t index = frame->local_map[i].index;
            if (index < frame->local_count) {
                return &frame->locals[index];
            }
        }
    }

    return NULL;
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
        // 使用最后一个局部变量作为返回值
        RuntimeValue* ret_val = &frame->locals[frame->local_count - 1];
        if (ret_val->type == RT_VAL_I32) {
            return_value = ret_val->value.i32;
        } else if (ret_val->type == RT_VAL_I64) {
            return_value = (int)ret_val->value.i64;
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
                    // 将返回值存储在最后一个局部变量槽中（专门用于返回值）
                    vm->current_frame->locals[vm->current_frame->local_count - 1] = result;
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

        case ASTC_VAR_DECL: {
            // 变量声明语句
            const char* var_name = stmt->data.var_decl.name;

            // 计算初始值
            RuntimeValue init_value = runtime_value_i32(0);
            if (stmt->data.var_decl.initializer) {
                init_value = runtime_evaluate_expression(vm, stmt->data.var_decl.initializer);
            }

            // 如果在函数内，添加到局部变量
            if (vm->current_frame) {
                if (!runtime_add_local_variable(vm, var_name, init_value)) {
                    return false;
                }
            } else {
                // 添加到全局变量
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
                entry->name = strdup(var_name);
                entry->is_mutable = true;
                entry->value = init_value;
            }

            return true;
        }

        case ASTC_FOR_STMT: {
            // for循环语句
            // 执行初始化
            if (stmt->data.for_stmt.init) {
                if (!runtime_execute_statement(vm, stmt->data.for_stmt.init)) {
                    return false;
                }
            }

            // 循环执行
            while (true) {
                // 检查条件
                if (stmt->data.for_stmt.condition) {
                    RuntimeValue condition = runtime_evaluate_expression(vm, stmt->data.for_stmt.condition);
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
                }

                // 执行循环体
                if (!runtime_execute_statement(vm, stmt->data.for_stmt.body)) {
                    return false;
                }

                // 执行更新
                if (stmt->data.for_stmt.increment) {
                    runtime_evaluate_expression(vm, stmt->data.for_stmt.increment);
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
    RuntimeValue result = {0};
    
    if (!expr) {
        runtime_set_error(vm, "空表达式");
        return result;
    }
    
    // 递增指令计数
    vm->instruction_count++;
    
    // 按表达式类型分别处理
    switch (expr->type) {
        case ASTC_EXPR_CONSTANT:
            // 常量表达式
            switch (expr->data.constant.type) {
                case RT_VAL_I32:
                    result.type = RT_VAL_I32;
                    result.value.i32 = (int32_t)expr->data.constant.int_val;
                    break;
                case RT_VAL_I64:
                    result.type = RT_VAL_I64;
                    result.value.i64 = expr->data.constant.int_val;
                    break;
                case RT_VAL_F32:
                    result.type = RT_VAL_F32;
                    result.value.f32 = (float)expr->data.constant.float_val;
                    break;
                case RT_VAL_F64:
                    result.type = RT_VAL_F64;
                    result.value.f64 = expr->data.constant.float_val;
                    break;
                default:
                    runtime_set_error(vm, "不支持的常量类型");
                    break;
            }
            break;
            
        case ASTC_EXPR_IDENTIFIER:
            // 标识符表达式 - 查找局部变量或全局变量
            {
                // 首先查找局部变量
                RuntimeValue* local_val = runtime_find_local_variable(vm, expr->data.identifier.name);
                if (local_val) {
                    // 找到局部变量
                    return *local_val;
                }
                
                // 未找到局部变量，查找全局变量
                RuntimeGlobalEntry* global = runtime_find_global(vm, expr->data.identifier.name);
                if (global) {
                    // 找到全局变量
                    return global->value;
                }
                
                // 未找到变量
                runtime_set_error(vm, "未定义的变量: %s", expr->data.identifier.name);
            }
            break;
            
        case ASTC_BINARY_OP:
            // 二元操作
            {
                // 递归求值左右操作数
                RuntimeValue left = runtime_evaluate_expression(vm, expr->data.binary_op.left);
                RuntimeValue right = runtime_evaluate_expression(vm, expr->data.binary_op.right);
                
                // 确保类型一致（简化版）
                if (left.type != right.type) {
                    runtime_set_error(vm, "二元操作类型不匹配");
                    break;
                }
                
                // 根据操作符和类型执行操作
                switch (expr->data.binary_op.op) {
                    case ASTC_OP_ADD:
                        switch (left.type) {
                            case RT_VAL_I32:
                                result.type = RT_VAL_I32;
                                result.value.i32 = left.value.i32 + right.value.i32;
                                break;
                            case RT_VAL_I64:
                                result.type = RT_VAL_I64;
                                result.value.i64 = left.value.i64 + right.value.i64;
                                break;
                            case RT_VAL_F32:
                                result.type = RT_VAL_F32;
                                result.value.f32 = left.value.f32 + right.value.f32;
                                break;
                            case RT_VAL_F64:
                                result.type = RT_VAL_F64;
                                result.value.f64 = left.value.f64 + right.value.f64;
                                break;
                            default:
                                runtime_set_error(vm, "不支持的加法类型");
                                break;
                        }
                        break;
                    case ASTC_OP_SUB:
                        switch (left.type) {
                            case RT_VAL_I32:
                                result.type = RT_VAL_I32;
                                result.value.i32 = left.value.i32 - right.value.i32;
                                break;
                            case RT_VAL_I64:
                                result.type = RT_VAL_I64;
                                result.value.i64 = left.value.i64 - right.value.i64;
                                break;
                            case RT_VAL_F32:
                                result.type = RT_VAL_F32;
                                result.value.f32 = left.value.f32 - right.value.f32;
                                break;
                            case RT_VAL_F64:
                                result.type = RT_VAL_F64;
                                result.value.f64 = left.value.f64 - right.value.f64;
                                break;
                            default:
                                runtime_set_error(vm, "不支持的减法类型");
                                break;
                        }
                        break;
                    
                    // 其他操作符的实现...
                    // ... 这里省略了大量实现 ...
                    
                    default:
                        runtime_set_error(vm, "不支持的二元操作: %d", expr->data.binary_op.op);
                        break;
                }
            }
            break;
            
        case ASTC_UNARY_OP:
            // 一元操作
            {
                // 递归求值操作数
                RuntimeValue operand = runtime_evaluate_expression(vm, expr->data.unary_op.operand);
                
                // 根据操作符和类型执行操作
                switch (expr->data.unary_op.op) {
                    case ASTC_OP_NEG:
                        switch (operand.type) {
                            case RT_VAL_I32:
                                result.type = RT_VAL_I32;
                                result.value.i32 = -operand.value.i32;
                                break;
                            case RT_VAL_I64:
                                result.type = RT_VAL_I64;
                                result.value.i64 = -operand.value.i64;
                                break;
                            case RT_VAL_F32:
                                result.type = RT_VAL_F32;
                                result.value.f32 = -operand.value.f32;
                                break;
                            case RT_VAL_F64:
                                result.type = RT_VAL_F64;
                                result.value.f64 = -operand.value.f64;
                                break;
                            default:
                                runtime_set_error(vm, "不支持的取负类型");
                                break;
                        }
                        break;
                    
                    // 其他一元操作符...
                    
                    default:
                        runtime_set_error(vm, "不支持的一元操作: %d", expr->data.unary_op.op);
                        break;
                }
            }
            break;
            
        // 其他表达式类型...
        
        default:
            runtime_set_error(vm, "不支持的表达式类型: %d", expr->type);
            break;
    }
    
    return result;
}

// ===============================================
// 原生函数调用实现
// ===============================================

// 调用原生函数
static int runtime_call_native_function(RuntimeVM* vm, const char* func_name, RuntimeValue* args, size_t arg_count) {
    // 处理标准库函数调用
    if (strcmp(func_name, "printf") == 0) {
        // 改进的printf实现，支持基本格式化
        if (arg_count > 0 && args[0].type == RT_VAL_PTR) {
            const char* format = (char*)args[0].value.ptr;

            if (arg_count == 1) {
                // 只有格式字符串，直接输出
                printf("%s", format);
            } else if (arg_count == 2) {
                // 一个参数的格式化
                if (args[1].type == RT_VAL_I32) {
                    printf(format, args[1].value.i32);
                } else if (args[1].type == RT_VAL_I64) {
                    printf(format, args[1].value.i64);
                } else if (args[1].type == RT_VAL_PTR) {
                    printf(format, (char*)args[1].value.ptr);
                } else {
                    printf("%s", format);
                }
            } else {
                // 多个参数的简化处理
                printf("%s", format);
            }
            fflush(stdout);
        }
        return 0;
    }

    if (strcmp(func_name, "fopen") == 0) {
        // fopen实现
        if (arg_count >= 2 && args[0].type == RT_VAL_PTR && args[1].type == RT_VAL_PTR) {
            const char* filename = (char*)args[0].value.ptr;
            const char* mode = (char*)args[1].value.ptr;
            FILE* fp = fopen(filename, mode);
            printf("Runtime: fopen(%s, %s) = %p\n", filename, mode, fp);
            return (int)(intptr_t)fp;
        }
        return 0;
    }

    if (strcmp(func_name, "fwrite") == 0) {
        // fwrite实现
        if (arg_count >= 4) {
            void* ptr = args[0].value.ptr;
            size_t size = args[1].value.i32;
            size_t count = args[2].value.i32;
            FILE* fp = (FILE*)(intptr_t)args[3].value.i32;

            if (fp) {
                return fwrite(ptr, size, count, fp);
            }
        }
        return 0;
    }

    if (strcmp(func_name, "fclose") == 0) {
        // fclose实现
        if (arg_count >= 1) {
            FILE* fp = (FILE*)(intptr_t)args[0].value.i32;
            if (fp) {
                return fclose(fp);
            }
        }
        return 0;
    }

    // 未知函数
    runtime_set_error(vm, "未知的原生函数: %s", func_name);
    return -1;
}

// ===============================================
// Runtime系统调用实现
// ===============================================

// 读取文件内容
int runtime_syscall_read_file(RuntimeVM* vm, const char* filename, char** content, size_t* size) {
    if (!vm || !filename || !content || !size) {
        runtime_set_error(vm, "Invalid parameters for read_file");
        return -1;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        runtime_set_error(vm, "Cannot open file: %s", filename);
        return -1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 分配内存
    *content = (char*)malloc(*size + 1);
    if (!*content) {
        runtime_set_error(vm, "Memory allocation failed");
        fclose(file);
        return -1;
    }

    // 读取文件
    size_t bytes_read = fread(*content, 1, *size, file);
    if (bytes_read != *size) {
        runtime_set_error(vm, "Failed to read file: %s", filename);
        free(*content);
        *content = NULL;
        fclose(file);
        return -1;
    }

    (*content)[*size] = '\0';
    fclose(file);
    return 0;
}

// 写入文件内容
int runtime_syscall_write_file(RuntimeVM* vm, const char* filename, const char* content, size_t size) {
    if (!vm || !filename || !content) {
        runtime_set_error(vm, "Invalid parameters for write_file");
        return -1;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        runtime_set_error(vm, "Cannot create file: %s", filename);
        return -1;
    }

    size_t bytes_written = fwrite(content, 1, size, file);
    fclose(file);

    if (bytes_written != size) {
        runtime_set_error(vm, "Failed to write file: %s", filename);
        return -1;
    }

    return 0;
}

// 复制文件
int runtime_syscall_copy_file(RuntimeVM* vm, const char* src, const char* dst) {
    if (!vm || !src || !dst) {
        runtime_set_error(vm, "Invalid parameters for copy_file");
        return -1;
    }

    char* content;
    size_t size;

    // 读取源文件
    if (runtime_syscall_read_file(vm, src, &content, &size) != 0) {
        return -1;
    }

    // 写入目标文件
    int result = runtime_syscall_write_file(vm, dst, content, size);
    free(content);

    return result;
}

// ===============================================
// 性能统计功能实现
// ===============================================

static void runtime_perf_init(RuntimeVM* vm) {
    vm->perf_events = NULL;
    vm->perf_event_count = 0;
    vm->perf_event_capacity = 0;
    vm->perf_start_time = time(NULL);
}

static void runtime_perf_update(RuntimeVM* vm, const char* event) {
    if (!vm->debug_mode) return;
    
    // 确保容量足够
    if (vm->perf_event_count >= vm->perf_event_capacity) {
        size_t new_capacity = vm->perf_event_capacity == 0 ? 16 : vm->perf_event_capacity * 2;
        RuntimePerfEvent* new_events = (RuntimePerfEvent*)realloc(
            vm->perf_events, new_capacity * sizeof(RuntimePerfEvent));
            
        if (!new_events) {
            // 无法扩展，跳过记录
            return;
        }
        
        vm->perf_events = new_events;
        vm->perf_event_capacity = new_capacity;
    }
    
    // 记录事件
    RuntimePerfEvent* perf_event = &vm->perf_events[vm->perf_event_count++];
    strncpy(perf_event->name, event, sizeof(perf_event->name) - 1);
    perf_event->name[sizeof(perf_event->name) - 1] = '\0';
    perf_event->timestamp = time(NULL);
    perf_event->instruction_count = vm->instruction_count;
}

static void runtime_perf_report(RuntimeVM* vm) {
    printf("\n===== ASTC 虚拟机性能报告 =====\n");
    printf("执行的总指令数: %zu\n", vm->instruction_count);
    printf("函数调用次数: %zu\n", vm->function_call_count);
    
    // 执行时间
    time_t total_time = time(NULL) - vm->perf_start_time;
    printf("总执行时间: %ld 秒\n", total_time);
    
    if (total_time > 0) {
        printf("平均指令执行速度: %.2f 指令/秒\n", 
               (float)vm->instruction_count / total_time);
    }
    
    // 显示关键事件
    printf("\n关键事件:\n");
    for (size_t i = 0; i < vm->perf_event_count && i < 10; i++) {
        RuntimePerfEvent* event = &vm->perf_events[i];
        time_t event_time = event->timestamp - vm->perf_start_time;
        printf("[%ld秒] %s (指令计数: %zu)\n", 
               event_time, event->name, event->instruction_count);
    }
    
    printf("===== 报告结束 =====\n\n");
}

// 设置虚拟机调试模式
void runtime_set_debug_mode(RuntimeVM* vm, bool debug_mode) {
    if (vm) {
        vm->debug_mode = debug_mode;
    }
}

// 获取性能统计信息
const RuntimeStats* runtime_get_stats(RuntimeVM* vm) {
    if (!vm) return NULL;

    static RuntimeStats stats;
    stats.instruction_count = vm->instruction_count;
    stats.function_call_count = vm->function_call_count;
    stats.total_execution_time = time(NULL) - vm->perf_start_time;

    return &stats;
}

// ===============================================
// Evolver0 Runtime 特定实现 (合并自evolver0_runtime.c)
// ===============================================

#include "libc_forward.h"

// ===============================================
// ASTC虚拟机状态 (轻量化)
// ===============================================

typedef struct {
    uint8_t* code;              // ASTC字节码
    size_t code_size;           // 代码大小
    uint32_t pc;                // 程序计数器
    uint32_t stack[512];        // 操作数栈 (减小到512)
    int32_t stack_top;          // 栈顶指针
    uint32_t locals[128];       // 局部变量 (减小到128)
    bool running;               // 运行状态
} ASTCVirtualMachine;

// ===============================================
// ASTC虚拟机核心功能
// ===============================================

void astc_vm_init(ASTCVirtualMachine* vm, uint8_t* code, size_t code_size) {
    vm->code = code;
    vm->code_size = code_size;
    vm->pc = 0;
    vm->stack_top = -1;
    vm->running = true;
    memset(vm->locals, 0, sizeof(vm->locals));
    memset(vm->stack, 0, sizeof(vm->stack));
}

void astc_vm_push(ASTCVirtualMachine* vm, uint32_t value) {
    if (vm->stack_top < 511) {
        vm->stack[++vm->stack_top] = value;
    }
}

uint32_t astc_vm_pop(ASTCVirtualMachine* vm) {
    if (vm->stack_top >= 0) {
        return vm->stack[vm->stack_top--];
    }
    return 0;
}

// 执行单条ASTC指令
int astc_execute_instruction(ASTCVirtualMachine* vm) {
    if (vm->pc >= vm->code_size || !vm->running) {
        vm->running = false;
        return 0;
    }

    uint8_t opcode = vm->code[vm->pc++];

    switch (opcode) {
        case 0x00: // NOP
            break;

        case 0x01: // HALT
            vm->running = false;
            return astc_vm_pop(vm);

        case 0x10: // CONST_I32
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t value = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                astc_vm_push(vm, value);
            }
            break;

        case 0x12: // CONST_STRING
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t str_len = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;

                if (vm->pc + str_len <= vm->code_size) {
                    // 将字符串地址推入栈
                    astc_vm_push(vm, (uint32_t)(vm->code + vm->pc));
                    vm->pc += str_len;
                } else {
                    // 字符串数据不完整，推入NULL
                    astc_vm_push(vm, 0);
                }
            }
            break;

        case 0x20: // ADD
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, a + b);
            }
            break;

        case 0x21: // SUB
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, a - b);
            }
            break;

        case 0x22: // MUL
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, a * b);
            }
            break;

        case 0x23: // DIV
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                if (b != 0) {
                    astc_vm_push(vm, a / b);
                } else {
                    // 除零错误，停止执行
                    vm->running = false;
                    return -1;
                }
            }
            break;

        case 0x24: // MOD
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                if (b != 0) {
                    astc_vm_push(vm, a % b);
                } else {
                    vm->running = false;
                    return -1;
                }
            }
            break;

        // 比较指令
        case 0x30: // EQ (==)
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, (a == b) ? 1 : 0);
            }
            break;

        case 0x31: // NE (!=)
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, (a != b) ? 1 : 0);
            }
            break;

        case 0x32: // LT (<)
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, (a < b) ? 1 : 0);
            }
            break;

        case 0x33: // LE (<=)
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, (a <= b) ? 1 : 0);
            }
            break;

        case 0x34: // GT (>)
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, (a > b) ? 1 : 0);
            }
            break;

        case 0x35: // GE (>=)
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, (a >= b) ? 1 : 0);
            }
            break;

        // 逻辑指令
        case 0x40: // AND (&&)
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, (a && b) ? 1 : 0);
            }
            break;

        case 0x41: // OR (||)
            {
                uint32_t b = astc_vm_pop(vm);
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, (a || b) ? 1 : 0);
            }
            break;

        case 0x42: // NOT (!)
            {
                uint32_t a = astc_vm_pop(vm);
                astc_vm_push(vm, (!a) ? 1 : 0);
            }
            break;

        // 跳转指令
        case 0x50: // JMP - 无条件跳转
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t offset = *(uint32_t*)(vm->code + vm->pc);
                vm->pc = offset;
            }
            break;

        case 0x51: // JZ - 零跳转
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t offset = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                uint32_t condition = astc_vm_pop(vm);
                if (condition == 0) {
                    vm->pc = offset;
                }
            }
            break;

        case 0x52: // JNZ - 非零跳转
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t offset = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                uint32_t condition = astc_vm_pop(vm);
                if (condition != 0) {
                    vm->pc = offset;
                }
            }
            break;

        // 变量操作指令
        case 0x60: // LOAD_LOCAL - 加载局部变量
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t index = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                if (index < 512) {
                    astc_vm_push(vm, vm->locals[index]);
                }
            }
            break;

        case 0x61: // STORE_LOCAL - 存储局部变量
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t index = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                if (index < 512) {
                    vm->locals[index] = astc_vm_pop(vm);
                }
            }
            break;

        case 0x62: // LOAD_GLOBAL - 加载全局变量
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t index = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                if (index < 1024) {
                    astc_vm_push(vm, vm->globals[index]);
                }
            }
            break;

        case 0x63: // STORE_GLOBAL - 存储全局变量
            if (vm->pc + 4 <= vm->code_size) {
                uint32_t index = *(uint32_t*)(vm->code + vm->pc);
                vm->pc += 4;
                if (index < 1024) {
                    vm->globals[index] = astc_vm_pop(vm);
                }
            }
            break;

        // 数组和结构体操作指令
        case 0x72: // ARRAY_ACCESS - 数组访问
            {
                uint32_t index = astc_vm_pop(vm);
                uint32_t array_addr = astc_vm_pop(vm);
                // 简化实现：假设数组元素是32位整数
                uint32_t *array = (uint32_t*)array_addr;
                if (array && index < 1024) { // 简单边界检查
                    astc_vm_push(vm, array[index]);
                } else {
                    astc_vm_push(vm, 0); // 错误时返回0
                }
            }
            break;

        case 0x73: // PTR_MEMBER_ACCESS - 指针成员访问
            {
                uint32_t ptr_addr = astc_vm_pop(vm);
                // 简化实现：暂时只支持基本的指针解引用
                uint32_t *ptr = (uint32_t*)ptr_addr;
                if (ptr) {
                    astc_vm_push(vm, *ptr);
                } else {
                    astc_vm_push(vm, 0);
                }
            }
            break;

        case 0x74: // MEMBER_ACCESS - 成员访问
            {
                uint32_t obj_addr = astc_vm_pop(vm);
                // 简化实现：假设访问第一个成员
                uint32_t *obj = (uint32_t*)obj_addr;
                if (obj) {
                    astc_vm_push(vm, *obj);
                } else {
                    astc_vm_push(vm, 0);
                }
            }
            break;

        case 0xF0: // LIBC_CALL - libc转发调用
            {
                uint16_t func_id = astc_vm_pop(vm);
                uint16_t arg_count = astc_vm_pop(vm);

                LibcCall call;
                call.func_id = func_id;
                call.arg_count = arg_count;

                // 从栈中获取参数
                for (int i = arg_count - 1; i >= 0; i--) {
                    call.args[i] = astc_vm_pop(vm);
                }

                // 执行libc转发
                int result = libc_forward_call(&call);
                if (result == 0) {
                    astc_vm_push(vm, (uint32_t)call.return_value);
                } else {
                    astc_vm_push(vm, 0); // 错误时返回0
                }
            }
            break;

        default:
            // 未知指令，停止执行
            vm->running = false;
            break;
    }

    return 0;
}

// ===============================================
// Runtime主入口点 (PRD.md要求的简单结构)
// ===============================================

/**
 * Runtime主入口点，由Loader调用
 * 参数：ASTC程序数据和大小
 *
 * PRD.md要求：结构简单，基本只有main()入口点和ASTC解释器功能
 */
int evolver0_runtime_main(void* program_data, size_t program_size) {
    // 初始化libc转发系统
    libc_forward_init();

    if (!program_data || program_size == 0) {
        return 1;
    }

    // 检查ASTC格式
    if (program_size >= 16 && memcmp(program_data, "ASTC", 4) == 0) {
        // 解析ASTC头部
        uint32_t* header = (uint32_t*)program_data;
        uint32_t version = header[1];
        uint32_t data_size = header[2];
        uint32_t entry_point = header[3];

        // 获取ASTC代码段
        uint8_t* astc_code = (uint8_t*)program_data + 16;
        size_t astc_code_size = program_size - 16;

        // 初始化ASTC虚拟机
        ASTCVirtualMachine vm;
        astc_vm_init(&vm, astc_code, astc_code_size);

        // 执行ASTC程序
        int result = 0;
        int instruction_count = 0;
        while (vm.running && instruction_count < 100000) {
            result = astc_execute_instruction(&vm);
            instruction_count++;
        }

        // 清理libc转发系统
        libc_forward_cleanup();

        return result;
    } else {
        libc_forward_cleanup();
        return 1;
    }
}

// ===============================================
// 无头二进制入口点
// ===============================================

/**
 * 无头二进制的入口点
 * 这个函数会被Loader通过函数指针调用
 *
 * PRD.md要求：不需要PE/ELF/MACHO头
 */
int _start(void) {
    // 无头二进制的入口点
    // 实际的参数会通过Loader传递
    return 0;
}