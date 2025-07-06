/**
 * memory_module.c - Memory Management Module
 * 
 * Provides memory management functionality as a module.
 */

#include "../module.h"
#include <stdlib.h>
#include <string.h>

// Module name
static const char* MODULE_NAME = "memory";

// Memory allocation function
static void* memory_alloc(size_t size) {
    return malloc(size);
}

// Memory reallocation function
static void* memory_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

// Memory free function
static void memory_free(void* ptr) {
    free(ptr);
}

// Memory copy function
static void* memory_copy(void* dest, const void* src, size_t size) {
    return memcpy(dest, src, size);
}

// Memory set function
static void* memory_set(void* dest, int value, size_t size) {
    return memset(dest, value, size);
}

// Symbol table
static struct {
    const char* name;
    void* symbol;
} memory_symbols[] = {
    {"alloc", memory_alloc},
    {"realloc", memory_realloc},
    {"free", memory_free},
    {"copy", memory_copy},
    {"set", memory_set},
    {NULL, NULL}  // Sentinel
};

// Module load function
static int memory_load(void) {
    // Nothing to initialize
    return 0;
}

// Module unload function
static void memory_unload(void) {
    // Nothing to clean up
}

// Symbol resolution function
static void* memory_resolve(const char* symbol) {
    if (!symbol) {
        return NULL;
    }
    
    for (int i = 0; memory_symbols[i].name; i++) {
        if (strcmp(memory_symbols[i].name, symbol) == 0) {
            return memory_symbols[i].symbol;
        }
    }
    
    return NULL;
}

// Module definition
static Module module_memory = {
    .name = MODULE_NAME,
    .handle = NULL,
    .state = MODULE_UNLOADED,
    .error = NULL,
    .load = memory_load,
    .unload = memory_unload,
    .resolve = memory_resolve,
    .on_init = NULL,
    .on_exit = NULL,
    .on_error = NULL
};

// Register module
REGISTER_MODULE(memory); 