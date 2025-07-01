/**
 * native_format.h - Custom .native Module Format Definition
 * 
 * Defines the V1 format for .native modules as specified in PRD.md
 * This is the core format for our custom architecture.
 */

#ifndef NATIVE_FORMAT_H
#define NATIVE_FORMAT_H

#include <stdint.h>
#include <stddef.h>

// Magic number for .native files: "NATV"
#define NATIVE_MAGIC 0x5654414E

// Current format version
#define NATIVE_VERSION_V1 1

// Maximum number of exports per module
#define NATIVE_MAX_EXPORTS 1024

// Maximum length of export names
#define NATIVE_MAX_NAME_LENGTH 256

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
    NATIVE_EXPORT_CONSTANT = 3
} NativeExportType;

// .native file header (64 bytes, aligned)
typedef struct {
    uint32_t magic;              // Magic number: "NATV"
    uint32_t version;            // Format version
    uint32_t architecture;       // Target architecture
    uint32_t module_type;        // Module type
    
    uint64_t code_offset;        // Offset to machine code
    uint64_t code_size;          // Size of machine code
    uint64_t data_offset;        // Offset to data section
    uint64_t data_size;          // Size of data section
    
    uint64_t export_table_offset; // Offset to export table
    uint32_t export_count;       // Number of exports
    uint32_t entry_point_offset; // Entry point offset in code
    
    uint64_t checksum;           // CRC64 checksum
    uint32_t reserved[2];        // Reserved for future use
} NativeHeader;

// Export table entry
typedef struct {
    char name[NATIVE_MAX_NAME_LENGTH]; // Export name (null-terminated)
    uint32_t type;                     // Export type
    uint32_t flags;                    // Export flags
    uint64_t offset;                   // Offset in code/data section
    uint64_t size;                     // Size of exported item
} NativeExport;

// Export table
typedef struct {
    uint32_t count;                    // Number of exports
    uint32_t reserved;                 // Alignment padding
    NativeExport exports[];            // Variable-length array
} NativeExportTable;

// Complete .native module structure
typedef struct {
    NativeHeader header;
    uint8_t* code_section;
    uint8_t* data_section;
    NativeExportTable* export_table;
} NativeModule;

// Function prototypes for .native format handling

/**
 * Create a new native module
 */
NativeModule* native_module_create(NativeArchitecture arch, NativeModuleType type);

/**
 * Free a native module
 */
void native_module_free(NativeModule* module);

/**
 * Add machine code to module
 */
int native_module_set_code(NativeModule* module, const uint8_t* code, size_t size, uint32_t entry_point);

/**
 * Add data section to module
 */
int native_module_set_data(NativeModule* module, const uint8_t* data, size_t size);

/**
 * Add an export to the module
 */
int native_module_add_export(NativeModule* module, const char* name, 
                             NativeExportType type, uint64_t offset, uint64_t size);

/**
 * Write module to file
 */
int native_module_write_file(const NativeModule* module, const char* filename);

/**
 * Load module from file
 */
NativeModule* native_module_load_file(const char* filename);

/**
 * Validate module format
 */
int native_module_validate(const NativeModule* module);

/**
 * Calculate checksum for module
 */
uint64_t native_module_calculate_checksum(const NativeModule* module);

/**
 * Find export by name
 */
const NativeExport* native_module_find_export(const NativeModule* module, const char* name);

/**
 * Get export address (for runtime linking)
 */
void* native_module_get_export_address(const NativeModule* module, const char* name);

// Utility macros
#define NATIVE_ALIGN(size, alignment) (((size) + (alignment) - 1) & ~((alignment) - 1))
#define NATIVE_IS_ALIGNED(ptr, alignment) (((uintptr_t)(ptr) & ((alignment) - 1)) == 0)

// Error codes
#define NATIVE_SUCCESS           0
#define NATIVE_ERROR_INVALID     -1
#define NATIVE_ERROR_NO_MEMORY   -2
#define NATIVE_ERROR_IO          -3
#define NATIVE_ERROR_CHECKSUM    -4
#define NATIVE_ERROR_NOT_FOUND   -5
#define NATIVE_ERROR_TOO_MANY    -6

#endif // NATIVE_FORMAT_H
