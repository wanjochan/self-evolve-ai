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
// Formatted I/O (Simplified Implementation)
// ===============================================

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vfprintf(stdout, format, args);
    va_end(args);
    return result;
}

int fprintf(FILE* stream, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vfprintf(stream, format, args);
    va_end(args);
    return result;
}

int vfprintf(FILE* stream, const char* format, va_list args) {
    // Simplified implementation
    if (!stream || !format) return -1;
    
    int count = 0;
    const char* p = format;
    
    while (*p) {
        if (*p != '%') {
            if (fputc(*p, stream) == EOF) return -1;
            count++;
            p++;
            continue;
        }
        
        p++; // Skip '%'
        
        switch (*p) {
            case 'd': {
                int val = va_arg(args, int);
                char buffer[32];
                sprintf(buffer, "%d", val);
                if (fputs(buffer, stream) == EOF) return -1;
                count += strlen(buffer);
                break;
            }
            case 's': {
                const char* str = va_arg(args, const char*);
                if (!str) str = "(null)";
                if (fputs(str, stream) == EOF) return -1;
                count += strlen(str);
                break;
            }
            case 'c': {
                int ch = va_arg(args, int);
                if (fputc(ch, stream) == EOF) return -1;
                count++;
                break;
            }
            case '%':
                if (fputc('%', stream) == EOF) return -1;
                count++;
                break;
            default:
                if (fputc('%', stream) == EOF) return -1;
                if (fputc(*p, stream) == EOF) return -1;
                count += 2;
                break;
        }
        
        p++;
    }
    
    return count;
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
