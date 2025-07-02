/**
 * platform.h - 平台抽象层
 * 
 * 提供跨平台统一接口，隐藏底层实现细节
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>
#include <stdint.h>

/**
 * 分配可执行内存
 * 
 * @param size 要分配的内存大小（字节）
 * @return 指向分配内存的指针，失败时返回NULL
 */
void* platform_alloc_executable(size_t size);

/**
 * 释放可执行内存
 * 
 * @param ptr 之前由platform_alloc_executable分配的内存
 * @param size 内存大小（某些平台需要）
 */
void platform_free_executable(void* ptr, size_t size);

/**
 * 判断当前系统是否为Windows
 * 
 * @return 1表示Windows，0表示其他系统
 */
int platform_is_windows(void);

// ===============================================
// 动态库加载功能 (dlopen系列兼容)
// ===============================================

/**
 * 动态库句柄类型
 */
typedef void* platform_dl_handle;

/**
 * 加载动态库
 * 
 * @param path 动态库路径
 * @return 动态库句柄，失败时返回NULL
 */
platform_dl_handle platform_dl_open(const char* path);

/**
 * 获取动态库中的符号地址
 *
 * @param handle 动态库句柄
 * @param symbol 符号名称
 * @return 符号地址，失败时返回NULL
 */
void* platform_dl_sym(platform_dl_handle handle, const char* symbol);

/**
 * 关闭动态库
 *
 * @param handle 动态库句柄
 * @return 成功返回0，失败返回非0值
 */
int platform_dl_close(platform_dl_handle handle);

/**
 * 获取动态库操作的最近一次错误信息
 *
 * @return 错误信息字符串，无错误时返回NULL
 */
const char* platform_dl_error(void);

// ===============================================
// 网络通信抽象层
// ===============================================

/**
 * 网络套接字类型
 */
#ifdef _WIN32
typedef uintptr_t platform_socket_t;
#else
typedef int platform_socket_t;
#endif

/**
 * 事件循环句柄
 */
typedef void* platform_event_loop_t;

/**
 * 套接字事件类型
 */
typedef enum {
    PLATFORM_SOCKET_READ = 1,      // 可读事件
    PLATFORM_SOCKET_WRITE = 2,     // 可写事件
    PLATFORM_SOCKET_EXCEPTION = 4  // 异常事件
} platform_socket_event_type;

/**
 * 事件结构
 */
typedef struct {
    platform_socket_t socket;      // 触发事件的套接字
    int events;                    // 事件类型
    void* user_data;               // 用户数据
} platform_socket_event;

/**
 * 地址族
 */
typedef enum {
    PLATFORM_AF_INET = 0,          // IPv4
    PLATFORM_AF_INET6 = 1,         // IPv6
    PLATFORM_AF_UNIX = 2           // Unix域套接字
} platform_address_family;

/**
 * 套接字类型
 */
typedef enum {
    PLATFORM_SOCK_STREAM = 0,      // 流套接字(TCP)
    PLATFORM_SOCK_DGRAM = 1,       // 数据报套接字(UDP)
    PLATFORM_SOCK_RAW = 2          // 原始套接字
} platform_socket_type;

/**
 * 协议类型
 */
typedef enum {
    PLATFORM_IPPROTO_TCP = 0,      // TCP协议
    PLATFORM_IPPROTO_UDP = 1,      // UDP协议
    PLATFORM_IPPROTO_ICMP = 2      // ICMP协议
} platform_protocol_type;

/**
 * 地址结构
 */
typedef struct {
    platform_address_family family; // 地址族
    uint16_t port;                  // 端口
    union {
        uint32_t ipv4;              // IPv4地址
        uint8_t ipv6[16];           // IPv6地址
        char path[108];             // Unix域路径
    } addr;
} platform_sockaddr;

/**
 * 创建套接字
 *
 * @param family 地址族
 * @param type 套接字类型
 * @param protocol 协议类型
 * @return 套接字句柄，失败时返回INVALID_SOCKET(-1)
 */
platform_socket_t platform_socket_create(platform_address_family family, 
                                        platform_socket_type type,
                                        platform_protocol_type protocol);

/**
 * 绑定套接字到地址
 *
 * @param sock 套接字句柄
 * @param addr 地址结构
 * @return 成功返回0，失败返回-1
 */
int platform_socket_bind(platform_socket_t sock, const platform_sockaddr* addr);

/**
 * 监听连接
 *
 * @param sock 套接字句柄
 * @param backlog 最大连接队列长度
 * @return 成功返回0，失败返回-1
 */
int platform_socket_listen(platform_socket_t sock, int backlog);

/**
 * 接受连接
 *
 * @param sock 监听套接字句柄
 * @param addr 返回客户端地址
 * @return 新连接套接字，失败时返回INVALID_SOCKET(-1)
 */
platform_socket_t platform_socket_accept(platform_socket_t sock, platform_sockaddr* addr);

/**
 * 连接到服务器
 *
 * @param sock 套接字句柄
 * @param addr 服务器地址
 * @return 成功返回0，失败返回-1
 */
int platform_socket_connect(platform_socket_t sock, const platform_sockaddr* addr);

/**
 * 发送数据
 *
 * @param sock 套接字句柄
 * @param buffer 数据缓冲区
 * @param length 数据长度
 * @return 已发送的字节数，失败时返回-1
 */
int platform_socket_send(platform_socket_t sock, const void* buffer, size_t length);

/**
 * 接收数据
 *
 * @param sock 套接字句柄
 * @param buffer 数据缓冲区
 * @param length 缓冲区大小
 * @return 已接收的字节数，失败时返回-1，远端关闭返回0
 */
int platform_socket_recv(platform_socket_t sock, void* buffer, size_t length);

/**
 * 关闭套接字
 *
 * @param sock 套接字句柄
 * @return 成功返回0，失败返回-1
 */
int platform_socket_close(platform_socket_t sock);

/**
 * 创建事件循环
 *
 * @return 事件循环句柄，失败时返回NULL
 */
platform_event_loop_t platform_event_loop_create(void);

/**
 * 销毁事件循环
 *
 * @param loop 事件循环句柄
 * @return 成功返回0，失败返回-1
 */
int platform_event_loop_destroy(platform_event_loop_t loop);

/**
 * 向事件循环添加套接字事件
 *
 * @param loop 事件循环句柄
 * @param sock 套接字句柄
 * @param events 事件类型组合
 * @param user_data 用户数据
 * @return 成功返回0，失败返回-1
 */
int platform_event_add(platform_event_loop_t loop, platform_socket_t sock, 
                       int events, void* user_data);

/**
 * 修改事件循环中的套接字事件
 *
 * @param loop 事件循环句柄
 * @param sock 套接字句柄
 * @param events 事件类型组合
 * @param user_data 用户数据
 * @return 成功返回0，失败返回-1
 */
int platform_event_mod(platform_event_loop_t loop, platform_socket_t sock,
                       int events, void* user_data);

/**
 * 从事件循环删除套接字事件
 *
 * @param loop 事件循环句柄
 * @param sock 套接字句柄
 * @return 成功返回0，失败返回-1
 */
int platform_event_del(platform_event_loop_t loop, platform_socket_t sock);

/**
 * 等待事件发生
 *
 * @param loop 事件循环句柄
 * @param events 事件结果数组
 * @param max_events 数组最大容量
 * @param timeout 超时时间(毫秒)，-1表示无限等待
 * @return 触发的事件数量，超时返回0，失败返回-1
 */
int platform_event_wait(platform_event_loop_t loop, platform_socket_event* events, 
                        int max_events, int timeout);

/**
 * 设置套接字为非阻塞模式
 *
 * @param sock 套接字句柄
 * @param nonblock 0表示阻塞模式，1表示非阻塞模式
 * @return 成功返回0，失败返回-1
 */
int platform_socket_set_nonblock(platform_socket_t sock, int nonblock);

/**
 * 设置地址为IPv4地址
 *
 * @param addr 地址结构
 * @param ip_str IPv4地址字符串(xx.xx.xx.xx)
 * @param port 端口号
 * @return 成功返回0，失败返回-1
 */
int platform_sockaddr_from_ipv4(platform_sockaddr* addr, const char* ip_str, uint16_t port);

/**
 * 获取套接字错误
 *
 * @return 错误码
 */
int platform_socket_get_error(void);

/**
 * 获取套接字错误信息
 *
 * @param errcode 错误码
 * @return 错误信息字符串
 */
const char* platform_socket_error_string(int errcode);

#endif /* PLATFORM_H */ 