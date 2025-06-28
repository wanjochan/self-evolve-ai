/**
 * stdlib.h - C99标准库头文件
 */

#ifndef _STDLIB_H
#define _STDLIB_H

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

// 退出状态
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

// 随机数常量
#define RAND_MAX 32767

// ===============================================
// 内存管理函数
// ===============================================

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

// ===============================================
// 程序控制函数
// ===============================================

void exit(int status);
void abort(void);
int atexit(void (*function)(void));

// ===============================================
// 字符串转换函数
// ===============================================

int atoi(const char *str);
long atol(const char *str);
double atof(const char *str);

long strtol(const char *str, char **endptr, int base);
unsigned long strtoul(const char *str, char **endptr, int base);
double strtod(const char *str, char **endptr);

// ===============================================
// 随机数函数
// ===============================================

int rand(void);
void srand(unsigned int seed);

// ===============================================
// 搜索和排序函数
// ===============================================

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));

// ===============================================
// 绝对值函数
// ===============================================

int abs(int x);
long labs(long x);

// ===============================================
// 环境函数
// ===============================================

char *getenv(const char *name);
int system(const char *command);

#endif /* _STDLIB_H */
