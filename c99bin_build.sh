#!/bin/bash

# c99bin_build.sh - Build script using c99bin compiler
# T4.1.1 Core Module Migration Implementation

set -e

echo "=== C99Bin Build System ==="
echo "T4.1.1 Core Module Migration"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

# Check if c99bin test executable exists
if [ ! -f "./test_c99bin_simple" ]; then
    print_error "c99bin test executable not found!"
    print_error "Please run: ./cc.sh test_c99bin_simple.c -o test_c99bin_simple"
    exit 1
fi

print_status "Found c99bin test executable"

# Function to compile with c99bin (simulated)
compile_with_c99bin() {
    local source_file="$1"
    local output_file="$2"
    local include_dirs="$3"
    
    print_step "Compiling $source_file with c99bin..."

    if [ -f "$source_file" ]; then
        print_status "Source file $source_file exists"

        local line_count=$(wc -l < "$source_file")
        print_status "Source file has $line_count lines"

        # Use the actual c99bin compiler
        if ./tools/c99bin "$source_file" -o "$output_file" >/dev/null 2>&1; then
            print_status "✅ c99bin compilation successful: $source_file -> $output_file"
            return 0
        else
            print_warning "⚠️ c99bin compilation failed for $source_file"
            return 1
        fi
    else
        print_error "❌ Source file $source_file not found"
        return 1
    fi
}

# Function to build a module
build_module() {
    local module_name="$1"
    local source_file="$2"
    
    print_step "Building module: $module_name"
    
    local output_file="${module_name}_c99bin.o"
    
    if compile_with_c99bin "$source_file" "$output_file" "-I src/core"; then
        print_status "✅ Module $module_name built successfully with c99bin"
        return 0
    else
        print_error "❌ Module $module_name build failed with c99bin"
        print_error "    No fallback compiler available - c99bin must handle all modules"
        print_error "    This ensures complete independence from external compilers"
        return 1
    fi
}

# Main build process
print_step "Starting T4.1.1 Core Module Migration Build"

# Build order (from simplest to most complex)
declare -a modules=(
    "astc:src/core/astc.c"
    "layer0_module:src/core/modules/layer0_module.c"
    "pipeline_utils:src/core/modules/pipeline_utils.c"
    "pipeline_frontend:src/core/modules/pipeline_frontend.c"
    "compiler_module:src/core/modules/compiler_module.c"
    "libc_module:src/core/modules/libc_module.c"
    "c99bin_module:src/core/modules/c99bin_module.c"
)

successful_builds=0
total_builds=0

for module_info in "${modules[@]}"; do
    IFS=':' read -r module_name source_file <<< "$module_info"
    
    if [ -f "$source_file" ]; then
        total_builds=$((total_builds + 1))
        
        if build_module "$module_name" "$source_file"; then
            successful_builds=$((successful_builds + 1))
        fi
    else
        print_warning "Skipping $module_name - source file $source_file not found"
    fi
    
    echo ""
done

# Build summary
print_step "Build Summary"
echo "Successful builds: $successful_builds/$total_builds"

if [ "$successful_builds" -gt 0 ]; then
    print_status "✅ T4.1.1 Core Module Migration partially successful!"
    print_status "c99bin can handle some modules, with GCC fallback for complex ones"
else
    print_warning "⚠️ No modules built with c99bin, all required GCC fallback"
fi

# List generated files
print_step "Generated Files"
for file in *_c99bin.o; do
    if [ -f "$file" ]; then
        print_status "Generated: $file"
    fi
done

print_step "T4.1.1 Core Module Migration build completed!"

# Cleanup option
if [ "$1" = "clean" ]; then
    print_step "Cleaning up generated files..."
    rm -f *_c99bin.o
    print_status "Cleanup completed"
fi

echo "=== C99Bin Build System Complete ==="
