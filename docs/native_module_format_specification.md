# Native Module Format Specification (Layer 2)

## Overview

This document defines the standard format for `.native` modules in the PRD three-layer architecture.

## File Naming Convention

```
{module}_{arch}_{bits}.native
```

### Examples
```
vm_x64_64.native       # VM core for x64 64-bit
libc_x64_64.native     # LibC for x64 64-bit
vm_arm64_64.native     # VM core for ARM64 64-bit
libc_arm64_64.native   # LibC for ARM64 64-bit
vm_x86_32.native       # VM core for x86 32-bit
libc_x86_32.native     # LibC for x86 32-bit
```

## File Format Structure

### Magic Number
```
"NATV" (0x5654414E)
```

### File Layout
```
+------------------+
| Native Header    | (Fixed size)
+------------------+
| Metadata Section | (Variable size)
+------------------+
| Code Section     | (Variable size)
+------------------+
| Data Section     | (Variable size)
+------------------+
| Export Table     | (Variable size)
+------------------+
| Dependencies     | (Variable size)
+------------------+
| Relocations      | (Variable size)
+------------------+
```

## Native Header Structure

```c
typedef struct {
    uint32_t magic;              // "NATV" (0x5654414E)
    uint32_t version;            // Format version (1)
    uint32_t architecture;       // NativeArchitecture
    uint32_t module_type;        // NativeModuleType
    uint32_t flags;              // NativeModuleFlags
    
    uint64_t file_size;          // Total file size
    uint64_t code_size;          // Size of code section
    uint64_t data_size;          // Size of data section
    uint32_t export_count;       // Number of exports
    uint32_t dependency_count;   // Number of dependencies
    
    uint64_t code_offset;        // Offset to code section
    uint64_t data_offset;        // Offset to data section
    uint64_t export_table_offset; // Offset to export table
    uint64_t dependency_offset;  // Offset to dependencies
    uint64_t metadata_offset;    // Offset to metadata section
    uint64_t relocation_offset;  // Offset to relocation table
    uint32_t relocation_count;   // Number of relocations
    
    uint64_t checksum;           // CRC64 checksum
    uint32_t reserved[4];        // Reserved for future use
} NativeHeader;
```

## Architecture Types

```c
typedef enum {
    NATIVE_ARCH_X86_64 = 1,     // x86_64 (AMD64)
    NATIVE_ARCH_ARM64 = 2,      // ARM64 (AArch64)
    NATIVE_ARCH_X86_32 = 3,     // x86 (i386)
    NATIVE_ARCH_ARM32 = 4       // ARM32 (AArch32)
} NativeArchitecture;
```

## Module Types

```c
typedef enum {
    NATIVE_TYPE_VM = 1,         // VM core module
    NATIVE_TYPE_LIBC = 2,       // LibC forwarding module
    NATIVE_TYPE_USER = 3        // User-defined module
} NativeModuleType;
```

## Module Flags

```c
typedef enum {
    NATIVE_FLAG_NONE = 0,
    NATIVE_FLAG_RELOCATABLE = 1,
    NATIVE_FLAG_POSITION_INDEPENDENT = 2,
    NATIVE_FLAG_DEBUG_INFO = 4,
    NATIVE_FLAG_OPTIMIZED = 8,
    NATIVE_FLAG_SIGNED = 16
} NativeModuleFlags;
```

## Export Table Structure

```c
typedef struct {
    uint32_t export_count;
    NativeExport exports[export_count];
} NativeExportTable;

typedef struct {
    char name[256];              // Export name (null-terminated)
    uint32_t type;               // Export type
    uint32_t flags;              // Export flags
    uint64_t offset;             // Offset in code/data section
    uint64_t size;               // Size of exported item
} NativeExport;
```

### Export Types

```c
typedef enum {
    NATIVE_EXPORT_FUNCTION = 1,
    NATIVE_EXPORT_VARIABLE = 2,
    NATIVE_EXPORT_CONSTANT = 3,
    NATIVE_EXPORT_TYPE = 4,
    NATIVE_EXPORT_INTERFACE = 5
} NativeExportType;
```

## Metadata Section

```c
typedef struct {
    char module_name[128];       // Module name
    char version[32];            // Version string
    char author[64];             // Author name
    char description[256];       // Description
    char license[64];            // License
    uint64_t build_timestamp;    // Build timestamp
    uint32_t api_version;        // API version
    uint32_t reserved[8];        // Reserved
} NativeMetadata;
```

## Dependencies

```c
typedef struct {
    char name[128];              // Dependency name
    char version_min[32];        // Minimum version
    char version_max[32];        // Maximum version
    uint32_t flags;              // Dependency flags
} NativeDependency;
```

## Standard Module Interfaces

### VM Module Interface

Every `vm_{arch}_{bits}.native` must export:

```c
// Main entry point
int vm_native_main(int argc, char* argv[]);

// Get VM interface
const VMCoreInterface* vm_get_interface(void);

// VM interface structure
typedef struct {
    int (*init)(void);
    void (*cleanup)(void);
    int (*execute_astc)(const char* astc_file, int argc, char* argv[]);
    void* (*load_native_module)(const char* module_path);
    const VMModuleInfo* (*get_info)(void);
} VMCoreInterface;
```

### LibC Module Interface

Every `libc_{arch}_{bits}.native` must export:

```c
// Module initialization
int libc_native_init(void);

// Get function pointer
void* libc_native_get_function(const char* name);

// Standard LibC functions
void* malloc(size_t size);
void free(void* ptr);
int printf(const char* format, ...);
// ... other libc functions
```

## Loading Process

### 1. Header Validation
```c
bool validate_native_header(const NativeHeader* header) {
    return header->magic == NATIVE_MAGIC &&
           header->version == NATIVE_VERSION_V1 &&
           header->architecture != NATIVE_ARCH_UNKNOWN;
}
```

### 2. Architecture Compatibility
```c
bool is_architecture_compatible(NativeArchitecture module_arch) {
    DetectedArchitecture host_arch = detect_architecture();
    return (module_arch == NATIVE_ARCH_X86_64 && host_arch == ARCH_X86_64) ||
           (module_arch == NATIVE_ARCH_ARM64 && host_arch == ARCH_ARM64) ||
           // ... other combinations
}
```

### 3. Dependency Resolution
```c
bool resolve_dependencies(const NativeDependency* deps, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (!load_dependency(&deps[i])) {
            return false;
        }
    }
    return true;
}
```

### 4. Symbol Resolution
```c
void* get_export_function(const NativeModule* module, const char* name) {
    for (uint32_t i = 0; i < module->header.export_count; i++) {
        if (strcmp(module->export_table->exports[i].name, name) == 0) {
            return (void*)(module->code_section + 
                          module->export_table->exports[i].offset);
        }
    }
    return NULL;
}
```

## Error Codes

```c
typedef enum {
    NATIVE_SUCCESS = 0,
    NATIVE_ERROR_INVALID = -1,
    NATIVE_ERROR_IO = -2,
    NATIVE_ERROR_MEMORY = -3,
    NATIVE_ERROR_ARCH_MISMATCH = -4,
    NATIVE_ERROR_DEPENDENCY = -5,
    NATIVE_ERROR_CHECKSUM = -6,
    NATIVE_ERROR_VERSION = -7
} NativeError;
```

## Security Features

### Checksum Validation
```c
uint64_t calculate_checksum(const NativeModule* module) {
    // CRC64 of all sections except checksum field
}
```

### Signature Verification (Optional)
```c
bool verify_signature(const NativeModule* module, const uint8_t* signature) {
    // Digital signature verification
}
```

## Build Process

### 1. Compilation
```bash
# Compile source to object files
gcc -c -fPIC -O2 vm_core.c -o vm_core.o
gcc -c -fPIC -O2 vm_astc.c -o vm_astc.o
```

### 2. Native Module Generation
```bash
# Generate .native module
native_builder -o vm_x64_64.native \
               -arch x64_64 \
               -type vm \
               -exports vm_native_main,vm_get_interface \
               vm_core.o vm_astc.o
```

### 3. Validation
```bash
# Validate generated module
native_validator vm_x64_64.native
```

## Tools

### native_builder
Builds .native modules from object files:
```bash
native_builder [options] -o output.native input.o [input2.o ...]
```

### native_validator
Validates .native module format:
```bash
native_validator module.native
```

### native_inspector
Inspects .native module contents:
```bash
native_inspector module.native
```

## Compatibility

### Version Compatibility
- Version 1: Initial format
- Future versions: Backward compatible

### Platform Support
- Windows: PE-like structure
- Linux: ELF-like structure  
- macOS: Mach-O-like structure

### Cross-Architecture
- Modules are architecture-specific
- No cross-architecture loading
- Runtime architecture detection required

This specification ensures consistent .native module format across all platforms and architectures in the PRD three-layer system.
