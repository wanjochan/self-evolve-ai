/**
 * libc_forward.c - libc转发系统实现
 * 
 * 实现Runtime层的libc转发封装，符合PRD.md轻量化设计
 * 核心思想：不重新实现libc，而是转发到系统libc
 */

#include "core_libc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

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

        // 扩展stdio函数
        case LIBC_PUTS:
            call->return_value = puts((const char*)call->args[0]);
            break;

        case LIBC_PUTCHAR:
            call->return_value = putchar((int)call->args[0]);
            break;

        case LIBC_GETCHAR:
            call->return_value = getchar();
            break;

        case LIBC_FGETC:
            call->return_value = fgetc((FILE*)call->args[0]);
            break;

        case LIBC_FPUTC:
            call->return_value = fputc((int)call->args[0], (FILE*)call->args[1]);
            break;

        // 扩展string.h函数
        case LIBC_STRDUP:
            call->return_value = (uint64_t)strdup((const char*)call->args[0]);
            break;

        case LIBC_STRTOK:
            call->return_value = (uint64_t)strtok((char*)call->args[0], (const char*)call->args[1]);
            break;

        case LIBC_STRRCHR:
            call->return_value = (uint64_t)strrchr((const char*)call->args[0], (int)call->args[1]);
            break;

        // ctype.h函数
        case LIBC_ISALPHA:
            call->return_value = isalpha((int)call->args[0]);
            break;

        case LIBC_ISDIGIT:
            call->return_value = isdigit((int)call->args[0]);
            break;

        case LIBC_ISSPACE:
            call->return_value = isspace((int)call->args[0]);
            break;

        case LIBC_TOUPPER:
            call->return_value = toupper((int)call->args[0]);
            break;

        case LIBC_TOLOWER:
            call->return_value = tolower((int)call->args[0]);
            break;

        // time.h函数
        case LIBC_TIME:
            call->return_value = (uint64_t)time((time_t*)call->args[0]);
            break;

        case LIBC_CLOCK:
            call->return_value = (uint64_t)clock();
            break;

        // stdlib.h函数
        case LIBC_RAND:
            call->return_value = rand();
            break;

        case LIBC_SRAND:
            srand((unsigned int)call->args[0]);
            call->return_value = 0;
            break;

        case LIBC_STRTOL:
            call->return_value = strtol((const char*)call->args[0], (char**)call->args[1], (int)call->args[2]);
            break;

        case LIBC_STRTOD:
            call->return_value = (uint64_t)(strtod((const char*)call->args[0], (char**)call->args[1]) * 1000000);
            break;

        // 更多stdio.h函数
        case LIBC_FFLUSH:
            call->return_value = fflush((FILE*)call->args[0]);
            break;

        case LIBC_FSEEK:
            call->return_value = fseek((FILE*)call->args[0], (long)call->args[1], (int)call->args[2]);
            break;

        case LIBC_FTELL:
            call->return_value = ftell((FILE*)call->args[0]);
            break;

        case LIBC_REWIND:
            rewind((FILE*)call->args[0]);
            call->return_value = 0;
            break;

        case LIBC_FEOF:
            call->return_value = feof((FILE*)call->args[0]);
            break;

        case LIBC_FERROR:
            call->return_value = ferror((FILE*)call->args[0]);
            break;

        case LIBC_CLEARERR:
            clearerr((FILE*)call->args[0]);
            call->return_value = 0;
            break;

        // math.h函数 (使用原有ID)
        case LIBC_SIN:
            call->return_value = (uint64_t)(sin(*(double*)&call->args[0]) * 1000000);
            break;

        case LIBC_COS:
            call->return_value = (uint64_t)(cos(*(double*)&call->args[0]) * 1000000);
            break;

        case LIBC_TAN:
            call->return_value = (uint64_t)(tan(*(double*)&call->args[0]) * 1000000);
            break;

        case LIBC_LOG:
            call->return_value = (uint64_t)(log(*(double*)&call->args[0]) * 1000000);
            break;

        case LIBC_LOG10:
            call->return_value = (uint64_t)(log10(*(double*)&call->args[0]) * 1000000);
            break;

        case LIBC_EXP:
            call->return_value = (uint64_t)(exp(*(double*)&call->args[0]) * 1000000);
            break;

        case LIBC_POW:
            call->return_value = (uint64_t)(pow(*(double*)&call->args[0], *(double*)&call->args[1]) * 1000000);
            break;

        case LIBC_FLOOR:
            call->return_value = (uint64_t)(floor(*(double*)&call->args[0]) * 1000000);
            break;

        case LIBC_CEIL:
            call->return_value = (uint64_t)(ceil(*(double*)&call->args[0]) * 1000000);
            break;

        case LIBC_FABS:
            call->return_value = (uint64_t)(fabs(*(double*)&call->args[0]) * 1000000);
            break;

        // 缺失的字符串函数
        case LIBC_STRCAT:
            g_stats.string_operations++;
            call->return_value = (uint64_t)strcat((char*)call->args[0], (const char*)call->args[1]);
            break;

        case LIBC_STRNCAT:
            g_stats.string_operations++;
            call->return_value = (uint64_t)strncat((char*)call->args[0], (const char*)call->args[1], (size_t)call->args[2]);
            break;

        case LIBC_STRCHR:
            g_stats.string_operations++;
            call->return_value = (uint64_t)strchr((const char*)call->args[0], (int)call->args[1]);
            break;

        case LIBC_STRSTR:
            g_stats.string_operations++;
            call->return_value = (uint64_t)strstr((const char*)call->args[0], (const char*)call->args[1]);
            break;

        case LIBC_STRSPN:
            g_stats.string_operations++;
            call->return_value = strspn((const char*)call->args[0], (const char*)call->args[1]);
            break;

        case LIBC_STRCSPN:
            g_stats.string_operations++;
            call->return_value = strcspn((const char*)call->args[0], (const char*)call->args[1]);
            break;

        // 缺失的输入输出函数
        case LIBC_SNPRINTF:
            call->return_value = snprintf((char*)call->args[0], (size_t)call->args[1], (const char*)call->args[2]);
            break;

        case LIBC_SCANF:
            call->return_value = scanf((const char*)call->args[0]);
            break;

        case LIBC_FSCANF:
            call->return_value = fscanf((FILE*)call->args[0], (const char*)call->args[1]);
            break;

        case LIBC_SSCANF:
            call->return_value = sscanf((const char*)call->args[0], (const char*)call->args[1]);
            break;

        case LIBC_FGETS:
            call->return_value = (uint64_t)fgets((char*)call->args[0], (int)call->args[1], (FILE*)call->args[2]);
            break;

        case LIBC_FPUTS:
            call->return_value = fputs((const char*)call->args[0], (FILE*)call->args[1]);
            break;

        // 缺失的字符类型函数
        case LIBC_ISALNUM:
            call->return_value = isalnum((int)call->args[0]);
            break;

        case LIBC_ISUPPER:
            call->return_value = isupper((int)call->args[0]);
            break;

        case LIBC_ISLOWER:
            call->return_value = islower((int)call->args[0]);
            break;

        // 缺失的数学函数
        case LIBC_LABS:
            call->return_value = labs((long)call->args[0]);
            break;

        case LIBC_ATOF:
            call->return_value = (uint64_t)(atof((const char*)call->args[0]) * 1000000);
            break;

        // 缺失的时间函数
        case LIBC_DIFFTIME:
            call->return_value = (uint64_t)(difftime((time_t)call->args[0], (time_t)call->args[1]) * 1000000);
            break;

        // 缺失的stdlib函数
        case LIBC_QSORT:
            qsort((void*)call->args[0], (size_t)call->args[1], (size_t)call->args[2], (int(*)(const void*, const void*))call->args[3]);
            call->return_value = 0;
            break;

        case LIBC_BSEARCH:
            call->return_value = (uint64_t)bsearch((const void*)call->args[0], (const void*)call->args[1], (size_t)call->args[2], (size_t)call->args[3], (int(*)(const void*, const void*))call->args[4]);
            break;

        case LIBC_ABORT:
            abort();
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
        // 内存管理
        case LIBC_MALLOC: return "malloc";
        case LIBC_FREE: return "free";
        case LIBC_CALLOC: return "calloc";
        case LIBC_REALLOC: return "realloc";

        // 字符串操作
        case LIBC_STRLEN: return "strlen";
        case LIBC_STRCPY: return "strcpy";
        case LIBC_STRNCPY: return "strncpy";
        case LIBC_STRCMP: return "strcmp";
        case LIBC_STRNCMP: return "strncmp";
        case LIBC_STRCAT: return "strcat";
        case LIBC_STRNCAT: return "strncat";
        case LIBC_STRCHR: return "strchr";
        case LIBC_STRRCHR: return "strrchr";
        case LIBC_STRSTR: return "strstr";
        case LIBC_STRDUP: return "strdup";
        case LIBC_STRTOK: return "strtok";
        case LIBC_STRSPN: return "strspn";
        case LIBC_STRCSPN: return "strcspn";

        // 内存操作
        case LIBC_MEMCPY: return "memcpy";
        case LIBC_MEMMOVE: return "memmove";
        case LIBC_MEMSET: return "memset";
        case LIBC_MEMCMP: return "memcmp";

        // 输入输出
        case LIBC_PRINTF: return "printf";
        case LIBC_FPRINTF: return "fprintf";
        case LIBC_SPRINTF: return "sprintf";
        case LIBC_SNPRINTF: return "snprintf";
        case LIBC_SCANF: return "scanf";
        case LIBC_FSCANF: return "fscanf";
        case LIBC_SSCANF: return "sscanf";
        case LIBC_PUTS: return "puts";
        case LIBC_PUTCHAR: return "putchar";
        case LIBC_GETCHAR: return "getchar";
        case LIBC_FGETC: return "fgetc";
        case LIBC_FPUTC: return "fputc";
        case LIBC_FGETS: return "fgets";
        case LIBC_FPUTS: return "fputs";

        // 文件操作
        case LIBC_FOPEN: return "fopen";
        case LIBC_FCLOSE: return "fclose";
        case LIBC_FREAD: return "fread";
        case LIBC_FWRITE: return "fwrite";
        case LIBC_FSEEK: return "fseek";
        case LIBC_FTELL: return "ftell";
        case LIBC_FEOF: return "feof";
        case LIBC_FERROR: return "ferror";
        case LIBC_FFLUSH: return "fflush";
        case LIBC_REWIND: return "rewind";
        case LIBC_CLEARERR: return "clearerr";

        // 数学函数
        case LIBC_ABS: return "abs";
        case LIBC_LABS: return "labs";
        case LIBC_SQRT: return "sqrt";
        case LIBC_POW: return "pow";
        case LIBC_SIN: return "sin";
        case LIBC_COS: return "cos";
        case LIBC_TAN: return "tan";
        case LIBC_LOG: return "log";
        case LIBC_LOG10: return "log10";
        case LIBC_EXP: return "exp";
        case LIBC_FLOOR: return "floor";
        case LIBC_CEIL: return "ceil";
        case LIBC_FABS: return "fabs";

        // 转换函数
        case LIBC_ATOI: return "atoi";
        case LIBC_ATOL: return "atol";
        case LIBC_ATOF: return "atof";
        case LIBC_STRTOL: return "strtol";
        case LIBC_STRTOD: return "strtod";

        // 字符类型
        case LIBC_ISALPHA: return "isalpha";
        case LIBC_ISDIGIT: return "isdigit";
        case LIBC_ISALNUM: return "isalnum";
        case LIBC_ISSPACE: return "isspace";
        case LIBC_ISUPPER: return "isupper";
        case LIBC_ISLOWER: return "islower";
        case LIBC_TOUPPER: return "toupper";
        case LIBC_TOLOWER: return "tolower";

        // 时间函数
        case LIBC_TIME: return "time";
        case LIBC_CLOCK: return "clock";
        case LIBC_DIFFTIME: return "difftime";

        // 系统调用
        case LIBC_EXIT: return "exit";
        case LIBC_ABORT: return "abort";
        case LIBC_SYSTEM: return "system";
        case LIBC_GETENV: return "getenv";

        // 随机数
        case LIBC_RAND: return "rand";
        case LIBC_SRAND: return "srand";

        // 排序搜索
        case LIBC_QSORT: return "qsort";
        case LIBC_BSEARCH: return "bsearch";

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
