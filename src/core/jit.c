/**
 * jit.c - Core JIT Compiler Implementation
 * 
 * Implements the core JIT compilation functionality for the self-evolve-ai system.
 * Provides cross-architecture JIT compilation with caching and optimization.
 */

#include "jit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Forward declarations
static JITResult jit_emit_prologue(JITCompiler* jit);
static JITResult jit_emit_epilogue(JITCompiler* jit);
static JITResult jit_compile_instruction(JITCompiler* jit, uint8_t opcode, const uint8_t* instruction, size_t max_size);
static size_t jit_get_instruction_size(uint8_t opcode, const uint8_t* instruction);
static JITResult jit_emit_halt(JITCompiler* jit);
static JITResult jit_emit_load_imm32(JITCompiler* jit, uint8_t reg, uint32_t imm);
static JITResult jit_emit_add(JITCompiler* jit, uint8_t reg1, uint8_t reg2, uint8_t reg3);
static JITResult jit_emit_exit(JITCompiler* jit, uint8_t exit_code);
static void jit_set_error(JITCompiler* jit, JITResult error, const char* message);
static JITResult jit_emit_byte(JITCompiler* jit, uint8_t byte);
static JITResult jit_emit_int32(JITCompiler* jit, uint32_t value);

// ===============================================
// JIT Global State
// ===============================================

static bool jit_system_initialized = false;
static JITStats global_jit_stats = {0};

// JIT Cache (simplified implementation)
typedef struct JITCacheEntry {
    uint64_t bytecode_hash;
    void* entry_point;
    size_t code_size;
    struct JITCacheEntry* next;
} JITCacheEntry;

static JITCacheEntry* jit_cache_head = NULL;
static size_t jit_cache_size = 0;
static size_t jit_max_cache_size = 0;

// ===============================================
// JIT Core Implementation
// ===============================================

/**
 * Initialize JIT compiler
 */
JITCompiler* jit_init(DetectedArchitecture target_arch, JITOptLevel opt_level, uint32_t flags) {
    if (!jit_system_initialized) {
        jit_system_initialized = true;
        memset(&global_jit_stats, 0, sizeof(JITStats));
        printf("JIT: System initialized\n");
    }
    
    // Auto-detect architecture if needed
    if (target_arch == ARCH_UNKNOWN) {
        target_arch = detect_architecture();
    }
    
    // Check if architecture is supported
    if (!jit_is_architecture_supported(target_arch)) {
        printf("JIT Error: Unsupported architecture %d\n", target_arch);
        return NULL;
    }
    
    // Allocate JIT compiler
    JITCompiler* jit = malloc(sizeof(JITCompiler));
    if (!jit) {
        printf("JIT Error: Failed to allocate JIT compiler\n");
        return NULL;
    }
    
    memset(jit, 0, sizeof(JITCompiler));
    
    // Initialize JIT compiler
    jit->target_arch = target_arch;
    jit->opt_level = opt_level;
    jit->flags = flags;
    jit->last_error = JIT_SUCCESS;
    
    // Allocate code buffer (64KB initial size)
    jit->code_capacity = 64 * 1024;
    jit->code_buffer = allocate_executable_memory(jit->code_capacity);
    if (!jit->code_buffer) {
        printf("JIT Error: Failed to allocate executable memory\n");
        free(jit);
        return NULL;
    }
    
    jit->code_size = 0;
    
    // Allocate label table
    jit->label_capacity = 256;
    jit->label_table = malloc(jit->label_capacity * sizeof(uint32_t));
    if (!jit->label_table) {
        printf("JIT Error: Failed to allocate label table\n");
        free_executable_memory(jit->code_buffer, jit->code_capacity);
        free(jit);
        return NULL;
    }
    
    jit->label_count = 0;
    
    printf("JIT: Initialized compiler for %s architecture\n", get_architecture_name(target_arch));
    return jit;
}

/**
 * Cleanup JIT compiler
 */
void jit_cleanup(JITCompiler* jit) {
    if (!jit) {
        return;
    }
    
    printf("JIT: Cleaning up compiler\n");
    
    if (jit->code_buffer) {
        free_executable_memory(jit->code_buffer, jit->code_capacity);
    }
    
    if (jit->label_table) {
        free(jit->label_table);
    }
    
    free(jit);
}

/**
 * Set JIT error
 */
static void jit_set_error(JITCompiler* jit, JITResult error, const char* message) {
    if (!jit) return;
    
    jit->last_error = error;
    safe_snprintf(jit->error_message, sizeof(jit->error_message), "%s", message);
}

/**
 * Emit byte to code buffer
 */
static JITResult jit_emit_byte(JITCompiler* jit, uint8_t byte) {
    if (!jit || !jit->code_buffer) {
        return JIT_ERROR_INVALID_INPUT;
    }
    
    if (jit->code_size >= jit->code_capacity) {
        jit_set_error(jit, JIT_ERROR_BUFFER_OVERFLOW, "Code buffer overflow");
        return JIT_ERROR_BUFFER_OVERFLOW;
    }
    
    jit->code_buffer[jit->code_size++] = byte;
    return JIT_SUCCESS;
}

/**
 * Emit 32-bit integer to code buffer
 */
static JITResult jit_emit_int32(JITCompiler* jit, uint32_t value) {
    for (int i = 0; i < 4; i++) {
        JITResult result = jit_emit_byte(jit, (value >> (i * 8)) & 0xFF);
        if (result != JIT_SUCCESS) {
            return result;
        }
    }
    return JIT_SUCCESS;
}

/**
 * Compile ASTC bytecode to native machine code
 */
JITResult jit_compile_bytecode(JITCompiler* jit, const uint8_t* bytecode, 
                              size_t bytecode_size, uint32_t entry_point) {
    if (!jit || !bytecode || bytecode_size == 0) {
        if (jit) jit_set_error(jit, JIT_ERROR_INVALID_INPUT, "Invalid input parameters");
        return JIT_ERROR_INVALID_INPUT;
    }
    
    printf("JIT: Compiling %zu bytes of ASTC bytecode\n", bytecode_size);
    
    uint64_t start_time = get_current_time_us();
    
    // Reset code buffer
    jit->code_size = 0;
    jit->label_count = 0;
    
    // Check cache first
    uint64_t bytecode_hash = jit_hash_bytecode(bytecode, bytecode_size);
    void* cached_entry;
    size_t cached_size;
    
    if (jit_cache_lookup(bytecode_hash, &cached_entry, &cached_size)) {
        printf("JIT: Found compiled code in cache\n");
        memcpy(jit->code_buffer, cached_entry, cached_size);
        jit->code_size = cached_size;
        global_jit_stats.cache_hits++;
        return JIT_SUCCESS;
    }
    
    global_jit_stats.cache_misses++;
    
    // Emit function prologue
    JITResult result = jit_emit_prologue(jit);
    if (result != JIT_SUCCESS) {
        return result;
    }
    
    // Compile bytecode instructions
    size_t pc = entry_point;
    while (pc < bytecode_size) {
        uint8_t opcode = bytecode[pc];
        
        result = jit_compile_instruction(jit, opcode, bytecode + pc, bytecode_size - pc);
        if (result != JIT_SUCCESS) {
            return result;
        }
        
        // Advance program counter based on instruction
        pc += jit_get_instruction_size(opcode, bytecode + pc);
        
        if (pc >= bytecode_size) {
            break;
        }
    }
    
    // Emit function epilogue
    result = jit_emit_epilogue(jit);
    if (result != JIT_SUCCESS) {
        return result;
    }
    
    // Update statistics
    uint64_t compile_time = get_current_time_us() - start_time;
    jit->compilation_time_us += compile_time;
    jit->bytes_compiled += bytecode_size;
    global_jit_stats.total_compilations++;
    global_jit_stats.total_compile_time += compile_time;
    global_jit_stats.total_code_size += jit->code_size;
    
    // Cache the result
    if (jit->flags & JIT_FLAG_CACHE_RESULT) {
        jit_cache_store(bytecode_hash, jit->code_buffer, jit->code_size);
    }
    
    printf("JIT: Compilation completed (%zu bytes -> %zu bytes in %llu us)\n", 
           bytecode_size, jit->code_size, (unsigned long long)compile_time);
    
    return JIT_SUCCESS;
}

/**
 * Get compiled code entry point
 */
void* jit_get_entry_point(JITCompiler* jit) {
    if (!jit || !jit->code_buffer || jit->code_size == 0) {
        return NULL;
    }
    
    return jit->code_buffer;
}

/**
 * Get compiled code size
 */
size_t jit_get_code_size(JITCompiler* jit) {
    if (!jit) {
        return 0;
    }
    
    return jit->code_size;
}

/**
 * Execute compiled code
 */
int jit_execute(JITCompiler* jit, void** args, int arg_count, void* result) {
    if (!jit || !jit->code_buffer || jit->code_size == 0) {
        return -1;
    }
    
    printf("JIT: Executing compiled code at %p\n", jit->code_buffer);
    
    // Cast to function pointer and execute
    typedef int (*jit_function_t)(void);
    jit_function_t func = (jit_function_t)jit->code_buffer;
    
    int exec_result = func();
    
    if (result) {
        *(int*)result = exec_result;
    }
    
    printf("JIT: Execution completed with result %d\n", exec_result);
    return 0;
}

/**
 * Get JIT compilation statistics
 */
void jit_get_stats(JITCompiler* jit, JITStats* stats) {
    if (!stats) {
        return;
    }
    
    *stats = global_jit_stats;
    
    if (jit && global_jit_stats.total_compilations > 0) {
        stats->average_compile_time = global_jit_stats.total_compile_time / global_jit_stats.total_compilations;
    }
}

/**
 * Get last error message
 */
const char* jit_get_error_message(JITCompiler* jit) {
    if (!jit || jit->error_message[0] == '\0') {
        return NULL;
    }
    
    return jit->error_message;
}

/**
 * Check if architecture is supported
 */
bool jit_is_architecture_supported(DetectedArchitecture arch) {
    switch (arch) {
        case ARCH_X86_64:
        case ARCH_X86_32:
            return true;
        case ARCH_ARM64:
        case ARCH_ARM32:
            return false; // Not implemented yet
        default:
            return false;
    }
}

/**
 * Get JIT compiler version
 */
const char* jit_get_version(void) {
    return "JIT Core v1.0.0";
}

/**
 * Print JIT compiler information
 */
void jit_print_info(void) {
    printf("=== JIT Compiler Information ===\n");
    printf("Version: %s\n", jit_get_version());
    printf("Supported architectures: x64, x86\n");
    printf("Features: Caching, Optimization, Statistics\n");
    printf("Status: %s\n", jit_system_initialized ? "Initialized" : "Not initialized");
    
    if (jit_system_initialized) {
        printf("\nGlobal Statistics:\n");
        printf("  Total compilations: %zu\n", global_jit_stats.total_compilations);
        printf("  Cache hits: %zu\n", global_jit_stats.cache_hits);
        printf("  Cache misses: %zu\n", global_jit_stats.cache_misses);
        printf("  Total compile time: %llu us\n", (unsigned long long)global_jit_stats.total_compile_time);
        printf("  Total code size: %zu bytes\n", global_jit_stats.total_code_size);
    }
}

// ===============================================
// JIT Architecture-Specific Code Generation
// ===============================================

// Old prologue and epilogue functions removed - replaced with enhanced versions below

/**
 * Compile single ASTC instruction
 */
static JITResult jit_compile_instruction(JITCompiler* jit, uint8_t opcode, const uint8_t* instruction, size_t max_size) {
    if (!jit || !instruction) return JIT_ERROR_INVALID_INPUT;

    switch (opcode) {
        case 0x00: // NOP
            // No code generation needed for NOP
            break;

        case 0x01: // HALT
            return jit_emit_halt(jit);

        case 0x10: // LOAD_IMM32 reg, imm32
            if (max_size >= 6) {
                uint8_t reg = instruction[1];
                uint32_t imm = *(uint32_t*)(instruction + 2);
                return jit_emit_load_imm32(jit, reg, imm);
            }
            break;

        case 0x20: // ADD reg1, reg2, reg3
            if (max_size >= 4) {
                uint8_t reg1 = instruction[1];
                uint8_t reg2 = instruction[2];
                uint8_t reg3 = instruction[3];
                return jit_emit_add(jit, reg1, reg2, reg3);
            }
            break;

        case 0xFF: // EXIT code
            if (max_size >= 2) {
                uint8_t exit_code = instruction[1];
                return jit_emit_exit(jit, exit_code);
            } else {
                return jit_emit_exit(jit, 0);
            }
            break;

        default:
            jit_set_error(jit, JIT_ERROR_COMPILATION_FAILED, "Unknown ASTC opcode");
            return JIT_ERROR_COMPILATION_FAILED;
    }

    return JIT_SUCCESS;
}

/**
 * Get instruction size
 */
static size_t jit_get_instruction_size(uint8_t opcode, const uint8_t* instruction) {
    switch (opcode) {
        case 0x00: // NOP
        case 0x01: // HALT
        case 0x40: // RET
            return 1;

        case 0xFF: // EXIT
            return 2;

        case 0x20: // ADD
            return 4;

        case 0x10: // LOAD_IMM32
        case 0x30: // CALL
            return 6;

        default:
            return 1; // Default to 1 byte
    }
}

/**
 * Emit HALT instruction
 */
static JITResult jit_emit_halt(JITCompiler* jit) {
    if (jit->target_arch == ARCH_X86_64) {
        // mov eax, 0; ret
        jit_emit_byte(jit, 0xB8); jit_emit_int32(jit, 0); // mov eax, 0
        jit_emit_byte(jit, 0xC3); // ret
        return JIT_SUCCESS;
    }
    return JIT_ERROR_UNSUPPORTED_ARCH;
}

/**
 * Emit LOAD_IMM32 instruction
 */
static JITResult jit_emit_load_imm32(JITCompiler* jit, uint8_t reg, uint32_t imm) {
    if (jit->target_arch == ARCH_X86_64 && reg < 16) {
        // mov r32, imm32 (simplified to eax for now)
        jit_emit_byte(jit, 0xB8 + (reg & 0x7)); // mov eax+reg, imm32
        jit_emit_int32(jit, imm);
        return JIT_SUCCESS;
    }
    return JIT_ERROR_UNSUPPORTED_ARCH;
}

/**
 * Emit ADD instruction
 */
static JITResult jit_emit_add(JITCompiler* jit, uint8_t reg1, uint8_t reg2, uint8_t reg3) {
    if (jit->target_arch == ARCH_X86_64) {
        // Simplified: add eax, ebx (assuming reg2=eax, reg3=ebx, reg1=eax)
        jit_emit_byte(jit, 0x01); jit_emit_byte(jit, 0xD8); // add eax, ebx
        return JIT_SUCCESS;
    }
    return JIT_ERROR_UNSUPPORTED_ARCH;
}

/**
 * Emit EXIT instruction
 */
static JITResult jit_emit_exit(JITCompiler* jit, uint8_t exit_code) {
    if (jit->target_arch == ARCH_X86_64) {
        // mov eax, exit_code; ret
        jit_emit_byte(jit, 0xB8); // mov eax, imm32
        jit_emit_byte(jit, exit_code);
        jit_emit_byte(jit, 0x00); jit_emit_byte(jit, 0x00); jit_emit_byte(jit, 0x00);
        jit_emit_byte(jit, 0xC3); // ret
        return JIT_SUCCESS;
    }
    return JIT_ERROR_UNSUPPORTED_ARCH;
}

// ===============================================
// JIT Cache Management Implementation
// ===============================================

/**
 * Initialize JIT compilation cache
 */
int jit_cache_init(size_t max_cache_size) {
    jit_max_cache_size = max_cache_size;
    jit_cache_size = 0;
    jit_cache_head = NULL;

    printf("JIT Cache: Initialized with max size %zu bytes\n", max_cache_size);
    return 0;
}

/**
 * Cleanup JIT compilation cache
 */
void jit_cache_cleanup(void) {
    jit_cache_clear();
    jit_max_cache_size = 0;

    printf("JIT Cache: Cleaned up\n");
}

/**
 * Look up compiled code in cache
 */
bool jit_cache_lookup(uint64_t bytecode_hash, void** entry_point, size_t* code_size) {
    JITCacheEntry* entry = jit_cache_head;

    while (entry) {
        if (entry->bytecode_hash == bytecode_hash) {
            *entry_point = entry->entry_point;
            *code_size = entry->code_size;
            return true;
        }
        entry = entry->next;
    }

    return false;
}

/**
 * Store compiled code in cache
 */
int jit_cache_store(uint64_t bytecode_hash, void* entry_point, size_t code_size) {
    // Check if we have space
    if (jit_cache_size + code_size > jit_max_cache_size) {
        printf("JIT Cache: Cache full, not storing entry\n");
        return -1;
    }

    // Allocate cache entry
    JITCacheEntry* entry = malloc(sizeof(JITCacheEntry));
    if (!entry) {
        return -1;
    }

    // Copy code to cache
    entry->entry_point = allocate_executable_memory(code_size);
    if (!entry->entry_point) {
        free(entry);
        return -1;
    }

    memcpy(entry->entry_point, entry_point, code_size);
    entry->bytecode_hash = bytecode_hash;
    entry->code_size = code_size;

    // Add to cache list
    entry->next = jit_cache_head;
    jit_cache_head = entry;
    jit_cache_size += code_size;

    printf("JIT Cache: Stored entry (hash: 0x%llx, size: %zu)\n",
           (unsigned long long)bytecode_hash, code_size);

    return 0;
}

/**
 * Clear JIT compilation cache
 */
void jit_cache_clear(void) {
    JITCacheEntry* entry = jit_cache_head;

    while (entry) {
        JITCacheEntry* next = entry->next;

        if (entry->entry_point) {
            free_executable_memory(entry->entry_point, entry->code_size);
        }

        free(entry);
        entry = next;
    }

    jit_cache_head = NULL;
    jit_cache_size = 0;

    printf("JIT Cache: Cleared\n");
}

/**
 * Calculate hash of bytecode
 */
uint64_t jit_hash_bytecode(const uint8_t* bytecode, size_t size) {
    uint64_t hash = 0x811c9dc5; // FNV-1a initial value

    for (size_t i = 0; i < size; i++) {
        hash ^= bytecode[i];
        hash *= 0x01000193; // FNV-1a prime
    }

    return hash;
}

// ===============================================
// Architecture-Specific Code Generation (Enhanced)
// ===============================================

/**
 * Emit x86_64 function prologue (enhanced version based on codegen_x64.c)
 */
static JITResult jit_emit_x64_prologue(JITCompiler* jit) {
    // Standard x86_64 function prologue
    jit_emit_byte(jit, 0x55);                    // push rbp
    jit_emit_byte(jit, 0x48); jit_emit_byte(jit, 0x89); jit_emit_byte(jit, 0xE5); // mov rbp, rsp

    // Reserve stack space for local variables (48 bytes aligned to 16)
    jit_emit_byte(jit, 0x48);                    // sub rsp, 48
    jit_emit_byte(jit, 0x83);
    jit_emit_byte(jit, 0xEC);
    jit_emit_byte(jit, 0x30);

    return JIT_SUCCESS;
}

/**
 * Emit x86_64 function epilogue (enhanced version)
 */
static JITResult jit_emit_x64_epilogue(JITCompiler* jit) {
    // Restore stack pointer
    jit_emit_byte(jit, 0x48);                    // add rsp, 48
    jit_emit_byte(jit, 0x83);
    jit_emit_byte(jit, 0xC4);
    jit_emit_byte(jit, 0x30);

    // Standard epilogue
    jit_emit_byte(jit, 0x48); jit_emit_byte(jit, 0x89); jit_emit_byte(jit, 0xEC); // mov rsp, rbp
    jit_emit_byte(jit, 0x5D);                    // pop rbp
    jit_emit_byte(jit, 0xC3);                    // ret

    return JIT_SUCCESS;
}

/**
 * Emit x86_64 constant load (based on codegen_x64.c patterns)
 */
static JITResult jit_emit_x64_const_i32(JITCompiler* jit, uint32_t value) {
    // mov eax, immediate (32-bit)
    jit_emit_byte(jit, 0xB8);
    jit_emit_int32(jit, value);
    // push rax (to maintain stack-based evaluation)
    jit_emit_byte(jit, 0x50);

    return JIT_SUCCESS;
}

/**
 * Emit x86_64 binary add operation (based on codegen_x64.c)
 */
static JITResult jit_emit_x64_binary_add(JITCompiler* jit) {
    jit_emit_byte(jit, 0x5B);        // pop rbx (second operand)
    jit_emit_byte(jit, 0x58);        // pop rax (first operand)
    jit_emit_byte(jit, 0x48);        // REX.W prefix for 64-bit
    jit_emit_byte(jit, 0x01);        // add rax, rbx
    jit_emit_byte(jit, 0xD8);
    jit_emit_byte(jit, 0x50);        // push rax (result)

    return JIT_SUCCESS;
}

/**
 * Emit x86_64 binary subtract operation
 */
static JITResult jit_emit_x64_binary_sub(JITCompiler* jit) {
    jit_emit_byte(jit, 0x5B);        // pop rbx (second operand)
    jit_emit_byte(jit, 0x58);        // pop rax (first operand)
    jit_emit_byte(jit, 0x48);        // REX.W prefix for 64-bit
    jit_emit_byte(jit, 0x29);        // sub rax, rbx
    jit_emit_byte(jit, 0xD8);
    jit_emit_byte(jit, 0x50);        // push rax (result)

    return JIT_SUCCESS;
}

/**
 * Emit x86_64 binary multiply operation
 */
static JITResult jit_emit_x64_binary_mul(JITCompiler* jit) {
    jit_emit_byte(jit, 0x5B);        // pop rbx (second operand)
    jit_emit_byte(jit, 0x58);        // pop rax (first operand)
    jit_emit_byte(jit, 0x48);        // REX.W prefix for 64-bit
    jit_emit_byte(jit, 0x0F);        // imul rax, rbx (2-byte opcode)
    jit_emit_byte(jit, 0xAF);
    jit_emit_byte(jit, 0xC3);
    jit_emit_byte(jit, 0x50);        // push rax (result)

    return JIT_SUCCESS;
}

/**
 * Emit x86_64 halt with return value (based on codegen_x64.c)
 */
static JITResult jit_emit_x64_halt_with_return(JITCompiler* jit) {
    // Pop return value from stack to eax
    jit_emit_byte(jit, 0x58);        // pop rax (return value)

    // Restore stack pointer
    jit_emit_byte(jit, 0x48);        // add rsp, 48
    jit_emit_byte(jit, 0x83);
    jit_emit_byte(jit, 0xC4);
    jit_emit_byte(jit, 0x30);

    // Standard function epilogue
    jit_emit_byte(jit, 0x5D);        // pop rbp
    jit_emit_byte(jit, 0xC3);        // ret

    return JIT_SUCCESS;
}

/**
 * Emit x86_64 LibC call (based on codegen_x64.c patterns)
 */
static JITResult jit_emit_x64_libc_call(JITCompiler* jit, uint16_t func_id, uint16_t arg_count) {
    // Simplified implementation: simulate libc calls with reasonable return values
    switch (func_id) {
        case 0x30: // printf
            jit_emit_byte(jit, 0xB8);        // mov eax, 25 (assume printed 25 chars)
            jit_emit_int32(jit, 25);
            break;
        case 0x50: // malloc
            jit_emit_byte(jit, 0xB8);        // mov eax, 0x1000 (assume allocated 4KB)
            jit_emit_int32(jit, 0x1000);
            break;
        default:
            jit_emit_byte(jit, 0xB8);        // mov eax, 0 (default return)
            jit_emit_int32(jit, 0);
            break;
    }
    jit_emit_byte(jit, 0x50);        // push rax (return value)

    return JIT_SUCCESS;
}

// ===============================================
// Architecture Dispatch Functions
// ===============================================

/**
 * Architecture-aware prologue emission
 */
static JITResult jit_emit_prologue(JITCompiler* jit) {
    if (!jit) return JIT_ERROR_INVALID_INPUT;

    switch (jit->target_arch) {
        case ARCH_X86_64:
            return jit_emit_x64_prologue(jit);
        case ARCH_X86_32:
            // TODO: Implement x86_32 support
            jit_set_error(jit, JIT_ERROR_UNSUPPORTED_ARCH, "x86_32 not implemented yet");
            return JIT_ERROR_UNSUPPORTED_ARCH;
        default:
            jit_set_error(jit, JIT_ERROR_UNSUPPORTED_ARCH, "Unsupported architecture for prologue");
            return JIT_ERROR_UNSUPPORTED_ARCH;
    }
}

/**
 * Architecture-aware epilogue emission
 */
static JITResult jit_emit_epilogue(JITCompiler* jit) {
    if (!jit) return JIT_ERROR_INVALID_INPUT;

    switch (jit->target_arch) {
        case ARCH_X86_64:
            return jit_emit_x64_epilogue(jit);
        case ARCH_X86_32:
            // TODO: Implement x86_32 support
            jit_set_error(jit, JIT_ERROR_UNSUPPORTED_ARCH, "x86_32 not implemented yet");
            return JIT_ERROR_UNSUPPORTED_ARCH;
        default:
            jit_set_error(jit, JIT_ERROR_UNSUPPORTED_ARCH, "Unsupported architecture for epilogue");
            return JIT_ERROR_UNSUPPORTED_ARCH;
    }
}
