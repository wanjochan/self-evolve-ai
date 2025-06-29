/**
 * libc_forward.h - libc转发系统头文件
 * 
 * 实现Runtime层的libc转发封装，符合PRD.md轻量化设计
 * 目标：为ASTC程序提供完整的C标准库接口
 */

#ifndef LIBC_FORWARD_H
#define LIBC_FORWARD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// libc转发函数ID定义
// ===============================================

// 内存管理
#define LIBC_MALLOC     0x0001
#define LIBC_FREE       0x0002
#define LIBC_CALLOC     0x0003
#define LIBC_REALLOC    0x0004

// 字符串操作
#define LIBC_STRLEN     0x0010
#define LIBC_STRCPY     0x0011
#define LIBC_STRNCPY    0x0012
#define LIBC_STRCMP     0x0013
#define LIBC_STRNCMP    0x0014
#define LIBC_STRCAT     0x0015
#define LIBC_STRNCAT    0x0016
#define LIBC_STRCHR     0x0017
#define LIBC_STRSTR     0x0018

// 内存操作
#define LIBC_MEMCPY     0x0020
#define LIBC_MEMMOVE    0x0021
#define LIBC_MEMSET     0x0022
#define LIBC_MEMCMP     0x0023

// 输入输出
#define LIBC_PRINTF     0x0030
#define LIBC_FPRINTF    0x0031
#define LIBC_SPRINTF    0x0032
#define LIBC_SNPRINTF   0x0033
#define LIBC_SCANF      0x0034
#define LIBC_FSCANF     0x0035
#define LIBC_SSCANF     0x0036

// 文件操作
#define LIBC_FOPEN      0x0040
#define LIBC_FCLOSE     0x0041
#define LIBC_FREAD      0x0042
#define LIBC_FWRITE     0x0043
#define LIBC_FSEEK      0x0044
#define LIBC_FTELL      0x0045
#define LIBC_FEOF       0x0046
#define LIBC_FERROR     0x0047

// 数学函数
#define LIBC_ABS        0x0050
#define LIBC_LABS       0x0051
#define LIBC_SQRT       0x0052
#define LIBC_POW        0x0053
#define LIBC_SIN        0x0054
#define LIBC_COS        0x0055
#define LIBC_TAN        0x0056

// 转换函数
#define LIBC_ATOI       0x0060
#define LIBC_ATOL       0x0061
#define LIBC_ATOF       0x0062
#define LIBC_STRTOL     0x0063
#define LIBC_STRTOD     0x0064

// 系统调用
#define LIBC_EXIT       0x0070
#define LIBC_ABORT      0x0071
#define LIBC_SYSTEM     0x0072
#define LIBC_GETENV     0x0073

// 扩展stdio函数
#define LIBC_PUTS       0x0080
#define LIBC_PUTCHAR    0x0081
#define LIBC_GETCHAR    0x0082
#define LIBC_FGETC      0x0083
#define LIBC_FPUTC      0x0084
#define LIBC_FGETS      0x0085
#define LIBC_FPUTS      0x0086

// 扩展string.h函数
#define LIBC_STRDUP     0x0090
#define LIBC_STRTOK     0x0091
#define LIBC_STRRCHR    0x0092
#define LIBC_STRSPN     0x0093
#define LIBC_STRCSPN    0x0094

// ctype.h函数
#define LIBC_ISALPHA    0x00A0
#define LIBC_ISDIGIT    0x00A1
#define LIBC_ISALNUM    0x00A2
#define LIBC_ISSPACE    0x00A3
#define LIBC_ISUPPER    0x00A4
#define LIBC_ISLOWER    0x00A5
#define LIBC_TOUPPER    0x00A6
#define LIBC_TOLOWER    0x00A7

// time.h函数
#define LIBC_TIME       0x00B0
#define LIBC_CLOCK      0x00B1
#define LIBC_DIFFTIME   0x00B2

// 扩展stdlib.h函数
#define LIBC_QSORT      0x00C0
#define LIBC_BSEARCH    0x00C1
#define LIBC_RAND       0x00C2
#define LIBC_SRAND      0x00C3
#define LIBC_STRTOL     0x00C4
#define LIBC_STRTOD     0x00C5

// 更多stdio.h函数
#define LIBC_FFLUSH     0x00D0
#define LIBC_FSEEK      0x00D1
#define LIBC_FTELL      0x00D2
#define LIBC_REWIND     0x00D3
#define LIBC_FEOF       0x00D4
#define LIBC_FERROR     0x00D5
#define LIBC_CLEARERR   0x00D6

// 更多string.h函数
#define LIBC_STRCAT     0x00E0
#define LIBC_STRNCAT    0x00E1
#define LIBC_STRNCPY    0x00E2
#define LIBC_STRNCMP    0x00E3
#define LIBC_STRCHR     0x00E4
#define LIBC_STRSTR     0x00E5
#define LIBC_MEMCPY     0x00E6
#define LIBC_MEMSET     0x00E7
#define LIBC_MEMCMP     0x00E8

// 更多math.h函数
#define LIBC_LOG        0x00F3
#define LIBC_LOG10      0x00F4
#define LIBC_EXP        0x00F5
#define LIBC_FLOOR      0x00F7
#define LIBC_CEIL       0x00F8
#define LIBC_FABS       0x00F9

// ===============================================
// libc转发调用结构
// ===============================================

typedef struct {
    uint16_t func_id;       // 函数ID
    uint16_t arg_count;     // 参数数量
    uint64_t args[8];       // 参数数组（最多8个参数）
    uint64_t return_value;  // 返回值
    int32_t error_code;     // 错误码
} LibcCall;

// ===============================================
// libc转发系统接口
// ===============================================

/**
 * 初始化libc转发系统
 * @return 0成功，非0失败
 */
int libc_forward_init(void);

/**
 * 清理libc转发系统
 */
void libc_forward_cleanup(void);

/**
 * 执行libc函数调用
 * @param call 函数调用结构
 * @return 0成功，非0失败
 */
int libc_forward_call(LibcCall* call);

/**
 * 获取函数名称（用于调试）
 * @param func_id 函数ID
 * @return 函数名称字符串
 */
const char* libc_get_function_name(uint16_t func_id);

// ===============================================
// ASTC虚拟机libc指令
// ===============================================

// ASTC指令：调用libc函数
#define ASTC_LIBC_CALL  0xF0

/**
 * 在ASTC虚拟机中执行libc调用
 * @param vm 虚拟机状态
 * @param func_id 函数ID
 * @param arg_count 参数数量
 * @return 0成功，非0失败
 */
int astc_vm_libc_call(void* vm, uint16_t func_id, uint16_t arg_count);

// ===============================================
// 调试和统计
// ===============================================

typedef struct {
    uint64_t total_calls;       // 总调用次数
    uint64_t malloc_calls;      // malloc调用次数
    uint64_t file_operations;   // 文件操作次数
    uint64_t string_operations; // 字符串操作次数
} LibcStats;

/**
 * 获取libc调用统计
 * @param stats 统计结构指针
 */
void libc_get_stats(LibcStats* stats);

/**
 * 重置统计计数器
 */
void libc_reset_stats(void);

/**
 * 打印调试信息
 * @param verbose 是否显示详细信息
 */
void libc_print_debug_info(int verbose);

#ifdef __cplusplus
}
#endif

#endif // LIBC_FORWARD_H
