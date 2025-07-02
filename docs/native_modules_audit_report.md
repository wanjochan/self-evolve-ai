# Native Modules Audit Report

## Executive Summary

The codebase has **inconsistent .native module implementation** with multiple formats and naming conventions that violate PRD.md specifications. Standardization is urgently needed.

## Current .native Files Found

### In bin/ Directory
1. `libc_minimal.native` (258 bytes)
2. `libc_os.native` (419 bytes)  
3. `libc_version_manager.native` (499 bytes)
4. `loader.native` (1,413 bytes)
5. `module_call_optimizer.native` (1,233 bytes)
6. `rt_module_manager.native` (183 bytes)
7. `universal_loader.native` (923 bytes)

### In Root Directory
1. `libc_x86_64_64.native` (unknown size)
2. `vm_x86_64_64.native` (unknown size)
3. `test_enhanced_module.native` (unknown size)

## PRD Compliance Analysis

### ✅ Partially Compliant
- `vm_x86_64_64.native` - Follows `{module}_{arch}_{bits}.native` convention
- `libc_x86_64_64.native` - Follows `{module}_{arch}_{bits}.native` convention

### ❌ Non-Compliant Naming
- `libc_minimal.native` → Should be `libc_x64_64.native`
- `libc_os.native` → Should be `libc_x64_64.native` 
- `libc_version_manager.native` → Should be `libc_x64_64.native`
- `loader.native` → Should be in Layer 1, not Layer 2
- `module_call_optimizer.native` → Non-standard module
- `rt_module_manager.native` → Non-standard module
- `universal_loader.native` → Should be in Layer 1, not Layer 2

## Format Analysis

### Defined Format (src/core/include/native_format.h)
```c
// Magic number: "NATV" (0x5654414E)
typedef struct {
    uint32_t magic;              // "NATV"
    uint32_t version;            // Format version
    uint32_t architecture;       // NATIVE_ARCH_X86_64, etc.
    uint32_t module_type;        // NATIVE_TYPE_VM, NATIVE_TYPE_LIBC, etc.
    uint64_t code_size;          // Size of code section
    uint64_t data_size;          // Size of data section
    uint32_t export_count;       // Number of exports
    // ... additional fields
} NativeHeader;
```

### Actual File Analysis
- **libc_minimal.native**: Starts with `52 54 4D 45` ("RTME") - Wrong magic!
- **File sizes**: Very small (183-1413 bytes) - Likely incomplete implementations
- **Format inconsistency**: Files don't follow the defined NativeHeader format

## Implementation Status

### VM Module (`vm_x64_64.native`)
- **Source**: `src/legacy/vm_x64_64_native.c`
- **Interface**: Defines `VMCoreInterface` with proper entry points
- **Functions**: `vm_native_main()`, `vm_get_interface()`
- **Status**: ✅ Well-defined interface, needs format standardization

### LibC Module (`libc_x64_64.native`)
- **Source**: Multiple implementations scattered
- **Interface**: Inconsistent across implementations
- **Functions**: Basic libc forwarding (malloc, free, printf, etc.)
- **Status**: ⚠️ Multiple versions, needs consolidation

### Format Implementation
- **Loading**: `src/core/native_format.c` has complete loader
- **Validation**: Proper validation functions exist
- **Export system**: Well-defined export table structure
- **Status**: ✅ Good foundation, not used consistently

## Critical Issues

### 1. Magic Number Mismatch
- Defined: "NATV" (0x5654414E)
- Actual files: "RTME" (0x454D5452)
- **Impact**: Loader cannot recognize files

### 2. Naming Convention Violations
- PRD requires: `{module}_{arch}_{bits}.native`
- Current: Random names like `libc_minimal.native`
- **Impact**: Loader cannot find correct modules

### 3. Multiple LibC Implementations
- `libc_minimal.native`
- `libc_os.native` 
- `libc_version_manager.native`
- `libc_x86_64_64.native`
- **Impact**: Confusion about which to use

### 4. Missing Standard Modules
- No `vm_x64_64.native` in bin/
- No `libc_x64_64.native` in bin/
- **Impact**: Loader cannot find required modules

## Standardization Requirements

### 1. Naming Convention
```
vm_x64_64.native      # VM core for x64 64-bit
libc_x64_64.native    # LibC for x64 64-bit
vm_arm64_64.native    # VM core for ARM64 64-bit
libc_arm64_64.native  # LibC for ARM64 64-bit
```

### 2. File Format
- Use "NATV" magic number
- Follow NativeHeader structure
- Include proper export tables
- Implement validation checksums

### 3. Interface Standardization
```c
// Standard VM interface
int vm_native_main(int argc, char* argv[]);
const VMCoreInterface* vm_get_interface(void);

// Standard LibC interface  
int libc_native_init(void);
void* libc_native_get_function(const char* name);
```

## Cleanup Actions Required

### Files to Delete
- `bin/libc_minimal.native`
- `bin/libc_os.native`
- `bin/libc_version_manager.native`
- `bin/loader.native` (move to Layer 1)
- `bin/module_call_optimizer.native`
- `bin/rt_module_manager.native`
- `bin/universal_loader.native` (move to Layer 1)

### Files to Create
- `bin/vm_x64_64.native` (standardized)
- `bin/libc_x64_64.native` (standardized)
- `bin/vm_arm64_64.native` (future)
- `bin/libc_arm64_64.native` (future)

### Implementation Tasks
1. Fix magic number in format
2. Consolidate LibC implementations
3. Standardize VM module interface
4. Implement proper module loading
5. Create validation tools

## Next Steps

1. **Define specification** for .native format
2. **Consolidate implementations** into standard modules
3. **Fix format compliance** with proper headers
4. **Update build system** to generate correct files
5. **Test module loading** with standardized format
