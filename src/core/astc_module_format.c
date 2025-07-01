/**
 * astc_module_format.c - Enhanced ASTC Module Format Implementation
 * 
 * Implements enhanced ASTC bytecode format with comprehensive module support,
 * including imports, exports, dependencies, and metadata.
 */

#include "include/core_astc.h"
#include "include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ASTC Module File Format Constants
#define ASTC_MAGIC_NUMBER 0x43545341  // "ASTC" in little-endian
#define ASTC_VERSION_MAJOR 1
#define ASTC_VERSION_MINOR 0
#define ASTC_VERSION_PATCH 0

// Section types in ASTC module file
typedef enum {
    ASTC_SECTION_MODULE_INFO = 0x01,
    ASTC_SECTION_IMPORTS = 0x02,
    ASTC_SECTION_EXPORTS = 0x03,
    ASTC_SECTION_DEPENDENCIES = 0x04,
    ASTC_SECTION_FUNCTIONS = 0x05,
    ASTC_SECTION_GLOBALS = 0x06,
    ASTC_SECTION_DATA = 0x07,
    ASTC_SECTION_CODE = 0x08,
    ASTC_SECTION_DEBUG_INFO = 0x09,
    ASTC_SECTION_CUSTOM = 0xFF
} ASTCSectionType;

// ASTC Module File Header
typedef struct {
    uint32_t magic;              // Magic number: "ASTC"
    uint8_t version_major;       // Major version
    uint8_t version_minor;       // Minor version
    uint8_t version_patch;       // Patch version
    uint8_t flags;               // Module flags
    uint32_t section_count;      // Number of sections
    uint64_t total_size;         // Total file size
    uint64_t checksum;           // CRC64 checksum
} ASTCModuleHeader;

// Section header
typedef struct {
    uint8_t section_type;        // Section type
    uint8_t flags;               // Section flags
    uint16_t reserved;           // Reserved
    uint64_t section_size;       // Section size in bytes
    uint64_t section_offset;     // Offset from file start
} ASTCSectionHeader;

// Module information section
typedef struct {
    char name[128];              // Module name
    char version[32];            // Module version
    char author[64];             // Module author
    char description[256];       // Module description
    char license[64];            // Module license
    uint32_t build_timestamp;    // Build timestamp
    uint32_t target_arch;        // Target architecture
    uint32_t module_flags;       // Module flags
    uint32_t reserved[4];        // Reserved for future use
} ASTCModuleInfo;

// Import entry
typedef struct {
    char module_name[128];       // Source module name
    char import_name[128];       // Imported symbol name
    char local_name[128];        // Local alias name
    char version_requirement[32]; // Version requirement
    uint8_t import_type;         // Import type (function, variable, etc.)
    uint8_t flags;               // Import flags (weak, lazy, etc.)
    uint16_t reserved;           // Reserved
} ASTCImportEntry;

// Export entry
typedef struct {
    char export_name[128];       // Export name
    char alias[128];             // Export alias
    uint8_t export_type;         // Export type
    uint8_t flags;               // Export flags
    uint16_t reserved;           // Reserved
    uint32_t symbol_index;       // Index into symbol table
    uint64_t offset;             // Offset in code/data section
} ASTCExportEntry;

// Dependency entry
typedef struct {
    char module_name[128];       // Dependency module name
    char version_requirement[32]; // Version requirement
    uint8_t flags;               // Dependency flags (optional, etc.)
    uint8_t reserved[3];         // Reserved
} ASTCDependencyEntry;

// Serialize module information to buffer
static int serialize_module_info(const ASTNode* module, uint8_t** buffer, size_t* size) {
    if (!module || module->type != ASTC_MODULE_DECL) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Invalid module node");
        return -1;
    }

    ASTCModuleInfo info = {0};
    
    // Fill module information
    if (module->data.module_decl.name) {
        strncpy(info.name, module->data.module_decl.name, sizeof(info.name) - 1);
    }
    if (module->data.module_decl.version) {
        strncpy(info.version, module->data.module_decl.version, sizeof(info.version) - 1);
    }
    if (module->data.module_decl.author) {
        strncpy(info.author, module->data.module_decl.author, sizeof(info.author) - 1);
    }
    if (module->data.module_decl.description) {
        strncpy(info.description, module->data.module_decl.description, sizeof(info.description) - 1);
    }
    if (module->data.module_decl.license) {
        strncpy(info.license, module->data.module_decl.license, sizeof(info.license) - 1);
    }
    
    info.build_timestamp = (uint32_t)time(NULL);
    info.target_arch = 0x01; // x64 for now
    info.module_flags = 0;

    *size = sizeof(ASTCModuleInfo);
    *buffer = malloc(*size);
    if (!*buffer) {
        SET_ERROR(ERROR_MEMORY_ALLOCATION, "Failed to allocate buffer for module info");
        return -1;
    }

    memcpy(*buffer, &info, *size);
    return 0;
}

// Serialize imports to buffer
static int serialize_imports(const ASTNode* module, uint8_t** buffer, size_t* size) {
    if (!module || module->type != ASTC_MODULE_DECL) {
        return -1;
    }

    int import_count = module->data.module_decl.import_count;
    *size = sizeof(uint32_t) + import_count * sizeof(ASTCImportEntry);
    *buffer = malloc(*size);
    if (!*buffer) {
        SET_ERROR(ERROR_MEMORY_ALLOCATION, "Failed to allocate buffer for imports");
        return -1;
    }

    uint8_t* ptr = *buffer;
    
    // Write import count
    *(uint32_t*)ptr = import_count;
    ptr += sizeof(uint32_t);

    // Write import entries
    for (int i = 0; i < import_count; i++) {
        ASTNode* import_node = module->data.module_decl.imports[i];
        if (import_node->type != ASTC_IMPORT_DECL) continue;

        ASTCImportEntry entry = {0};
        
        if (import_node->data.import_decl.module_name) {
            strncpy(entry.module_name, import_node->data.import_decl.module_name, sizeof(entry.module_name) - 1);
        }
        if (import_node->data.import_decl.import_name) {
            strncpy(entry.import_name, import_node->data.import_decl.import_name, sizeof(entry.import_name) - 1);
        }
        if (import_node->data.import_decl.local_name) {
            strncpy(entry.local_name, import_node->data.import_decl.local_name, sizeof(entry.local_name) - 1);
        }
        if (import_node->data.import_decl.version_requirement) {
            strncpy(entry.version_requirement, import_node->data.import_decl.version_requirement, sizeof(entry.version_requirement) - 1);
        }
        
        entry.import_type = (uint8_t)import_node->data.import_decl.import_type;
        entry.flags = 0;
        if (import_node->data.import_decl.is_weak) entry.flags |= 0x01;
        if (import_node->data.import_decl.is_lazy) entry.flags |= 0x02;

        memcpy(ptr, &entry, sizeof(ASTCImportEntry));
        ptr += sizeof(ASTCImportEntry);
    }

    return 0;
}

// Serialize exports to buffer
static int serialize_exports(const ASTNode* module, uint8_t** buffer, size_t* size) {
    if (!module || module->type != ASTC_MODULE_DECL) {
        return -1;
    }

    int export_count = module->data.module_decl.export_count;
    *size = sizeof(uint32_t) + export_count * sizeof(ASTCExportEntry);
    *buffer = malloc(*size);
    if (!*buffer) {
        SET_ERROR(ERROR_MEMORY_ALLOCATION, "Failed to allocate buffer for exports");
        return -1;
    }

    uint8_t* ptr = *buffer;
    
    // Write export count
    *(uint32_t*)ptr = export_count;
    ptr += sizeof(uint32_t);

    // Write export entries
    for (int i = 0; i < export_count; i++) {
        ASTNode* export_node = module->data.module_decl.exports[i];
        if (export_node->type != ASTC_EXPORT_DECL) continue;

        ASTCExportEntry entry = {0};
        
        if (export_node->data.export_decl.name) {
            strncpy(entry.export_name, export_node->data.export_decl.name, sizeof(entry.export_name) - 1);
        }
        if (export_node->data.export_decl.alias) {
            strncpy(entry.alias, export_node->data.export_decl.alias, sizeof(entry.alias) - 1);
        }
        
        entry.export_type = (uint8_t)export_node->data.export_decl.export_type;
        entry.flags = 0;
        if (export_node->data.export_decl.is_default) entry.flags |= 0x01;
        entry.symbol_index = i; // Simple indexing for now
        entry.offset = 0; // Will be filled during linking

        memcpy(ptr, &entry, sizeof(ASTCExportEntry));
        ptr += sizeof(ASTCExportEntry);
    }

    return 0;
}

// Enhanced module serialization function
int ast_serialize_module_enhanced(ASTNode* module, uint8_t** buffer, size_t* total_size) {
    if (!module || !buffer || !total_size) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Invalid arguments to serialize_module_enhanced");
        return -1;
    }

    if (module->type != ASTC_MODULE_DECL) {
        SET_ERROR(ERROR_INVALID_ARGUMENT, "Node is not a module declaration");
        return -1;
    }

    LOG_COMPILER_INFO("Serializing enhanced ASTC module: %s", 
                     module->data.module_decl.name ? module->data.module_decl.name : "unnamed");

    // Calculate sections
    uint8_t* module_info_buf = NULL;
    uint8_t* imports_buf = NULL;
    uint8_t* exports_buf = NULL;
    size_t module_info_size = 0;
    size_t imports_size = 0;
    size_t exports_size = 0;

    // Serialize sections
    if (serialize_module_info(module, &module_info_buf, &module_info_size) != 0) {
        LOG_COMPILER_ERROR("Failed to serialize module info");
        return -1;
    }

    if (serialize_imports(module, &imports_buf, &imports_size) != 0) {
        LOG_COMPILER_ERROR("Failed to serialize imports");
        free(module_info_buf);
        return -1;
    }

    if (serialize_exports(module, &exports_buf, &exports_size) != 0) {
        LOG_COMPILER_ERROR("Failed to serialize exports");
        free(module_info_buf);
        free(imports_buf);
        return -1;
    }

    // Calculate total size
    size_t header_size = sizeof(ASTCModuleHeader);
    size_t section_headers_size = 3 * sizeof(ASTCSectionHeader); // 3 sections for now
    *total_size = header_size + section_headers_size + module_info_size + imports_size + exports_size;

    // Allocate final buffer
    *buffer = malloc(*total_size);
    if (!*buffer) {
        SET_ERROR(ERROR_MEMORY_ALLOCATION, "Failed to allocate final buffer");
        free(module_info_buf);
        free(imports_buf);
        free(exports_buf);
        return -1;
    }

    uint8_t* ptr = *buffer;

    // Write module header
    ASTCModuleHeader header = {0};
    header.magic = ASTC_MAGIC_NUMBER;
    header.version_major = ASTC_VERSION_MAJOR;
    header.version_minor = ASTC_VERSION_MINOR;
    header.version_patch = ASTC_VERSION_PATCH;
    header.flags = 0;
    header.section_count = 3;
    header.total_size = *total_size;
    header.checksum = 0; // TODO: Calculate actual checksum

    memcpy(ptr, &header, sizeof(ASTCModuleHeader));
    ptr += sizeof(ASTCModuleHeader);

    // Write section headers
    uint64_t current_offset = header_size + section_headers_size;

    // Module info section header
    ASTCSectionHeader section_header = {0};
    section_header.section_type = ASTC_SECTION_MODULE_INFO;
    section_header.section_size = module_info_size;
    section_header.section_offset = current_offset;
    memcpy(ptr, &section_header, sizeof(ASTCSectionHeader));
    ptr += sizeof(ASTCSectionHeader);
    current_offset += module_info_size;

    // Imports section header
    section_header.section_type = ASTC_SECTION_IMPORTS;
    section_header.section_size = imports_size;
    section_header.section_offset = current_offset;
    memcpy(ptr, &section_header, sizeof(ASTCSectionHeader));
    ptr += sizeof(ASTCSectionHeader);
    current_offset += imports_size;

    // Exports section header
    section_header.section_type = ASTC_SECTION_EXPORTS;
    section_header.section_size = exports_size;
    section_header.section_offset = current_offset;
    memcpy(ptr, &section_header, sizeof(ASTCSectionHeader));
    ptr += sizeof(ASTCSectionHeader);

    // Write section data
    memcpy(ptr, module_info_buf, module_info_size);
    ptr += module_info_size;
    memcpy(ptr, imports_buf, imports_size);
    ptr += imports_size;
    memcpy(ptr, exports_buf, exports_size);

    // Cleanup temporary buffers
    free(module_info_buf);
    free(imports_buf);
    free(exports_buf);

    LOG_COMPILER_INFO("Successfully serialized ASTC module, size: %zu bytes", *total_size);
    return 0;
}
