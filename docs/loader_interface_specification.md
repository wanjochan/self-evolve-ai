# Loader Interface Specification (Layer 1)

## Overview

This document defines the standard interface for the PRD-compliant loader that loads `vm_{arch}_{bits}.native` modules.

## Loader Naming Convention

```
loader_x64_64.exe      # 64-bit x86_64 loader
loader_arm64_64.exe    # 64-bit ARM64 loader  
loader_x86_32.exe      # 32-bit x86 loader
loader_arm32_32.exe    # 32-bit ARM loader
```

## Command Line Interface

### Basic Usage
```bash
loader_{arch}.exe <program.astc> [program_args...]
```

### Advanced Usage
```bash
loader_{arch}.exe [options] <program.astc> [-- program_args...]
```

### Options
```
-v, --verbose          Enable verbose output
-d, --debug            Enable debug mode
-p, --performance      Show performance statistics
-i, --interactive      Start in interactive mode
-a, --autonomous       Enable autonomous AI evolution
-m, --vm-module PATH   Override VM module path
-c, --config FILE      Load configuration file
-s, --security LEVEL   Set security level (0-3)
-h, --help             Show help message
```

### Examples
```bash
# Basic execution
loader_x64_64.exe c99.astc

# Debug mode with performance stats
loader_x64_64.exe -v -d -p evolver0.astc

# Custom VM module
loader_x64_64.exe -m custom_vm_x64_64.native program.astc

# Autonomous evolution mode
loader_x64_64.exe -a evolver1.astc

# Interactive mode
loader_x64_64.exe -i
```

## Architecture Detection

### Supported Architectures
```c
typedef enum {
    ARCH_UNKNOWN = 0,
    ARCH_X86_64 = 1,    // 64-bit x86_64
    ARCH_ARM64 = 2,     // 64-bit ARM64
    ARCH_X86_32 = 3,    // 32-bit x86
    ARCH_ARM32 = 4      // 32-bit ARM
} DetectedArchitecture;
```

### Detection Function
```c
DetectedArchitecture detect_architecture(void);
```

### VM Module Path Resolution
```c
const char* get_vm_module_path(DetectedArchitecture arch);
```

Returns:
- `vm_x64_64.native` for ARCH_X86_64
- `vm_arm64_64.native` for ARCH_ARM64
- `vm_x86_32.native` for ARCH_X86_32
- `vm_arm32_32.native` for ARCH_ARM32

## VM Module Loading Interface

### VM Module Structure
```c
typedef struct {
    void* handle;                  // Module handle (HMODULE/dlopen)
    const char* module_path;       // Path to loaded module
    DetectedArchitecture arch;     // Architecture type
    int (*vm_main)(int argc, char* argv[]);
    const void* (*get_interface)(void);
} LoadedVMModule;
```

### Loading Functions
```c
LoadedVMModule* load_vm_module(DetectedArchitecture arch, const char* override_path);
void unload_vm_module(LoadedVMModule* vm_module);
```

### Required VM Module Exports
Every `vm_{arch}_{bits}.native` module must export:

1. **vm_native_main**: Main entry point
   ```c
   int vm_native_main(int argc, char* argv[]);
   ```

2. **vm_get_interface**: Get VM interface
   ```c
   const VMCoreInterface* vm_get_interface(void);
   ```

## Configuration Interface

### Configuration Structure
```c
typedef struct {
    const char* program_file;      // ASTC program to execute
    const char* vm_module_override; // Manual VM module override
    bool verbose_mode;             // Detailed output
    bool debug_mode;               // Debug information
    bool performance_stats;        // Performance measurement
    bool interactive_mode;         // Interactive mode
    bool autonomous_mode;          // AI autonomous evolution
    uint32_t security_level;       // Security clearance level
    const char* config_file;       // Configuration file path
    int program_argc;              // Arguments to pass to program
    char** program_argv;           // Program arguments
} UnifiedLoaderConfig;
```

### Configuration Functions
```c
int parse_arguments(int argc, char* argv[], UnifiedLoaderConfig* config);
```

## Performance Monitoring Interface

### Performance Statistics
```c
typedef struct {
    clock_t start_time;
    clock_t detection_time;
    clock_t vm_load_time;
    clock_t program_load_time;
    clock_t execution_time;
    clock_t end_time;
} PerformanceStats;
```

### Performance Functions
```c
void print_performance_stats(const PerformanceStats* stats, const UnifiedLoaderConfig* config);
```

## Error Handling

### Return Codes
```
0   - Success
1   - General error
2   - Invalid arguments
3   - Architecture not supported
4   - VM module not found
5   - VM module load failed
6   - Program file not found
7   - Program execution failed
```

### Error Messages
All error messages follow the format:
```
Loader Error: <description>
```

## Execution Flow

### Standard Flow
1. **Parse arguments** → UnifiedLoaderConfig
2. **Detect architecture** → DetectedArchitecture
3. **Load VM module** → LoadedVMModule
4. **Load ASTC program** → program data
5. **Execute program** → vm_native_main()
6. **Cleanup** → unload modules

### Interactive Flow
1. **Parse arguments** → interactive mode
2. **Detect architecture** → DetectedArchitecture
3. **Load VM module** → LoadedVMModule
4. **Interactive loop** → user commands
5. **Cleanup** → unload modules

## File System Interface

### Search Paths
VM modules are searched in this order:
1. Override path (if specified)
2. `bin/layer2/vm_{arch}_{bits}.native`
3. `vm_{arch}_{bits}.native` (current directory)
4. System PATH

### File Validation
Before loading, the loader validates:
1. File exists and is readable
2. File has correct .native format
3. Architecture matches detected architecture
4. Required exports are present

## Security Interface

### Security Levels
```
0 - No security (development)
1 - Basic validation (default)
2 - Signature verification
3 - Sandboxed execution
```

### Security Functions
```c
bool validate_module_security(const char* module_path, uint32_t security_level);
bool validate_program_security(const char* program_path, uint32_t security_level);
```

## Extension Points

### Custom VM Modules
The loader supports custom VM modules that implement the standard interface:
```c
int vm_native_main(int argc, char* argv[]);
const VMCoreInterface* vm_get_interface(void);
```

### Configuration Files
JSON configuration files can override default settings:
```json
{
    "vm_module_override": "custom_vm_x64_64.native",
    "security_level": 2,
    "performance_stats": true,
    "debug_mode": false
}
```

## Compatibility

### PRD Compliance
- ✅ Layer 1 architecture
- ✅ Cross-platform support
- ✅ Hardware detection
- ✅ Module loading interface
- ✅ Unified entry point

### Platform Support
- ✅ Windows (x64, x86, ARM64)
- ✅ Linux (x64, x86, ARM64, ARM32)
- ✅ macOS (x64, ARM64)

This interface specification ensures consistent behavior across all loader implementations and provides a stable foundation for the three-layer architecture.
