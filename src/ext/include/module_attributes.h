/**
 * module_attributes.h - C99 Module System Attributes
 * 
 * Defines module system attributes using __attribute__ for C99 compatibility
 * as specified in cursor.md
 */

#ifndef MODULE_ATTRIBUTES_H
#define MODULE_ATTRIBUTES_H

// ===============================================
// Module System Attributes
// ===============================================

/**
 * MODULE(name) - Declare a module
 * Usage: MODULE("math") void init_math(void) { ... }
 */
#define MODULE(name) __attribute__((annotate("module:" name)))

/**
 * EXPORT - Mark a function or variable for export
 * Usage: EXPORT int add(int a, int b) { ... }
 */
#define EXPORT __attribute__((annotate("export")))

/**
 * IMPORT(module) - Import from another module
 * Usage: IMPORT("libc") extern void* malloc(size_t size);
 */
#define IMPORT(module) __attribute__((annotate("import:" module)))

/**
 * PRIVATE - Mark as module-private (not exported)
 * Usage: PRIVATE static int internal_func(void) { ... }
 */
#define PRIVATE __attribute__((annotate("private")))

/**
 * INIT - Mark module initialization function
 * Usage: INIT void module_init(void) { ... }
 */
#define INIT __attribute__((annotate("init")))

/**
 * CLEANUP - Mark module cleanup function
 * Usage: CLEANUP void module_cleanup(void) { ... }
 */
#define CLEANUP __attribute__((annotate("cleanup")))

// ===============================================
// Module Versioning
// ===============================================

/**
 * VERSION(major, minor, patch) - Declare module version
 * Usage: VERSION(1, 0, 0) MODULE("math") void init_math(void) { ... }
 */
#define VERSION(major, minor, patch) \
    __attribute__((annotate("version:" #major "." #minor "." #patch)))

/**
 * REQUIRES(module, version) - Declare module dependency
 * Usage: REQUIRES("libc", "2.0.0") IMPORT("libc") extern void* malloc(size_t);
 */
#define REQUIRES(module, version) \
    __attribute__((annotate("requires:" module ":" version)))

// ===============================================
// Export Types and Flags
// ===============================================

/**
 * EXPORT_FUNC - Export a function
 * Usage: EXPORT_FUNC int add(int a, int b) { ... }
 */
#define EXPORT_FUNC __attribute__((annotate("export:function")))

/**
 * EXPORT_VAR - Export a variable
 * Usage: EXPORT_VAR int global_counter = 0;
 */
#define EXPORT_VAR __attribute__((annotate("export:variable")))

/**
 * EXPORT_CONST - Export a constant
 * Usage: EXPORT_CONST const int MAX_SIZE = 1024;
 */
#define EXPORT_CONST __attribute__((annotate("export:constant")))

/**
 * EXPORT_TYPE - Export a type definition
 * Usage: EXPORT_TYPE typedef struct { ... } MyStruct;
 */
#define EXPORT_TYPE __attribute__((annotate("export:type")))

// ===============================================
// Import Flags
// ===============================================

/**
 * IMPORT_WEAK(module) - Weak import (optional dependency)
 * Usage: IMPORT_WEAK("optional") extern int optional_func(void);
 */
#define IMPORT_WEAK(module) __attribute__((annotate("import:weak:" module)))

/**
 * IMPORT_LAZY(module) - Lazy import (load on first use)
 * Usage: IMPORT_LAZY("heavy") extern int heavy_func(void);
 */
#define IMPORT_LAZY(module) __attribute__((annotate("import:lazy:" module)))

// ===============================================
// Module Metadata
// ===============================================

/**
 * AUTHOR(name) - Declare module author
 * Usage: AUTHOR("John Doe") MODULE("math") void init_math(void) { ... }
 */
#define AUTHOR(name) __attribute__((annotate("author:" name)))

/**
 * DESCRIPTION(text) - Declare module description
 * Usage: DESCRIPTION("Math utilities") MODULE("math") void init_math(void) { ... }
 */
#define DESCRIPTION(text) __attribute__((annotate("description:" text)))

/**
 * LICENSE(type) - Declare module license
 * Usage: LICENSE("MIT") MODULE("math") void init_math(void) { ... }
 */
#define LICENSE(type) __attribute__((annotate("license:" type)))

// ===============================================
// Validation Macros
// ===============================================

/**
 * Check if a combination of attributes is valid
 */
#define MODULE_VALIDATE_EXPORT_IMPORT() \
    _Static_assert(0, "EXPORT and IMPORT cannot be used together")

/**
 * Check if module name is valid
 */
#define MODULE_VALIDATE_NAME(name) \
    _Static_assert(sizeof(name) > 1, "Module name cannot be empty")

// ===============================================
// Compatibility Macros
// ===============================================

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if !__has_attribute(annotate)
// Fallback for compilers without annotate support
#undef MODULE
#undef EXPORT
#undef IMPORT
#undef PRIVATE
#undef INIT
#undef CLEANUP

#define MODULE(name)
#define EXPORT
#define IMPORT(module)
#define PRIVATE
#define INIT
#define CLEANUP

#warning "Compiler does not support __attribute__((annotate)). Module system disabled."
#endif

// ===============================================
// Usage Examples
// ===============================================

#ifdef MODULE_ATTRIBUTES_EXAMPLES

// Example 1: Math module
MODULE("math")
VERSION(1, 0, 0)
AUTHOR("Math Team")
DESCRIPTION("Basic math operations")
LICENSE("MIT")
void init_math(void) {
    // Module initialization
}

EXPORT_FUNC
int add(int a, int b) {
    return a + b;
}

EXPORT_FUNC
int multiply(int a, int b) {
    return a * b;
}

PRIVATE
static int internal_helper(void) {
    return 42;
}

// Example 2: String module with dependencies
MODULE("string")
VERSION(2, 1, 0)
REQUIRES("libc", "2.0.0")
void init_string(void) {
    // Module initialization
}

IMPORT("libc")
extern void* malloc(size_t size);

IMPORT("libc")
extern void free(void* ptr);

EXPORT_FUNC
char* string_duplicate(const char* src) {
    // Implementation using imported malloc
    return NULL; // Placeholder
}

// Example 3: Optional dependency
MODULE("graphics")
VERSION(1, 0, 0)
void init_graphics(void) {
    // Module initialization
}

IMPORT_WEAK("opengl")
extern int gl_init(void);

EXPORT_FUNC
int graphics_init(void) {
    // Use OpenGL if available
    return 0;
}

#endif // MODULE_ATTRIBUTES_EXAMPLES

#endif // MODULE_ATTRIBUTES_H
