/**
 * c99_error.c - C99 Compiler Error Handling System Implementation
 */

#include "c99_error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
static void error_info_free(ErrorInfo* error);

// ===============================================
// Error Type Names
// ===============================================

static const char* error_type_names[] = {
    "lexical", "syntax", "semantic", "type", "scope",
    "redefinition", "undefined", "conversion", "assignment",
    "function-call", "array-access", "pointer", "control-flow",
    "preprocessor", "internal"
};

static const char* severity_names[] = {
    "note", "warning", "error", "fatal"
};

// ===============================================
// Error Manager Functions
// ===============================================

ErrorManager* error_manager_create(void) {
    ErrorManager* manager = malloc(sizeof(ErrorManager));
    if (!manager) return NULL;
    
    memset(manager, 0, sizeof(ErrorManager));
    
    // Set default limits and options
    manager->max_errors = 20;
    manager->max_warnings = 100;
    manager->show_column_numbers = true;
    manager->show_source_context = true;
    manager->show_suggestions = true;
    manager->color_output = false;
    
    return manager;
}

void error_manager_destroy(ErrorManager* manager) {
    if (!manager) return;
    
    // Free all errors
    ErrorInfo* error = manager->first_error;
    while (error) {
        ErrorInfo* next = error->next;
        error_info_free(error);
        error = next;
    }
    
    // Free source lines
    if (manager->source_lines) {
        for (size_t i = 0; i < manager->source_line_count; i++) {
            if (manager->source_lines[i]) {
                free(manager->source_lines[i]);
            }
        }
        free(manager->source_lines);
    }
    
    if (manager->current_filename) {
        free(manager->current_filename);
    }
    
    free(manager);
}

static void error_info_free(ErrorInfo* error) {
    if (!error) return;
    
    if (error->filename) free(error->filename);
    if (error->message) free(error->message);
    if (error->suggestion) free(error->suggestion);
    if (error->context) free(error->context);
    if (error->source_line) free(error->source_line);
    if (error->highlight) free(error->highlight);
    
    if (error->related) {
        for (size_t i = 0; i < error->related_count; i++) {
            if (error->related[i]) {
                error_info_free(error->related[i]);
            }
        }
        free(error->related);
    }
    
    free(error);
}

void error_manager_set_source(ErrorManager* manager, const char* filename, 
                             const char* source_content) {
    if (!manager || !filename) return;
    
    // Set filename
    if (manager->current_filename) {
        free(manager->current_filename);
    }
    manager->current_filename = strdup(filename);
    
    // Split source into lines
    if (manager->source_lines) {
        for (size_t i = 0; i < manager->source_line_count; i++) {
            if (manager->source_lines[i]) {
                free(manager->source_lines[i]);
            }
        }
        free(manager->source_lines);
    }
    
    if (!source_content) {
        manager->source_lines = NULL;
        manager->source_line_count = 0;
        return;
    }
    
    // Count lines
    size_t line_count = 1;
    for (const char* p = source_content; *p; p++) {
        if (*p == '\n') line_count++;
    }
    
    manager->source_lines = malloc(sizeof(char*) * line_count);
    manager->source_line_count = 0;
    
    // Split into lines
    const char* line_start = source_content;
    const char* p = source_content;
    
    while (*p) {
        if (*p == '\n' || *(p + 1) == '\0') {
            size_t line_length = p - line_start;
            if (*(p + 1) == '\0' && *p != '\n') line_length++;
            
            manager->source_lines[manager->source_line_count] = malloc(line_length + 1);
            memcpy(manager->source_lines[manager->source_line_count], line_start, line_length);
            manager->source_lines[manager->source_line_count][line_length] = '\0';
            manager->source_line_count++;
            
            line_start = p + 1;
        }
        p++;
    }
}

// ===============================================
// Error Reporting Functions
// ===============================================

void error_report(ErrorManager* manager, ErrorType type, ErrorSeverity severity,
                 const char* filename, int line, int column, 
                 const char* message, const char* suggestion) {
    error_report_with_context(manager, type, severity, filename, line, column, 
                             line, column, message, suggestion, NULL);
}

void error_report_with_context(ErrorManager* manager, ErrorType type, ErrorSeverity severity,
                              const char* filename, int line, int column, int end_line, int end_column,
                              const char* message, const char* suggestion, const char* context) {
    if (!manager) return;
    
    // Check limits
    if (severity == SEVERITY_ERROR && manager->error_count >= manager->max_errors) {
        return;
    }
    if (severity == SEVERITY_WARNING && manager->warning_count >= manager->max_warnings) {
        return;
    }
    
    // Create error info
    ErrorInfo* error = malloc(sizeof(ErrorInfo));
    if (!error) return;
    
    memset(error, 0, sizeof(ErrorInfo));
    error->type = type;
    error->severity = severity;
    error->error_code = (int)type * 1000 + (int)severity;
    error->filename = filename ? strdup(filename) : NULL;
    error->line = line;
    error->column = column;
    error->end_line = end_line;
    error->end_column = end_column;
    error->message = message ? strdup(message) : NULL;
    error->suggestion = suggestion ? strdup(suggestion) : NULL;
    error->context = context ? strdup(context) : NULL;
    
    // Get source line if available
    if (manager->source_lines && line > 0 && (size_t)line <= manager->source_line_count) {
        error->source_line = strdup(manager->source_lines[line - 1]);
        error->highlight = error_create_highlight(column, end_column - column + 1);
    }
    
    // Add to error list
    if (!manager->first_error) {
        manager->first_error = error;
        manager->last_error = error;
    } else {
        manager->last_error->next = error;
        manager->last_error = error;
    }
    
    // Update counters
    switch (severity) {
        case SEVERITY_NOTE:
            manager->note_count++;
            break;
        case SEVERITY_WARNING:
            manager->warning_count++;
            break;
        case SEVERITY_ERROR:
        case SEVERITY_FATAL:
            manager->error_count++;
            break;
    }
}

// ===============================================
// Convenience Functions
// ===============================================

void error_lexical(ErrorManager* manager, const char* filename, int line, int column,
                  const char* message) {
    error_report(manager, ERROR_LEXICAL, SEVERITY_ERROR, filename, line, column, message, NULL);
}

void error_syntax(ErrorManager* manager, const char* filename, int line, int column,
                 const char* message, const char* suggestion) {
    error_report(manager, ERROR_SYNTAX, SEVERITY_ERROR, filename, line, column, message, suggestion);
}

void error_semantic(ErrorManager* manager, const char* filename, int line, int column,
                   const char* message) {
    error_report(manager, ERROR_SEMANTIC, SEVERITY_ERROR, filename, line, column, message, NULL);
}

void error_warning(ErrorManager* manager, const char* filename, int line, int column,
                  const char* message) {
    error_report(manager, ERROR_SEMANTIC, SEVERITY_WARNING, filename, line, column, message, NULL);
}

// ===============================================
// Error Display Functions
// ===============================================

void error_print_all(ErrorManager* manager) {
    if (!manager) return;
    
    ErrorInfo* error = manager->first_error;
    while (error) {
        error_print(manager, error);
        error = error->next;
    }
    
    error_print_summary(manager);
}

void error_print(ErrorManager* manager, ErrorInfo* error) {
    if (!manager || !error) return;
    
    // Print main error message
    printf("%s:%d:%d: %s: %s\n",
           error->filename ? error->filename : "<unknown>",
           error->line,
           error->column,
           error_get_severity_name(error->severity),
           error->message ? error->message : "Unknown error");
    
    // Print source context if available
    if (manager->show_source_context && error->source_line) {
        printf("  %s\n", error->source_line);
        if (error->highlight) {
            printf("  %s\n", error->highlight);
        }
    }
    
    // Print suggestion if available
    if (manager->show_suggestions && error->suggestion) {
        printf("  suggestion: %s\n", error->suggestion);
    }
    
    printf("\n");
}

void error_print_summary(ErrorManager* manager) {
    if (!manager) return;
    
    if (manager->error_count > 0 || manager->warning_count > 0) {
        printf("Compilation summary: %zu error(s), %zu warning(s)\n",
               manager->error_count, manager->warning_count);
    }
}

// ===============================================
// Utility Functions
// ===============================================

const char* error_get_type_name(ErrorType type) {
    if (type >= 0 && type < ERROR_COUNT) {
        return error_type_names[type];
    }
    return "unknown";
}

const char* error_get_severity_name(ErrorSeverity severity) {
    if (severity >= 0 && severity <= SEVERITY_FATAL) {
        return severity_names[severity];
    }
    return "unknown";
}

char* error_create_highlight(int column, int length) {
    if (column <= 0 || length <= 0) return NULL;
    
    char* highlight = malloc(column + length + 1);
    if (!highlight) return NULL;
    
    // Add spaces before highlight
    for (int i = 0; i < column - 1; i++) {
        highlight[i] = ' ';
    }
    
    // Add highlight characters
    for (int i = 0; i < length; i++) {
        highlight[column - 1 + i] = '^';
    }
    
    highlight[column - 1 + length] = '\0';
    
    return highlight;
}

bool error_has_errors(ErrorManager* manager) {
    return manager && manager->error_count > 0;
}

bool error_has_warnings(ErrorManager* manager) {
    return manager && manager->warning_count > 0;
}

size_t error_get_error_count(ErrorManager* manager) {
    return manager ? manager->error_count : 0;
}

size_t error_get_warning_count(ErrorManager* manager) {
    return manager ? manager->warning_count : 0;
}

bool error_should_stop_compilation(ErrorManager* manager) {
    return manager && (manager->error_count >= manager->max_errors);
}
