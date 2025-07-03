/**
 * astc_module.h - ASTC Module Interface Header
 * 
 * Provides unified interface for ASTC compilation and native code generation.
 * This module acts as a bridge between the loader and the core compiler functions.
 */

#ifndef ASTC_MODULE_H
#define ASTC_MODULE_H

#include <stddef.h>
#include <stdint.h>

// Forward declarations
struct C2AstcOptions;

#ifdef __cplusplus
extern "C" {
#endif

// ===============================================
// ASTC Module Interface Functions
// ===============================================

/**
 * Convert C source file to ASTC bytecode
 * 
 * @param c_file_path Path to the C source file
 * @param astc_file_path Path for the output ASTC file
 * @param options Compilation options (can be NULL for defaults)
 * @return 0 on success, -1 on error
 */
int c2astc(const char* c_file_path, const char* astc_file_path, const struct C2AstcOptions* options);

/**
 * Convert ASTC bytecode to native executable
 * 
 * @param astc_file_path Path to the ASTC bytecode file
 * @param native_file_path Path for the output native file
 * @param target_arch Target architecture (can be NULL for auto-detect)
 * @return 0 on success, -1 on error
 */
int astc2native(const char* astc_file_path, const char* native_file_path, const char* target_arch);

/**
 * Direct C to native compilation (combines c2astc and astc2native)
 * 
 * @param c_file_path Path to the C source file
 * @param native_file_path Path for the output native file
 * @param options Compilation options (can be NULL for defaults)
 * @param target_arch Target architecture (can be NULL for auto-detect)
 * @return 0 on success, -1 on error
 */
int c2native(const char* c_file_path, const char* native_file_path, const struct C2AstcOptions* options, const char* target_arch);

/**
 * Initialize ASTC module
 * @return 0 on success, -1 on error
 */
int astc_module_init(void);

/**
 * Cleanup ASTC module resources
 */
void astc_module_cleanup(void);

/**
 * Get ASTC module version information
 */
void astc_module_print_version(void);

// ===============================================
// ASTC Module Interface Structure
// ===============================================

typedef struct {
    int (*init)(void);
    void (*cleanup)(void);
    int (*c2astc)(const char* c_file, const char* astc_file, const struct C2AstcOptions* options);
    int (*astc2native)(const char* astc_file, const char* native_file, const char* target_arch);
    int (*c2native)(const char* c_file, const char* native_file, const struct C2AstcOptions* options, const char* target_arch);
    void (*print_version)(void);
} AstcModuleInterface;

/**
 * Get the ASTC module interface
 * @return Pointer to the module interface
 */
const AstcModuleInterface* get_astc_module_interface(void);

#ifdef __cplusplus
}
#endif

#endif // ASTC_MODULE_H
