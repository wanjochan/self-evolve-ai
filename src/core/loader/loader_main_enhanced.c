/**
 * loader_main_enhanced.c - Enhanced Universal Loader
 * 
 * Advanced universal loader with module system integration, AI evolution support,
 * cross-platform compatibility, and enhanced security features.
 */

#include "../include/core_astc.h"
#include "../include/logger.h"
#include "../include/native_format.h"
#include "../include/module_communication.h"
#include "../include/astc_platform_compat.h"
#include "../include/astc_program_modules.h"
#include "../include/vm_enhanced.h"
#include "../../ai/evolution_engine_enhanced.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Enhanced loader configuration
typedef struct {
    // Basic options
    char program_path[256];
    char vm_module_path[256];
    bool verbose_mode;
    bool debug_mode;
    bool interactive_mode;
    
    // Module system options
    bool enable_module_system;
    bool auto_load_modules;
    char module_search_paths[16][256];
    int module_search_path_count;
    
    // AI evolution options
    bool enable_ai_evolution;
    bool autonomous_evolution;
    EvolutionStrategy evolution_strategy;
    
    // Security options
    bool enable_sandboxing;
    bool verify_signatures;
    bool enforce_resource_limits;
    
    // Performance options
    bool enable_jit;
    bool enable_profiling;
    VMExecutionMode vm_mode;
    int optimization_level;
    
    // Platform options
    bool force_architecture;
    ASTCArchitectureType target_arch;
    ASTCPlatformType target_platform;
} EnhancedLoaderConfig;

// Global loader state
static struct {
    EnhancedLoaderConfig config;
    LoadedModule* vm_module;
    bool initialized;
    time_t start_time;
    uint64_t programs_executed;
    uint64_t evolution_cycles;
} g_loader_state = {0};

// Default loader configuration
static EnhancedLoaderConfig get_default_loader_config(void) {
    EnhancedLoaderConfig config = {0};
    
    config.verbose_mode = false;
    config.debug_mode = false;
    config.interactive_mode = false;
    config.enable_module_system = true;
    config.auto_load_modules = true;
    config.enable_ai_evolution = true;
    config.autonomous_evolution = false;
    config.evolution_strategy = EVOLUTION_STRATEGY_HYBRID;
    config.enable_sandboxing = false;
    config.verify_signatures = false;
    config.enforce_resource_limits = true;
    config.enable_jit = true;
    config.enable_profiling = false;
    config.vm_mode = VM_MODE_HYBRID;
    config.optimization_level = 1;
    config.force_architecture = false;
    
    // Add default module search paths
    strcpy(config.module_search_paths[0], "./modules/");
    strcpy(config.module_search_paths[1], "./lib/");
    config.module_search_path_count = 2;
    
    return config;
}

// Print enhanced usage
void print_enhanced_loader_usage(const char* program_name) {
    printf("Enhanced Universal Loader v2.0 - Self-Evolve AI System\n");
    printf("Usage: %s [options] [program.astc]\n\n", program_name);
    
    printf("Basic Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --verbose           Enable verbose output\n");
    printf("  -d, --debug             Enable debug mode\n");
    printf("  -i, --interactive       Start in interactive mode\n");
    
    printf("\nModule System:\n");
    printf("  --enable-modules        Enable module system (default)\n");
    printf("  --disable-modules       Disable module system\n");
    printf("  --auto-load             Auto-load standard modules (default)\n");
    printf("  --module-path <dir>     Add module search path\n");
    
    printf("\nAI Evolution:\n");
    printf("  --enable-ai             Enable AI evolution (default)\n");
    printf("  --disable-ai            Disable AI evolution\n");
    printf("  --autonomous            Enable autonomous evolution\n");
    printf("  --strategy <s>          Evolution strategy (random/guided/genetic/neural/hybrid)\n");
    
    printf("\nVM Options:\n");
    printf("  --vm-mode <mode>        VM execution mode (interpreter/jit/hybrid)\n");
    printf("  --enable-jit            Enable JIT compilation (default)\n");
    printf("  --disable-jit           Disable JIT compilation\n");
    printf("  --profile               Enable profiling\n");
    printf("  -O <level>              Optimization level (0-3)\n");
    
    printf("\nSecurity:\n");
    printf("  --sandbox               Enable sandboxing\n");
    printf("  --verify-signatures     Verify module signatures\n");
    printf("  --resource-limits       Enforce resource limits (default)\n");
    
    printf("\nPlatform:\n");
    printf("  --force-arch <arch>     Force specific architecture\n");
    printf("  --target-platform <p>   Target platform\n");
    
    printf("\nExamples:\n");
    printf("  %s program.astc                    # Basic execution\n", program_name);
    printf("  %s --verbose --debug program.astc  # Debug mode\n", program_name);
    printf("  %s --autonomous --strategy hybrid  # Autonomous AI evolution\n", program_name);
    printf("  %s --interactive                   # Interactive mode\n", program_name);
}

// Initialize enhanced loader
int loader_enhanced_init(const EnhancedLoaderConfig* config) {
    g_loader_state.config = *config;
    g_loader_state.start_time = time(NULL);
    
    LOG_LOADER_INFO("Enhanced Universal Loader initializing...");
    
    // Initialize platform compatibility
    if (astc_platform_compat_init() != 0) {
        LOG_LOADER_ERROR("Failed to initialize platform compatibility");
        return -1;
    }
    
    // Initialize module system if enabled
    if (config->enable_module_system) {
        if (module_comm_init() != 0) {
            LOG_LOADER_ERROR("Failed to initialize module communication");
            return -1;
        }
        
        if (astc_program_modules_init("enhanced_loader", "loader_main_enhanced") != 0) {
            LOG_LOADER_ERROR("Failed to initialize program module system");
            return -1;
        }
        
        // Add module search paths
        for (int i = 0; i < config->module_search_path_count; i++) {
            astc_program_add_module_search_path(config->module_search_paths[i]);
        }
    }
    
    // Initialize AI evolution if enabled
    if (config->enable_ai_evolution) {
        if (evolution_engine_enhanced_init(config->evolution_strategy) != 0) {
            LOG_LOADER_ERROR("Failed to initialize AI evolution engine");
            return -1;
        }
        
        evolution_set_autonomous_mode(config->autonomous_evolution);
    }
    
    // Initialize enhanced VM
    if (vm_enhanced_init(config->vm_mode) != 0) {
        LOG_LOADER_ERROR("Failed to initialize enhanced VM");
        return -1;
    }
    
    // Configure VM
    VMConfig vm_config = {0};
    vm_config.mode = config->vm_mode;
    vm_config.enable_jit = config->enable_jit;
    vm_config.enable_profiling = config->enable_profiling;
    vm_config.enable_optimization = (config->optimization_level > 0);
    vm_config.stack_size = 64 * 1024; // 64KB
    vm_config.jit_threshold = 10;
    vm_config.max_heap_size = 64 * 1024 * 1024; // 64MB
    
    vm_enhanced_configure(&vm_config);
    
    g_loader_state.initialized = true;
    
    LOG_LOADER_INFO("Enhanced Universal Loader initialized successfully");
    return 0;
}

// Detect and construct VM module path
int construct_enhanced_vm_module_path(char* path, size_t path_size) {
    const ASTCPlatformInfo* platform_info = astc_get_platform_info();
    
    const char* arch_str = astc_architecture_type_to_string(platform_info->architecture);
    const char* platform_str = astc_platform_type_to_string(platform_info->platform);
    
    snprintf(path, path_size, "vm_%s_%d.native", arch_str, platform_info->pointer_size * 8);
    
    LOG_LOADER_DEBUG("Constructed VM module path: %s", path);
    return 0;
}

// Load and execute ASTC program
int execute_astc_program_enhanced(const char* program_path) {
    if (!program_path) {
        LOG_LOADER_ERROR("No program path specified");
        return -1;
    }
    
    LOG_LOADER_INFO("Loading ASTC program: %s", program_path);
    
    // Read ASTC file
    FILE* file = fopen(program_path, "rb");
    if (!file) {
        LOG_LOADER_ERROR("Cannot open program file: %s", program_path);
        return -1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read program data
    uint8_t* program_data = malloc(file_size);
    if (!program_data) {
        LOG_LOADER_ERROR("Memory allocation failed");
        fclose(file);
        return -1;
    }
    
    fread(program_data, 1, file_size, file);
    fclose(file);
    
    // Parse ASTC program
    ASTNode* program_ast = ast_parse_bytecode(program_data, file_size);
    if (!program_ast) {
        LOG_LOADER_ERROR("Failed to parse ASTC program");
        free(program_data);
        return -1;
    }
    
    LOG_LOADER_INFO("ASTC program parsed successfully");
    
    // Check platform compatibility
    ASTCProgramHeader* header = (ASTCProgramHeader*)program_data;
    if (!astc_is_program_compatible(header)) {
        LOG_LOADER_ERROR("Program is not compatible with current platform");
        ast_free(program_ast);
        free(program_data);
        return -1;
    }
    
    // Auto-load modules if enabled
    if (g_loader_state.config.auto_load_modules) {
        LOG_LOADER_INFO("Auto-loading standard modules...");
        astc_program_auto_import_system_modules();
    }
    
    // Execute program using enhanced VM
    int result = vm_enhanced_execute_module(program_ast);
    if (result != 0) {
        LOG_LOADER_ERROR("Program execution failed");
        ast_free(program_ast);
        free(program_data);
        return -1;
    }
    
    LOG_LOADER_INFO("Program executed successfully");
    g_loader_state.programs_executed++;
    
    // Run AI evolution if enabled and autonomous
    if (g_loader_state.config.enable_ai_evolution && g_loader_state.config.autonomous_evolution) {
        LOG_LOADER_INFO("Running AI evolution cycle...");
        if (evolution_run_iteration() == 0) {
            g_loader_state.evolution_cycles++;
            LOG_LOADER_INFO("AI evolution cycle completed");
        }
    }
    
    // Cleanup
    ast_free(program_ast);
    free(program_data);
    
    return 0;
}

// Interactive mode
int run_interactive_mode(void) {
    LOG_LOADER_INFO("Starting interactive mode");
    printf("\nEnhanced Universal Loader - Interactive Mode\n");
    printf("Type 'help' for commands, 'exit' to quit\n\n");
    
    char input[512];
    while (1) {
        printf("loader> ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            continue;
        }
        
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            break;
        } else if (strcmp(input, "help") == 0) {
            printf("Available commands:\n");
            printf("  load <program.astc>     Load and execute ASTC program\n");
            printf("  modules                 List loaded modules\n");
            printf("  stats                   Show VM statistics\n");
            printf("  evolve                  Run AI evolution cycle\n");
            printf("  platform                Show platform information\n");
            printf("  help                    Show this help\n");
            printf("  exit                    Exit interactive mode\n");
        } else if (strncmp(input, "load ", 5) == 0) {
            char* program_path = input + 5;
            execute_astc_program_enhanced(program_path);
        } else if (strcmp(input, "modules") == 0) {
            astc_program_list_modules();
        } else if (strcmp(input, "stats") == 0) {
            vm_enhanced_get_stats();
        } else if (strcmp(input, "evolve") == 0) {
            if (g_loader_state.config.enable_ai_evolution) {
                evolution_run_iteration();
                g_loader_state.evolution_cycles++;
            } else {
                printf("AI evolution is disabled\n");
            }
        } else if (strcmp(input, "platform") == 0) {
            const ASTCPlatformInfo* info = astc_get_platform_info();
            printf("Platform: %s %s (%d-bit)\n", 
                   info->platform_name, info->arch_name, info->pointer_size * 8);
        } else {
            printf("Unknown command: %s (type 'help' for available commands)\n", input);
        }
    }
    
    LOG_LOADER_INFO("Interactive mode ended");
    return 0;
}

// Parse command line arguments
int parse_loader_arguments(int argc, char* argv[], EnhancedLoaderConfig* config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_enhanced_loader_usage(argv[0]);
            return 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            config->verbose_mode = true;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            config->debug_mode = true;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            config->interactive_mode = true;
        } else if (strcmp(argv[i], "--enable-modules") == 0) {
            config->enable_module_system = true;
        } else if (strcmp(argv[i], "--disable-modules") == 0) {
            config->enable_module_system = false;
        } else if (strcmp(argv[i], "--enable-ai") == 0) {
            config->enable_ai_evolution = true;
        } else if (strcmp(argv[i], "--disable-ai") == 0) {
            config->enable_ai_evolution = false;
        } else if (strcmp(argv[i], "--autonomous") == 0) {
            config->autonomous_evolution = true;
        } else if (strcmp(argv[i], "--enable-jit") == 0) {
            config->enable_jit = true;
        } else if (strcmp(argv[i], "--disable-jit") == 0) {
            config->enable_jit = false;
        } else if (strcmp(argv[i], "--profile") == 0) {
            config->enable_profiling = true;
        } else if (argv[i][0] != '-') {
            // Program path
            strncpy(config->program_path, argv[i], sizeof(config->program_path) - 1);
        }
    }
    
    return 0;
}

// Cleanup enhanced loader
void loader_enhanced_cleanup(void) {
    if (!g_loader_state.initialized) {
        return;
    }
    
    LOG_LOADER_INFO("Enhanced Universal Loader shutting down...");
    
    // Show final statistics
    time_t end_time = time(NULL);
    double runtime = difftime(end_time, g_loader_state.start_time);
    
    LOG_LOADER_INFO("Runtime: %.2f seconds", runtime);
    LOG_LOADER_INFO("Programs executed: %llu", g_loader_state.programs_executed);
    LOG_LOADER_INFO("Evolution cycles: %llu", g_loader_state.evolution_cycles);
    
    // Cleanup subsystems
    if (g_loader_state.config.enable_ai_evolution) {
        evolution_engine_enhanced_cleanup();
    }
    
    vm_enhanced_cleanup();
    
    if (g_loader_state.config.enable_module_system) {
        astc_program_modules_cleanup();
        module_comm_cleanup();
    }
    
    astc_platform_compat_cleanup();
    
    g_loader_state.initialized = false;
    LOG_LOADER_INFO("Enhanced Universal Loader shutdown complete");
}

// Main function
int main(int argc, char* argv[]) {
    // Initialize logger first
    if (logger_init() != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }
    
    // Get default configuration
    EnhancedLoaderConfig config = get_default_loader_config();
    
    // Parse command line arguments
    int parse_result = parse_loader_arguments(argc, argv, &config);
    if (parse_result != 0) {
        logger_cleanup();
        return parse_result > 0 ? 0 : 1;
    }
    
    // Set logger level based on verbosity
    if (config.verbose_mode) {
        logger_set_level(LOG_LEVEL_DEBUG);
    } else if (config.debug_mode) {
        logger_set_level(LOG_LEVEL_DEBUG);
    }
    
    // Initialize enhanced loader
    if (loader_enhanced_init(&config) != 0) {
        fprintf(stderr, "Failed to initialize enhanced loader\n");
        logger_cleanup();
        return 1;
    }
    
    int result = 0;
    
    // Run in appropriate mode
    if (config.interactive_mode) {
        result = run_interactive_mode();
    } else if (strlen(config.program_path) > 0) {
        result = execute_astc_program_enhanced(config.program_path);
    } else {
        printf("No program specified. Use --interactive for interactive mode or specify a program.\n");
        print_enhanced_loader_usage(argv[0]);
        result = 1;
    }
    
    // Cleanup
    loader_enhanced_cleanup();
    logger_cleanup();
    
    return result;
}
