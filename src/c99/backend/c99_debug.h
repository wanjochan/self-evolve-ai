/**
 * c99_debug.h - C99 Debug Information Generator
 * 
 * Debug information generation for C99 compiler including source mapping,
 * variable information, and debugging metadata for ASTC bytecode.
 */

#ifndef C99_DEBUG_H
#define C99_DEBUG_H

#include "../../core/astc.h"
#include "c99_target.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// Debug Information Types
// ===============================================

typedef enum {
    DEBUG_INFO_NONE = 0,        // No debug information
    DEBUG_INFO_MINIMAL,         // Minimal debug info (line numbers only)
    DEBUG_INFO_STANDARD,        // Standard debug info (DWARF-like)
    DEBUG_INFO_FULL            // Full debug info with optimizations
} DebugInfoLevel;

typedef enum {
    DEBUG_FORMAT_CUSTOM = 0,    // Custom ASTC debug format
    DEBUG_FORMAT_DWARF,         // DWARF debug format
    DEBUG_FORMAT_PDB,           // Microsoft PDB format
    DEBUG_FORMAT_STABS          // STABS debug format
} DebugFormat;

// ===============================================
// Source Location Information
// ===============================================

typedef struct {
    char* filename;             // Source filename
    int line;                   // Line number (1-based)
    int column;                 // Column number (1-based)
    int end_line;               // End line number
    int end_column;             // End column number
    size_t bytecode_offset;     // Corresponding bytecode offset
} SourceLocation;

// ===============================================
// Variable Debug Information
// ===============================================

typedef enum {
    VAR_SCOPE_GLOBAL,           // Global variable
    VAR_SCOPE_LOCAL,            // Local variable
    VAR_SCOPE_PARAMETER,        // Function parameter
    VAR_SCOPE_STATIC,           // Static variable
    VAR_SCOPE_REGISTER          // Register variable
} VariableScope;

typedef struct {
    char* name;                 // Variable name
    char* type_name;            // Type name
    VariableScope scope;        // Variable scope
    int stack_offset;           // Stack offset (for local variables)
    int register_id;            // Register ID (for register variables)
    SourceLocation* declaration; // Declaration location
    bool is_parameter;          // Is function parameter
    bool is_const;              // Is constant
    bool is_volatile;           // Is volatile
    size_t size;                // Variable size in bytes
} VariableInfo;

// ===============================================
// Function Debug Information
// ===============================================

typedef struct {
    char* name;                 // Function name
    char* mangled_name;         // Mangled name (if any)
    char* return_type;          // Return type name
    SourceLocation* declaration; // Declaration location
    SourceLocation* definition;  // Definition location
    size_t bytecode_start;      // Function start in bytecode
    size_t bytecode_end;        // Function end in bytecode
    VariableInfo** parameters;  // Function parameters
    size_t parameter_count;     // Number of parameters
    VariableInfo** locals;      // Local variables
    size_t local_count;         // Number of local variables
    bool is_inline;             // Is inline function
    bool is_static;             // Is static function
} FunctionInfo;

// ===============================================
// Debug Context
// ===============================================

typedef struct {
    DebugInfoLevel level;       // Debug information level
    DebugFormat format;         // Debug format
    TargetInfo* target;         // Target information
    
    // Source mapping
    SourceLocation** locations; // Source location table
    size_t location_count;      // Number of locations
    size_t location_capacity;   // Location table capacity
    
    // Symbol information
    FunctionInfo** functions;   // Function information
    size_t function_count;      // Number of functions
    size_t function_capacity;   // Function table capacity
    
    VariableInfo** globals;     // Global variables
    size_t global_count;        // Number of globals
    size_t global_capacity;     // Global table capacity
    
    // Source files
    char** source_files;        // Source file list
    size_t source_file_count;   // Number of source files
    
    // Debug data
    uint8_t* debug_data;        // Generated debug data
    size_t debug_data_size;     // Debug data size
    size_t debug_data_capacity; // Debug data capacity
    
    // Options
    bool include_source;        // Include source code in debug info
    bool compress_debug;        // Compress debug information
    bool strip_unused;          // Strip unused debug information
    
    // Error handling
    char error_message[512];
    bool has_error;
    int error_count;
} DebugContext;

// ===============================================
// Debug Context Management
// ===============================================

/**
 * Create debug context
 */
DebugContext* debug_create(DebugInfoLevel level, DebugFormat format, TargetInfo* target);

/**
 * Destroy debug context
 */
void debug_destroy(DebugContext* debug);

/**
 * Set debug options
 */
void debug_set_options(DebugContext* debug, bool include_source, 
                      bool compress, bool strip_unused);

// ===============================================
// Debug Information Generation
// ===============================================

/**
 * Generate debug information for AST
 */
bool debug_generate_info(DebugContext* debug, struct ASTNode* ast);

/**
 * Generate debug information for translation unit
 */
bool debug_generate_translation_unit(DebugContext* debug, struct ASTNode* ast);

/**
 * Generate debug information for function
 */
bool debug_generate_function(DebugContext* debug, struct ASTNode* func);

/**
 * Generate debug information for variable
 */
bool debug_generate_variable(DebugContext* debug, struct ASTNode* var);

// ===============================================
// Source Location Management
// ===============================================

/**
 * Add source location mapping
 */
void debug_add_location(DebugContext* debug, const char* filename, 
                       int line, int column, size_t bytecode_offset);

/**
 * Find source location by bytecode offset
 */
SourceLocation* debug_find_location(DebugContext* debug, size_t bytecode_offset);

/**
 * Get source line for bytecode offset
 */
int debug_get_source_line(DebugContext* debug, size_t bytecode_offset);

// ===============================================
// Symbol Information Management
// ===============================================

/**
 * Add function information
 */
FunctionInfo* debug_add_function(DebugContext* debug, const char* name,
                                const char* return_type, SourceLocation* location);

/**
 * Add variable information
 */
VariableInfo* debug_add_variable(DebugContext* debug, const char* name,
                                const char* type_name, VariableScope scope,
                                SourceLocation* location);

/**
 * Find function by name
 */
FunctionInfo* debug_find_function(DebugContext* debug, const char* name);

/**
 * Find variable by name
 */
VariableInfo* debug_find_variable(DebugContext* debug, const char* name);

// ===============================================
// Debug Data Emission
// ===============================================

/**
 * Emit debug data header
 */
void debug_emit_header(DebugContext* debug);

/**
 * Emit source file table
 */
void debug_emit_source_files(DebugContext* debug);

/**
 * Emit line number table
 */
void debug_emit_line_numbers(DebugContext* debug);

/**
 * Emit symbol table
 */
void debug_emit_symbols(DebugContext* debug);

/**
 * Finalize debug data
 */
void debug_finalize(DebugContext* debug);

// ===============================================
// Debug Data Output
// ===============================================

/**
 * Get debug data
 */
uint8_t* debug_get_data(DebugContext* debug, size_t* size);

/**
 * Write debug data to file
 */
bool debug_write_to_file(DebugContext* debug, const char* filename);

// ===============================================
// Utility Functions
// ===============================================

/**
 * Print debug information summary
 */
void debug_print_summary(DebugContext* debug);

/**
 * Print source location table
 */
void debug_print_locations(DebugContext* debug);

/**
 * Print symbol table
 */
void debug_print_symbols(DebugContext* debug);

/**
 * Validate debug information
 */
bool debug_validate(DebugContext* debug);

/**
 * Check if debug context has error
 */
bool debug_has_error(DebugContext* debug);

/**
 * Get error message
 */
const char* debug_get_error(DebugContext* debug);

#ifdef __cplusplus
}
#endif

#endif // C99_DEBUG_H
