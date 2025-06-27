/**
 * libc_forward.c - libc转发系统实现
 * 
 * 实现Runtime层的libc转发封装，符合PRD.md轻量化设计
 * 核心思想：不重新实现libc，而是转发到系统libc
 */

#include "libc_forward.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ===============================================
// 全局状态
// ===============================================

static LibcStats g_stats = {0};
static int g_initialized = 0;

// ===============================================
// 初始化和清理
// ===============================================

int libc_forward_init(void) {
    if (g_initialized) {
        return 0; // 已经初始化
    }
    
    memset(&g_stats, 0, sizeof(g_stats));
    g_initialized = 1;
    
    return 0;
}

void libc_forward_cleanup(void) {
    g_initialized = 0;
}

// ===============================================
// 核心转发函数
// ===============================================

int libc_forward_call(LibcCall* call) {
    if (!g_initialized) {
        libc_forward_init();
    }
    
    if (!call) {
        return -1;
    }
    
    g_stats.total_calls++;
    call->error_code = 0;
    
    switch (call->func_id) {
        // 内存管理
        case LIBC_MALLOC:
            g_stats.malloc_calls++;
            call->return_value = (uint64_t)malloc((size_t)call->args[0]);
            break;
            
        case LIBC_FREE:
            free((void*)call->args[0]);
            call->return_value = 0;
            break;
            
        case LIBC_CALLOC:
            g_stats.malloc_calls++;
            call->return_value = (uint64_t)calloc((size_t)call->args[0], (size_t)call->args[1]);
            break;
            
        case LIBC_REALLOC:
            g_stats.malloc_calls++;
            call->return_value = (uint64_t)realloc((void*)call->args[0], (size_t)call->args[1]);
            break;
            
        // 字符串操作
        case LIBC_STRLEN:
            g_stats.string_operations++;
            call->return_value = strlen((const char*)call->args[0]);
            break;
            
        case LIBC_STRCPY:
            g_stats.string_operations++;
            call->return_value = (uint64_t)strcpy((char*)call->args[0], (const char*)call->args[1]);
            break;
            
        case LIBC_STRNCPY:
            g_stats.string_operations++;
            call->return_value = (uint64_t)strncpy((char*)call->args[0], (const char*)call->args[1], (size_t)call->args[2]);
            break;
            
        case LIBC_STRCMP:
            g_stats.string_operations++;
            call->return_value = strcmp((const char*)call->args[0], (const char*)call->args[1]);
            break;
            
        case LIBC_STRNCMP:
            g_stats.string_operations++;
            call->return_value = strncmp((const char*)call->args[0], (const char*)call->args[1], (size_t)call->args[2]);
            break;
            
        // 内存操作
        case LIBC_MEMCPY:
            call->return_value = (uint64_t)memcpy((void*)call->args[0], (const void*)call->args[1], (size_t)call->args[2]);
            break;
            
        case LIBC_MEMMOVE:
            call->return_value = (uint64_t)memmove((void*)call->args[0], (const void*)call->args[1], (size_t)call->args[2]);
            break;
            
        case LIBC_MEMSET:
            call->return_value = (uint64_t)memset((void*)call->args[0], (int)call->args[1], (size_t)call->args[2]);
            break;
            
        case LIBC_MEMCMP:
            call->return_value = memcmp((const void*)call->args[0], (const void*)call->args[1], (size_t)call->args[2]);
            break;
            
        // 输入输出
        case LIBC_PRINTF:
            call->return_value = printf((const char*)call->args[0]);
            break;
            
        case LIBC_FPRINTF:
            call->return_value = fprintf((FILE*)call->args[0], (const char*)call->args[1]);
            break;
            
        case LIBC_SPRINTF:
            call->return_value = sprintf((char*)call->args[0], (const char*)call->args[1]);
            break;
            
        // 文件操作
        case LIBC_FOPEN:
            g_stats.file_operations++;
            call->return_value = (uint64_t)fopen((const char*)call->args[0], (const char*)call->args[1]);
            break;
            
        case LIBC_FCLOSE:
            g_stats.file_operations++;
            call->return_value = fclose((FILE*)call->args[0]);
            break;
            
        case LIBC_FREAD:
            g_stats.file_operations++;
            call->return_value = fread((void*)call->args[0], (size_t)call->args[1], (size_t)call->args[2], (FILE*)call->args[3]);
            break;
            
        case LIBC_FWRITE:
            g_stats.file_operations++;
            call->return_value = fwrite((const void*)call->args[0], (size_t)call->args[1], (size_t)call->args[2], (FILE*)call->args[3]);
            break;
            
        // 数学函数
        case LIBC_ABS:
            call->return_value = abs((int)call->args[0]);
            break;
            
        case LIBC_SQRT:
            call->return_value = (uint64_t)(sqrt((double)call->args[0]) * 1000000); // 定点数表示
            break;
            
        // 转换函数
        case LIBC_ATOI:
            call->return_value = atoi((const char*)call->args[0]);
            break;
            
        case LIBC_ATOL:
            call->return_value = atol((const char*)call->args[0]);
            break;
            
        // 系统调用
        case LIBC_EXIT:
            exit((int)call->args[0]);
            break;
            
        case LIBC_SYSTEM:
            call->return_value = system((const char*)call->args[0]);
            break;
            
        case LIBC_GETENV:
            call->return_value = (uint64_t)getenv((const char*)call->args[0]);
            break;
            
        default:
            call->error_code = -1; // 未知函数
            return -1;
    }
    
    return 0;
}

// ===============================================
// 辅助函数
// ===============================================

const char* libc_get_function_name(uint16_t func_id) {
    switch (func_id) {
        case LIBC_MALLOC: return "malloc";
        case LIBC_FREE: return "free";
        case LIBC_STRLEN: return "strlen";
        case LIBC_STRCPY: return "strcpy";
        case LIBC_STRCMP: return "strcmp";
        case LIBC_PRINTF: return "printf";
        case LIBC_FOPEN: return "fopen";
        case LIBC_FCLOSE: return "fclose";
        default: return "unknown";
    }
}

void libc_get_stats(LibcStats* stats) {
    if (stats) {
        *stats = g_stats;
    }
}

void libc_reset_stats(void) {
    memset(&g_stats, 0, sizeof(g_stats));
}

void libc_print_debug_info(int verbose) {
    printf("=== libc转发系统统计 ===\n");
    printf("总调用次数: %llu\n", g_stats.total_calls);
    printf("内存分配调用: %llu\n", g_stats.malloc_calls);
    printf("文件操作调用: %llu\n", g_stats.file_operations);
    printf("字符串操作调用: %llu\n", g_stats.string_operations);
    
    if (verbose) {
        printf("系统状态: %s\n", g_initialized ? "已初始化" : "未初始化");
    }
}
