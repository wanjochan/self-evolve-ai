#include "ffi_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 平台特定头文件
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

// 全局FFI状态
static bool ffi_initialized = false;
static FFILibrary* loaded_libraries = NULL;
static int library_count = 0;

// ===============================================
// FFI核心接口实现
// ===============================================

int ffi_init(void) {
    if (ffi_initialized) {
        return 0; // 已经初始化
    }
    
    printf("FFI: Initializing Foreign Function Interface\n");
    
    loaded_libraries = NULL;
    library_count = 0;
    ffi_initialized = true;
    
    return 0;
}

void ffi_cleanup(void) {
    if (!ffi_initialized) {
        return;
    }
    
    printf("FFI: Cleaning up Foreign Function Interface\n");
    
    // 卸载所有已加载的库
    for (int i = 0; i < library_count; i++) {
        if (loaded_libraries[i].is_loaded) {
            ffi_unload_library(&loaded_libraries[i]);
        }
    }
    
    if (loaded_libraries) {
        free(loaded_libraries);
        loaded_libraries = NULL;
    }
    
    library_count = 0;
    ffi_initialized = false;
}

FFILibrary* ffi_load_library(const char* library_name) {
    if (!ffi_initialized) {
        printf("FFI Error: FFI not initialized\n");
        return NULL;
    }
    
    printf("FFI: Loading library: %s\n", library_name);
    
    // 检查是否已经加载
    for (int i = 0; i < library_count; i++) {
        if (loaded_libraries[i].library_name && 
            strcmp(loaded_libraries[i].library_name, library_name) == 0) {
            printf("FFI: Library already loaded: %s\n", library_name);
            return &loaded_libraries[i];
        }
    }
    
    // 加载新库
    void* handle = ffi_platform_load_library(library_name);
    if (!handle) {
        printf("FFI Error: Failed to load library: %s\n", library_name);
        return NULL;
    }
    
    // 扩展库数组
    loaded_libraries = realloc(loaded_libraries, (library_count + 1) * sizeof(FFILibrary));
    if (!loaded_libraries) {
        printf("FFI Error: Memory allocation failed\n");
        ffi_platform_unload_library(handle);
        return NULL;
    }
    
    // 初始化新库
    FFILibrary* library = &loaded_libraries[library_count];
    library->handle = handle;
    library->library_name = malloc(strlen(library_name) + 1);
    strcpy(library->library_name, library_name);
    library->is_loaded = true;
    
    library_count++;
    
    printf("FFI: Successfully loaded library: %s\n", library_name);
    return library;
}

void ffi_unload_library(FFILibrary* library) {
    if (!library || !library->is_loaded) {
        return;
    }
    
    printf("FFI: Unloading library: %s\n", library->library_name);
    
    ffi_platform_unload_library(library->handle);
    
    if (library->library_name) {
        free(library->library_name);
        library->library_name = NULL;
    }
    
    library->handle = NULL;
    library->is_loaded = false;
}

FFIFunction* ffi_get_function(FFILibrary* library, const char* function_name, FFISignature* signature) {
    if (!library || !library->is_loaded || !function_name || !signature) {
        printf("FFI Error: Invalid parameters for ffi_get_function\n");
        return NULL;
    }
    
    printf("FFI: Getting function: %s from library: %s\n", function_name, library->library_name);
    
    void* function_ptr = ffi_platform_get_function(library->handle, function_name);
    if (!function_ptr) {
        printf("FFI Error: Function not found: %s\n", function_name);
        return NULL;
    }
    
    FFIFunction* function = malloc(sizeof(FFIFunction));
    if (!function) {
        printf("FFI Error: Memory allocation failed\n");
        return NULL;
    }
    
    function->function_ptr = function_ptr;
    function->function_name = malloc(strlen(function_name) + 1);
    strcpy(function->function_name, function_name);
    function->signature = *signature;
    function->library = library;
    
    printf("FFI: Successfully got function: %s\n", function_name);
    return function;
}

FFIValue ffi_call_function(FFIFunction* function, FFIValue* args, int arg_count) {
    FFIValue result = {0};
    
    if (!function || !function->function_ptr) {
        printf("FFI Error: Invalid function for call\n");
        return result;
    }
    
    printf("FFI: Calling function: %s with %d arguments\n", function->function_name, arg_count);
    
    // 简化实现：只支持基本的函数调用
    // 实际实现需要根据函数签名进行复杂的参数传递和调用约定处理
    
    switch (function->signature.return_type) {
        case FFI_TYPE_INT32: {
            typedef int32_t (*func_int32_t)(void);
            func_int32_t func = (func_int32_t)function->function_ptr;
            result.i32 = func();
            break;
        }
        case FFI_TYPE_POINTER: {
            typedef void* (*func_ptr_t)(void);
            func_ptr_t func = (func_ptr_t)function->function_ptr;
            result.ptr = func();
            break;
        }
        default:
            printf("FFI Warning: Unsupported return type, returning 0\n");
            result.i32 = 0;
            break;
    }
    
    printf("FFI: Function call completed\n");
    return result;
}

void ffi_free_function(FFIFunction* function) {
    if (!function) {
        return;
    }
    
    if (function->function_name) {
        free(function->function_name);
    }
    
    free(function);
}

// ===============================================
// 便利函数实现
// ===============================================

FFIValue ffi_call_system_function(const char* library_name, const char* function_name, 
                                   FFISignature* signature, FFIValue* args, int arg_count) {
    FFIValue result = {0};
    
    FFILibrary* library = ffi_load_library(library_name);
    if (!library) {
        return result;
    }
    
    FFIFunction* function = ffi_get_function(library, function_name, signature);
    if (!function) {
        return result;
    }
    
    result = ffi_call_function(function, args, arg_count);
    
    ffi_free_function(function);
    return result;
}

FFISignature* ffi_create_signature(FFIType return_type, FFIType* param_types, int param_count) {
    FFISignature* signature = malloc(sizeof(FFISignature));
    if (!signature) {
        return NULL;
    }
    
    signature->return_type = return_type;
    signature->param_count = param_count;
    
    if (param_count > 0 && param_types) {
        signature->param_types = malloc(param_count * sizeof(FFIType));
        memcpy(signature->param_types, param_types, param_count * sizeof(FFIType));
    } else {
        signature->param_types = NULL;
    }
    
    return signature;
}

void ffi_free_signature(FFISignature* signature) {
    if (!signature) {
        return;
    }
    
    if (signature->param_types) {
        free(signature->param_types);
    }
    
    free(signature);
}

FFIValue ffi_create_value_int32(int32_t value) {
    FFIValue result;
    result.i32 = value;
    return result;
}

FFIValue ffi_create_value_string(const char* value) {
    FFIValue result;
    result.str = (char*)value;
    return result;
}

FFIValue ffi_create_value_pointer(void* value) {
    FFIValue result;
    result.ptr = value;
    return result;
}

// ===============================================
// 平台特定实现
// ===============================================

void* ffi_platform_load_library(const char* library_name) {
#ifdef _WIN32
    return LoadLibraryA(library_name);
#else
    return dlopen(library_name, RTLD_LAZY);
#endif
}

void ffi_platform_unload_library(void* handle) {
#ifdef _WIN32
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
}

void* ffi_platform_get_function(void* handle, const char* function_name) {
#ifdef _WIN32
    return GetProcAddress((HMODULE)handle, function_name);
#else
    return dlsym(handle, function_name);
#endif
}
