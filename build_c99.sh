#!/bin/bash

# build_c99.sh - C99 Compiler Build Script
# 
# This script builds the complete C99 compiler system including:
# - Lexical analyzer
# - Syntax parser
# - Code generator
# - Error handling system
# - Test programs

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
SRC_DIR="src"
C99_DIR="$SRC_DIR/c99"
EXAMPLES_DIR="examples"
BIN_DIR="bin"
CC_SCRIPT="./cc.sh"

# Create directories
mkdir -p "$BUILD_DIR"
mkdir -p "$BIN_DIR"

echo -e "${BLUE}=== C99 Compiler Build System ===${NC}"
echo "Building C99 compiler components..."
echo

# Function to print status
print_status() {
    local status=$1
    local message=$2
    if [ "$status" = "OK" ]; then
        echo -e "${GREEN}✓${NC} $message"
    elif [ "$status" = "WARN" ]; then
        echo -e "${YELLOW}⚠${NC} $message"
    elif [ "$status" = "ERROR" ]; then
        echo -e "${RED}✗${NC} $message"
    else
        echo -e "${BLUE}•${NC} $message"
    fi
}

# Function to compile a source file
compile_source() {
    local src_file=$1
    local obj_file=$2
    local include_dirs=$3
    
    print_status "INFO" "Compiling $src_file..."
    
    if [ -n "$include_dirs" ]; then
        $CC_SCRIPT -c "$src_file" $include_dirs -o "$obj_file"
    else
        $CC_SCRIPT -c "$src_file" -o "$obj_file"
    fi
    
    if [ $? -eq 0 ]; then
        print_status "OK" "Compiled $src_file successfully"
        return 0
    else
        print_status "ERROR" "Failed to compile $src_file"
        return 1
    fi
}

# Function to build executable
build_executable() {
    local name=$1
    local obj_files=$2
    local output_file=$3
    local extra_libs=$4

    print_status "INFO" "Linking $name..."

    if [ -n "$extra_libs" ]; then
        $CC_SCRIPT $obj_files $extra_libs -o "$output_file"
    else
        $CC_SCRIPT $obj_files -o "$output_file"
    fi

    if [ $? -eq 0 ]; then
        print_status "OK" "Built $name successfully"
        return 0
    else
        print_status "ERROR" "Failed to build $name"
        return 1
    fi
}

# Phase 1: Build C99 Frontend Components
echo -e "${YELLOW}Phase 1: Building C99 Frontend Components${NC}"

# Build lexer
if [ -f "$C99_DIR/frontend/c99_lexer.c" ]; then
    compile_source "$C99_DIR/frontend/c99_lexer.c" "$BUILD_DIR/c99_lexer.o" "-I$SRC_DIR/core"
else
    print_status "WARN" "Lexer source not found, skipping"
fi

# Build parser
if [ -f "$C99_DIR/frontend/c99_parser.c" ]; then
    compile_source "$C99_DIR/frontend/c99_parser.c" "$BUILD_DIR/c99_parser.o" "-I$SRC_DIR/core"
else
    print_status "WARN" "Parser source not found, skipping"
fi

# Build error handler
if [ -f "$C99_DIR/frontend/c99_error.c" ]; then
    compile_source "$C99_DIR/frontend/c99_error.c" "$BUILD_DIR/c99_error.o" "-I$SRC_DIR/core"
else
    print_status "WARN" "Error handler source not found, skipping"
fi

# Build semantic analyzer (AST compatibility issues fixed)
if [ -f "$C99_DIR/frontend/c99_semantic.c" ]; then
    compile_source "$C99_DIR/frontend/c99_semantic.c" "$BUILD_DIR/c99_semantic.o" "-I$SRC_DIR/core"
else
    print_status "WARN" "Semantic analyzer source not found, skipping"
fi

echo

# Phase 2: Build C99 Backend Components
echo -e "${YELLOW}Phase 2: Building C99 Backend Components${NC}"

# Build code generator
if [ -f "$C99_DIR/backend/c99_codegen.c" ]; then
    compile_source "$C99_DIR/backend/c99_codegen.c" "$BUILD_DIR/c99_codegen.o" "-I$SRC_DIR/core"
else
    print_status "WARN" "Code generator source not found, skipping"
fi

# Build optimizer
if [ -f "$C99_DIR/backend/c99_optimizer.c" ]; then
    compile_source "$C99_DIR/backend/c99_optimizer.c" "$BUILD_DIR/c99_optimizer.o" "-I$SRC_DIR/core"
else
    print_status "WARN" "Optimizer source not found, skipping"
fi

# Build target support
if [ -f "$C99_DIR/backend/c99_target.c" ]; then
    compile_source "$C99_DIR/backend/c99_target.c" "$BUILD_DIR/c99_target.o" "-I$SRC_DIR/core"
else
    print_status "WARN" "Target support source not found, skipping"
fi

# Build debug support
if [ -f "$C99_DIR/backend/c99_debug.c" ]; then
    compile_source "$C99_DIR/backend/c99_debug.c" "$BUILD_DIR/c99_debug.o" "-I$SRC_DIR/core"
else
    print_status "WARN" "Debug support source not found, skipping"
fi

echo

# Phase 3: Build Core ASTC System
echo -e "${YELLOW}Phase 3: Building Core ASTC System${NC}"

# Build core ASTC
if [ -f "$SRC_DIR/core/astc.c" ]; then
    compile_source "$SRC_DIR/core/astc.c" "$BUILD_DIR/astc.o" "-I$SRC_DIR/core"
else
    print_status "WARN" "Core ASTC source not found, skipping"
fi

echo

# Phase 4: Build C99 Main Driver
echo -e "${YELLOW}Phase 4: Building C99 Main Driver${NC}"

if [ -f "$C99_DIR/tools/c99_main.c" ]; then
    compile_source "$C99_DIR/tools/c99_main.c" "$BUILD_DIR/c99_main.o" "-I$SRC_DIR/core -I$C99_DIR/frontend -I$C99_DIR/backend"
    
    # Link C99 compiler
    OBJ_FILES="$BUILD_DIR/c99_main.o"
    if [ -f "$BUILD_DIR/c99_lexer.o" ]; then OBJ_FILES="$OBJ_FILES $BUILD_DIR/c99_lexer.o"; fi
    if [ -f "$BUILD_DIR/c99_parser.o" ]; then OBJ_FILES="$OBJ_FILES $BUILD_DIR/c99_parser.o"; fi
    if [ -f "$BUILD_DIR/c99_semantic.o" ]; then OBJ_FILES="$OBJ_FILES $BUILD_DIR/c99_semantic.o"; fi
    if [ -f "$BUILD_DIR/c99_codegen.o" ]; then OBJ_FILES="$OBJ_FILES $BUILD_DIR/c99_codegen.o"; fi
    if [ -f "$BUILD_DIR/c99_error.o" ]; then OBJ_FILES="$OBJ_FILES $BUILD_DIR/c99_error.o"; fi
    if [ -f "$BUILD_DIR/astc.o" ]; then OBJ_FILES="$OBJ_FILES $BUILD_DIR/astc.o"; fi
    
    build_executable "C99 Compiler" "$OBJ_FILES" "$BIN_DIR/c99_compiler"
else
    print_status "WARN" "C99 main driver source not found, skipping"
fi

echo

# Phase 5: Build Test Programs
echo -e "${YELLOW}Phase 5: Building Test Programs${NC}"

# List of test programs
TEST_PROGRAMS=(
    "hello_world"
    "c99_features_test"
    "performance_test"
    "c99_complex_syntax_test"
    "c99_stdlib_test"
    "c99_error_handling_test"
)

for program in "${TEST_PROGRAMS[@]}"; do
    if [ -f "$EXAMPLES_DIR/${program}.c" ]; then
        print_status "INFO" "Building test program: $program"
        compile_source "$EXAMPLES_DIR/${program}.c" "$BUILD_DIR/${program}.o" ""

        # Add math library for programs that need it
        EXTRA_LIBS=""
        if [[ "$program" == *"stdlib"* ]] || [[ "$program" == *"performance"* ]] || [[ "$program" == *"error_handling"* ]]; then
            EXTRA_LIBS="-lm"
        fi

        build_executable "$program" "$BUILD_DIR/${program}.o" "$BIN_DIR/$program" "$EXTRA_LIBS"
    else
        print_status "WARN" "Test program $program.c not found, skipping"
    fi
done

echo

# Phase 6: Build C99 Lexer Test
echo -e "${YELLOW}Phase 6: Building C99 Component Tests${NC}"

if [ -f "$C99_DIR/test_c99_lexer.c" ]; then
    compile_source "$C99_DIR/test_c99_lexer.c" "$BUILD_DIR/test_c99_lexer.o" "-I$C99_DIR/frontend"

    # Check if we have the old C99 lexer implementation
    if [ -f "$C99_DIR/c99_lexer.c" ]; then
        compile_source "$C99_DIR/c99_lexer.c" "$BUILD_DIR/c99_lexer_old.o" ""
        OBJ_FILES="$BUILD_DIR/test_c99_lexer.o $BUILD_DIR/c99_lexer_old.o"
    else
        OBJ_FILES="$BUILD_DIR/test_c99_lexer.o"
        if [ -f "$BUILD_DIR/c99_lexer.o" ]; then OBJ_FILES="$OBJ_FILES $BUILD_DIR/c99_lexer.o"; fi
    fi

    build_executable "C99 Lexer Test" "$OBJ_FILES" "$BIN_DIR/test_c99_lexer"
else
    print_status "WARN" "C99 lexer test not found, skipping"
fi

echo

# Phase 7: Verification
echo -e "${YELLOW}Phase 7: Build Verification${NC}"

# Check if key executables were built
EXECUTABLES=(
    "$BIN_DIR/c99_compiler"
    "$BIN_DIR/hello_world"
    "$BIN_DIR/c99_features_test"
    "$BIN_DIR/performance_test"
)

SUCCESS_COUNT=0
TOTAL_COUNT=${#EXECUTABLES[@]}

for exe in "${EXECUTABLES[@]}"; do
    if [ -f "$exe" ] && [ -x "$exe" ]; then
        print_status "OK" "Executable built: $(basename $exe)"
        ((SUCCESS_COUNT++))
    else
        print_status "ERROR" "Missing executable: $(basename $exe)"
    fi
done

echo

# Phase 8: Quick Test Run
echo -e "${YELLOW}Phase 8: Quick Test Run${NC}"

# Test hello_world if it exists
if [ -f "$BIN_DIR/hello_world" ] && [ -x "$BIN_DIR/hello_world" ]; then
    print_status "INFO" "Running hello_world test..."
    if timeout 10s "$BIN_DIR/hello_world" > /dev/null 2>&1; then
        print_status "OK" "hello_world test passed"
    else
        print_status "WARN" "hello_world test failed or timed out"
    fi
fi

# Test C99 lexer if it exists
if [ -f "$BIN_DIR/test_c99_lexer" ] && [ -x "$BIN_DIR/test_c99_lexer" ]; then
    print_status "INFO" "Running C99 lexer test..."
    if timeout 10s "$BIN_DIR/test_c99_lexer" > /dev/null 2>&1; then
        print_status "OK" "C99 lexer test passed"
    else
        print_status "WARN" "C99 lexer test failed or timed out"
    fi
fi

echo

# Summary
echo -e "${BLUE}=== Build Summary ===${NC}"
echo "Built executables: $SUCCESS_COUNT/$TOTAL_COUNT"
echo "Build directory: $BUILD_DIR"
echo "Binary directory: $BIN_DIR"

if [ $SUCCESS_COUNT -eq $TOTAL_COUNT ]; then
    print_status "OK" "All components built successfully!"
    echo
    echo "Next steps:"
    echo "  1. Run './build_core.sh' to build the core system"
    echo "  2. Test the C99 compiler: '$BIN_DIR/c99_compiler --help'"
    echo "  3. Run test programs in $BIN_DIR/"
    exit 0
else
    print_status "WARN" "Some components failed to build"
    echo
    echo "Check the error messages above and ensure all dependencies are available."
    exit 1
fi
