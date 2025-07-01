/**
 * platform.c - 平台抽象层实现
 * 
 * 根据不同平台提供统一接口的具体实现
 */

#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h> // Unix domain sockets
#endif

// ===============================================
// 内存管理实现
// ===============================================

// 判断当前系统是否为Windows
int platform_is_windows(void) {
#ifdef _WIN32
    return 1;
#else
    return 0;
#endif
}

// 分配可执行内存
void* platform_alloc_executable(size_t size) {
#ifdef _WIN32
    // Windows实现：使用VirtualAlloc
    void* ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!ptr) {
        fprintf(stderr, "Error: VirtualAlloc failed to allocate %zu bytes\n", size);
        return NULL;
    }
    return ptr;
#else
    // POSIX实现：使用mmap
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed to allocate %zu bytes\n", size);
        return NULL;
    }
    return ptr;
#endif
}

// 释放可执行内存
void platform_free_executable(void* ptr, size_t size) {
    if (!ptr) return;

#ifdef _WIN32
    // Windows实现：使用VirtualFree
    // 注意：Windows的VirtualFree不需要size参数
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    // POSIX实现：使用munmap
    munmap(ptr, size);
#endif
}

// ===============================================
// 动态库加载实现
// ===============================================

// 打开动态库
platform_dl_handle platform_dl_open(const char* path) {
#ifdef _WIN32
    // Windows实现: LoadLibrary
    HMODULE handle = LoadLibraryA(path);
    return (platform_dl_handle)handle;
#else
    // POSIX实现: dlopen
    void* handle = dlopen(path, RTLD_NOW);
    return (platform_dl_handle)handle;
#endif
}

// 获取符号地址
void* platform_dl_sym(platform_dl_handle handle, const char* symbol) {
#ifdef _WIN32
    // Windows实现: GetProcAddress
    return (void*)GetProcAddress((HMODULE)handle, symbol);
#else
    // POSIX实现: dlsym
    return dlsym(handle, symbol);
#endif
}

// 关闭动态库
int platform_dl_close(platform_dl_handle handle) {
#ifdef _WIN32
    // Windows实现: FreeLibrary
    if (FreeLibrary((HMODULE)handle)) {
        return 0; // 成功
    }
    return 1; // 失败
#else
    // POSIX实现: dlclose
    return dlclose(handle);
#endif
}

// 获取错误信息
const char* platform_dl_error(void) {
#ifdef _WIN32
    // Windows实现: FormatMessage
    static char error_buffer[256];
    DWORD error_code = GetLastError();
    if (error_code == 0) {
        return NULL; // 没有错误
    }
    
    DWORD result = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error_code, 0, error_buffer, sizeof(error_buffer), NULL);
    
    if (result == 0) {
        snprintf(error_buffer, sizeof(error_buffer), 
                "Unknown error code: %lu", error_code);
    }
    
    return error_buffer;
#else
    // POSIX实现: dlerror
    return dlerror();
#endif
}

// ===============================================
// 网络抽象层实现
// ===============================================

// Windows下的WSA初始化
#ifdef _WIN32
static int wsa_initialized = 0;

static int initialize_wsa(void) {
    if (!wsa_initialized) {
        WSADATA wsa_data;
        int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0) {
            return -1;
        }
        wsa_initialized = 1;
    }
    return 0;
}
#endif

// 创建套接字
platform_socket_t platform_socket_create(platform_address_family family, 
                                        platform_socket_type type,
                                        platform_protocol_type protocol) {
#ifdef _WIN32
    if (initialize_wsa() != 0) {
        return (platform_socket_t)INVALID_SOCKET;
    }
    
    int af, sock_type, proto;
    
    // 地址族转换
    switch (family) {
        case PLATFORM_AF_INET: af = AF_INET; break;
        case PLATFORM_AF_INET6: af = AF_INET6; break;
        case PLATFORM_AF_UNIX: af = AF_UNIX; break;
        default: return (platform_socket_t)INVALID_SOCKET;
    }
    
    // 套接字类型转换
    switch (type) {
        case PLATFORM_SOCK_STREAM: sock_type = SOCK_STREAM; break;
        case PLATFORM_SOCK_DGRAM: sock_type = SOCK_DGRAM; break;
        case PLATFORM_SOCK_RAW: sock_type = SOCK_RAW; break;
        default: return (platform_socket_t)INVALID_SOCKET;
    }
    
    // 协议类型转换
    switch (protocol) {
        case PLATFORM_IPPROTO_TCP: proto = IPPROTO_TCP; break;
        case PLATFORM_IPPROTO_UDP: proto = IPPROTO_UDP; break;
        case PLATFORM_IPPROTO_ICMP: proto = IPPROTO_ICMP; break;
        default: proto = 0;
    }
    
    SOCKET sock = socket(af, sock_type, proto);
    if (sock == INVALID_SOCKET) {
        return (platform_socket_t)INVALID_SOCKET;
    }
    return (platform_socket_t)sock;
#else
    int af, sock_type, proto;
    
    // 地址族转换
    switch (family) {
        case PLATFORM_AF_INET: af = AF_INET; break;
        case PLATFORM_AF_INET6: af = AF_INET6; break;
        case PLATFORM_AF_UNIX: af = AF_UNIX; break;
        default: return -1;
    }
    
    // 套接字类型转换
    switch (type) {
        case PLATFORM_SOCK_STREAM: sock_type = SOCK_STREAM; break;
        case PLATFORM_SOCK_DGRAM: sock_type = SOCK_DGRAM; break;
        case PLATFORM_SOCK_RAW: sock_type = SOCK_RAW; break;
        default: return -1;
    }
    
    // 协议类型转换
    switch (protocol) {
        case PLATFORM_IPPROTO_TCP: proto = IPPROTO_TCP; break;
        case PLATFORM_IPPROTO_UDP: proto = IPPROTO_UDP; break;
        case PLATFORM_IPPROTO_ICMP: proto = IPPROTO_ICMP; break;
        default: proto = 0;
    }
    
    int sock = socket(af, sock_type, proto);
    if (sock < 0) {
        return -1;
    }
    return sock;
#endif
}

// 绑定套接字
int platform_socket_bind(platform_socket_t sock, const platform_sockaddr* addr) {
    if (sock < 0) return -1;
    
    switch (addr->family) {
        case PLATFORM_AF_INET: {
            struct sockaddr_in sin;
            memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_port = htons(addr->port);
            sin.sin_addr.s_addr = htonl(addr->addr.ipv4);
            
#ifdef _WIN32
            return bind((SOCKET)sock, (struct sockaddr*)&sin, sizeof(sin));
#else
            return bind(sock, (struct sockaddr*)&sin, sizeof(sin));
#endif
        }
        
        case PLATFORM_AF_INET6: {
            // IPv6地址绑定 (简化版，实际需要IPv6地址转换)
            return -1;
        }
        
        case PLATFORM_AF_UNIX: {
#ifdef _WIN32
            // Windows不完全支持Unix域套接字
            return -1;
#else
            struct sockaddr_un sun;
            memset(&sun, 0, sizeof(sun));
            sun.sun_family = AF_UNIX;
            strncpy(sun.sun_path, addr->addr.path, sizeof(sun.sun_path) - 1);
            
            return bind(sock, (struct sockaddr*)&sun, sizeof(sun));
#endif
        }
        
        default:
            return -1;
    }
}

// 监听连接
int platform_socket_listen(platform_socket_t sock, int backlog) {
#ifdef _WIN32
    return listen((SOCKET)sock, backlog);
#else
    return listen(sock, backlog);
#endif
}

// 接受连接
platform_socket_t platform_socket_accept(platform_socket_t sock, platform_sockaddr* addr) {
#ifdef _WIN32
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    
    SOCKET client_sock = accept((SOCKET)sock, (struct sockaddr*)&client_addr, &addr_len);
    if (client_sock == INVALID_SOCKET) {
        return (platform_socket_t)INVALID_SOCKET;
    }
    
    if (addr) {
        addr->family = PLATFORM_AF_INET;
        addr->port = ntohs(client_addr.sin_port);
        addr->addr.ipv4 = ntohl(client_addr.sin_addr.s_addr);
    }
    
    return (platform_socket_t)client_sock;
#else
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);
    if (client_sock < 0) {
        return -1;
    }
    
    if (addr) {
        addr->family = PLATFORM_AF_INET;
        addr->port = ntohs(client_addr.sin_port);
        addr->addr.ipv4 = ntohl(client_addr.sin_addr.s_addr);
    }
    
    return client_sock;
#endif
}

// 连接到服务器
int platform_socket_connect(platform_socket_t sock, const platform_sockaddr* addr) {
    if (sock < 0 || !addr) return -1;
    
    switch (addr->family) {
        case PLATFORM_AF_INET: {
            struct sockaddr_in sin;
            memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_port = htons(addr->port);
            sin.sin_addr.s_addr = htonl(addr->addr.ipv4);
            
#ifdef _WIN32
            return connect((SOCKET)sock, (struct sockaddr*)&sin, sizeof(sin));
#else
            return connect(sock, (struct sockaddr*)&sin, sizeof(sin));
#endif
        }
        
        // 其他地址族类似，此处省略...
        
        default:
            return -1;
    }
}

// 发送数据
int platform_socket_send(platform_socket_t sock, const void* buffer, size_t length) {
#ifdef _WIN32
    return send((SOCKET)sock, (const char*)buffer, (int)length, 0);
#else
    return send(sock, buffer, length, 0);
#endif
}

// 接收数据
int platform_socket_recv(platform_socket_t sock, void* buffer, size_t length) {
#ifdef _WIN32
    return recv((SOCKET)sock, (char*)buffer, (int)length, 0);
#else
    return recv(sock, buffer, length, 0);
#endif
}

// 关闭套接字
int platform_socket_close(platform_socket_t sock) {
#ifdef _WIN32
    return closesocket((SOCKET)sock) == 0 ? 0 : -1;
#else
    return close(sock);
#endif
}

// 设置非阻塞模式
int platform_socket_set_nonblock(platform_socket_t sock, int nonblock) {
#ifdef _WIN32
    u_long mode = nonblock ? 1 : 0;
    return ioctlsocket((SOCKET)sock, FIONBIO, &mode) == 0 ? 0 : -1;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -1;
    
    if (nonblock) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    
    return fcntl(sock, F_SETFL, flags) == 0 ? 0 : -1;
#endif
}

// 获取套接字错误
int platform_socket_get_error(void) {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

// 获取套接字错误信息
const char* platform_socket_error_string(int errcode) {
#ifdef _WIN32
    static char error_buffer[256];
    
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errcode, 0, error_buffer, sizeof(error_buffer), NULL);
    
    return error_buffer;
#else
    return strerror(errcode);
#endif
}

// 从字符串创建IPv4地址
int platform_sockaddr_from_ipv4(platform_sockaddr* addr, const char* ip_str, uint16_t port) {
    if (!addr || !ip_str) return -1;
    
    addr->family = PLATFORM_AF_INET;
    addr->port = port;
    
    // 将x.x.x.x形式的地址转换为网络字节序的32位整数
    struct in_addr in_addr;
    if (inet_pton(AF_INET, ip_str, &in_addr) <= 0) {
        return -1;
    }
    
    addr->addr.ipv4 = ntohl(in_addr.s_addr);
    return 0;
}

// ===============================================
// 事件循环实现 (Windows: IOCP / Linux: epoll)
// ===============================================

// 事件循环结构定义
#ifdef _WIN32
typedef struct {
    HANDLE iocp;
} win_event_loop_t;
#else
typedef struct {
    int epoll_fd;
} unix_event_loop_t;
#endif

// 创建事件循环
platform_event_loop_t platform_event_loop_create(void) {
#ifdef _WIN32
    if (initialize_wsa() != 0) {
        return NULL;
    }
    
    win_event_loop_t* loop = (win_event_loop_t*)malloc(sizeof(win_event_loop_t));
    if (!loop) return NULL;
    
    // 创建IOCP
    loop->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (loop->iocp == NULL) {
        free(loop);
        return NULL;
    }
    
    return loop;
#else
    unix_event_loop_t* loop = (unix_event_loop_t*)malloc(sizeof(unix_event_loop_t));
    if (!loop) return NULL;
    
    // 创建epoll实例
    loop->epoll_fd = epoll_create1(0);
    if (loop->epoll_fd < 0) {
        free(loop);
        return NULL;
    }
    
    return loop;
#endif
}

// 销毁事件循环
int platform_event_loop_destroy(platform_event_loop_t loop) {
    if (!loop) return -1;
    
#ifdef _WIN32
    win_event_loop_t* win_loop = (win_event_loop_t*)loop;
    CloseHandle(win_loop->iocp);
    free(win_loop);
    return 0;
#else
    unix_event_loop_t* unix_loop = (unix_event_loop_t*)loop;
    close(unix_loop->epoll_fd);
    free(unix_loop);
    return 0;
#endif
}

// 添加事件
int platform_event_add(platform_event_loop_t loop, platform_socket_t sock, 
                      int events, void* user_data) {
    if (!loop || sock < 0) return -1;
    
#ifdef _WIN32
    win_event_loop_t* win_loop = (win_event_loop_t*)loop;
    
    // 将套接字关联到IOCP
    HANDLE result = CreateIoCompletionPort((HANDLE)sock, win_loop->iocp, 
                                          (ULONG_PTR)user_data, 0);
    if (result == NULL) {
        return -1;
    }
    
    // 在实际的Windows实现中，我们需要启动异步操作(如WSARecv/WSASend)
    // 这里简化处理，只返回成功
    return 0;
#else
    unix_event_loop_t* unix_loop = (unix_event_loop_t*)loop;
    
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    
    // 转换事件类型
    if (events & PLATFORM_SOCKET_READ) {
        ev.events |= EPOLLIN;
    }
    if (events & PLATFORM_SOCKET_WRITE) {
        ev.events |= EPOLLOUT;
    }
    if (events & PLATFORM_SOCKET_EXCEPTION) {
        ev.events |= EPOLLERR;
    }
    
    ev.data.ptr = user_data;
    
    return epoll_ctl(unix_loop->epoll_fd, EPOLL_CTL_ADD, sock, &ev);
#endif
}

// 修改事件
int platform_event_mod(platform_event_loop_t loop, platform_socket_t sock,
                      int events, void* user_data) {
    if (!loop || sock < 0) return -1;
    
#ifdef _WIN32
    // Windows IOCP不需要修改事件，因为它是基于完成通知的
    // 实际应用中需要取消当前I/O操作并启动新操作
    return 0;
#else
    unix_event_loop_t* unix_loop = (unix_event_loop_t*)loop;
    
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    
    // 转换事件类型
    if (events & PLATFORM_SOCKET_READ) {
        ev.events |= EPOLLIN;
    }
    if (events & PLATFORM_SOCKET_WRITE) {
        ev.events |= EPOLLOUT;
    }
    if (events & PLATFORM_SOCKET_EXCEPTION) {
        ev.events |= EPOLLERR;
    }
    
    ev.data.ptr = user_data;
    
    return epoll_ctl(unix_loop->epoll_fd, EPOLL_CTL_MOD, sock, &ev);
#endif
}

// 删除事件
int platform_event_del(platform_event_loop_t loop, platform_socket_t sock) {
    if (!loop || sock < 0) return -1;
    
#ifdef _WIN32
    // Windows IOCP没有直接删除事件的方法，通常通过关闭套接字来停止事件
    return 0;
#else
    unix_event_loop_t* unix_loop = (unix_event_loop_t*)loop;
    
    return epoll_ctl(unix_loop->epoll_fd, EPOLL_CTL_DEL, sock, NULL);
#endif
}

// 等待事件
int platform_event_wait(platform_event_loop_t loop, platform_socket_event* events, 
                       int max_events, int timeout) {
    if (!loop || !events || max_events <= 0) return -1;
    
#ifdef _WIN32
    win_event_loop_t* win_loop = (win_event_loop_t*)loop;
    int event_count = 0;
    
    // 设置超时
    DWORD wait_ms = (timeout < 0) ? INFINITE : (DWORD)timeout;
    
    for (int i = 0; i < max_events && event_count < max_events; i++) {
        DWORD bytes;
        ULONG_PTR key;
        OVERLAPPED* overlapped;
        
        // 等待完成通知
        BOOL result = GetQueuedCompletionStatus(
            win_loop->iocp, &bytes, &key, &overlapped, wait_ms);
        
        // 第一次之后使用0超时，以便立即返回没有更多事件
        wait_ms = 0;
        
        if (!result && !overlapped) {
            // 超时或错误
            if (GetLastError() == WAIT_TIMEOUT && event_count == 0) {
                return 0; // 超时
            }
            break;
        }
        
        // 填充事件结构
        events[event_count].user_data = (void*)key;
        events[event_count].events = 0; // 需要根据实际完成的操作设置
        event_count++;
    }
    
    return event_count;
#else
    unix_event_loop_t* unix_loop = (unix_event_loop_t*)loop;
    
    struct epoll_event epoll_events[max_events];
    int ready = epoll_wait(unix_loop->epoll_fd, epoll_events, max_events, timeout);
    
    if (ready <= 0) {
        return ready; // 错误或超时
    }
    
    // 转换事件
    for (int i = 0; i < ready; i++) {
        events[i].socket = 0; // 在epoll中我们需要特别处理这个，在某些情况下获取fd
        events[i].user_data = epoll_events[i].data.ptr;
        events[i].events = 0;
        
        if (epoll_events[i].events & EPOLLIN) {
            events[i].events |= PLATFORM_SOCKET_READ;
        }
        if (epoll_events[i].events & EPOLLOUT) {
            events[i].events |= PLATFORM_SOCKET_WRITE;
        }
        if (epoll_events[i].events & (EPOLLERR | EPOLLHUP)) {
            events[i].events |= PLATFORM_SOCKET_EXCEPTION;
        }
    }
    
    return ready;
#endif
} 