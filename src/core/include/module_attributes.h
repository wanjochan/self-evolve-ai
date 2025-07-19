#ifndef MODULE_ATTRIBUTES_H
#define MODULE_ATTRIBUTES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 模块属性类型
typedef enum {
    MODULE_ATTR_VERSION = 1,
    MODULE_ATTR_DESCRIPTION = 2,
    MODULE_ATTR_DEPENDENCIES = 3,
    MODULE_ATTR_CAPABILITIES = 4
} ModuleAttributeType;

// 模块属性结构
typedef struct {
    ModuleAttributeType type;
    uint32_t size;
    void* value;
} ModuleAttribute;

// 模块属性管理
typedef struct {
    ModuleAttribute* attributes;
    size_t count;
    size_t capacity;
} ModuleAttributeSet;

// 基本属性操作
ModuleAttributeSet* module_attr_create(void);
void module_attr_destroy(ModuleAttributeSet* attrs);
int module_attr_add(ModuleAttributeSet* attrs, ModuleAttributeType type, const void* value, size_t size);
const ModuleAttribute* module_attr_get(const ModuleAttributeSet* attrs, ModuleAttributeType type);

#ifdef __cplusplus
}
#endif

#endif // MODULE_ATTRIBUTES_H