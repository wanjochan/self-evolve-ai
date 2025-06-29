#ifndef FFI_INTERFACE_H
#define FFI_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// FFI类型定义
typedef enum {
    FFI_TYPE_VOID,
    FFI_TYPE_INT8,
    FFI_TYPE_UINT8,
    FFI_TYPE_INT16,
    FFI_TYPE_UINT16,
    FFI_TYPE_INT32,
    FFI_TYPE_UINT32,
    FFI_TYPE_INT64,
    FFI_TYPE_UINT64,
    FFI_TYPE_FLOAT,
    FFI_TYPE_DOUBLE,
    FFI_TYPE_POINTER,
    FFI_TYPE_STRING
} FFIType;

// FFI函数签名
typedef struct {
    FFIType return_type;
    FFIType* param_types;
    int param_count;
} FFISignature;

// FFI库句柄
typedef struct {
    void* handle;           // 动态库句柄
    char* library_name;     // 库名称
    bool is_loaded;         // 是否已加载
} FFILibrary;

// FFI函数句柄
typedef struct {
    void* function_ptr;     // 函数指针
    char* function_name;    // 函数名称
    FFISignature signature; // 函数签名
    FFILibrary* library;    // 所属库
} FFIFunction;

// FFI调用参数
typedef union {
    int8_t i8;
    uint8_t u8;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    float f32;
    double f64;
    void* ptr;
    char* str;
} FFIValue;

// ===============================================
// FFI核心接口
// ===============================================

/**
 * 初始化FFI系统
 */
int ffi_init(void);

/**
 * 清理FFI系统
 */
void ffi_cleanup(void);

/**
 * 加载动态库
 */
FFILibrary* ffi_load_library(const char* library_name);

/**
 * 卸载动态库
 */
void ffi_unload_library(FFILibrary* library);

/**
 * 获取函数地址
 */
FFIFunction* ffi_get_function(FFILibrary* library, const char* function_name, FFISignature* signature);

/**
 * 调用FFI函数
 */
FFIValue ffi_call_function(FFIFunction* function, FFIValue* args, int arg_count);

/**
 * 释放FFI函数
 */
void ffi_free_function(FFIFunction* function);

// ===============================================
// 便利函数
// ===============================================

/**
 * 快速调用系统函数
 */
FFIValue ffi_call_system_function(const char* library_name, const char* function_name, 
                                   FFISignature* signature, FFIValue* args, int arg_count);

/**
 * 创建FFI签名
 */
FFISignature* ffi_create_signature(FFIType return_type, FFIType* param_types, int param_count);

/**
 * 释放FFI签名
 */
void ffi_free_signature(FFISignature* signature);

/**
 * 创建FFI值
 */
FFIValue ffi_create_value_int32(int32_t value);
FFIValue ffi_create_value_string(const char* value);
FFIValue ffi_create_value_pointer(void* value);

// ===============================================
// 平台特定实现
// ===============================================

#ifdef _WIN32
    // Windows平台使用LoadLibrary/GetProcAddress
    #define FFI_LIBRARY_EXTENSION ".dll"
#elif defined(__APPLE__)
    // macOS平台使用dlopen/dlsym
    #define FFI_LIBRARY_EXTENSION ".dylib"
#else
    // Linux平台使用dlopen/dlsym
    #define FFI_LIBRARY_EXTENSION ".so"
#endif

/**
 * 平台特定的库加载实现
 */
void* ffi_platform_load_library(const char* library_name);
void ffi_platform_unload_library(void* handle);
void* ffi_platform_get_function(void* handle, const char* function_name);

#ifdef __cplusplus
}
#endif

#endif // FFI_INTERFACE_H
