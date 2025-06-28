/**
 * stdio.h - C99标准输入输出库头文件
 * 
 * 为我们的C99编译器提供标准库函数声明
 */

#ifndef _STDIO_H
#define _STDIO_H

// 定义__libc__属性，用于标记系统库函数
// 编译器会识别这个属性，将标记的函数转换为LIBC_CALL指令
#ifndef __libc__
#define __libc__ __attribute__((libc))
#endif

// 基本类型定义
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned long size_t;
#endif

#ifndef _FILE_DEFINED
#define _FILE_DEFINED
typedef struct _FILE FILE;
#endif

// 标准文件指针
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

// 文件操作常量
#define EOF (-1)
#define NULL ((void*)0)

// 文件定位常量
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// ===============================================
// 格式化输入输出函数
// ===============================================

// 使用__libc__属性标记系统库函数，编译器会将其转换为LIBC_CALL指令
__libc__ int printf(const char *format, ...);
__libc__ int fprintf(FILE *stream, const char *format, ...);
__libc__ int sprintf(char *str, const char *format, ...);
__libc__ int snprintf(char *str, size_t size, const char *format, ...);

__libc__ int scanf(const char *format, ...);
__libc__ int fscanf(FILE *stream, const char *format, ...);
__libc__ int sscanf(const char *str, const char *format, ...);

// ===============================================
// 文件操作函数
// ===============================================

__libc__ FILE *fopen(const char *filename, const char *mode);
__libc__ int fclose(FILE *stream);
__libc__ int fflush(FILE *stream);

__libc__ size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
__libc__ size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);

int fgetc(FILE *stream);
int fputc(int c, FILE *stream);
char *fgets(char *str, int n, FILE *stream);
int fputs(const char *str, FILE *stream);

int getc(FILE *stream);
int putc(int c, FILE *stream);
int getchar(void);
int putchar(int c);

char *gets(char *str);
int puts(const char *str);

// ===============================================
// 错误处理函数
// ===============================================

void perror(const char *str);
int ferror(FILE *stream);
int feof(FILE *stream);
void clearerr(FILE *stream);

// ===============================================
// 临时文件函数
// ===============================================

FILE *tmpfile(void);
char *tmpnam(char *str);

// ===============================================
// 其他函数
// ===============================================

int remove(const char *filename);
int rename(const char *old_name, const char *new_name);

#endif /* _STDIO_H */
