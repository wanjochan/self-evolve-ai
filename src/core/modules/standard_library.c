/**
 * standard_library.c - C99Bin Standard Library Support
 * 
 * T2.2: 标准库集成 - 为自举编译提供核心C标准库函数
 * 实现malloc, printf, string functions等基础功能
 */

#include "pipeline_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

// 标准库模块类型
typedef enum {
    STDLIB_MEMORY,      // 内存管理 (malloc/free)
    STDLIB_IO,          // 输入输出 (printf/scanf)
    STDLIB_STRING,      // 字符串处理
    STDLIB_MATH,        // 数学函数
    STDLIB_SETJMP,      // setjmp/longjmp实现
    STDLIB_SYSTEM,      // 系统调用包装
    STDLIB_CTYPE,       // 字符分类
    STDLIB_TIME         // 时间处理
} StdlibModule;

// 内存分配器状态
typedef struct MemoryAllocator {
    void* heap_start;
    void* heap_current;
    size_t heap_size;
    size_t allocated_bytes;
    size_t free_bytes;
    int allocation_count;
    bool enable_debug;
    struct MemoryBlock* free_list;
} MemoryAllocator;

// 内存块头部
typedef struct MemoryBlock {
    size_t size;
    bool is_free;
    struct MemoryBlock* next;
    struct MemoryBlock* prev;
    char magic[4];  // "HEAP"
} MemoryBlock;

// I/O缓冲区管理
typedef struct IOBuffer {
    char* buffer;
    size_t size;
    size_t position;
    bool is_output;
    FILE* file_handle;
} IOBuffer;

// 标准库上下文
typedef struct {
    MemoryAllocator* allocator;
    IOBuffer* stdout_buffer;
    IOBuffer* stderr_buffer;
    IOBuffer* stdin_buffer;
    bool enable_setjmp_longjmp;
    bool enable_debug_malloc;
    bool enable_io_buffering;
    jmp_buf* global_error_handler;
    int error_code;
    char error_message[256];
} StandardLibContext;

// 外部结构声明
typedef struct jmp_buf jmp_buf;

// 标准库接口
bool initialize_standard_library(void);
void cleanup_standard_library(void);
StandardLibContext* get_stdlib_context(void);

// 内存管理函数
void* c99bin_malloc(size_t size);
void* c99bin_calloc(size_t nmemb, size_t size);
void* c99bin_realloc(void* ptr, size_t size);
void c99bin_free(void* ptr);

// I/O函数
int c99bin_printf(const char* format, ...);
int c99bin_fprintf(FILE* stream, const char* format, ...);
int c99bin_sprintf(char* str, const char* format, ...);
int c99bin_scanf(const char* format, ...);

// 字符串函数
char* c99bin_strcpy(char* dest, const char* src);
char* c99bin_strncpy(char* dest, const char* src, size_t n);
char* c99bin_strcat(char* dest, const char* src);
int c99bin_strcmp(const char* s1, const char* s2);
size_t c99bin_strlen(const char* s);

// setjmp/longjmp函数
int c99bin_setjmp(jmp_buf env);
void c99bin_longjmp(jmp_buf env, int val);

// 全局标准库上下文
static StandardLibContext* g_stdlib_ctx = NULL;

// 初始化标准库
bool initialize_standard_library(void) {
    printf("🔧 Initializing C99Bin Standard Library...\n");
    printf("=========================================\n");
    
    if (g_stdlib_ctx) {
        printf("⚠️  Standard library already initialized\n");
        return true;
    }
    
    g_stdlib_ctx = malloc(sizeof(StandardLibContext));
    if (!g_stdlib_ctx) {
        printf("❌ Failed to allocate standard library context\n");
        return false;
    }
    memset(g_stdlib_ctx, 0, sizeof(StandardLibContext));
    
    // 初始化内存分配器
    printf("💾 Phase 1: Memory Allocator Initialization\n");
    printf("===========================================\n");
    if (!initialize_memory_allocator()) {
        printf("❌ Memory allocator initialization failed\n");
        cleanup_standard_library();
        return false;
    }
    
    // 初始化I/O系统
    printf("\n📝 Phase 2: I/O System Initialization\n");
    printf("=====================================\n");
    if (!initialize_io_system()) {
        printf("❌ I/O system initialization failed\n");
        cleanup_standard_library();
        return false;
    }
    
    // 初始化setjmp/longjmp支持
    printf("\n🎯 Phase 3: setjmp/longjmp Support\n");
    printf("==================================\n");
    if (!initialize_setjmp_support()) {
        printf("❌ setjmp/longjmp initialization failed\n");
        cleanup_standard_library();
        return false;
    }
    
    // 设置全局错误处理
    printf("\n🛡️ Phase 4: Error Handling Setup\n");
    printf("=================================\n");
    if (!setup_error_handling()) {
        printf("❌ Error handling setup failed\n");
        cleanup_standard_library();
        return false;
    }
    
    printf("✅ C99Bin Standard Library initialized successfully!\n");
    printf("   - Memory allocator: Ready (heap size: %zu KB)\n", 
           g_stdlib_ctx->allocator->heap_size / 1024);
    printf("   - I/O system: Ready (buffered)\n");
    printf("   - setjmp/longjmp: Ready\n");
    printf("   - Error handling: Active\n");
    
    return true;
}

// 初始化内存分配器
bool initialize_memory_allocator(void) {
    printf("💾 Setting up memory allocator...\n");
    
    g_stdlib_ctx->allocator = malloc(sizeof(MemoryAllocator));
    if (!g_stdlib_ctx->allocator) {
        printf("❌ Failed to allocate memory allocator\n");
        return false;
    }
    
    MemoryAllocator* alloc = g_stdlib_ctx->allocator;
    memset(alloc, 0, sizeof(MemoryAllocator));
    
    // 分配初始堆空间 (1MB)
    alloc->heap_size = 1024 * 1024;
    alloc->heap_start = malloc(alloc->heap_size);
    if (!alloc->heap_start) {
        printf("❌ Failed to allocate initial heap\n");
        return false;
    }
    
    alloc->heap_current = alloc->heap_start;
    alloc->free_bytes = alloc->heap_size;
    alloc->allocated_bytes = 0;
    alloc->allocation_count = 0;
    alloc->enable_debug = true;
    alloc->free_list = NULL;
    
    // 初始化空闲块链表
    MemoryBlock* initial_block = (MemoryBlock*)alloc->heap_start;
    initial_block->size = alloc->heap_size - sizeof(MemoryBlock);
    initial_block->is_free = true;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    memcpy(initial_block->magic, "HEAP", 4);
    alloc->free_list = initial_block;
    
    printf("✅ Memory allocator initialized\n");
    printf("   - Heap size: %zu KB\n", alloc->heap_size / 1024);
    printf("   - Debug mode: %s\n", alloc->enable_debug ? "Enabled" : "Disabled");
    printf("   - Initial free block: %zu bytes\n", initial_block->size);
    
    return true;
}

// 初始化I/O系统
bool initialize_io_system(void) {
    printf("📝 Setting up I/O system...\n");
    
    // 初始化stdout缓冲区
    g_stdlib_ctx->stdout_buffer = malloc(sizeof(IOBuffer));
    if (!g_stdlib_ctx->stdout_buffer) {
        printf("❌ Failed to allocate stdout buffer\n");
        return false;
    }
    
    IOBuffer* stdout_buf = g_stdlib_ctx->stdout_buffer;
    stdout_buf->size = 8192; // 8KB缓冲区
    stdout_buf->buffer = malloc(stdout_buf->size);
    stdout_buf->position = 0;
    stdout_buf->is_output = true;
    stdout_buf->file_handle = stdout;
    
    // 初始化stderr缓冲区
    g_stdlib_ctx->stderr_buffer = malloc(sizeof(IOBuffer));
    IOBuffer* stderr_buf = g_stdlib_ctx->stderr_buffer;
    stderr_buf->size = 4096; // 4KB缓冲区
    stderr_buf->buffer = malloc(stderr_buf->size);
    stderr_buf->position = 0;
    stderr_buf->is_output = true;
    stderr_buf->file_handle = stderr;
    
    // 初始化stdin缓冲区
    g_stdlib_ctx->stdin_buffer = malloc(sizeof(IOBuffer));
    IOBuffer* stdin_buf = g_stdlib_ctx->stdin_buffer;
    stdin_buf->size = 4096; // 4KB缓冲区
    stdin_buf->buffer = malloc(stdin_buf->size);
    stdin_buf->position = 0;
    stdin_buf->is_output = false;
    stdin_buf->file_handle = stdin;
    
    g_stdlib_ctx->enable_io_buffering = true;
    
    printf("✅ I/O system initialized\n");
    printf("   - stdout buffer: %zu KB\n", stdout_buf->size / 1024);
    printf("   - stderr buffer: %zu KB\n", stderr_buf->size / 1024);
    printf("   - stdin buffer: %zu KB\n", stdin_buf->size / 1024);
    printf("   - Buffering: %s\n", g_stdlib_ctx->enable_io_buffering ? "Enabled" : "Disabled");
    
    return true;
}

// 初始化setjmp/longjmp支持
bool initialize_setjmp_support(void) {
    printf("🎯 Setting up setjmp/longjmp support...\n");
    
    g_stdlib_ctx->enable_setjmp_longjmp = true;
    g_stdlib_ctx->global_error_handler = malloc(sizeof(jmp_buf));
    if (!g_stdlib_ctx->global_error_handler) {
        printf("❌ Failed to allocate global error handler\n");
        return false;
    }
    
    printf("✅ setjmp/longjmp support initialized\n");
    printf("   - Global error handler: Ready\n");
    printf("   - Context preservation: Enabled\n");
    printf("   - Non-local jumps: Supported\n");
    
    return true;
}

// 设置错误处理
bool setup_error_handling(void) {
    printf("🛡️ Setting up error handling...\n");
    
    g_stdlib_ctx->error_code = 0;
    memset(g_stdlib_ctx->error_message, 0, sizeof(g_stdlib_ctx->error_message));
    strcpy(g_stdlib_ctx->error_message, "No error");
    
    // 设置全局错误处理器
    if (g_stdlib_ctx->enable_setjmp_longjmp) {
        if (setjmp(*g_stdlib_ctx->global_error_handler) != 0) {
            printf("🚨 Global error handler activated: %s\n", g_stdlib_ctx->error_message);
            return false;
        }
    }
    
    printf("✅ Error handling setup complete\n");
    printf("   - Global error handler: Active\n");
    printf("   - Error reporting: Ready\n");
    
    return true;
}

// C99Bin malloc实现
void* c99bin_malloc(size_t size) {
    if (!g_stdlib_ctx || !g_stdlib_ctx->allocator) {
        return NULL;
    }
    
    MemoryAllocator* alloc = g_stdlib_ctx->allocator;
    
    // 对齐到8字节边界
    size_t aligned_size = (size + 7) & ~7;
    size_t total_size = aligned_size + sizeof(MemoryBlock);
    
    // 查找合适的空闲块
    MemoryBlock* block = find_free_block(alloc, total_size);
    if (!block) {
        if (alloc->enable_debug) {
            printf("⚠️  malloc(%zu): No suitable free block found\n", size);
        }
        return NULL;
    }
    
    // 分割块(如果需要)
    if (block->size > total_size + sizeof(MemoryBlock) + 32) {
        split_memory_block(block, total_size);
    }
    
    // 标记块为已使用
    block->is_free = false;
    remove_from_free_list(alloc, block);
    
    alloc->allocated_bytes += block->size;
    alloc->free_bytes -= block->size;
    alloc->allocation_count++;
    
    if (alloc->enable_debug) {
        printf("📦 malloc(%zu) -> %p (block size: %zu)\n", 
               size, (char*)block + sizeof(MemoryBlock), block->size);
    }
    
    return (char*)block + sizeof(MemoryBlock);
}

// C99Bin free实现
void c99bin_free(void* ptr) {
    if (!ptr || !g_stdlib_ctx || !g_stdlib_ctx->allocator) {
        return;
    }
    
    MemoryAllocator* alloc = g_stdlib_ctx->allocator;
    MemoryBlock* block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
    
    // 验证魔数
    if (memcmp(block->magic, "HEAP", 4) != 0) {
        printf("❌ free(%p): Invalid magic number - possible corruption\n", ptr);
        return;
    }
    
    if (block->is_free) {
        printf("❌ free(%p): Double free detected\n", ptr);
        return;
    }
    
    // 标记为空闲
    block->is_free = true;
    add_to_free_list(alloc, block);
    
    alloc->allocated_bytes -= block->size;
    alloc->free_bytes += block->size;
    alloc->allocation_count--;
    
    // 尝试与相邻空闲块合并
    merge_free_blocks(alloc, block);
    
    if (alloc->enable_debug) {
        printf("🗑️  free(%p) (block size: %zu)\n", ptr, block->size);
    }
}

// C99Bin printf实现
int c99bin_printf(const char* format, ...) {
    if (!g_stdlib_ctx || !g_stdlib_ctx->stdout_buffer) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    
    char temp_buffer[4096];
    int result = vsnprintf(temp_buffer, sizeof(temp_buffer), format, args);
    
    if (g_stdlib_ctx->enable_io_buffering) {
        // 添加到输出缓冲区
        IOBuffer* buf = g_stdlib_ctx->stdout_buffer;
        size_t len = strlen(temp_buffer);
        
        if (buf->position + len < buf->size) {
            memcpy(buf->buffer + buf->position, temp_buffer, len);
            buf->position += len;
        } else {
            // 缓冲区满，刷新并直接输出
            flush_io_buffer(buf);
            fwrite(temp_buffer, 1, len, buf->file_handle);
        }
    } else {
        // 直接输出
        fwrite(temp_buffer, 1, strlen(temp_buffer), stdout);
    }
    
    va_end(args);
    return result;
}

// C99Bin字符串函数
char* c99bin_strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}

size_t c99bin_strlen(const char* s) {
    const char* p = s;
    while (*p) p++;
    return p - s;
}

int c99bin_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// 查找空闲块
MemoryBlock* find_free_block(MemoryAllocator* alloc, size_t size) {
    MemoryBlock* current = alloc->free_list;
    MemoryBlock* best_fit = NULL;
    
    while (current) {
        if (current->is_free && current->size >= size) {
            if (!best_fit || current->size < best_fit->size) {
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    return best_fit;
}

// 分割内存块
void split_memory_block(MemoryBlock* block, size_t size) {
    MemoryBlock* new_block = (MemoryBlock*)((char*)block + sizeof(MemoryBlock) + size);
    new_block->size = block->size - size - sizeof(MemoryBlock);
    new_block->is_free = true;
    new_block->next = block->next;
    new_block->prev = block;
    memcpy(new_block->magic, "HEAP", 4);
    
    if (block->next) {
        block->next->prev = new_block;
    }
    block->next = new_block;
    block->size = size;
}

// 刷新I/O缓冲区
void flush_io_buffer(IOBuffer* buffer) {
    if (buffer->position > 0) {
        fwrite(buffer->buffer, 1, buffer->position, buffer->file_handle);
        fflush(buffer->file_handle);
        buffer->position = 0;
    }
}

// 获取标准库上下文
StandardLibContext* get_stdlib_context(void) {
    return g_stdlib_ctx;
}

// 清理标准库
void cleanup_standard_library(void) {
    if (g_stdlib_ctx) {
        if (g_stdlib_ctx->allocator) {
            printf("📊 Memory allocator statistics:\n");
            printf("   - Total allocations: %d\n", g_stdlib_ctx->allocator->allocation_count);
            printf("   - Allocated bytes: %zu\n", g_stdlib_ctx->allocator->allocated_bytes);
            printf("   - Free bytes: %zu\n", g_stdlib_ctx->allocator->free_bytes);
            
            if (g_stdlib_ctx->allocator->heap_start) {
                free(g_stdlib_ctx->allocator->heap_start);
            }
            free(g_stdlib_ctx->allocator);
        }
        
        if (g_stdlib_ctx->stdout_buffer) {
            flush_io_buffer(g_stdlib_ctx->stdout_buffer);
            free(g_stdlib_ctx->stdout_buffer->buffer);
            free(g_stdlib_ctx->stdout_buffer);
        }
        
        if (g_stdlib_ctx->stderr_buffer) {
            flush_io_buffer(g_stdlib_ctx->stderr_buffer);
            free(g_stdlib_ctx->stderr_buffer->buffer);
            free(g_stdlib_ctx->stderr_buffer);
        }
        
        if (g_stdlib_ctx->stdin_buffer) {
            free(g_stdlib_ctx->stdin_buffer->buffer);
            free(g_stdlib_ctx->stdin_buffer);
        }
        
        if (g_stdlib_ctx->global_error_handler) {
            free(g_stdlib_ctx->global_error_handler);
        }
        
        free(g_stdlib_ctx);
        g_stdlib_ctx = NULL;
    }
    
    printf("✅ C99Bin Standard Library cleanup completed\n");
}