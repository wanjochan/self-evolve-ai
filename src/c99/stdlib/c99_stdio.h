/**
 * c99_stdio.h - C99 Standard I/O Library Implementation
 * 
 * Implementation of stdio.h functions for C99 compiler including
 * printf, scanf, file operations, and stream management.
 */

#ifndef C99_STDIO_H
#define C99_STDIO_H

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// File Stream Structure
// ===============================================

typedef struct FILE {
    int fd;                     // File descriptor
    char* buffer;               // I/O buffer
    size_t buffer_size;         // Buffer size
    size_t buffer_pos;          // Current position in buffer
    size_t buffer_end;          // End of valid data in buffer
    
    // Stream state
    bool is_open;               // Is file open
    bool is_eof;                // End of file reached
    bool has_error;             // Error occurred
    bool is_readable;           // Stream is readable
    bool is_writable;           // Stream is writable
    bool is_binary;             // Binary mode
    
    // Buffering mode
    int buffer_mode;            // _IOFBF, _IOLBF, _IONBF
    bool owns_buffer;           // Does FILE own the buffer
    
    // Position tracking
    long position;              // Current file position
    
    // File information
    char* filename;             // Filename (if known)
    char* mode;                 // Open mode string
} FILE;

// ===============================================
// Standard Streams
// ===============================================

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

// ===============================================
// Buffer Mode Constants
// ===============================================

#define _IOFBF  0               // Full buffering
#define _IOLBF  1               // Line buffering
#define _IONBF  2               // No buffering

// ===============================================
// Seek Constants
// ===============================================

#define SEEK_SET    0           // Seek from beginning
#define SEEK_CUR    1           // Seek from current position
#define SEEK_END    2           // Seek from end

// ===============================================
// Other Constants
// ===============================================

#define EOF         (-1)        // End of file indicator
#define BUFSIZ      8192        // Default buffer size
#define FILENAME_MAX 260        // Maximum filename length
#define FOPEN_MAX   20          // Maximum open files
#define TMP_MAX     32767       // Maximum temporary files

// ===============================================
// File Operations
// ===============================================

/**
 * Open file
 */
FILE* fopen(const char* filename, const char* mode);

/**
 * Close file
 */
int fclose(FILE* stream);

/**
 * Flush stream
 */
int fflush(FILE* stream);

// ===============================================
// Character I/O
// ===============================================

/**
 * Get character
 */
int fgetc(FILE* stream);

/**
 * Get character from stdin
 */
int getchar(void);

/**
 * Put character
 */
int fputc(int c, FILE* stream);

/**
 * Put character to stdout
 */
int putchar(int c);

// ===============================================
// String I/O
// ===============================================

/**
 * Get string
 */
char* fgets(char* str, int n, FILE* stream);

/**
 * Put string
 */
int fputs(const char* str, FILE* stream);

/**
 * Put string to stdout
 */
int puts(const char* str);

// ===============================================
// Formatted I/O
// ===============================================

/**
 * Formatted print to stream
 */
int fprintf(FILE* stream, const char* format, ...);

/**
 * Formatted print to stdout
 */
int printf(const char* format, ...);

/**
 * Formatted print to string
 */
int sprintf(char* str, const char* format, ...);

/**
 * Formatted print with va_list
 */
int vfprintf(FILE* stream, const char* format, va_list args);

/**
 * Formatted print to stdout with va_list
 */
int vprintf(const char* format, va_list args);

/**
 * Formatted print to string with va_list
 */
int vsprintf(char* str, const char* format, va_list args);

/**
 * Formatted print to string with size limit
 */
int snprintf(char* str, size_t size, const char* format, ...);

/**
 * Formatted print to string with size limit and va_list
 */
int vsnprintf(char* str, size_t size, const char* format, va_list args);

// ===============================================
// Binary I/O
// ===============================================

/**
 * Read binary data
 */
size_t fread(void* ptr, size_t size, size_t count, FILE* stream);

/**
 * Write binary data
 */
size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream);

// ===============================================
// File Positioning
// ===============================================

/**
 * Get file position
 */
long ftell(FILE* stream);

/**
 * Set file position
 */
int fseek(FILE* stream, long offset, int whence);

/**
 * Rewind file
 */
void rewind(FILE* stream);

// ===============================================
// Error Handling
// ===============================================

/**
 * Clear error indicators
 */
void clearerr(FILE* stream);

/**
 * Test end-of-file indicator
 */
int feof(FILE* stream);

/**
 * Test error indicator
 */
int ferror(FILE* stream);

/**
 * Print error message
 */
void perror(const char* str);

// ===============================================
// File Management
// ===============================================

/**
 * Remove file
 */
int remove(const char* filename);

/**
 * Rename file
 */
int rename(const char* old_name, const char* new_name);

// ===============================================
// Implementation-Specific Types
// ===============================================

typedef long fpos_t;            // File position type

// ===============================================
// Internal Functions (Implementation Details)
// ===============================================

/**
 * Initialize standard streams
 */
void _stdio_init(void);

/**
 * Cleanup standard streams
 */
void _stdio_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // C99_STDIO_H
