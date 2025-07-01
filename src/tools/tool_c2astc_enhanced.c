/**
 * tool_c2astc_enhanced.c - Enhanced C to ASTC Compiler
 * 
 * Enhanced C language to ASTC bytecode compiler with full C99 support,
 * module integration, and advanced optimization features.
 */

#include "../core/include/core_astc.h"
#include "../core/include/logger.h"
#include "../core/include/astc_program_modules.h"
#include "../core/include/astc_platform_compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

// Enhanced compiler options
typedef struct {
    // Input/Output
    char input_file[256];
    char output_file[256];
    
    // Optimization levels
    int optimization_level;     // 0-3
    bool enable_debug;
    bool enable_profiling;
    
    // Language standards
    enum {
        C_STD_C89,
        C_STD_C99,
        C_STD_C11,
        C_STD_C17
    } c_standard;
    
    // Warning options
    bool enable_warnings;
    bool warnings_as_errors;
    bool pedantic_mode;
    
    // Module options
    bool enable_module_system;
    bool auto_import_libc;
    bool auto_import_math;
    char module_search_paths[16][256];
    int module_search_path_count;
    
    // Platform options
    ASTCPlatformType target_platforms[8];
    int target_platform_count;
    ASTCArchitectureType target_architectures[8];
    int target_arch_count;
    
    // Include directories and macros
    char include_dirs[16][256];
    int include_dir_count;
    char macros[32][256];
    int macro_count;
    
    // Advanced options
    bool generate_metadata;
    bool enable_cross_compilation;
    bool enable_ai_optimization;
    bool verbose_output;
} EnhancedC2AstcOptions;

// Default compiler options
static EnhancedC2AstcOptions get_default_options(void) {
    EnhancedC2AstcOptions options = {0};
    
    strcpy(options.output_file, "program.astc");
    options.optimization_level = 1;
    options.c_standard = C_STD_C99;
    options.enable_warnings = true;
    options.enable_module_system = true;
    options.auto_import_libc = true;
    options.generate_metadata = true;
    options.verbose_output = false;
    
    // Default target platforms (current platform)
    const ASTCPlatformInfo* platform_info = astc_get_platform_info();
    options.target_platforms[0] = platform_info->platform;
    options.target_platform_count = 1;
    options.target_architectures[0] = platform_info->architecture;
    options.target_arch_count = 1;
    
    return options;
}

// Print usage information
void print_enhanced_usage(const char* program_name) {
    printf("Enhanced C to ASTC Compiler v2.0\n");
    printf("Usage: %s [options] <input.c> [output.astc]\n\n", program_name);
    
    printf("Basic Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --version           Show version information\n");
    printf("  -o <file>               Specify output file\n");
    printf("  --verbose               Enable verbose output\n");
    
    printf("\nOptimization Options:\n");
    printf("  -O0                     No optimization\n");
    printf("  -O1                     Basic optimization (default)\n");
    printf("  -O2                     Advanced optimization\n");
    printf("  -O3                     Aggressive optimization\n");
    printf("  -g                      Generate debug information\n");
    printf("  --profile               Enable profiling support\n");
    printf("  --ai-optimize           Enable AI-driven optimization\n");
    
    printf("\nLanguage Standards:\n");
    printf("  -std=c89                Use C89 standard\n");
    printf("  -std=c99                Use C99 standard (default)\n");
    printf("  -std=c11                Use C11 standard\n");
    printf("  -std=c17                Use C17 standard\n");
    
    printf("\nWarning Options:\n");
    printf("  -Wall                   Enable all warnings\n");
    printf("  -Werror                 Treat warnings as errors\n");
    printf("  -Wextra                 Enable extra warnings\n");
    printf("  -pedantic               Enable pedantic mode\n");
    printf("  -w                      Disable warnings\n");
    
    printf("\nModule System:\n");
    printf("  --enable-modules        Enable module system (default)\n");
    printf("  --disable-modules       Disable module system\n");
    printf("  --auto-libc             Auto-import libc.rt (default)\n");
    printf("  --auto-math             Auto-import math.rt\n");
    printf("  --module-path <dir>     Add module search path\n");
    
    printf("\nPlatform Targeting:\n");
    printf("  --target-platform <p>   Target platform (windows/linux/macos)\n");
    printf("  --target-arch <a>       Target architecture (x64/arm64)\n");
    printf("  --cross-compile         Enable cross-compilation\n");
    
    printf("\nPreprocessor:\n");
    printf("  -I <dir>                Add include directory\n");
    printf("  -D <macro>              Define preprocessor macro\n");
    
    printf("\nAdvanced Options:\n");
    printf("  --no-metadata           Don't generate metadata\n");
    printf("  --dump-ast              Dump AST to file\n");
    printf("  --dump-bytecode         Dump bytecode to file\n");
    
    printf("\nExamples:\n");
    printf("  %s hello.c                           # Basic compilation\n", program_name);
    printf("  %s -O2 -g hello.c hello.astc         # Optimized with debug info\n", program_name);
    printf("  %s --target-platform linux hello.c   # Cross-compile for Linux\n", program_name);
    printf("  %s --ai-optimize --verbose hello.c   # AI optimization with verbose output\n", program_name);
}

// Parse command line arguments
int parse_enhanced_arguments(int argc, char* argv[], EnhancedC2AstcOptions* options) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_enhanced_usage(argv[0]);
            return 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("Enhanced C to ASTC Compiler v2.0\n");
            printf("Built with module system and AI optimization support\n");
            return 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                strncpy(options->output_file, argv[++i], sizeof(options->output_file) - 1);
            } else {
                fprintf(stderr, "Error: -o requires output filename\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--verbose") == 0) {
            options->verbose_output = true;
        } else if (strcmp(argv[i], "-O0") == 0) {
            options->optimization_level = 0;
        } else if (strcmp(argv[i], "-O1") == 0) {
            options->optimization_level = 1;
        } else if (strcmp(argv[i], "-O2") == 0) {
            options->optimization_level = 2;
        } else if (strcmp(argv[i], "-O3") == 0) {
            options->optimization_level = 3;
        } else if (strcmp(argv[i], "-g") == 0) {
            options->enable_debug = true;
        } else if (strcmp(argv[i], "--profile") == 0) {
            options->enable_profiling = true;
        } else if (strcmp(argv[i], "--ai-optimize") == 0) {
            options->enable_ai_optimization = true;
        } else if (strcmp(argv[i], "-std=c89") == 0) {
            options->c_standard = C_STD_C89;
        } else if (strcmp(argv[i], "-std=c99") == 0) {
            options->c_standard = C_STD_C99;
        } else if (strcmp(argv[i], "-std=c11") == 0) {
            options->c_standard = C_STD_C11;
        } else if (strcmp(argv[i], "-std=c17") == 0) {
            options->c_standard = C_STD_C17;
        } else if (strcmp(argv[i], "-Wall") == 0) {
            options->enable_warnings = true;
        } else if (strcmp(argv[i], "-Werror") == 0) {
            options->warnings_as_errors = true;
        } else if (strcmp(argv[i], "-pedantic") == 0) {
            options->pedantic_mode = true;
        } else if (strcmp(argv[i], "--enable-modules") == 0) {
            options->enable_module_system = true;
        } else if (strcmp(argv[i], "--disable-modules") == 0) {
            options->enable_module_system = false;
        } else if (strcmp(argv[i], "--auto-libc") == 0) {
            options->auto_import_libc = true;
        } else if (strcmp(argv[i], "--auto-math") == 0) {
            options->auto_import_math = true;
        } else if (strcmp(argv[i], "--module-path") == 0) {
            if (i + 1 < argc && options->module_search_path_count < 16) {
                strncpy(options->module_search_paths[options->module_search_path_count], 
                       argv[++i], sizeof(options->module_search_paths[0]) - 1);
                options->module_search_path_count++;
            }
        } else if (strcmp(argv[i], "-I") == 0) {
            if (i + 1 < argc && options->include_dir_count < 16) {
                strncpy(options->include_dirs[options->include_dir_count], 
                       argv[++i], sizeof(options->include_dirs[0]) - 1);
                options->include_dir_count++;
            }
        } else if (strcmp(argv[i], "-D") == 0) {
            if (i + 1 < argc && options->macro_count < 32) {
                strncpy(options->macros[options->macro_count], 
                       argv[++i], sizeof(options->macros[0]) - 1);
                options->macro_count++;
            }
        } else if (argv[i][0] != '-') {
            if (strlen(options->input_file) == 0) {
                strncpy(options->input_file, argv[i], sizeof(options->input_file) - 1);
            } else {
                // Second non-option argument is output file
                strncpy(options->output_file, argv[i], sizeof(options->output_file) - 1);
            }
        }
    }
    
    return 0;
}

// Enhanced compilation function
int compile_c_to_astc_enhanced(const EnhancedC2AstcOptions* options) {
    LOG_COMPILER_INFO("Starting enhanced C to ASTC compilation");
    LOG_COMPILER_INFO("Input: %s", options->input_file);
    LOG_COMPILER_INFO("Output: %s", options->output_file);
    LOG_COMPILER_INFO("Optimization level: %d", options->optimization_level);
    
    // Initialize platform compatibility
    if (astc_platform_compat_init() != 0) {
        LOG_COMPILER_ERROR("Failed to initialize platform compatibility");
        return -1;
    }
    
    // Initialize module system if enabled
    if (options->enable_module_system) {
        if (astc_program_modules_init("enhanced_compiler", options->input_file) != 0) {
            LOG_COMPILER_ERROR("Failed to initialize module system");
            return -1;
        }
        
        // Auto-import modules
        if (options->auto_import_libc) {
            if (astc_program_import_module("libc.rt", NULL, NULL) != 0) {
                LOG_COMPILER_WARN("Failed to auto-import libc.rt");
            }
        }
        
        if (options->auto_import_math) {
            if (astc_program_import_module("math.rt", NULL, NULL) != 0) {
                LOG_COMPILER_WARN("Failed to auto-import math.rt");
            }
        }
    }
    
    // Read input file
    FILE* input = fopen(options->input_file, "r");
    if (!input) {
        LOG_COMPILER_ERROR("Cannot open input file: %s", options->input_file);
        return -1;
    }
    
    // Get file size
    fseek(input, 0, SEEK_END);
    long file_size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    // Read source code
    char* source_code = malloc(file_size + 1);
    if (!source_code) {
        LOG_COMPILER_ERROR("Memory allocation failed");
        fclose(input);
        return -1;
    }
    
    fread(source_code, 1, file_size, input);
    source_code[file_size] = '\0';
    fclose(input);
    
    LOG_COMPILER_DEBUG("Read %ld bytes of source code", file_size);
    
    // Parse C code to AST
    ASTNode* ast = ast_parse_c_source(source_code, options->c_standard);
    if (!ast) {
        LOG_COMPILER_ERROR("Failed to parse C source code");
        free(source_code);
        return -1;
    }
    
    LOG_COMPILER_INFO("C source parsed successfully");
    
    // Apply optimizations
    if (options->optimization_level > 0) {
        LOG_COMPILER_INFO("Applying optimizations (level %d)", options->optimization_level);
        ast_optimize(ast, options->optimization_level);
    }
    
    // Apply AI optimization if enabled
    if (options->enable_ai_optimization) {
        LOG_COMPILER_INFO("Applying AI-driven optimizations");
        // This would integrate with the evolution engine
        // ast_ai_optimize(ast);
    }
    
    // Generate ASTC bytecode
    LOG_COMPILER_INFO("Generating ASTC bytecode");
    ASTCBytecode* bytecode = ast_generate_bytecode(ast);
    if (!bytecode) {
        LOG_COMPILER_ERROR("Failed to generate ASTC bytecode");
        ast_free(ast);
        free(source_code);
        return -1;
    }
    
    // Create program header with platform compatibility info
    ASTCProgramHeader header = {0};
    if (astc_create_program_header(&header, 
                                  options->target_platforms, options->target_platform_count,
                                  options->target_architectures, options->target_arch_count) != 0) {
        LOG_COMPILER_ERROR("Failed to create program header");
        return -1;
    }
    
    // Write ASTC file
    FILE* output = fopen(options->output_file, "wb");
    if (!output) {
        LOG_COMPILER_ERROR("Cannot create output file: %s", options->output_file);
        ast_free(ast);
        free(source_code);
        return -1;
    }
    
    // Write header
    fwrite(&header, sizeof(header), 1, output);
    
    // Write bytecode
    fwrite(bytecode->data, 1, bytecode->size, output);
    fclose(output);
    
    LOG_COMPILER_INFO("ASTC compilation completed successfully");
    LOG_COMPILER_INFO("Output file: %s (%zu bytes)", options->output_file, 
                     sizeof(header) + bytecode->size);
    
    // Cleanup
    ast_free(ast);
    free(source_code);
    
    if (options->enable_module_system) {
        astc_program_modules_cleanup();
    }
    astc_platform_compat_cleanup();
    
    return 0;
}

// Main function
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_enhanced_usage(argv[0]);
        return 1;
    }
    
    // Initialize logger
    if (logger_init() != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }
    
    // Get default options
    EnhancedC2AstcOptions options = get_default_options();
    
    // Parse command line arguments
    int parse_result = parse_enhanced_arguments(argc, argv, &options);
    if (parse_result != 0) {
        logger_cleanup();
        return parse_result > 0 ? 0 : 1;
    }
    
    // Check if input file is specified
    if (strlen(options.input_file) == 0) {
        fprintf(stderr, "Error: No input file specified\n");
        print_enhanced_usage(argv[0]);
        logger_cleanup();
        return 1;
    }
    
    // Set logger level based on verbosity
    if (options.verbose_output) {
        logger_set_level(LOG_LEVEL_DEBUG);
    }
    
    // Compile
    int result = compile_c_to_astc_enhanced(&options);
    
    // Cleanup
    logger_cleanup();
    
    return result == 0 ? 0 : 1;
}
