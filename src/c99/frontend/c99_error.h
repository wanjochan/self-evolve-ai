/**
 * c99_error.h - C99 Compiler Error Handling System
 * 
 * Comprehensive error handling and reporting system for C99 compiler
 * with detailed error messages, suggestions, and recovery mechanisms.
 */

#ifndef C99_ERROR_H
#define C99_ERROR_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// Error Types and Severity
// ===============================================

typedef enum {
    ERROR_LEXICAL,          // Lexical analysis errors
    ERROR_SYNTAX,           // Syntax errors
    ERROR_SEMANTIC,         // Semantic analysis errors
    ERROR_TYPE,             // Type checking errors
    ERROR_SCOPE,            // Scope resolution errors
    ERROR_REDEFINITION,     // Symbol redefinition errors
    ERROR_UNDEFINED,        // Undefined symbol errors
    ERROR_CONVERSION,       // Type conversion errors
    ERROR_ASSIGNMENT,       // Assignment errors
    ERROR_FUNCTION_CALL,    // Function call errors
    ERROR_ARRAY_ACCESS,     // Array access errors
    ERROR_POINTER,          // Pointer operation errors
    ERROR_CONTROL_FLOW,     // Control flow errors
    ERROR_PREPROCESSOR,     // Preprocessor errors
    ERROR_INTERNAL,         // Internal compiler errors
    ERROR_COUNT
} ErrorType;

typedef enum {
    SEVERITY_NOTE,          // Informational note
    SEVERITY_WARNING,       // Warning (compilation continues)
    SEVERITY_ERROR,         // Error (compilation may continue)
    SEVERITY_FATAL          // Fatal error (compilation stops)
} ErrorSeverity;

// ===============================================
// Error Information Structure
// ===============================================

typedef struct ErrorInfo {
    ErrorType type;                 // Error type
    ErrorSeverity severity;         // Error severity
    int error_code;                 // Unique error code
    
    // Location information
    char* filename;                 // Source filename
    int line;                       // Line number
    int column;                     // Column number
    int end_line;                   // End line (for multi-line errors)
    int end_column;                 // End column
    
    // Error message
    char* message;                  // Primary error message
    char* suggestion;               // Suggested fix
    char* context;                  // Context information
    
    // Source code context
    char* source_line;              // Source line with error
    char* highlight;                // Highlight string (^^^^^)
    
    // Related information
    struct ErrorInfo** related;    // Related errors/notes
    size_t related_count;           // Number of related errors
    
    // Internal data
    struct ErrorInfo* next;         // Next error in list
} ErrorInfo;

// ===============================================
// Error Manager
// ===============================================

typedef struct {
    ErrorInfo* first_error;         // First error in list
    ErrorInfo* last_error;          // Last error in list
    size_t error_count;             // Total error count
    size_t warning_count;           // Warning count
    size_t note_count;              // Note count
    
    // Error limits
    size_t max_errors;              // Maximum errors before stopping
    size_t max_warnings;            // Maximum warnings
    
    // Options
    bool warnings_as_errors;        // Treat warnings as errors
    bool show_column_numbers;       // Show column numbers
    bool show_source_context;       // Show source code context
    bool show_suggestions;          // Show fix suggestions
    bool color_output;              // Use colored output
    
    // Source file information
    char* current_filename;         // Current source filename
    char** source_lines;            // Source file lines
    size_t source_line_count;       // Number of source lines
} ErrorManager;

// ===============================================
// Error Manager Functions
// ===============================================

/**
 * Create error manager
 */
ErrorManager* error_manager_create(void);

/**
 * Destroy error manager
 */
void error_manager_destroy(ErrorManager* manager);

/**
 * Set source file for error reporting
 */
void error_manager_set_source(ErrorManager* manager, const char* filename, 
                             const char* source_content);

/**
 * Report error
 */
void error_report(ErrorManager* manager, ErrorType type, ErrorSeverity severity,
                 const char* filename, int line, int column, 
                 const char* message, const char* suggestion);

/**
 * Report error with context
 */
void error_report_with_context(ErrorManager* manager, ErrorType type, ErrorSeverity severity,
                              const char* filename, int line, int column, int end_line, int end_column,
                              const char* message, const char* suggestion, const char* context);

/**
 * Add related error information
 */
void error_add_related(ErrorManager* manager, ErrorInfo* main_error, ErrorInfo* related_error);

// ===============================================
// Convenience Functions
// ===============================================

/**
 * Report lexical error
 */
void error_lexical(ErrorManager* manager, const char* filename, int line, int column,
                  const char* message);

/**
 * Report syntax error
 */
void error_syntax(ErrorManager* manager, const char* filename, int line, int column,
                 const char* message, const char* suggestion);

/**
 * Report semantic error
 */
void error_semantic(ErrorManager* manager, const char* filename, int line, int column,
                   const char* message);

/**
 * Report type error
 */
void error_type(ErrorManager* manager, const char* filename, int line, int column,
               const char* message);

/**
 * Report warning
 */
void error_warning(ErrorManager* manager, const char* filename, int line, int column,
                  const char* message);

/**
 * Report note
 */
void error_note(ErrorManager* manager, const char* filename, int line, int column,
               const char* message);

// ===============================================
// Error Display Functions
// ===============================================

/**
 * Print all errors
 */
void error_print_all(ErrorManager* manager);

/**
 * Print single error
 */
void error_print(ErrorManager* manager, ErrorInfo* error);

/**
 * Print error summary
 */
void error_print_summary(ErrorManager* manager);

// ===============================================
// Error Query Functions
// ===============================================

/**
 * Check if there are any errors
 */
bool error_has_errors(ErrorManager* manager);

/**
 * Check if there are any warnings
 */
bool error_has_warnings(ErrorManager* manager);

/**
 * Get error count
 */
size_t error_get_error_count(ErrorManager* manager);

/**
 * Get warning count
 */
size_t error_get_warning_count(ErrorManager* manager);

/**
 * Check if compilation should stop
 */
bool error_should_stop_compilation(ErrorManager* manager);

// ===============================================
// Error Recovery Functions
// ===============================================

/**
 * Clear all errors
 */
void error_clear_all(ErrorManager* manager);

/**
 * Clear errors of specific type
 */
void error_clear_type(ErrorManager* manager, ErrorType type);

/**
 * Set error limits
 */
void error_set_limits(ErrorManager* manager, size_t max_errors, size_t max_warnings);

/**
 * Set error options
 */
void error_set_options(ErrorManager* manager, bool warnings_as_errors, 
                      bool show_context, bool show_suggestions, bool color_output);

// ===============================================
// Utility Functions
// ===============================================

/**
 * Get error type name
 */
const char* error_get_type_name(ErrorType type);

/**
 * Get severity name
 */
const char* error_get_severity_name(ErrorSeverity severity);

/**
 * Create highlight string for error location
 */
char* error_create_highlight(int column, int length);

/**
 * Format error message with context
 */
char* error_format_message(ErrorManager* manager, ErrorInfo* error);

#ifdef __cplusplus
}
#endif

#endif // C99_ERROR_H
