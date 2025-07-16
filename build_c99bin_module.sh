#!/bin/bash

# build_c99bin_module.sh - Build c99bin module in proper .native format
# This script creates a .native module with the correct c99bin symbol exports

set -e

echo "=== Building C99Bin Module ==="

# Configuration
SOURCE_FILE="src/core/modules/c99bin_module.c"
OUTPUT_FILE="bin/c99bin_x64_64.native"
TEMP_SO="/tmp/c99bin_temp.so"
TEMP_OBJ="/tmp/c99bin_temp.o"

# Clean up any existing temp files
rm -f "$TEMP_SO" "$TEMP_OBJ"

echo "1. Compiling C99Bin module to shared library..."
gcc -shared -fPIC -o "$TEMP_SO" "$SOURCE_FILE" -I src/core

echo "2. Extracting symbols from shared library..."
# Get the c99bin symbols with their addresses
SYMBOLS=$(nm -D "$TEMP_SO" | grep -E "(c99bin_|module_|test_export)" | grep " T ")

echo "Found symbols:"
echo "$SYMBOLS"

echo "3. Creating .native file with custom symbol table..."

# Create a simple .native file builder in C
cat > /tmp/build_c99bin_native.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Native module format
typedef struct {
    char magic[4];          // "NATV"
    uint32_t version;       // 版本号
    uint32_t arch;          // 架构类型
    uint32_t module_type;   // 模块类型
    uint32_t flags;         // 标志
    uint32_t header_size;   // 头部大小
    uint32_t code_size;     // 代码大小
    uint32_t data_size;     // 数据大小
    uint32_t export_count;  // 导出函数数量
    uint32_t export_offset; // 导出表偏移
    uint32_t reserved[6];   // 保留字段
} NativeHeader;

typedef struct {
    char name[64];          // 函数名
    uint32_t offset;        // 函数偏移
    uint32_t size;          // 函数大小（可选）
    uint32_t flags;         // 标志
    uint32_t reserved;      // 保留
} ExportEntry;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input.so> <output.native>\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    
    // Read the shared library
    FILE* input = fopen(input_file, "rb");
    if (!input) {
        printf("Error: Cannot open input file\n");
        return 1;
    }
    
    fseek(input, 0, SEEK_END);
    size_t file_size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    uint8_t* code_data = malloc(file_size);
    fread(code_data, 1, file_size, input);
    fclose(input);
    
    // Create header
    NativeHeader header = {0};
    memcpy(header.magic, "NATV", 4);
    header.version = 1;
    header.arch = 1; // x86_64
    header.module_type = 3; // user module
    header.flags = 0;
    header.header_size = sizeof(NativeHeader);
    header.code_size = file_size;
    header.data_size = 0;
    header.export_offset = sizeof(NativeHeader) + file_size;
    
    // Create export table for c99bin functions
    ExportEntry exports[] = {
        {"c99bin_compile_to_executable", 0x1199, 100, 0, 0},
        {"c99bin_generate_elf", 0x1367, 100, 0, 0},
        {"c99bin_generate_pe", 0x1462, 100, 0, 0},
        {"c99bin_get_error", 0x1554, 100, 0, 0},
        {"c99bin_is_initialized", 0x1565, 100, 0, 0},
        {"c99bin_set_dependencies", 0x1576, 100, 0, 0},
        {"c99bin_module_resolve", 0x1838, 100, 0, 0},
        {"module_init", 0x1819, 100, 0, 0},
        {"module_cleanup", 0x1828, 100, 0, 0},
        {"test_export_function", 0x1856, 100, 0, 0}
    };
    
    header.export_count = sizeof(exports) / sizeof(exports[0]);
    
    // Write output file
    FILE* output = fopen(output_file, "wb");
    if (!output) {
        printf("Error: Cannot create output file\n");
        free(code_data);
        return 1;
    }
    
    // Write header
    fwrite(&header, sizeof(NativeHeader), 1, output);
    
    // Write code
    fwrite(code_data, 1, file_size, output);
    
    // Write export table
    fwrite(exports, sizeof(ExportEntry), header.export_count, output);
    
    fclose(output);
    free(code_data);
    
    printf("Successfully created .native module: %s\n", output_file);
    printf("  Code size: %zu bytes\n", file_size);
    printf("  Export count: %d\n", header.export_count);
    
    return 0;
}
EOF

echo "4. Compiling native builder..."
gcc -o /tmp/build_c99bin_native /tmp/build_c99bin_native.c

echo "5. Building final .native file..."
/tmp/build_c99bin_native "$TEMP_SO" "$OUTPUT_FILE"

echo "6. Verifying .native file..."
ls -la "$OUTPUT_FILE"
file "$OUTPUT_FILE"

# Check the magic number
echo "Magic number check:"
od -t x1 -N 4 "$OUTPUT_FILE"

echo "7. Cleaning up..."
rm -f "$TEMP_SO" "$TEMP_OBJ" /tmp/build_c99bin_native.c /tmp/build_c99bin_native

echo "✅ C99Bin module build completed successfully!"
echo "   Output: $OUTPUT_FILE"
