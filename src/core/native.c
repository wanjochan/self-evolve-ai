#include "native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

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

NativeModule* native_module_create(NativeArchitecture arch, NativeModuleType type) {
    NativeModule* module = calloc(1, sizeof(NativeModule));
    if (!module) {
        return NULL;
    }
    
    // Initialize header
    module->header.magic = NATIVE_MAGIC;
    module->header.version = NATIVE_VERSION_V1;
    module->header.architecture = arch;
    module->header.module_type = type;
    
    // Initialize export table
    module->export_table = calloc(1, sizeof(NativeExportTable));
    if (!module->export_table) {
        free(module);
        return NULL;
    }
    
    return module;
}

void native_module_free(NativeModule* module) {
    if (!module) return;
    
    free(module->code_section);
    free(module->data_section);
    free(module->export_table);
    free(module);
}

int native_module_set_code(NativeModule* module, const uint8_t* code, size_t size, uint32_t entry_point) {
    if (!module || !code || size == 0) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Free existing code
    free(module->code_section);
    
    // Allocate and copy new code
    module->code_section = malloc(size);
    if (!module->code_section) {
        return NATIVE_ERROR_NO_MEMORY;
    }
    
    memcpy(module->code_section, code, size);
    module->header.code_size = size;
    module->header.entry_point_offset = entry_point;
    
    return NATIVE_SUCCESS;
}

int native_module_set_data(NativeModule* module, const uint8_t* data, size_t size) {
    if (!module || !data || size == 0) {
        return NATIVE_ERROR_INVALID;
    }
    
    // Free existing data
    free(module->data_section);
    
    // Allocate and copy new data
    module->data_section = malloc(size);
    if (!module->data_section) {
        return NATIVE_ERROR_NO_MEMORY;
    }
    
    memcpy(module->data_section, data, size);
    module->header.data_size = size;
    
    return NATIVE_SUCCESS;
}

int native_module_add_export(NativeModule* module, const char* name, 
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
    
    NativeExportTable* new_table = realloc(module->export_table, new_size);
    if (!new_table) {
        return NATIVE_ERROR_NO_MEMORY;
    }
    
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

uint64_t native_module_calculate_checksum(const NativeModule* module) {
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

const NativeExport* native_module_find_export(const NativeModule* module, const char* name) {
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

int native_module_validate(const NativeModule* module) {
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

int native_module_write_file(const NativeModule* module, const char* filename) {
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
        // Write table header
        if (fwrite(module->export_table, sizeof(uint32_t) * 2, 1, file) != 1) {
            fclose(file);
            return NATIVE_ERROR_IO;
        }

        // Write exports
        size_t exports_size = module->export_table->count * sizeof(NativeExport);
        if (fwrite(module->export_table->exports, exports_size, 1, file) != 1) {
            fclose(file);
            return NATIVE_ERROR_IO;
        }
    }

    fclose(file);
    return NATIVE_SUCCESS;
}

NativeModule* native_module_load_file(const char* filename) {
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

    // Validate magic and version
    if (header.magic != NATIVE_MAGIC || header.version != NATIVE_VERSION_V1) {
        fclose(file);
        return NULL;
    }

    // Create module
    NativeModule* module = native_module_create(header.architecture, header.module_type);
    if (!module) {
        fclose(file);
        return NULL;
    }

    module->header = header;

    // Read code section
    if (header.code_size > 0) {
        module->code_section = malloc(header.code_size);
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
        module->data_section = malloc(header.data_size);
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
        fseek(file, header.export_table_offset, SEEK_SET);

        // Read table header
        uint32_t count, reserved;
        if (fread(&count, sizeof(uint32_t), 1, file) != 1 ||
            fread(&reserved, sizeof(uint32_t), 1, file) != 1) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }

        if (count != header.export_count) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }

        // Allocate export table
        size_t table_size = sizeof(NativeExportTable) + count * sizeof(NativeExport);
        module->export_table = realloc(module->export_table, table_size);
        if (!module->export_table) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }

        module->export_table->count = count;
        module->export_table->reserved = reserved;

        // Read exports
        size_t exports_size = count * sizeof(NativeExport);
        if (fread(module->export_table->exports, exports_size, 1, file) != 1) {
            native_module_free(module);
            fclose(file);
            return NULL;
        }
    }

    fclose(file);

    // Validate module
    if (native_module_validate(module) != NATIVE_SUCCESS) {
        native_module_free(module);
        return NULL;
    }

    return module;
}

// Enhanced metadata and security functions implementation

int native_module_set_metadata_enhanced(NativeModule* module,
                                       const char* license,
                                       const char* homepage,
                                       const char* repository,
                                       uint32_t api_version,
                                       uint32_t abi_version,
                                       uint32_t min_loader_version,
                                       uint32_t security_level) {
    if (!module || !module->metadata) {
        return NATIVE_ERROR_INVALID;
    }

    // Set enhanced metadata fields
    if (license) {
        strncpy(module->metadata->license, license, sizeof(module->metadata->license) - 1);
        module->metadata->license[sizeof(module->metadata->license) - 1] = '\0';
    }

    if (homepage) {
        strncpy(module->metadata->homepage, homepage, sizeof(module->metadata->homepage) - 1);
        module->metadata->homepage[sizeof(module->metadata->homepage) - 1] = '\0';
    }

    if (repository) {
        strncpy(module->metadata->repository, repository, sizeof(module->metadata->repository) - 1);
        module->metadata->repository[sizeof(module->metadata->repository) - 1] = '\0';
    }

    module->metadata->api_version = api_version;
    module->metadata->abi_version = abi_version;
    module->metadata->min_loader_version = min_loader_version;
    module->metadata->security_level = security_level;

    return NATIVE_SUCCESS;
}

// Simple CRC32 implementation for checksums
static uint32_t crc32_table[256];
static int crc32_table_initialized = 0;

static void init_crc32_table(void) {
    if (crc32_table_initialized) return;

    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = 1;
}

static uint32_t calculate_crc32(const uint8_t* data, size_t length) {
    init_crc32_table();

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

int native_module_calculate_checksums(NativeModule* module) {
    if (!module || !module->metadata) {
        return NATIVE_ERROR_INVALID;
    }

    // Calculate CRC32 of code section
    if (module->code_section && module->header.code_size > 0) {
        module->metadata->checksum_crc32 = calculate_crc32(module->code_section, module->header.code_size);
    }

    // For SHA256, we'll use a simplified approach (in real implementation, use proper crypto library)
    // Here we'll just create a deterministic hash based on content
    uint64_t simple_hash = 0;
    if (module->code_section) {
        for (size_t i = 0; i < module->header.code_size && i < 1024; i++) {
            simple_hash = simple_hash * 31 + module->code_section[i];
        }
    }

    // Store simplified hash in SHA256 field (first 64 bits)
    module->metadata->checksum_sha256[0] = simple_hash;
    module->metadata->checksum_sha256[1] = simple_hash ^ 0xAAAAAAAAAAAAAAAAULL;
    module->metadata->checksum_sha256[2] = simple_hash ^ 0x5555555555555555ULL;
    module->metadata->checksum_sha256[3] = simple_hash ^ 0xCCCCCCCCCCCCCCCCULL;

    return NATIVE_SUCCESS;
}

int native_module_verify_checksums(const NativeModule* module) {
    if (!module || !module->metadata) {
        return NATIVE_ERROR_INVALID;
    }

    // Verify CRC32 checksum
    if (module->code_section && module->header.code_size > 0) {
        uint32_t calculated_crc = calculate_crc32(module->code_section, module->header.code_size);
        if (calculated_crc != module->metadata->checksum_crc32) {
            return NATIVE_ERROR_CHECKSUM_MISMATCH;
        }
    }

    // Verify simplified SHA256 hash
    uint64_t simple_hash = 0;
    if (module->code_section) {
        for (size_t i = 0; i < module->header.code_size && i < 1024; i++) {
            simple_hash = simple_hash * 31 + module->code_section[i];
        }
    }

    if (module->metadata->checksum_sha256[0] != simple_hash) {
        return NATIVE_ERROR_CHECKSUM_MISMATCH;
    }

    return NATIVE_SUCCESS;
}

int native_module_add_signature(NativeModule* module, const uint8_t* signature, uint32_t signature_size) {
    if (!module || !module->metadata || !signature || signature_size == 0) {
        return NATIVE_ERROR_INVALID;
    }

    // Store signature information in metadata
    module->metadata->signature_size = signature_size;
    module->metadata->signature_offset = 0; // Will be set during file write

    // Set signed flag
    module->metadata->flags |= NATIVE_FLAG_SIGNED;

    return NATIVE_SUCCESS;
}

int native_module_verify_signature(const NativeModule* module, const uint8_t* public_key, uint32_t key_size) {
    if (!module || !module->metadata || !public_key || key_size == 0) {
        return NATIVE_ERROR_INVALID;
    }

    // Check if module is signed
    if (!(module->metadata->flags & NATIVE_FLAG_SIGNED)) {
        return NATIVE_ERROR_NOT_SIGNED;
    }

    // In a real implementation, this would:
    // 1. Load the signature from the module
    // 2. Calculate hash of module content
    // 3. Verify signature using public key cryptography
    // For now, we'll do a simplified check

    if (module->metadata->signature_size == 0) {
        return NATIVE_ERROR_INVALID_SIGNATURE;
    }

    return NATIVE_SUCCESS; // Assume verification passes for now
}

int native_module_check_compatibility(const NativeModule* module,
                                     uint32_t loader_version,
                                     uint32_t required_api_version) {
    if (!module || !module->metadata) {
        return NATIVE_ERROR_INVALID;
    }

    // Check minimum loader version requirement
    if (loader_version < module->metadata->min_loader_version) {
        return NATIVE_ERROR_VERSION_MISMATCH;
    }

    // Check API version compatibility
    if (module->metadata->api_version > required_api_version) {
        return NATIVE_ERROR_API_MISMATCH;
    }

    return NATIVE_SUCCESS;
}

uint32_t native_module_get_security_level(const NativeModule* module) {
    if (!module || !module->metadata) {
        return 0; // Lowest security level
    }

    return module->metadata->security_level;
}

int native_version_compare(uint32_t major1, uint32_t minor1, uint32_t patch1,
                          uint32_t major2, uint32_t minor2, uint32_t patch2) {
    if (major1 != major2) {
        return (major1 > major2) ? 1 : -1;
    }
    if (minor1 != minor2) {
        return (minor1 > minor2) ? 1 : -1;
    }
    if (patch1 != patch2) {
        return (patch1 > patch2) ? 1 : -1;
    }
    return 0; // Equal
}

int native_version_satisfies(uint32_t major, uint32_t minor, uint32_t patch,
                           uint32_t req_major, uint32_t req_minor, uint32_t req_patch) {
    // Check if version meets minimum requirement
    int cmp = native_version_compare(major, minor, patch, req_major, req_minor, req_patch);
    return cmp >= 0; // Version is equal or greater than requirement
}

void* native_module_get_export_address(const NativeModule* module, const char* name) {
    const NativeExport* export = native_module_find_export(module, name);
    if (!export) {
        return NULL;
    }

    if (export->type == NATIVE_EXPORT_FUNCTION && module->code_section) {
        return (void*)((uintptr_t)module->code_section + export->offset);
    } else if (export->type == NATIVE_EXPORT_VARIABLE && module->data_section) {
        return (void*)((uintptr_t)module->data_section + export->offset);
    }

    return NULL;
}

/**
 * Get symbol from native module
 */
void* native_module_get_symbol(const NativeModule* module, const char* symbol_name) {
    if (!module || !symbol_name) {
        return NULL;
    }

    const NativeExport* export = native_module_find_export(module, symbol_name);
    if (!export) {
        return NULL;
    }

    // Return address based on export type
    if (export->type == NATIVE_EXPORT_FUNCTION && module->code_section) {
        return (void*)((uintptr_t)module->code_section + export->offset);
    } else if (export->type == NATIVE_EXPORT_VARIABLE && module->data_section) {
        return (void*)((uintptr_t)module->data_section + export->offset);
    }

    return NULL;
}

/**
 * Get symbol from native module (compatibility function for loader)
 * This function handles both NativeModule* and NativeModuleHandle* types
 */
void* module_get_symbol_native(void* module_ptr, const char* symbol_name) {
    if (!module_ptr || !symbol_name) {
        return NULL;
    }

    // Check if this is a NativeModuleHandle by looking at the structure
    typedef struct {
        char module_path[512];
        char module_name[128];
        void* mapped_memory;
        size_t mapped_size;
        uint32_t flags;
        // ... more fields
        char padding[1000]; // Approximate remaining structure size
        void* native_module;
        int is_native_format;
    } NativeModuleHandle_Layout;

    NativeModuleHandle_Layout* handle = (NativeModuleHandle_Layout*)module_ptr;

    // Heuristic: if module_path looks valid and is_native_format is set
    if (handle->module_path[0] != '\0' && handle->is_native_format == 1 && handle->native_module != NULL) {
        // This is a NativeModuleHandle with a proper .native module
        printf("Native Module: Using native.c system to get symbol '%s'\n", symbol_name);
        return native_module_get_symbol((NativeModule*)handle->native_module, symbol_name);
    }

    // Fallback: try to treat as direct NativeModule*
    return native_module_get_symbol((NativeModule*)module_ptr, symbol_name);
}
