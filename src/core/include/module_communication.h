#ifndef MODULE_COMMUNICATION_H
#define MODULE_COMMUNICATION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 模块间通信结构
typedef struct {
    uint32_t type;
    uint32_t size;
    void* data;
} ModuleMessage;

// 通信状态
typedef enum {
    MODULE_COMM_SUCCESS = 0,
    MODULE_COMM_ERROR = -1,
    MODULE_COMM_TIMEOUT = -2,
    MODULE_COMM_INVALID_MSG = -3
} ModuleCommResult;

// 基本通信函数
ModuleCommResult module_send_message(const char* target_module, const ModuleMessage* msg);
ModuleCommResult module_receive_message(const char* source_module, ModuleMessage* msg);
void module_free_message(ModuleMessage* msg);

#ifdef __cplusplus
}
#endif

#endif // MODULE_COMMUNICATION_H