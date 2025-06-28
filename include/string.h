/**
 * string.h - C99字符串处理库头文件
 */

#ifndef _STRING_H
#define _STRING_H

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

// ===============================================
// 字符串长度函数
// ===============================================

size_t strlen(const char *str);

// ===============================================
// 字符串复制函数
// ===============================================

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

// ===============================================
// 字符串连接函数
// ===============================================

char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);

// ===============================================
// 字符串比较函数
// ===============================================

int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);
int strcoll(const char *str1, const char *str2);

// ===============================================
// 字符串搜索函数
// ===============================================

char *strchr(const char *str, int c);
char *strrchr(const char *str, int c);
char *strstr(const char *haystack, const char *needle);
char *strpbrk(const char *str1, const char *str2);
size_t strcspn(const char *str1, const char *str2);
size_t strspn(const char *str1, const char *str2);

// ===============================================
// 字符串标记化函数
// ===============================================

char *strtok(char *str, const char *delim);

// ===============================================
// 内存操作函数
// ===============================================

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *ptr, int value, size_t n);
int memcmp(const void *ptr1, const void *ptr2, size_t n);
void *memchr(const void *ptr, int value, size_t n);

// ===============================================
// 错误处理函数
// ===============================================

char *strerror(int errnum);

#endif /* _STRING_H */
