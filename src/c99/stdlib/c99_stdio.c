/**
 * c99_stdio.c - C99 Standard I/O Library Implementation
 */

#include "c99_stdio.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

// ===============================================
// Global Variables
// ===============================================

static FILE _stdin_file = {0};
static FILE _stdout_file = {1};
static FILE _stderr_file = {2};

FILE* stdin = &_stdin_file;
FILE* stdout = &_stdout_file;
FILE* stderr = &_stderr_file;

static bool _stdio_initialized = false;

// ===============================================
// Internal Helper Functions
// ===============================================

void _stdio_init(void) {
    if (_stdio_initialized) return;
    
    // Initialize stdin
    stdin->fd = 0;
    stdin->is_open = true;
    stdin->is_readable = true;
    stdin->is_writable = false;
    stdin->buffer_mode = _IOLBF;
    stdin->buffer_size = BUFSIZ;
    stdin->buffer = malloc(BUFSIZ);
    stdin->owns_buffer = true;
    stdin->filename = strdup("stdin");
    stdin->mode = strdup("r");
    
    // Initialize stdout
    stdout->fd = 1;
    stdout->is_open = true;
    stdout->is_readable = false;
    stdout->is_writable = true;
    stdout->buffer_mode = _IOLBF;
    stdout->buffer_size = BUFSIZ;
    stdout->buffer = malloc(BUFSIZ);
    stdout->owns_buffer = true;
    stdout->filename = strdup("stdout");
    stdout->mode = strdup("w");
    
    // Initialize stderr
    stderr->fd = 2;
    stderr->is_open = true;
    stderr->is_readable = false;
    stderr->is_writable = true;
    stderr->buffer_mode = _IONBF; // Unbuffered
    stderr->buffer_size = 0;
    stderr->buffer = NULL;
    stderr->owns_buffer = false;
    stderr->filename = strdup("stderr");
    stderr->mode = strdup("w");
    
    _stdio_initialized = true;
}

void _stdio_cleanup(void) {
    if (!_stdio_initialized) return;
    
    if (stdin->buffer && stdin->owns_buffer) {
        free(stdin->buffer);
    }
    if (stdin->filename) free(stdin->filename);
    if (stdin->mode) free(stdin->mode);
    
    if (stdout->buffer && stdout->owns_buffer) {
        free(stdout->buffer);
    }
    if (stdout->filename) free(stdout->filename);
    if (stdout->mode) free(stdout->mode);
    
    if (stderr->filename) free(stderr->filename);
    if (stderr->mode) free(stderr->mode);
    
    _stdio_initialized = false;
}

FILE* _file_alloc(void) {
    FILE* file = malloc(sizeof(FILE));
    if (!file) return NULL;
    
    memset(file, 0, sizeof(FILE));
    file->fd = -1;
    file->buffer_mode = _IOFBF;
    
    return file;
}

void _file_free(FILE* stream) {
    if (!stream) return;
    
    if (stream->buffer && stream->owns_buffer) {
        free(stream->buffer);
    }
    
    if (stream->filename) {
        free(stream->filename);
    }
    
    if (stream->mode) {
        free(stream->mode);
    }
    
    free(stream);
}

int _parse_file_mode(const char* mode, bool* readable, bool* writable, bool* binary) {
    if (!mode || !readable || !writable || !binary) return -1;
    
    *readable = false;
    *writable = false;
    *binary = false;
    
    switch (mode[0]) {
        case 'r':
            *readable = true;
            break;
        case 'w':
            *writable = true;
            break;
        case 'a':
            *writable = true;
            break;
        default:
            return -1;
    }
    
    // Check for additional flags
    for (int i = 1; mode[i]; i++) {
        switch (mode[i]) {
            case '+':
                *readable = true;
                *writable = true;
                break;
            case 'b':
                *binary = true;
                break;
        }
    }
    
    return 0;
}

// ===============================================
// File Operations
// ===============================================

FILE* fopen(const char* filename, const char* mode) {
    if (!filename || !mode) return NULL;
    
    if (!_stdio_initialized) {
        _stdio_init();
    }
    
    bool readable, writable, binary;
    if (_parse_file_mode(mode, &readable, &writable, &binary) != 0) {
        return NULL;
    }
    
    FILE* file = _file_alloc();
    if (!file) return NULL;
    
    // Open file with appropriate flags
    int flags = 0;
    if (readable && writable) {
        flags = O_RDWR;
    } else if (writable) {
        flags = O_WRONLY;
        if (mode[0] == 'w') {
            flags |= O_CREAT | O_TRUNC;
        } else if (mode[0] == 'a') {
            flags |= O_CREAT | O_APPEND;
        }
    } else {
        flags = O_RDONLY;
    }
    
#ifdef _WIN32
    if (binary) {
        flags |= O_BINARY;
    } else {
        flags |= O_TEXT;
    }
    
    file->fd = _open(filename, flags, 0644);
#else
    file->fd = open(filename, flags, 0644);
#endif
    
    if (file->fd == -1) {
        _file_free(file);
        return NULL;
    }
    
    // Set up file structure
    file->is_open = true;
    file->is_readable = readable;
    file->is_writable = writable;
    file->is_binary = binary;
    file->filename = strdup(filename);
    file->mode = strdup(mode);
    
    // Set up buffering
    file->buffer_size = BUFSIZ;
    file->buffer = malloc(file->buffer_size);
    file->owns_buffer = true;
    file->buffer_mode = _IOFBF;
    
    if (!file->buffer) {
        fclose(file);
        return NULL;
    }
    
    return file;
}

int fclose(FILE* stream) {
    if (!stream || !stream->is_open) return EOF;
    
    // Flush any pending output
    if (stream->is_writable) {
        fflush(stream);
    }
    
    // Close file descriptor
    int result = 0;
    if (stream->fd >= 0) {
#ifdef _WIN32
        result = _close(stream->fd);
#else
        result = close(stream->fd);
#endif
    }
    
    stream->is_open = false;
    
    // Don't free standard streams
    if (stream != stdin && stream != stdout && stream != stderr) {
        _file_free(stream);
    }
    
    return (result == 0) ? 0 : EOF;
}

int fflush(FILE* stream) {
    if (!stream) {
        // Flush all open streams
        fflush(stdout);
        fflush(stderr);
        return 0;
    }
    
    if (!stream->is_open || !stream->is_writable) {
        return EOF;
    }
    
    return _file_flush_buffer(stream);
}

int _file_flush_buffer(FILE* stream) {
    if (!stream || !stream->buffer || stream->buffer_pos == 0) {
        return 0;
    }
    
    ssize_t written = 0;
    size_t to_write = stream->buffer_pos;
    
#ifdef _WIN32
    written = _write(stream->fd, stream->buffer, (unsigned int)to_write);
#else
    written = write(stream->fd, stream->buffer, to_write);
#endif
    
    if (written < 0) {
        stream->has_error = true;
        return EOF;
    }
    
    if ((size_t)written != to_write) {
        stream->has_error = true;
        return EOF;
    }
    
    stream->buffer_pos = 0;
    return 0;
}

// ===============================================
// Character I/O
// ===============================================

int fgetc(FILE* stream) {
    if (!stream || !stream->is_open || !stream->is_readable) {
        return EOF;
    }
    
    // Check if we need to fill buffer
    if (stream->buffer_pos >= stream->buffer_end) {
        if (_file_fill_buffer(stream) != 0) {
            return EOF;
        }
    }
    
    if (stream->buffer_pos >= stream->buffer_end) {
        stream->is_eof = true;
        return EOF;
    }
    
    return (unsigned char)stream->buffer[stream->buffer_pos++];
}

int _file_fill_buffer(FILE* stream) {
    if (!stream || !stream->buffer) return -1;
    
    ssize_t bytes_read = 0;
    
#ifdef _WIN32
    bytes_read = _read(stream->fd, stream->buffer, (unsigned int)stream->buffer_size);
#else
    bytes_read = read(stream->fd, stream->buffer, stream->buffer_size);
#endif
    
    if (bytes_read < 0) {
        stream->has_error = true;
        return -1;
    }
    
    if (bytes_read == 0) {
        stream->is_eof = true;
        return -1;
    }
    
    stream->buffer_pos = 0;
    stream->buffer_end = (size_t)bytes_read;
    
    return 0;
}

int getchar(void) {
    return fgetc(stdin);
}

int fputc(int c, FILE* stream) {
    if (!stream || !stream->is_open || !stream->is_writable) {
        return EOF;
    }
    
    // Handle unbuffered output
    if (stream->buffer_mode == _IONBF) {
        char ch = (char)c;
        ssize_t written = 0;
        
#ifdef _WIN32
        written = _write(stream->fd, &ch, 1);
#else
        written = write(stream->fd, &ch, 1);
#endif
        
        return (written == 1) ? c : EOF;
    }
    
    // Buffered output
    if (!stream->buffer) {
        return EOF;
    }
    
    // Flush buffer if full
    if (stream->buffer_pos >= stream->buffer_size) {
        if (_file_flush_buffer(stream) != 0) {
            return EOF;
        }
    }
    
    stream->buffer[stream->buffer_pos++] = (char)c;
    
    // Line buffered - flush on newline
    if (stream->buffer_mode == _IOLBF && c == '\n') {
        if (_file_flush_buffer(stream) != 0) {
            return EOF;
        }
    }
    
    return c;
}

int putchar(int c) {
    return fputc(c, stdout);
}

// ===============================================
// String I/O
// ===============================================

char* fgets(char* str, int n, FILE* stream) {
    if (!str || n <= 0 || !stream) return NULL;
    
    int i = 0;
    int c;
    
    while (i < n - 1) {
        c = fgetc(stream);
        if (c == EOF) {
            if (i == 0) return NULL;
            break;
        }
        
        str[i++] = (char)c;
        
        if (c == '\n') {
            break;
        }
    }
    
    str[i] = '\0';
    return str;
}

int fputs(const char* str, FILE* stream) {
    if (!str || !stream) return EOF;
    
    while (*str) {
        if (fputc(*str++, stream) == EOF) {
            return EOF;
        }
    }
    
    return 0;
}

int puts(const char* str) {
    if (fputs(str, stdout) == EOF) {
        return EOF;
    }
    
    return fputc('\n', stdout);
}

// ===============================================
// Formatted I/O
// ===============================================

typedef struct {
    char* buffer;           // 输出缓冲区
    size_t size;           // 缓冲区大小
    size_t pos;            // 当前位置
    FILE* stream;          // 输出流
    bool is_str;           // 是否输出到字符串
} FormatContext;

static void format_putc(int c, FormatContext* ctx) {
    if (ctx->is_str) {
        if (ctx->pos < ctx->size - 1) {
            ctx->buffer[ctx->pos++] = (char)c;
        }
    } else {
        fputc(c, ctx->stream);
        ctx->pos++;
    }
}

static void format_puts(const char* s, FormatContext* ctx) {
    while (*s) {
        format_putc(*s++, ctx);
    }
}

static void format_pad(int width, char pad_char, FormatContext* ctx) {
    while (width-- > 0) {
        format_putc(pad_char, ctx);
    }
}

static void format_integer(long long value, int base, bool is_unsigned,
                         int width, int precision, bool left_align,
                         bool show_sign, bool space_sign, bool alt_form,
                         char pad_char, FormatContext* ctx) {
    char buffer[32];
    char* p = buffer + sizeof(buffer) - 1;
    *p = '\0';
    
    unsigned long long uvalue;
    if (is_unsigned) {
        uvalue = (unsigned long long)value;
    } else {
        uvalue = value < 0 ? -value : value;
    }
    
    // 转换数字到字符串
    const char* digits = "0123456789abcdef";
    do {
        *--p = digits[uvalue % base];
        uvalue /= base;
    } while (uvalue);
    
    // 计算长度
    int len = buffer + sizeof(buffer) - 1 - p;
    int num_width = len;
    
    // 处理符号
    bool need_sign = false;
    if (!is_unsigned) {
        if (value < 0) {
            need_sign = true;
            num_width++;
        } else if (show_sign) {
            need_sign = true;
            num_width++;
        } else if (space_sign) {
            need_sign = true;
            num_width++;
        }
    }
    
    // 处理替代形式
    if (alt_form && base == 16) {
        num_width += 2;
    }
    
    // 处理精度
    if (precision > len) {
        num_width += precision - len;
    }
    
    // 处理左对齐
    if (!left_align && width > num_width) {
        format_pad(width - num_width, pad_char, ctx);
    }
    
    // 输出符号
    if (need_sign) {
        if (value < 0) {
            format_putc('-', ctx);
        } else if (show_sign) {
            format_putc('+', ctx);
        } else if (space_sign) {
            format_putc(' ', ctx);
        }
    }
    
    // 输出替代形式
    if (alt_form && base == 16) {
        format_putc('0', ctx);
        format_putc('x', ctx);
    }
    
    // 输出前导零
    if (precision > len) {
        format_pad(precision - len, '0', ctx);
    }
    
    // 输出数字
    format_puts(p, ctx);
    
    // 处理右填充
    if (left_align && width > num_width) {
        format_pad(width - num_width, ' ', ctx);
    }
}

static void format_float(double value, int width, int precision,
                        bool left_align, bool show_sign, bool space_sign,
                        char format, FormatContext* ctx) {
    char buffer[32];
    char fmt[8];
    int i = 0;
    
    fmt[i++] = '%';
    if (show_sign) fmt[i++] = '+';
    if (space_sign) fmt[i++] = ' ';
    if (left_align) fmt[i++] = '-';
    fmt[i++] = '.';
    fmt[i++] = '*';
    fmt[i] = '\0';
    
    snprintf(buffer, sizeof(buffer), fmt, precision, value);
    
    int len = strlen(buffer);
    if (!left_align && width > len) {
        format_pad(width - len, ' ', ctx);
    }
    
    format_puts(buffer, ctx);
    
    if (left_align && width > len) {
        format_pad(width - len, ' ', ctx);
    }
}

static int do_printf(const char* format, va_list args, FormatContext* ctx) {
    if (!format) return -1;
    
    while (*format) {
        if (*format != '%') {
            format_putc(*format++, ctx);
            continue;
        }
        
        // 处理格式说明符
        format++;
        
        // 解析标志
        bool left_align = false;
        bool show_sign = false;
        bool space_sign = false;
        bool alt_form = false;
        char pad_char = ' ';
        
        while (true) {
            switch (*format) {
                case '-': left_align = true; format++; continue;
                case '+': show_sign = true; format++; continue;
                case ' ': space_sign = true; format++; continue;
                case '#': alt_form = true; format++; continue;
                case '0': pad_char = '0'; format++; continue;
            }
            break;
        }
        
        // 解析宽度
        int width = 0;
        if (*format == '*') {
            width = va_arg(args, int);
            if (width < 0) {
                left_align = true;
                width = -width;
            }
            format++;
        } else {
            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }
        }
        
        // 解析精度
        int precision = -1;
        if (*format == '.') {
            format++;
            precision = 0;
            if (*format == '*') {
                precision = va_arg(args, int);
                format++;
            } else {
                while (*format >= '0' && *format <= '9') {
                    precision = precision * 10 + (*format - '0');
                    format++;
                }
            }
        }
        
        // 解析长度修饰符
        int length = 0; // 0=default, 1=h, 2=hh, 3=l, 4=ll, 5=L
        while (true) {
            switch (*format) {
                case 'h':
                    length = length == 1 ? 2 : 1;
                    format++;
                    continue;
                case 'l':
                    length = length == 3 ? 4 : 3;
                    format++;
                    continue;
                case 'L':
                    length = 5;
                    format++;
                    continue;
            }
            break;
        }
        
        // 处理转换说明符
        switch (*format) {
            case 'd':
            case 'i': {
                long long value;
                switch (length) {
                    case 0: value = va_arg(args, int); break;
                    case 1: value = (short)va_arg(args, int); break;
                    case 2: value = (char)va_arg(args, int); break;
                    case 3: value = va_arg(args, long); break;
                    case 4: value = va_arg(args, long long); break;
                    default: value = va_arg(args, int); break;
                }
                format_integer(value, 10, false, width, precision,
                             left_align, show_sign, space_sign, alt_form,
                             pad_char, ctx);
                break;
            }
            
            case 'u':
            case 'x':
            case 'X': {
                unsigned long long value;
                switch (length) {
                    case 0: value = va_arg(args, unsigned int); break;
                    case 1: value = (unsigned short)va_arg(args, unsigned int); break;
                    case 2: value = (unsigned char)va_arg(args, unsigned int); break;
                    case 3: value = va_arg(args, unsigned long); break;
                    case 4: value = va_arg(args, unsigned long long); break;
                    default: value = va_arg(args, unsigned int); break;
                }
                int base = *format == 'u' ? 10 : 16;
                format_integer(value, base, true, width, precision,
                             left_align, show_sign, space_sign, alt_form,
                             pad_char, ctx);
                break;
            }
            
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G': {
                double value;
                if (length == 5) {
                    value = va_arg(args, long double);
                } else {
                    value = va_arg(args, double);
                }
                if (precision < 0) precision = 6;
                format_float(value, width, precision, left_align,
                           show_sign, space_sign, *format, ctx);
                break;
            }
            
            case 'c': {
                int value = va_arg(args, int);
                if (!left_align && width > 1) {
                    format_pad(width - 1, ' ', ctx);
                }
                format_putc(value, ctx);
                if (left_align && width > 1) {
                    format_pad(width - 1, ' ', ctx);
                }
                break;
            }
            
            case 's': {
                const char* value = va_arg(args, const char*);
                if (!value) value = "(null)";
                int len = strlen(value);
                if (precision >= 0 && len > precision) {
                    len = precision;
                }
                if (!left_align && width > len) {
                    format_pad(width - len, ' ', ctx);
                }
                for (int i = 0; i < len; i++) {
                    format_putc(value[i], ctx);
                }
                if (left_align && width > len) {
                    format_pad(width - len, ' ', ctx);
                }
                break;
            }
            
            case 'p': {
                void* value = va_arg(args, void*);
                alt_form = true;
                format_integer((long long)value, 16, true, width, precision,
                             left_align, show_sign, space_sign, alt_form,
                             pad_char, ctx);
                break;
            }
            
            case 'n': {
                switch (length) {
                    case 0: {
                        int* p = va_arg(args, int*);
                        if (p) *p = ctx->pos;
                        break;
                    }
                    case 1: {
                        short* p = va_arg(args, short*);
                        if (p) *p = ctx->pos;
                        break;
                    }
                    case 2: {
                        char* p = va_arg(args, char*);
                        if (p) *p = ctx->pos;
                        break;
                    }
                    case 3: {
                        long* p = va_arg(args, long*);
                        if (p) *p = ctx->pos;
                        break;
                    }
                    case 4: {
                        long long* p = va_arg(args, long long*);
                        if (p) *p = ctx->pos;
                        break;
                    }
                }
                break;
            }
            
            case '%':
                format_putc('%', ctx);
                break;
                
            default:
                format_putc('%', ctx);
                if (*format) {
                    format_putc(*format, ctx);
                }
                break;
        }
        
        if (*format) {
            format++;
        }
    }
    
    // 添加字符串结束符
    if (ctx->is_str && ctx->pos < ctx->size) {
        ctx->buffer[ctx->pos] = '\0';
    }
    
    return ctx->pos;
}

int vfprintf(FILE* stream, const char* format, va_list args) {
    if (!stream || !format) return -1;
    
    FormatContext ctx = {
        .stream = stream,
        .is_str = false,
        .pos = 0
    };
    
    return do_printf(format, args, &ctx);
}

int fprintf(FILE* stream, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vfprintf(stream, format, args);
    va_end(args);
    return result;
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vfprintf(stdout, format, args);
    va_end(args);
    return result;
}

int vsprintf(char* str, const char* format, va_list args) {
    if (!str || !format) return -1;
    
    FormatContext ctx = {
        .buffer = str,
        .size = SIZE_MAX,
        .is_str = true,
        .pos = 0
    };
    
    return do_printf(format, args, &ctx);
}

int sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsprintf(str, format, args);
    va_end(args);
    return result;
}

int vsnprintf(char* str, size_t size, const char* format, va_list args) {
    if (!format) return -1;
    if (!str || size == 0) return 0;
    
    FormatContext ctx = {
        .buffer = str,
        .size = size,
        .is_str = true,
        .pos = 0
    };
    
    return do_printf(format, args, &ctx);
}

int snprintf(char* str, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, size, format, args);
    va_end(args);
    return result;
}

// ===============================================
// Error Handling
// ===============================================

void clearerr(FILE* stream) {
    if (stream) {
        stream->has_error = false;
        stream->is_eof = false;
    }
}

int feof(FILE* stream) {
    return stream ? stream->is_eof : 0;
}

int ferror(FILE* stream) {
    return stream ? stream->has_error : 0;
}

void perror(const char* str) {
    if (str && *str) {
        fprintf(stderr, "%s: ", str);
    }
    fprintf(stderr, "Error occurred\n");
}

// ===============================================
// Binary I/O
// ===============================================

size_t fread(void* ptr, size_t size, size_t count, FILE* stream) {
    if (!ptr || !stream || !stream->is_open || !stream->is_readable) {
        return 0;
    }
    
    size_t total = size * count;
    size_t bytes_read = 0;
    unsigned char* dest = (unsigned char*)ptr;
    
    while (bytes_read < total) {
        int ch = fgetc(stream);
        if (ch == EOF) {
            break;
        }
        dest[bytes_read++] = (unsigned char)ch;
    }
    
    return bytes_read / size;
}

size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream) {
    if (!ptr || !stream || !stream->is_open || !stream->is_writable) {
        return 0;
    }
    
    size_t total = size * count;
    size_t bytes_written = 0;
    const unsigned char* src = (const unsigned char*)ptr;
    
    while (bytes_written < total) {
        if (fputc(src[bytes_written], stream) == EOF) {
            break;
        }
        bytes_written++;
    }
    
    return bytes_written / size;
}

// ===============================================
// File Positioning
// ===============================================

long ftell(FILE* stream) {
    if (!stream || !stream->is_open) {
        return -1L;
    }
    
    // Get current position
#ifdef _WIN32
    long pos = _lseek(stream->fd, 0L, SEEK_CUR);
#else
    long pos = lseek(stream->fd, 0L, SEEK_CUR);
#endif
    
    if (pos == -1) {
        stream->has_error = true;
        return -1L;
    }
    
    // Adjust for buffered data
    if (stream->is_readable) {
        pos -= (stream->buffer_end - stream->buffer_pos);
    } else if (stream->is_writable) {
        pos += stream->buffer_pos;
    }
    
    return pos;
}

int fseek(FILE* stream, long offset, int whence) {
    if (!stream || !stream->is_open) {
        return -1;
    }
    
    // Flush any buffered data
    if (stream->is_writable && stream->buffer_pos > 0) {
        if (_file_flush_buffer(stream) != 0) {
            return -1;
        }
    }
    
    // Reset buffer state
    stream->buffer_pos = 0;
    stream->buffer_end = 0;
    stream->is_eof = false;
    
    // Seek to new position
#ifdef _WIN32
    if (_lseek(stream->fd, offset, whence) == -1L) {
#else
    if (lseek(stream->fd, offset, whence) == -1L) {
#endif
        stream->has_error = true;
        return -1;
    }
    
    return 0;
}

void rewind(FILE* stream) {
    if (stream) {
        clearerr(stream);
        fseek(stream, 0L, SEEK_SET);
    }
}

// ===============================================
// File Management
// ===============================================

int remove(const char* filename) {
    if (!filename) return -1;
    
#ifdef _WIN32
    return _unlink(filename);
#else
    return unlink(filename);
#endif
}

int rename(const char* old_name, const char* new_name) {
    if (!old_name || !new_name) return -1;
    
    return rename(old_name, new_name);
}
