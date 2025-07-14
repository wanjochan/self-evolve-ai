# Self-Evolve AI Development Guide

## Getting Started

### Prerequisites
- **OS**: Linux (Ubuntu 22.04+ recommended)
- **Compiler**: GCC 11.4.0 or newer
- **Build System**: GNU Make 4.3+
- **Architecture**: x86_64 (ARM64 partial support)

### Quick Setup
```bash
# Clone the repository
git clone https://github.com/wanjochan/self-evolve-ai.git
cd self-evolve-ai

# Build the system
make all

# Run tests
cd tests && ./test_layer1_loader.sh
```

## Project Structure

```
self-evolve-ai/
├── src/                    # Source code
│   ├── core/              # Core modules
│   │   ├── modules/       # Dynamic modules
│   │   └── astc.c         # ASTC bytecode system
│   ├── c99/               # C99 compiler backend
│   └── layer1/            # Simple loader
├── tools/                 # Compilation tools
│   ├── c2astc.c          # C to ASTC compiler
│   ├── c2astc_direct.c   # Direct-linked compiler
│   └── c2native.c        # C to native compiler
├── tests/                 # Test suites
├── docs/                  # Documentation
├── bin/                   # Compiled binaries
└── build/                 # Build artifacts
```

## Build System

### Core Build Commands
```bash
# Build everything
make all

# Build core modules
./build_core.sh

# Build specific components
make simple_loader
make c2astc
make modules

# Clean build
make clean
```

### Build Scripts
- `build_core.sh`: Builds core modules
- `cc.sh`: Wrapper for GCC with project-specific flags
- `build_all.sh`: Complete system build

### Compiler Flags
```bash
# Standard flags (in cc.sh)
-std=c99 -O2 -Wall -Wextra -Werror
-I src/core -ldl -lm

# Debug flags
-g -DDEBUG -O0

# Release flags
-O3 -DNDEBUG -s
```

## Development Workflow

### 1. Setting Up Development Environment
```bash
# Create development branch
git checkout -b feature/your-feature

# Set up debug environment
export ASTC_DEBUG=1
export ASTC_MODULE_PATH="./bin"

# Build in debug mode
make debug
```

### 2. Code Style Guidelines

#### C Code Style
```c
// Function naming: snake_case
int my_function(int param);

// Variable naming: snake_case
int my_variable = 0;

// Constants: UPPER_CASE
#define MAX_SIZE 1024

// Structures: PascalCase
typedef struct {
    int field;
} MyStruct;

// Error handling: always check return values
if (!my_function(param)) {
    fprintf(stderr, "Error: function failed\n");
    return -1;
}
```

#### File Organization
```c
// File header
#include <standard_headers.h>
#include "project_headers.h"

// Constants and macros
#define CONSTANT_VALUE 42

// Type definitions
typedef struct { ... } MyType;

// Static function declarations
static int helper_function(void);

// Public function implementations
int public_function(void) { ... }

// Static function implementations
static int helper_function(void) { ... }
```

### 3. Testing Strategy

#### Test Categories
1. **Unit Tests**: Individual function testing
2. **Integration Tests**: Module interaction testing
3. **System Tests**: End-to-end functionality
4. **Performance Tests**: Benchmarking and profiling

#### Running Tests
```bash
# All tests
cd tests && ./run_all_tests.sh

# Specific test suites
./test_layer1_loader.sh      # Layer 1 tests
./test_layer2_modules.sh     # Layer 2 tests
./test_layer3_programs.sh    # Layer 3 tests
./test_stability_enhanced.sh # Stability tests
```

#### Writing New Tests
```bash
# Create test file
cat > tests/test_my_feature.sh << 'EOF'
#!/bin/bash
# Test description
run_test "Test name" "command" "expected_result"
EOF

# Make executable
chmod +x tests/test_my_feature.sh
```

### 4. Debugging

#### Debug Tools
```bash
# Enable debug output
export ASTC_DEBUG=1

# Use GDB for debugging
gdb ./bin/simple_loader
(gdb) run program.astc

# Memory debugging (if valgrind available)
valgrind --leak-check=full ./bin/simple_loader program.astc
```

#### Debug Macros
```c
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

// Usage
DEBUG_PRINT("Variable value: %d", my_var);
```

## Module Development

### Creating a New Module

#### 1. Module Structure
```c
// my_module.c
#include "module_common.h"

// Module initialization
int module_init(void) {
    // Initialize module
    return 0;
}

// Module cleanup
void module_cleanup(void) {
    // Cleanup resources
}

// Exported functions
int my_module_function(int param) {
    // Implementation
    return result;
}
```

#### 2. Build Integration
```bash
# Add to build_core.sh
./cc.sh -c src/core/modules/my_module.c -I src/core -o bin/my_module.o

# Create native module
./bin/c2native src/core/modules/my_module.c bin/my_module_x64_64.native
```

#### 3. Module Testing
```c
// Test module loading
Module* mod = module_load_smart("my_module");
if (mod) {
    void* func = resolve_function(mod, "my_module_function");
    // Test function
}
```

## Compiler Development

### Adding New Language Features

#### 1. Lexer Updates
```c
// Add new token type in astc.h
typedef enum {
    // ... existing tokens
    TOKEN_NEW_KEYWORD,
} TokenType;

// Add tokenization in pipeline_module.c
if (strncmp(current, "newkeyword", 10) == 0) {
    token = create_token(TOKEN_NEW_KEYWORD, "newkeyword", line, col);
}
```

#### 2. Parser Updates
```c
// Add parsing logic
ASTNode* parse_new_statement(Token** tokens, int* index) {
    // Parse new syntax
    return node;
}
```

#### 3. Code Generation
```c
// Add code generation in c99_codegen.c
void generate_new_statement(ASTNode* node, FILE* output) {
    // Generate C code
}
```

## Performance Optimization

### Profiling
```bash
# Compile with profiling
gcc -pg -o program program.c

# Run and generate profile
./program
gprof program gmon.out > profile.txt
```

### Memory Optimization
```c
// Use memory pools for frequent allocations
typedef struct {
    char* memory;
    size_t size;
    size_t used;
} MemoryPool;

// Optimize string operations
// Use string interning for repeated strings
```

### Compilation Optimization
```c
// Cache compilation results
// Use incremental compilation
// Optimize AST traversal
```

## Contributing Guidelines

### 1. Code Review Process
1. Create feature branch
2. Implement changes with tests
3. Run full test suite
4. Create pull request
5. Address review feedback
6. Merge after approval

### 2. Commit Message Format
```
type(scope): brief description

Detailed description of changes.

- Change 1
- Change 2

Fixes #issue_number
```

### 3. Documentation Requirements
- Update API documentation for public interfaces
- Add inline comments for complex logic
- Update user guides for new features
- Include examples for new APIs

## Troubleshooting Development Issues

### Common Build Issues
```bash
# Missing dependencies
sudo apt-get install build-essential

# Permission issues
chmod +x build_scripts/*.sh

# Module loading issues
export LD_LIBRARY_PATH=./bin:$LD_LIBRARY_PATH
```

### Common Runtime Issues
```bash
# Segmentation faults
gdb --args ./bin/simple_loader program.astc

# Module not found
ls -la bin/*_x64_64.native

# Compilation errors
./bin/c2astc_direct -v input.c output.astc
```

## Release Process

### 1. Version Management
```bash
# Update version numbers
# Update CHANGELOG.md
# Tag release
git tag -a v1.0.0 -m "Release version 1.0.0"
```

### 2. Testing Before Release
```bash
# Full test suite
cd tests && ./run_all_tests.sh

# Performance benchmarks
./test_performance_benchmark.sh

# Cross-platform testing
./test_cross_platform.sh
```

### 3. Documentation Updates
- Update API documentation
- Update user guides
- Update compatibility matrix
- Generate release notes

## Resources

### Internal Documentation
- [API Documentation](api_documentation.md)
- [Cross-Platform Compatibility](cross_platform_compatibility_report.md)
- [Technical Debt Inventory](technical_debt_inventory.md)

### External Resources
- [C99 Standard](https://www.iso.org/standard/29237.html)
- [POSIX Standards](https://pubs.opengroup.org/onlinepubs/9699919799/)
- [GCC Documentation](https://gcc.gnu.org/onlinedocs/)

### Community
- GitHub Issues: Report bugs and feature requests
- Discussions: Technical discussions and questions
- Wiki: Community-maintained documentation
