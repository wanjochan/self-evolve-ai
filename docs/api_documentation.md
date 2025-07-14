# Self-Evolve AI API Documentation

## Overview

The Self-Evolve AI system provides a three-layer architecture for compiling and executing C code through ASTC (Abstract Syntax Tree Compiled) bytecode. This document describes the public APIs and interfaces.

## Architecture Layers

### Layer 1: Simple Loader
The entry point for executing ASTC programs.

#### Command Line Interface
```bash
./bin/simple_loader [options] <astc_file>
```

**Options:**
- `--arch`: Display detected architecture
- `--version`: Display version information
- `--help`: Display usage information

**Return Values:**
- `0`: Success
- `1`: General error
- `2`: File not found
- `3`: Invalid ASTC format
- `4`: Module loading error

#### Example Usage
```bash
# Execute an ASTC program
./bin/simple_loader program.astc

# Check architecture
./bin/simple_loader --arch
```

### Layer 2: Native Modules
Dynamic modules providing core functionality.

#### Module Loading API
```c
// Load a module by name
Module* load_module(const char* module_name);

// Resolve a function from a module
void* resolve_function(Module* module, const char* function_name);

// Unload a module
void unload_module(Module* module);
```

#### Available Modules
- **pipeline_x64_64.native**: Compilation pipeline
- **layer0_x64_64.native**: Core runtime
- **compiler_x64_64.native**: JIT compilation
- **libc_x64_64.native**: Standard library functions

### Layer 3: ASTC Programs
Compiled C programs in ASTC bytecode format.

## Compilation Tools

### C2ASTC Compiler
Compiles C source code to ASTC bytecode.

#### Command Line Interface
```bash
./bin/c2astc <input.c> <output.astc>
```

#### Direct-Linked Version (Recommended)
```bash
./bin/c2astc_direct <input.c> <output.astc>
```

**Features:**
- Full C99 support
- Optimized compilation pipeline
- Comprehensive error reporting
- Fallback mechanisms for stability

#### Example Usage
```bash
# Compile a C program
./bin/c2astc_direct hello.c hello.astc

# Execute the compiled program
./bin/simple_loader hello.astc
```

### C2Native Compiler
Compiles C source code to native modules.

```bash
./bin/c2native <input.c> <output.native>
```

## Core APIs

### Pipeline Module API

#### Compilation Functions
```c
// Main compilation function
bool pipeline_compile(const char* source_code, CompileOptions* options);

// Get compilation error message
const char* pipeline_get_error(void);

// Get compiled ASTC program
ASTCBytecodeProgram* pipeline_get_astc_program(void);
```

#### CompileOptions Structure
```c
typedef struct {
    int optimize_level;      // 0-3, optimization level
    bool enable_debug;       // Enable debug information
    bool enable_warnings;    // Enable compiler warnings
    char output_file[256];   // Output file path
} CompileOptions;
```

### ASTC Module API

#### Bytecode Management
```c
// Create ASTC bytecode program
ASTCBytecodeProgram* astc_bytecode_create(void);

// Free ASTC bytecode program
void astc_bytecode_free(ASTCBytecodeProgram* program);

// Execute ASTC bytecode
int astc_execute(ASTCBytecodeProgram* program);
```

#### ASTC File Format
```c
// ASTC file header
typedef struct {
    char magic[4];           // "ASTC"
    uint32_t version;        // Format version
    uint32_t source_size;    // Source code size
    uint32_t bytecode_size;  // Bytecode size
    uint32_t metadata_size;  // Metadata size
} ASTCHeader;
```

### Module System API

#### Module Structure
```c
typedef struct {
    char* name;              // Module name
    void* handle;            // Dynamic library handle
    void* base_addr;         // Base memory address
    size_t size;             // Module size
    bool is_loaded;          // Loading status
} Module;
```

#### Module Functions
```c
// Smart module loading (tries multiple paths)
Module* module_load_smart(const char* base_name);

// Load module from specific path
Module* module_load(const char* path);

// Check if module is loaded
bool module_is_loaded(Module* module);

// Get module information
const char* module_get_info(Module* module);
```

## Error Handling

### Error Codes
```c
#define ASTC_SUCCESS           0
#define ASTC_ERROR_GENERAL     1
#define ASTC_ERROR_FILE_NOT_FOUND  2
#define ASTC_ERROR_INVALID_FORMAT  3
#define ASTC_ERROR_MODULE_LOAD     4
#define ASTC_ERROR_COMPILATION     5
#define ASTC_ERROR_EXECUTION       6
```

### Error Reporting
All APIs provide detailed error messages through:
- Return codes for status
- Error message functions for details
- Logging output for debugging

## Configuration

### Environment Variables
- `ASTC_MODULE_PATH`: Additional module search paths
- `ASTC_DEBUG`: Enable debug output (0/1)
- `ASTC_ARCH`: Override architecture detection

### Build Configuration
```bash
# Standard build
make all

# Debug build
make debug

# Release build
make release

# Clean build
make clean
```

## Examples

### Basic C Program Compilation and Execution
```c
// hello.c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

```bash
# Compile and run
./bin/c2astc_direct hello.c hello.astc
./bin/simple_loader hello.astc
```

### Using the Pipeline API Directly
```c
#include "pipeline_module.h"

int main() {
    const char* source = "int main() { return 42; }";
    CompileOptions options = {0};
    
    if (pipeline_compile(source, &options)) {
        ASTCBytecodeProgram* program = pipeline_get_astc_program();
        int result = astc_execute(program);
        printf("Program returned: %d\n", result);
    } else {
        printf("Compilation failed: %s\n", pipeline_get_error());
    }
    
    return 0;
}
```

### Module Loading Example
```c
#include "module_module.h"

int main() {
    Module* pipeline = module_load_smart("pipeline");
    if (pipeline) {
        void* compile_func = resolve_function(pipeline, "pipeline_compile");
        if (compile_func) {
            printf("Pipeline module loaded successfully\n");
        }
        unload_module(pipeline);
    }
    return 0;
}
```

## Performance Considerations

### Compilation Performance
- Direct-linked compiler: ~1000 lines/second
- Module-based compiler: ~800 lines/second (with module loading overhead)
- Memory usage: ~10MB for typical programs

### Execution Performance
- ASTC bytecode: ~70% of native performance
- Module loading: ~50ms overhead per module
- Memory overhead: ~2MB for runtime system

## Troubleshooting

### Common Issues
1. **Module not found**: Check module paths and architecture
2. **Compilation errors**: Use direct-linked compiler for better stability
3. **Segmentation faults**: Enable debug mode for detailed output

### Debug Mode
```bash
export ASTC_DEBUG=1
./bin/simple_loader program.astc
```

### Log Files
- Compilation logs: `./logs/compilation.log`
- Execution logs: `./logs/execution.log`
- Module loading logs: `./logs/modules.log`

## Version Information

- **API Version**: 2.0
- **ASTC Format Version**: 1
- **Compatibility**: C99, POSIX
- **Supported Architectures**: x86_64, ARM64 (partial)

## See Also

- [Cross-Platform Compatibility Report](cross_platform_compatibility_report.md)
- [Technical Debt Inventory](technical_debt_inventory.md)
- [Development Guide](development_guide.md)
- [Troubleshooting Guide](troubleshooting_guide.md)
