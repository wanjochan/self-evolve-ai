/**
 * c99_debug.c - C99 Debug Information Generator Implementation
 */

#include "c99_debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static void debug_free_function_info(FunctionInfo* func);
static void debug_free_variable_info(VariableInfo* var);

// ===============================================
// Debug Context Management
// ===============================================

DebugContext* debug_create(DebugInfoLevel level, DebugFormat format, TargetInfo* target) {
    DebugContext* debug = malloc(sizeof(DebugContext));
    if (!debug) return NULL;
    
    memset(debug, 0, sizeof(DebugContext));
    
    debug->level = level;
    debug->format = format;
    debug->target = target;
    
    // Initialize tables
    debug->location_capacity = 1024;
    debug->locations = malloc(sizeof(SourceLocation*) * debug->location_capacity);
    
    debug->function_capacity = 256;
    debug->functions = malloc(sizeof(FunctionInfo*) * debug->function_capacity);
    
    debug->global_capacity = 512;
    debug->globals = malloc(sizeof(VariableInfo*) * debug->global_capacity);
    
    debug->debug_data_capacity = 8192;
    debug->debug_data = malloc(debug->debug_data_capacity);
    
    if (!debug->locations || !debug->functions || !debug->globals || !debug->debug_data) {
        debug_destroy(debug);
        return NULL;
    }
    
    // Set default options
    debug->include_source = (level >= DEBUG_INFO_STANDARD);
    debug->compress_debug = false;
    debug->strip_unused = (level == DEBUG_INFO_MINIMAL);
    
    printf("Debug: Created debug context (level %d, format %d)\n", level, format);
    
    return debug;
}

void debug_destroy(DebugContext* debug) {
    if (!debug) return;
    
    // Free source locations
    for (size_t i = 0; i < debug->location_count; i++) {
        if (debug->locations[i]) {
            if (debug->locations[i]->filename) {
                free(debug->locations[i]->filename);
            }
            free(debug->locations[i]);
        }
    }
    if (debug->locations) free(debug->locations);
    
    // Free functions
    for (size_t i = 0; i < debug->function_count; i++) {
        if (debug->functions[i]) {
            debug_free_function_info(debug->functions[i]);
        }
    }
    if (debug->functions) free(debug->functions);
    
    // Free globals
    for (size_t i = 0; i < debug->global_count; i++) {
        if (debug->globals[i]) {
            debug_free_variable_info(debug->globals[i]);
        }
    }
    if (debug->globals) free(debug->globals);
    
    // Free source files
    for (size_t i = 0; i < debug->source_file_count; i++) {
        if (debug->source_files[i]) {
            free(debug->source_files[i]);
        }
    }
    if (debug->source_files) free(debug->source_files);
    
    if (debug->debug_data) free(debug->debug_data);
    
    free(debug);
}

static void debug_free_function_info(FunctionInfo* func) {
    if (!func) return;
    
    if (func->name) free(func->name);
    if (func->mangled_name) free(func->mangled_name);
    if (func->return_type) free(func->return_type);
    if (func->declaration) free(func->declaration);
    if (func->definition) free(func->definition);
    
    for (size_t i = 0; i < func->parameter_count; i++) {
        if (func->parameters[i]) {
            debug_free_variable_info(func->parameters[i]);
        }
    }
    if (func->parameters) free(func->parameters);
    
    for (size_t i = 0; i < func->local_count; i++) {
        if (func->locals[i]) {
            debug_free_variable_info(func->locals[i]);
        }
    }
    if (func->locals) free(func->locals);
    
    free(func);
}

static void debug_free_variable_info(VariableInfo* var) {
    if (!var) return;
    
    if (var->name) free(var->name);
    if (var->type_name) free(var->type_name);
    if (var->declaration) free(var->declaration);
    
    free(var);
}

void debug_set_options(DebugContext* debug, bool include_source, 
                      bool compress, bool strip_unused) {
    if (!debug) return;
    
    debug->include_source = include_source;
    debug->compress_debug = compress;
    debug->strip_unused = strip_unused;
    
    printf("Debug: Set options - include_source: %s, compress: %s, strip_unused: %s\n",
           include_source ? "yes" : "no",
           compress ? "yes" : "no", 
           strip_unused ? "yes" : "no");
}

// ===============================================
// Debug Information Generation
// ===============================================

bool debug_generate_info(DebugContext* debug, struct ASTNode* ast) {
    if (!debug || !ast) return false;
    
    printf("Debug: Generating debug information\n");
    
    if (debug->level == DEBUG_INFO_NONE) {
        return true;
    }
    
    bool result = debug_generate_translation_unit(debug, ast);
    
    if (result) {
        debug_emit_header(debug);
        debug_emit_source_files(debug);
        debug_emit_line_numbers(debug);
        debug_emit_symbols(debug);
        debug_finalize(debug);
        
        printf("Debug: Generated %zu bytes of debug information\n", debug->debug_data_size);
    }
    
    return result;
}

bool debug_generate_translation_unit(DebugContext* debug, struct ASTNode* ast) {
    if (!debug || !ast) return false;
    
    printf("Debug: Processing translation unit for debug info\n");
    
    // TODO: Process all external declarations
    
    return true;
}

bool debug_generate_function(DebugContext* debug, struct ASTNode* func) {
    if (!debug || !func) return false;
    
    // Create source location for function
    SourceLocation* location = malloc(sizeof(SourceLocation));
    if (!location) return false;
    
    memset(location, 0, sizeof(SourceLocation));
    location->filename = strdup("source.c"); // Placeholder
    location->line = func ? func->line : 1; // Get from AST node
    location->column = func ? func->column : 1; // Get from AST node
    location->bytecode_offset = 0; // Will be set during code generation
    
    // Add function information
    FunctionInfo* func_info = debug_add_function(debug, "function", "int", location);
    
    if (!func_info) {
        free(location);
        return false;
    }
    
    func_info->definition = location;
    func_info->is_static = false;
    func_info->is_inline = false;
    
    printf("Debug: Added function debug info\n");
    
    return true;
}

bool debug_generate_variable(DebugContext* debug, struct ASTNode* var) {
    if (!debug || !var) return false;
    
    printf("Debug: Processing variable declaration\n");
    return true;
}

// ===============================================
// Source Location Management
// ===============================================

void debug_add_location(DebugContext* debug, const char* filename, 
                       int line, int column, size_t bytecode_offset) {
    if (!debug || !filename) return;
    
    // Expand location table if needed
    if (debug->location_count >= debug->location_capacity) {
        debug->location_capacity *= 2;
        debug->locations = realloc(debug->locations, 
                                  sizeof(SourceLocation*) * debug->location_capacity);
    }
    
    // Create new location
    SourceLocation* location = malloc(sizeof(SourceLocation));
    if (!location) return;
    
    memset(location, 0, sizeof(SourceLocation));
    location->filename = strdup(filename);
    location->line = line;
    location->column = column;
    location->bytecode_offset = bytecode_offset;
    
    debug->locations[debug->location_count++] = location;
}

SourceLocation* debug_find_location(DebugContext* debug, size_t bytecode_offset) {
    if (!debug) return NULL;
    
    SourceLocation* closest = NULL;
    for (size_t i = 0; i < debug->location_count; i++) {
        SourceLocation* loc = debug->locations[i];
        if (loc->bytecode_offset <= bytecode_offset) {
            if (!closest || loc->bytecode_offset > closest->bytecode_offset) {
                closest = loc;
            }
        }
    }
    
    return closest;
}

int debug_get_source_line(DebugContext* debug, size_t bytecode_offset) {
    SourceLocation* location = debug_find_location(debug, bytecode_offset);
    return location ? location->line : 0;
}

// ===============================================
// Symbol Information Management
// ===============================================

FunctionInfo* debug_add_function(DebugContext* debug, const char* name,
                                const char* return_type, SourceLocation* location) {
    if (!debug || !name) return NULL;
    
    // Expand function table if needed
    if (debug->function_count >= debug->function_capacity) {
        debug->function_capacity *= 2;
        debug->functions = realloc(debug->functions,
                                  sizeof(FunctionInfo*) * debug->function_capacity);
    }
    
    // Create new function info
    FunctionInfo* func = malloc(sizeof(FunctionInfo));
    if (!func) return NULL;
    
    memset(func, 0, sizeof(FunctionInfo));
    func->name = strdup(name);
    func->return_type = return_type ? strdup(return_type) : NULL;
    func->declaration = location;
    
    debug->functions[debug->function_count++] = func;
    
    return func;
}

FunctionInfo* debug_find_function(DebugContext* debug, const char* name) {
    if (!debug || !name) return NULL;
    
    for (size_t i = 0; i < debug->function_count; i++) {
        if (strcmp(debug->functions[i]->name, name) == 0) {
            return debug->functions[i];
        }
    }
    
    return NULL;
}

// ===============================================
// Debug Data Emission
// ===============================================

static void debug_ensure_capacity(DebugContext* debug, size_t additional) {
    if (debug->debug_data_size + additional > debug->debug_data_capacity) {
        while (debug->debug_data_size + additional > debug->debug_data_capacity) {
            debug->debug_data_capacity *= 2;
        }
        debug->debug_data = realloc(debug->debug_data, debug->debug_data_capacity);
    }
}

static void debug_emit_bytes(DebugContext* debug, const void* data, size_t size) {
    debug_ensure_capacity(debug, size);
    memcpy(debug->debug_data + debug->debug_data_size, data, size);
    debug->debug_data_size += size;
}

void debug_emit_header(DebugContext* debug) {
    if (!debug) return;
    
    const char signature[] = "ASTCDBG1";
    debug_emit_bytes(debug, signature, 8);
    
    uint32_t level = (uint32_t)debug->level;
    debug_emit_bytes(debug, &level, 4);
    
    uint32_t format = (uint32_t)debug->format;
    debug_emit_bytes(debug, &format, 4);
    
    printf("Debug: Emitted debug header\n");
}

void debug_emit_source_files(DebugContext* debug) {
    if (!debug) return;
    
    uint32_t count = (uint32_t)debug->source_file_count;
    debug_emit_bytes(debug, &count, 4);
    
    printf("Debug: Emitted %zu source files\n", debug->source_file_count);
}

void debug_emit_line_numbers(DebugContext* debug) {
    if (!debug) return;
    
    uint32_t count = (uint32_t)debug->location_count;
    debug_emit_bytes(debug, &count, 4);
    
    printf("Debug: Emitted %zu line number entries\n", debug->location_count);
}

void debug_emit_symbols(DebugContext* debug) {
    if (!debug) return;
    
    uint32_t count = (uint32_t)debug->function_count;
    debug_emit_bytes(debug, &count, 4);
    
    printf("Debug: Emitted %zu function symbols\n", debug->function_count);
}

void debug_finalize(DebugContext* debug) {
    if (!debug) return;
    
    const char end_marker[] = "DBGEND";
    debug_emit_bytes(debug, end_marker, 6);
    
    printf("Debug: Finalized debug information (%zu bytes total)\n", debug->debug_data_size);
}

// ===============================================
// Utility Functions
// ===============================================

void debug_print_summary(DebugContext* debug) {
    if (!debug) return;
    
    printf("Debug Information Summary:\n");
    printf("  Level: %d\n", debug->level);
    printf("  Format: %d\n", debug->format);
    printf("  Source locations: %zu\n", debug->location_count);
    printf("  Functions: %zu\n", debug->function_count);
    printf("  Global variables: %zu\n", debug->global_count);
    printf("  Debug data size: %zu bytes\n", debug->debug_data_size);
    printf("  Errors: %d\n", debug->error_count);
}

bool debug_has_error(DebugContext* debug) {
    return debug && debug->has_error;
}

const char* debug_get_error(DebugContext* debug) {
    return debug ? debug->error_message : "Invalid debug context";
}
