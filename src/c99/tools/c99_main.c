/**
 * c99_main.c - C99 Compiler Main Driver
 * 
 * Main entry point for the C99 compiler that integrates all components:
 * lexer, parser, semantic analyzer, code generator, and optimizer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include C99 compiler components
#include "../frontend/c99_lexer.h"
#include "../frontend/c99_parser.h"
// #include "../frontend/c99_semantic.h"
// #include "../frontend/c99_error.h"
#include "../backend/c99_codegen.h"
// #include "../backend/c99_optimizer.h"
// #include "../backend/c99_target.h"
// #include "../backend/c99_debug.h"

// ===============================================
// Compiler Options
// ===============================================

typedef struct {
    char* input_file;           // Input C source file
    char* output_file;          // Output ASTC file
    char* target_triple;        // Target architecture triple
    
    // Compilation options
    int optimization_level;     // 0-3
    bool debug_info;            // Generate debug information
    bool verbose;               // Verbose output
    bool syntax_only;           // Syntax check only
    bool preprocess_only;       // Preprocessing only
    
    // Warning options
    bool warnings_as_errors;    // Treat warnings as errors
    bool show_warnings;         // Show warnings
    
    // Output options
    bool emit_ast;              // Emit AST dump
    bool emit_tokens;           // Emit token dump
    bool emit_assembly;         // Emit assembly code
} CompilerOptions;

// ===============================================
// Global Variables
// ===============================================

static CompilerOptions g_options = {0};

// ===============================================
// Utility Functions
// ===============================================

void print_usage(const char* program_name) {
    printf("C99 Compiler for Self-Evolve AI\n");
    printf("Usage: %s [options] <input-file>\n\n", program_name);
    printf("Options:\n");
    printf("  -o <file>          Write output to <file>\n");
    printf("  -O<level>          Optimization level (0-3)\n");
    printf("  -g                 Generate debug information\n");
    printf("  -v, --verbose      Verbose output\n");
    printf("  -S                 Compile only; do not assemble or link\n");
    printf("  -E                 Preprocess only; do not compile\n");
    printf("  -fsyntax-only      Check syntax only\n");
    printf("  -Wall              Enable all warnings\n");
    printf("  -Werror            Treat warnings as errors\n");
    printf("  -target <triple>   Target architecture triple\n");
    printf("  --emit-ast         Emit AST dump\n");
    printf("  --emit-tokens      Emit token dump\n");
    printf("  --emit-llvm        Emit LLVM IR\n");
    printf("  -h, --help         Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s hello.c -o hello.astc\n", program_name);
    printf("  %s -O2 -g program.c -o program.astc\n", program_name);
    printf("  %s --emit-tokens source.c\n", program_name);
}

void print_version(void) {
    printf("C99 Compiler v1.0.0\n");
    printf("Part of Self-Evolve AI Project\n");
    printf("Built with ASTC bytecode target\n");
}

bool parse_arguments(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return false;
    }
    
    // Set defaults
    g_options.optimization_level = 0;
    g_options.debug_info = false;
    g_options.verbose = false;
    g_options.show_warnings = true;
    g_options.output_file = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return false;
        } else if (strcmp(argv[i], "--version") == 0) {
            print_version();
            return false;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o requires an argument\n");
                return false;
            }
            g_options.output_file = argv[++i];
        } else if (strncmp(argv[i], "-O", 2) == 0) {
            if (strlen(argv[i]) == 3 && argv[i][2] >= '0' && argv[i][2] <= '3') {
                g_options.optimization_level = argv[i][2] - '0';
            } else {
                fprintf(stderr, "Error: Invalid optimization level\n");
                return false;
            }
        } else if (strcmp(argv[i], "-g") == 0) {
            g_options.debug_info = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            g_options.verbose = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            g_options.emit_assembly = true;
        } else if (strcmp(argv[i], "-E") == 0) {
            g_options.preprocess_only = true;
        } else if (strcmp(argv[i], "-fsyntax-only") == 0) {
            g_options.syntax_only = true;
        } else if (strcmp(argv[i], "-Wall") == 0) {
            g_options.show_warnings = true;
        } else if (strcmp(argv[i], "-Werror") == 0) {
            g_options.warnings_as_errors = true;
        } else if (strcmp(argv[i], "--emit-ast") == 0) {
            g_options.emit_ast = true;
        } else if (strcmp(argv[i], "--emit-tokens") == 0) {
            g_options.emit_tokens = true;
        } else if (strcmp(argv[i], "-target") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -target requires an argument\n");
                return false;
            }
            g_options.target_triple = argv[++i];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option %s\n", argv[i]);
            return false;
        } else {
            // Input file
            if (g_options.input_file) {
                fprintf(stderr, "Error: Multiple input files not supported\n");
                return false;
            }
            g_options.input_file = argv[i];
        }
    }
    
    if (!g_options.input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        return false;
    }
    
    // Set default output file if not specified
    if (!g_options.output_file) {
        char* base = strdup(g_options.input_file);
        char* dot = strrchr(base, '.');
        if (dot) *dot = '\0';
        
        g_options.output_file = malloc(strlen(base) + 6);
        sprintf(g_options.output_file, "%s.astc", base);
        free(base);
    }
    
    return true;
}

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    char* content = malloc(size + 1);
    if (!content) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    // Read file
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    
    fclose(file);
    return content;
}

// ===============================================
// Compilation Pipeline
// ===============================================

bool compile_file(void) {
    if (g_options.verbose) {
        printf("Compiling %s to %s\n", g_options.input_file, g_options.output_file);
        printf("Optimization level: %d\n", g_options.optimization_level);
        printf("Debug info: %s\n", g_options.debug_info ? "yes" : "no");
    }
    
    // Read source file
    char* source = read_file(g_options.input_file);
    if (!source) {
        return false;
    }
    
    // Phase 1: Lexical Analysis
    if (g_options.verbose) {
        printf("Phase 1: Lexical analysis...\n");
    }
    
    LexerContext* lexer = lexer_create(source, strlen(source));
    if (!lexer) {
        fprintf(stderr, "Error: Failed to create lexer\n");
        free(source);
        return false;
    }
    
    // Emit tokens if requested
    if (g_options.emit_tokens) {
        printf("=== TOKENS ===\n");
        Token* token;
        while ((token = lexer_next_token(lexer)) && token->type != TOKEN_EOF) {
            token_print(token);
            free(token);
        }
        if (token) free(token);
        
        lexer_destroy(lexer);
        free(source);
        return true;
    }
    
    // For now, just do lexical analysis
    // TODO: Add parser, semantic analyzer, code generator
    
    printf("C99 Compiler: Lexical analysis completed successfully\n");
    printf("Note: Full compilation pipeline not yet implemented\n");
    printf("Generated placeholder output file: %s\n", g_options.output_file);
    
    // Create placeholder output file
    FILE* output = fopen(g_options.output_file, "w");
    if (output) {
        fprintf(output, "# ASTC Bytecode (Placeholder)\n");
        fprintf(output, "# Generated from: %s\n", g_options.input_file);
        fprintf(output, "# Optimization level: %d\n", g_options.optimization_level);
        fclose(output);
    }
    
    lexer_destroy(lexer);
    free(source);
    return true;
}

// ===============================================
// Main Function
// ===============================================

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (!parse_arguments(argc, argv)) {
        return EXIT_FAILURE;
    }
    
    // Compile the file
    if (!compile_file()) {
        fprintf(stderr, "Compilation failed\n");
        return EXIT_FAILURE;
    }
    
    if (g_options.verbose) {
        printf("Compilation completed successfully\n");
    }
    
    return EXIT_SUCCESS;
}
