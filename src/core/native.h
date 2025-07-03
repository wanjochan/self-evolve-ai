/**
 * native.h - Custom .native Module Format Definition
 *
 * Defines the V1 format for .native modules as specified in PRD.md
 * This is the core format for our custom architecture.
 */

#ifndef NATIVE_H
#define NATIVE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

    uint64_t metadata_offset;    // Offset to metadata section
    uint64_t relocation_offset;  // Offset to relocation table
    uint32_t relocation_count;   // Number of relocations

    uint64_t checksum;           // CRC64 checksum
    uint32_t reserved[4];        // Reserved for future use
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

/**
 * Add dependency to module
 */
int native_module_add_dependency(NativeModule* module, const char* name,
                                uint32_t major, uint32_t minor, uint32_t patch);

/**
 * Enhanced metadata and security functions
 */

/**
 * Set enhanced metadata for module
 */
int native_module_set_metadata_enhanced(NativeModule* module,
                                       const char* license,
                                       const char* homepage,
                                       const char* repository,
                                       uint32_t api_version,
                                       uint32_t abi_version,
                                       uint32_t min_loader_version,
                                       uint32_t security_level);

/**
 * Calculate and set checksums for module
 */
int native_module_calculate_checksums(NativeModule* module);

/**
 * Verify module checksums
 */
int native_module_verify_checksums(const NativeModule* module);

/**
 * Add digital signature to module
 */
int native_module_add_signature(NativeModule* module, const uint8_t* signature, uint32_t signature_size);

/**
 * Verify digital signature of module
 */
int native_module_verify_signature(const NativeModule* module, const uint8_t* public_key, uint32_t key_size);

/**
 * Check version compatibility
 */
int native_module_check_compatibility(const NativeModule* module,
                                     uint32_t loader_version,
                                     uint32_t required_api_version);

/**
 * Get module security level
 */
uint32_t native_module_get_security_level(const NativeModule* module);

/**
 * Version comparison utilities
 */
int native_version_compare(uint32_t major1, uint32_t minor1, uint32_t patch1,
                          uint32_t major2, uint32_t minor2, uint32_t patch2);

/**
 * Check if version satisfies requirement
 */
int native_version_satisfies(uint32_t major, uint32_t minor, uint32_t patch,
                           uint32_t req_major, uint32_t req_minor, uint32_t req_patch);

/**
 * Add relocation to module
 */
int native_module_add_relocation(NativeModule* module, uint64_t offset,
                                NativeRelocationType type, uint32_t symbol_index, int64_t addend);

/**
 * Resolve relocations in loaded module
 */
int native_module_resolve_relocations(NativeModule* module);

/**
 * Check if module satisfies dependency
 */
bool native_module_satisfies_dependency(const NativeModule* module, const NativeDependency* dep);

/**
 * Get module metadata
 */
const NativeMetadata* native_module_get_metadata(const NativeModule* module);

/**
 * Set module metadata
 */
int native_module_set_metadata(NativeModule* module, const NativeMetadata* metadata);

/**
 * Free module memory
 */
void native_module_free(NativeModule* module);

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
#define NATIVE_ERROR_CHECKSUM_MISMATCH -7
#define NATIVE_ERROR_NOT_SIGNED  -8
#define NATIVE_ERROR_INVALID_SIGNATURE -9
#define NATIVE_ERROR_VERSION_MISMATCH -10
#define NATIVE_ERROR_API_MISMATCH -11

#endif // NATIVE_H
