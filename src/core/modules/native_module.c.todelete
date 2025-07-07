/**
 * native_module.c - Native Module System
 * 
 * Provides native module functionality as a module.
 * Depends on the memory module.
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

// Module name
#define MODULE_NAME "native"

// Dependency on memory module
MODULE_DEPENDS_ON("memory");

// Function type definitions for memory module functions
typedef void* (*memory_alloc_t)(size_t size, int pool);
typedef void (*memory_free_t)(void* ptr);
typedef void* (*memory_calloc_t)(size_t count, size_t size, int pool);

// Cached memory functions
static memory_alloc_t mem_alloc;
static memory_free_t mem_free;
static memory_calloc_t mem_calloc;

// ===============================================
// Memory Pool Types (from memory.h)
// ===============================================

typedef enum {
    MEMORY_POOL_GENERAL,    // General purpose allocations
    MEMORY_POOL_BYTECODE,   // ASTC bytecode storage
    MEMORY_POOL_JIT,        // JIT compiled code
    MEMORY_POOL_MODULES,    // Native modules
    MEMORY_POOL_TEMP,       // Temporary allocations
    MEMORY_POOL_C99_AST,    // C99 AST nodes
    MEMORY_POOL_C99_SYMBOLS,// C99 symbol table
    MEMORY_POOL_C99_STRINGS,// C99 string literals
    MEMORY_POOL_COUNT
} MemoryPoolType;

// ===============================================
// Native Module Types
// ===============================================

// Magic number for .native files: "NATV"
#define NATIVE_MAGIC 0x5654414E

// Current format version
#define NATIVE_VERSION_V1 1

// Maximum number of exports per module
#define NATIVE_MAX_EXPORTS 1024

// Maximum length of export names
#define NATIVE_MAX_NAME_LENGTH 256

// Maximum length of module metadata strings
#define NATIVE_MAX_MODULE_NAME 128
#define NATIVE_MAX_VERSION_STRING 32
#define NATIVE_MAX_AUTHOR_NAME 64
#define NATIVE_MAX_DESCRIPTION 256

// Architecture types
typedef enum {
    NATIVE_ARCH_X86_64 = 1,
    NATIVE_ARCH_ARM64 = 2,
    NATIVE_ARCH_X86_32 = 3
} NativeArchitecture;

// Module types
typedef enum {
    NATIVE_TYPE_VM = 1,        // VM core module
    NATIVE_TYPE_LIBC = 2,      // libc forwarding module
    NATIVE_TYPE_USER = 3       // User-defined module
} NativeModuleType;

// Export types
typedef enum {
    NATIVE_EXPORT_FUNCTION = 1,
    NATIVE_EXPORT_VARIABLE = 2,
    NATIVE_EXPORT_CONSTANT = 3,
    NATIVE_EXPORT_TYPE = 4,
    NATIVE_EXPORT_INTERFACE = 5
} NativeExportType;

// Module flags
typedef enum {
    NATIVE_FLAG_NONE = 0,
    NATIVE_FLAG_RELOCATABLE = 1,
    NATIVE_FLAG_POSITION_INDEPENDENT = 2,
    NATIVE_FLAG_DEBUG_INFO = 4,
    NATIVE_FLAG_OPTIMIZED = 8,
    NATIVE_FLAG_SIGNED = 16
} NativeModuleFlags;

// Enhanced module metadata (embedded in .native file)
typedef struct {
    char module_name[NATIVE_MAX_MODULE_NAME];
    char version_string[NATIVE_MAX_VERSION_STRING];
    char author[NATIVE_MAX_AUTHOR_NAME];
    char description[NATIVE_MAX_DESCRIPTION];
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    uint32_t build_timestamp;
    uint32_t flags;              // NativeModuleFlags
    uint32_t dependencies_count; // Number of dependencies
    uint64_t dependencies_offset; // Offset to dependencies table

    // Enhanced metadata fields
    char license[64];            // License information
    char homepage[128];          // Project homepage URL
    char repository[128];        // Source repository URL
    uint64_t file_size;          // Original file size
    uint64_t checksum_crc32;     // CRC32 checksum
    uint64_t checksum_sha256[4]; // SHA256 hash (256 bits = 4 * 64 bits)
    uint32_t api_version;        // API compatibility version
    uint32_t abi_version;        // ABI compatibility version
    uint32_t min_loader_version; // Minimum loader version required
    uint32_t security_level;     // Security clearance level (0-3)
    uint64_t signature_offset;   // Offset to digital signature
    uint32_t signature_size;     // Size of digital signature
    uint32_t reserved[8];        // Reserved for future use
} NativeMetadata;

// .native file header (128 bytes, aligned)
typedef struct {
    uint32_t magic;              // Magic number: "NATV" (0x5654414E)
    uint32_t version;            // Format version (1)
    uint32_t architecture;       // NativeArchitecture
    uint32_t module_type;        // NativeModuleType
    uint64_t code_size;          // Size of code section
    uint64_t data_size;          // Size of data section
    uint64_t code_offset;        // Offset to code section
    uint64_t data_offset;        // Offset to data section
    uint64_t export_table_offset; // Offset to export table
    uint32_t export_count;       // Number of exports
    uint32_t entry_point_offset;  // Offset to entry point in code section
    uint64_t metadata_offset;    // Offset to metadata
    uint64_t checksum;           // CRC64 checksum of code and data
    uint32_t flags;              // Module flags
    uint32_t relocation_count;   // Number of relocations
    uint64_t relocation_offset;  // Offset to relocation table
    uint8_t reserved[32];        // Reserved for future use
} NativeHeader;

// Export entry
typedef struct {
    char name[NATIVE_MAX_NAME_LENGTH];
    uint32_t type;               // NativeExportType
    uint32_t flags;              // Export flags
    uint64_t offset;             // Offset in code/data section
    uint64_t size;               // Size in bytes
} NativeExport;

// Export table
typedef struct {
    uint32_t count;              // Number of exports
    NativeExport exports[1];     // Flexible array member
} NativeExportTable;

// Dependency entry
typedef struct {
    char module_name[NATIVE_MAX_MODULE_NAME];
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    uint32_t flags;              // Dependency flags
} NativeDependency;

// Relocation types
typedef enum {
    NATIVE_RELOC_ABSOLUTE = 1,   // Absolute address
    NATIVE_RELOC_RELATIVE = 2,   // Relative address
    NATIVE_RELOC_SYMBOL = 3,     // Symbol reference
    NATIVE_RELOC_IMPORT = 4      // Import from dependency
} NativeRelocationType;

// Relocation entry
typedef struct {
    uint64_t offset;             // Offset in code/data to patch
    uint32_t type;               // NativeRelocationType
    uint32_t symbol_index;       // Index into symbol table
    int64_t addend;              // Addend for relocation
} NativeRelocation;

// Complete .native module structure
typedef struct {
    NativeHeader header;
    NativeMetadata* metadata;
    uint8_t* code_section;
    uint8_t* data_section;
    NativeExportTable* export_table;
    NativeDependency* dependencies;
    NativeRelocation* relocations;
} NativeModule;

// Error codes
#define NATIVE_SUCCESS           0
#define NATIVE_ERROR_INVALID     -1
#define NATIVE_ERROR_NO_MEMORY   -2
#define NATIVE_ERROR_IO          -3
#define NATIVE_ERROR_CHECKSUM    -4
#define NATIVE_ERROR_NOT_FOUND   -5
#define NATIVE_ERROR_TOO_MANY    -6
#define NATIVE_ERROR_CHECKSUM_MISMATCH -7
#define NATIVE_ERROR_NOT_SIGNED  -8
#define NATIVE_ERROR_INVALID_SIGNATURE -9
#define NATIVE_ERROR_VERSION_MISMATCH -10
#define NATIVE_ERROR_API_MISMATCH -11

// ===============================================
// CRC64 Checksum Calculation
// ===============================================

// CRC64 table for checksum calculation
static uint64_t crc64_table[256];
static int crc64_table_initialized = 0;

// Initialize CRC64 table
static void init_crc64_table(void) {
    if (crc64_table_initialized) return;
    
    const uint64_t poly = 0xC96C5795D7870F42ULL;
    
    for (int i = 0; i < 256; i++) {
        uint64_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ poly;
            } else {
                crc >>= 1;
            }
        }
        crc64_table[i] = crc;
    }
    
    crc64_table_initialized = 1;
}

// Calculate CRC64 checksum
static uint64_t calculate_crc64(const uint8_t* data, size_t length) {
    init_crc64_table();
    
    uint64_t crc = 0xFFFFFFFFFFFFFFFFULL;
    
    for (size_t i = 0; i < length; i++) {
        crc = crc64_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFFFFFFFFFFULL;
}

// ===============================================
// Native Module Implementation
// ===============================================

/**
 * Create a new native module
 */
static NativeModule* native_module_create(NativeArchitecture arch, NativeModuleType type) {
    NativeModule* module = mem_calloc(1, sizeof(NativeModule), MEMORY_POOL_MODULES);
    if (!module) {
        return NULL;
    }
    
    // Initialize header
    module->header.magic = NATIVE_MAGIC;
    module->header.version = NATIVE_VERSION_V1;
    module->header.architecture = arch;
    module->header.module_type = type;
    
    // Initialize export table
    module->export_table = mem_calloc(1, sizeof(NativeExportTable), MEMORY_POOL_MODULES);
    if (!module->export_table) {
        mem_free(module);
        return NULL;
    }
    
    return module;
}

/**
 * Free a native module
 */
static void native_module_free(NativeModule* module) {
    if (!module) return;
    
    mem_free(module->code_section);
    mem_free(module->data_section);
    mem_free(module->export_table);
    mem_free(module);
}

/**
 * Add machine code to module
 */
static int native_module_set_code(NativeModule* module, const uint8_t* code, size_t size, uint32_t entry_point) {
    if (!module || !code || size == 0) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Free existing code
    mem_free(module->code_section);
    
    // Allocate and copy new code
    module->code_section = mem_alloc(size, MEMORY_POOL_MODULES);
    if (!module->code_section) {
        return NATIVE_ERROR_NO_MEMORY;
    }
    
    memcpy(module->code_section, code, size);
    module->header.code_size = size;
    module->header.entry_point_offset = entry_point;
    
    return NATIVE_SUCCESS;
}

/**
 * Add data section to module
 */
static int native_module_set_data(NativeModule* module, const uint8_t* data, size_t size) {
    if (!module || !data || size == 0) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Free existing data
    mem_free(module->data_section);
    
    // Allocate and copy new data
    module->data_section = mem_alloc(size, MEMORY_POOL_MODULES);
    if (!module->data_section) {
        return NATIVE_ERROR_NO_MEMORY;
    }
    
    memcpy(module->data_section, data, size);
    module->header.data_size = size;
    
    return NATIVE_SUCCESS;
}

/**
 * Add an export to the module
 */
static int native_module_add_export(NativeModule* module, const char* name, 
                             NativeExportType type, uint64_t offset, uint64_t size) {
    if (!module || !name || !module->export_table) {
        return NATIVE_ERROR_INVALID;
    }
    
    if (module->export_table->count >= NATIVE_MAX_EXPORTS) {
        return NATIVE_ERROR_TOO_MANY;
    }
    
    if (strlen(name) >= NATIVE_MAX_NAME_LENGTH) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Reallocate export table
    size_t new_size = sizeof(NativeExportTable) + 
                      (module->export_table->count + 1) * sizeof(NativeExport);
    
    NativeExportTable* new_table = mem_alloc(new_size, MEMORY_POOL_MODULES);
    if (!new_table) {
        return NATIVE_ERROR_NO_MEMORY;
    }
    
    // Copy existing data
    memcpy(new_table, module->export_table, 
           sizeof(NativeExportTable) + module->export_table->count * sizeof(NativeExport));
    
    // Free old table
    mem_free(module->export_table);
    module->export_table = new_table;
    
    // Add new export
    NativeExport* export = &module->export_table->exports[module->export_table->count];
    strncpy(export->name, name, NATIVE_MAX_NAME_LENGTH - 1);
    export->name[NATIVE_MAX_NAME_LENGTH - 1] = '\0';
    export->type = type;
    export->flags = 0;
    export->offset = offset;
    export->size = size;
    
    module->export_table->count++;
    module->header.export_count = module->export_table->count;
    
    return NATIVE_SUCCESS;
}

/**
 * Calculate checksum for module
 */
static uint64_t native_module_calculate_checksum(const NativeModule* module) {
    if (!module) return 0;
    
    uint64_t checksum = 0;
    
    // Checksum code section
    if (module->code_section && module->header.code_size > 0) {
        checksum ^= calculate_crc64(module->code_section, module->header.code_size);
    }
    
    // Checksum data section
    if (module->data_section && module->header.data_size > 0) {
        checksum ^= calculate_crc64(module->data_section, module->header.data_size);
    }
    
    // Checksum export table
    if (module->export_table && module->export_table->count > 0) {
        size_t table_size = module->export_table->count * sizeof(NativeExport);
        checksum ^= calculate_crc64((uint8_t*)module->export_table->exports, table_size);
    }
    
    return checksum;
}

/**
 * Find export in module
 */
static const NativeExport* native_module_find_export(const NativeModule* module, const char* name) {
    if (!module || !name || !module->export_table) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < module->export_table->count; i++) {
        if (strcmp(module->export_table->exports[i].name, name) == 0) {
            return &module->export_table->exports[i];
        }
    }
    
    return NULL;
}

/**
 * Validate module format
 */
static int native_module_validate(const NativeModule* module) {
    if (!module) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Check magic number
    if (module->header.magic != NATIVE_MAGIC) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Check version
    if (module->header.version != NATIVE_VERSION_V1) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Check architecture
    if (module->header.architecture < NATIVE_ARCH_X86_64 || 
        module->header.architecture > NATIVE_ARCH_X86_32) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Check module type
    if (module->header.module_type < NATIVE_TYPE_VM || 
        module->header.module_type > NATIVE_TYPE_USER) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Validate checksum
    uint64_t calculated_checksum = native_module_calculate_checksum(module);
    if (calculated_checksum != module->header.checksum) {
        return NATIVE_ERROR_CHECKSUM;
    }
    
    return NATIVE_SUCCESS;
}

/**
 * Write module to file
 */
static int native_module_write_file(const NativeModule* module, const char* filename) {
    if (!module || !filename) {
        return NATIVE_ERROR_INVALID;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        return NATIVE_ERROR_IO;
    }

    // Calculate offsets
    NativeHeader header = module->header;
    size_t current_offset = sizeof(NativeHeader);

    // Code section
    header.code_offset = current_offset;
    current_offset += header.code_size;

    // Data section
    header.data_offset = current_offset;
    current_offset += header.data_size;

    // Export table
    header.export_table_offset = current_offset;

    // Calculate and set checksum
    header.checksum = native_module_calculate_checksum(module);

    // Write header
    if (fwrite(&header, sizeof(NativeHeader), 1, file) != 1) {
        fclose(file);
        return NATIVE_ERROR_IO;
    }

    // Write code section
    if (header.code_size > 0 && module->code_section) {
        if (fwrite(module->code_section, header.code_size, 1, file) != 1) {
            fclose(file);
            return NATIVE_ERROR_IO;
        }
    }

    // Write data section
    if (header.data_size > 0 && module->data_section) {
        if (fwrite(module->data_section, header.data_size, 1, file) != 1) {
            fclose(file);
            return NATIVE_ERROR_IO;
        }
    }

    // Write export table
    if (module->export_table && module->export_table->count > 0) {
        size_t table_size = sizeof(uint32_t) + module->export_table->count * sizeof(NativeExport);
        if (fwrite(module->export_table, table_size, 1, file) != 1) {
            fclose(file);
            return NATIVE_ERROR_IO;
        }
    }

    fclose(file);
    return NATIVE_SUCCESS;
}

/**
 * Load module from file
 */
static NativeModule* native_module_load_file(const char* filename) {
    if (!filename) {
        return NULL;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    // Read header
    NativeHeader header;
    if (fread(&header, sizeof(NativeHeader), 1, file) != 1) {
        fclose(file);
        return NULL;
    }

    // Validate header
    if (header.magic != NATIVE_MAGIC || header.version != NATIVE_VERSION_V1) {
        fclose(file);
        return NULL;
    }

    // Create module
    NativeModule* module = mem_calloc(1, sizeof(NativeModule), MEMORY_POOL_MODULES);
    if (!module) {
        fclose(file);
        return NULL;
    }

    // Copy header
    module->header = header;

    // Read code section
    if (header.code_size > 0) {
        module->code_section = mem_alloc(header.code_size, MEMORY_POOL_MODULES);
        if (!module->code_section) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }

        fseek(file, header.code_offset, SEEK_SET);
        if (fread(module->code_section, header.code_size, 1, file) != 1) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }
    }

    // Read data section
    if (header.data_size > 0) {
        module->data_section = mem_alloc(header.data_size, MEMORY_POOL_MODULES);
        if (!module->data_section) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }

        fseek(file, header.data_offset, SEEK_SET);
        if (fread(module->data_section, header.data_size, 1, file) != 1) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }
    }

    // Read export table
    if (header.export_count > 0) {
        size_t table_size = sizeof(NativeExportTable) + 
                          (header.export_count - 1) * sizeof(NativeExport);
        
        module->export_table = mem_alloc(table_size, MEMORY_POOL_MODULES);
        if (!module->export_table) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }

        fseek(file, header.export_table_offset, SEEK_SET);
        if (fread(&module->export_table->count, sizeof(uint32_t), 1, file) != 1) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }

        if (fread(module->export_table->exports, sizeof(NativeExport), 
                 module->export_table->count, file) != module->export_table->count) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }
    }

    fclose(file);

    // Validate checksum
    uint64_t calculated_checksum = native_module_calculate_checksum(module);
    if (calculated_checksum != header.checksum) {
        native_module_free(module);
        return NULL;
    }

    return module;
}

/**
 * Get symbol from native module
 */
static void* native_module_get_symbol(const NativeModule* module, const char* symbol_name) {
    if (!module || !symbol_name) {
        return NULL;
    }

    const NativeExport* export = native_module_find_export(module, symbol_name);
    if (!export) {
        return NULL;
    }

    // Symbol is in code section
    if (export->type == NATIVE_EXPORT_FUNCTION) {
        if (!module->code_section || export->offset >= module->header.code_size) {
            return NULL;
        }
        return (void*)(module->code_section + export->offset);
    }
    
    // Symbol is in data section
    if (export->type == NATIVE_EXPORT_VARIABLE || 
        export->type == NATIVE_EXPORT_CONSTANT) {
        if (!module->data_section || export->offset >= module->header.data_size) {
            return NULL;
        }
        return (void*)(module->data_section + export->offset);
    }

    return NULL;
}

// ===============================================
// Version Comparison Functions
// ===============================================

/**
 * Compare two version numbers
 * 
 * @return -1 if version1 < version2, 0 if equal, 1 if version1 > version2
 */
static int native_version_compare(uint32_t major1, uint32_t minor1, uint32_t patch1,
                          uint32_t major2, uint32_t minor2, uint32_t patch2) {
    if (major1 < major2) return -1;
    if (major1 > major2) return 1;
    
    if (minor1 < minor2) return -1;
    if (minor1 > minor2) return 1;
    
    if (patch1 < patch2) return -1;
    if (patch1 > patch2) return 1;
    
    return 0;
}

/**
 * Check if version satisfies requirement
 * 
 * @return 1 if satisfies, 0 if not
 */
static int native_version_satisfies(uint32_t major, uint32_t minor, uint32_t patch,
                           uint32_t req_major, uint32_t req_minor, uint32_t req_patch) {
    // Major version must match exactly
    if (major != req_major) {
        return 0;
    }
    
    // Minor version must be >= required
    if (minor < req_minor) {
        return 0;
    }
    
    // If minor is greater, patch doesn't matter
    if (minor > req_minor) {
        return 1;
    }
    
    // Minor versions match, check patch
    return patch >= req_patch ? 1 : 0;
}

// ===============================================
// Symbol Table
// ===============================================

// Symbol table
static struct {
    const char* name;
    void* symbol;
} native_symbols[] = {
    // Module management
    {"create", native_module_create},
    {"free", native_module_free},
    {"set_code", native_module_set_code},
    {"set_data", native_module_set_data},
    {"add_export", native_module_add_export},
    {"calculate_checksum", native_module_calculate_checksum},
    {"find_export", native_module_find_export},
    {"validate", native_module_validate},
    {"write_file", native_module_write_file},
    {"load_file", native_module_load_file},
    {"get_symbol", native_module_get_symbol},
    
    // Version utilities
    {"version_compare", native_version_compare},
    {"version_satisfies", native_version_satisfies},
    
    {NULL, NULL}  // Sentinel
};

// ===============================================
// Module Interface
// ===============================================

// Module load function
static int native_init(void) {
    // Resolve required memory functions
    Module* memory = module_get("memory");
    if (!memory) {
        return -1;
    }
    
    mem_alloc = module_resolve(memory, "alloc_pool");
    mem_free = module_resolve(memory, "free");
    mem_calloc = module_resolve(memory, "calloc");
    
    if (!mem_alloc || !mem_free || !mem_calloc) {
        return -1;
    }
    
    // Initialize CRC64 table
    init_crc64_table();
    
    return 0;
}

// Module unload function
static void native_cleanup(void) {
    // Nothing to clean up
}

// Symbol resolution function
static void* native_resolve(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    for (int i = 0; native_symbols[i].name; i++) {
        if (strcmp(native_symbols[i].name, symbol) == 0) {
            return native_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// Module definition - updated to match new module.h structure
Module module_native = {
    .name = MODULE_NAME,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .init = native_init,
    .cleanup = native_cleanup,
    .resolve = native_resolve
};

// 注意：不再需要REGISTER_MODULE，使用动态加载机制