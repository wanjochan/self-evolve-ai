/**
 * runtime_system.c - C99Bin Runtime System
 * 
 * T2.3: 运行时系统 - 程序启动、异常处理和系统调用支持
 * 为自举编译提供完整的运行时环境
 */

#include "pipeline_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// 运行时模块类型
typedef enum {
    RUNTIME_STARTUP,        // 程序启动
    RUNTIME_EXCEPTION,      // 异常处理
    RUNTIME_SYSCALL,        // 系统调用
    RUNTIME_SIGNAL,         // 信号处理
    RUNTIME_MEMORY,         // 内存管理
    RUNTIME_THREADING,      // 线程支持
    RUNTIME_CLEANUP,        // 清理机制
    RUNTIME_PROFILING       // 性能分析
} RuntimeModule;

// 程序启动信息
typedef struct ProgramInfo {
    int argc;
    char** argv;
    char** envp;
    char* program_name;
    char* working_directory;
    pid_t process_id;
    time_t start_time;
    bool is_self_hosted;
} ProgramInfo;

// 异常处理信息
typedef struct ExceptionInfo {
    int signal_number;
    char* signal_name;
    char* error_message;
    void* fault_address;
    bool is_recoverable;
    jmp_buf* recovery_point;
    struct ExceptionInfo* previous;
} ExceptionInfo;

// 系统调用包装器
typedef struct SystemCall {
    int syscall_number;
    char* syscall_name;
    void* handler;
    bool is_safe;
    bool is_setjmp_aware;
} SystemCall;

// 运行时上下文
typedef struct {
    ProgramInfo* program;
    ExceptionInfo* current_exception;
    SystemCall* syscall_table;
    int syscall_count;
    bool enable_exception_handling;
    bool enable_signal_handling;
    bool enable_profiling;
    bool enable_debug_mode;
    jmp_buf* global_exception_handler;
    void (*cleanup_handlers[16])(void);
    int cleanup_handler_count;
    FILE* runtime_log;
} RuntimeContext;

// 全局运行时上下文
static RuntimeContext* g_runtime_ctx = NULL;

// 运行时系统接口
bool initialize_runtime_system(int argc, char** argv, char** envp);
void cleanup_runtime_system(void);
RuntimeContext* get_runtime_context(void);

// 程序启动相关
bool setup_program_environment(int argc, char** argv, char** envp);
bool register_cleanup_handlers(void);
bool setup_signal_handlers(void);

// 异常处理相关
void handle_exception(int signal);
void handle_segmentation_fault(int signal);
void handle_floating_point_error(int signal);
bool install_exception_handlers(void);

// 系统调用相关
int c99bin_open(const char* pathname, int flags);
int c99bin_close(int fd);
ssize_t c99bin_read(int fd, void* buf, size_t count);
ssize_t c99bin_write(int fd, const void* buf, size_t count);
void* c99bin_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);

// 初始化运行时系统
bool initialize_runtime_system(int argc, char** argv, char** envp) {
    printf("🚀 Initializing C99Bin Runtime System...\n");
    printf("=========================================\n");
    printf("Program: %s\n", argc > 0 ? argv[0] : "unknown");
    printf("Arguments: %d\n", argc - 1);
    printf("\n");
    
    if (g_runtime_ctx) {
        printf("⚠️  Runtime system already initialized\n");
        return true;
    }
    
    g_runtime_ctx = malloc(sizeof(RuntimeContext));
    if (!g_runtime_ctx) {
        printf("❌ Failed to allocate runtime context\n");
        return false;
    }
    memset(g_runtime_ctx, 0, sizeof(RuntimeContext));
    
    // 设置默认配置
    g_runtime_ctx->enable_exception_handling = true;
    g_runtime_ctx->enable_signal_handling = true;
    g_runtime_ctx->enable_profiling = true;
    g_runtime_ctx->enable_debug_mode = true;
    
    // 阶段1: 程序环境设置
    printf("🔧 Phase 1: Program Environment Setup\n");
    printf("=====================================\n");
    if (!setup_program_environment(argc, argv, envp)) {
        printf("❌ Program environment setup failed\n");
        cleanup_runtime_system();
        return false;
    }
    
    // 阶段2: 异常处理安装
    if (g_runtime_ctx->enable_exception_handling) {
        printf("\n🛡️ Phase 2: Exception Handler Installation\n");
        printf("==========================================\n");
        if (!install_exception_handlers()) {
            printf("❌ Exception handler installation failed\n");
            cleanup_runtime_system();
            return false;
        }
    }
    
    // 阶段3: 信号处理设置
    if (g_runtime_ctx->enable_signal_handling) {
        printf("\n📡 Phase 3: Signal Handler Setup\n");
        printf("================================\n");
        if (!setup_signal_handlers()) {
            printf("❌ Signal handler setup failed\n");
            cleanup_runtime_system();
            return false;
        }
    }
    
    // 阶段4: 系统调用包装器
    printf("\n🔌 Phase 4: System Call Wrappers\n");
    printf("=================================\n");
    if (!setup_syscall_wrappers()) {
        printf("❌ System call wrapper setup failed\n");
        cleanup_runtime_system();
        return false;
    }
    
    // 阶段5: 清理处理器注册
    printf("\n🧹 Phase 5: Cleanup Handler Registration\n");
    printf("========================================\n");
    if (!register_cleanup_handlers()) {
        printf("❌ Cleanup handler registration failed\n");
        cleanup_runtime_system();
        return false;
    }
    
    // 开启运行时日志
    if (g_runtime_ctx->enable_debug_mode) {
        g_runtime_ctx->runtime_log = fopen("c99bin_runtime.log", "w");
        if (g_runtime_ctx->runtime_log) {
            fprintf(g_runtime_ctx->runtime_log, "C99Bin Runtime System Started\n");
            fprintf(g_runtime_ctx->runtime_log, "Program: %s\n", g_runtime_ctx->program->program_name);
            fprintf(g_runtime_ctx->runtime_log, "PID: %d\n", g_runtime_ctx->program->process_id);
            fflush(g_runtime_ctx->runtime_log);
        }
    }
    
    printf("✅ C99Bin Runtime System initialized successfully!\n");
    printf("   - Program: %s (PID: %d)\n", 
           g_runtime_ctx->program->program_name, 
           g_runtime_ctx->program->process_id);
    printf("   - Exception handling: %s\n", 
           g_runtime_ctx->enable_exception_handling ? "Enabled" : "Disabled");
    printf("   - Signal handling: %s\n", 
           g_runtime_ctx->enable_signal_handling ? "Enabled" : "Disabled");
    printf("   - System call wrappers: %d registered\n", g_runtime_ctx->syscall_count);
    printf("   - Cleanup handlers: %d registered\n", g_runtime_ctx->cleanup_handler_count);
    
    return true;
}

// 设置程序环境
bool setup_program_environment(int argc, char** argv, char** envp) {
    printf("🔧 Setting up program environment...\n");
    
    g_runtime_ctx->program = malloc(sizeof(ProgramInfo));
    if (!g_runtime_ctx->program) {
        printf("❌ Failed to allocate program info\n");
        return false;
    }
    
    ProgramInfo* prog = g_runtime_ctx->program;
    memset(prog, 0, sizeof(ProgramInfo));
    
    // 保存程序参数
    prog->argc = argc;
    prog->argv = malloc(sizeof(char*) * argc);
    for (int i = 0; i < argc; i++) {
        prog->argv[i] = strdup(argv[i]);
    }
    
    // 保存环境变量 (简化)
    int env_count = 0;
    if (envp) {
        while (envp[env_count]) env_count++;
    }
    prog->envp = malloc(sizeof(char*) * (env_count + 1));
    for (int i = 0; i < env_count; i++) {
        prog->envp[i] = strdup(envp[i]);
    }
    prog->envp[env_count] = NULL;
    
    // 程序基本信息
    prog->program_name = strdup(argc > 0 ? argv[0] : "c99bin");
    prog->working_directory = getcwd(NULL, 0);
    prog->process_id = getpid();
    prog->start_time = time(NULL);
    
    // 检测是否是自托管编译
    prog->is_self_hosted = strstr(prog->program_name, "c99bin") != NULL;
    
    printf("✅ Program environment setup complete\n");
    printf("   - Program name: %s\n", prog->program_name);
    printf("   - Working directory: %s\n", prog->working_directory);
    printf("   - Process ID: %d\n", prog->process_id);
    printf("   - Self-hosted: %s\n", prog->is_self_hosted ? "Yes" : "No");
    printf("   - Arguments: %d\n", prog->argc - 1);
    printf("   - Environment variables: %d\n", env_count);
    
    return true;
}

// 安装异常处理器
bool install_exception_handlers(void) {
    printf("🛡️ Installing exception handlers...\n");
    
    g_runtime_ctx->global_exception_handler = malloc(sizeof(jmp_buf));
    if (!g_runtime_ctx->global_exception_handler) {
        printf("❌ Failed to allocate global exception handler\n");
        return false;
    }
    
    // 设置全局异常处理点
    if (setjmp(*g_runtime_ctx->global_exception_handler) != 0) {
        printf("🚨 Exception caught by global handler\n");
        if (g_runtime_ctx->current_exception) {
            printf("   Signal: %s (%d)\n", 
                   g_runtime_ctx->current_exception->signal_name,
                   g_runtime_ctx->current_exception->signal_number);
            printf("   Message: %s\n", g_runtime_ctx->current_exception->error_message);
            printf("   Recoverable: %s\n", 
                   g_runtime_ctx->current_exception->is_recoverable ? "Yes" : "No");
        }
        return false;
    }
    
    printf("✅ Exception handlers installed\n");
    printf("   - Global exception handler: Ready\n");
    printf("   - setjmp/longjmp integration: Active\n");
    printf("   - Exception recovery: Enabled\n");
    
    return true;
}

// 设置信号处理器
bool setup_signal_handlers(void) {
    printf("📡 Setting up signal handlers...\n");
    
    // 注册关键信号处理器
    signal(SIGSEGV, handle_segmentation_fault);
    signal(SIGFPE, handle_floating_point_error);
    signal(SIGABRT, handle_exception);
    signal(SIGTERM, handle_exception);
    signal(SIGINT, handle_exception);
    
    printf("✅ Signal handlers setup complete\n");
    printf("   - SIGSEGV: Segmentation fault handler\n");
    printf("   - SIGFPE: Floating point error handler\n");
    printf("   - SIGABRT: Abort signal handler\n");
    printf("   - SIGTERM: Termination signal handler\n");
    printf("   - SIGINT: Interrupt signal handler\n");
    
    return true;
}

// 设置系统调用包装器
bool setup_syscall_wrappers(void) {
    printf("🔌 Setting up system call wrappers...\n");
    
    g_runtime_ctx->syscall_count = 5; // 基本系统调用数量
    g_runtime_ctx->syscall_table = malloc(sizeof(SystemCall) * g_runtime_ctx->syscall_count);
    
    SystemCall* syscalls = g_runtime_ctx->syscall_table;
    
    // 文件操作系统调用
    syscalls[0] = (SystemCall){
        .syscall_number = 2,    // open
        .syscall_name = strdup("open"),
        .handler = c99bin_open,
        .is_safe = true,
        .is_setjmp_aware = true
    };
    
    syscalls[1] = (SystemCall){
        .syscall_number = 3,    // close
        .syscall_name = strdup("close"),
        .handler = c99bin_close,
        .is_safe = true,
        .is_setjmp_aware = false
    };
    
    syscalls[2] = (SystemCall){
        .syscall_number = 0,    // read
        .syscall_name = strdup("read"),
        .handler = c99bin_read,
        .is_safe = true,
        .is_setjmp_aware = true
    };
    
    syscalls[3] = (SystemCall){
        .syscall_number = 1,    // write
        .syscall_name = strdup("write"),
        .handler = c99bin_write,
        .is_safe = true,
        .is_setjmp_aware = true
    };
    
    syscalls[4] = (SystemCall){
        .syscall_number = 9,    // mmap
        .syscall_name = strdup("mmap"),
        .handler = c99bin_mmap,
        .is_safe = false,
        .is_setjmp_aware = true
    };
    
    printf("✅ System call wrappers setup complete\n");
    printf("   - File operations: open, close, read, write\n");
    printf("   - Memory operations: mmap\n");
    printf("   - setjmp-aware wrappers: 4/5\n");
    printf("   - Safe wrappers: 4/5\n");
    
    return true;
}

// 注册清理处理器
bool register_cleanup_handlers(void) {
    printf("🧹 Registering cleanup handlers...\n");
    
    g_runtime_ctx->cleanup_handler_count = 0;
    
    // 注册标准库清理
    register_cleanup_handler(cleanup_standard_library);
    
    // 注册运行时系统清理
    register_cleanup_handler(cleanup_runtime_system);
    
    // 注册atexit处理器
    atexit(runtime_exit_handler);
    
    printf("✅ Cleanup handlers registered\n");
    printf("   - Standard library cleanup: Registered\n");
    printf("   - Runtime system cleanup: Registered\n");
    printf("   - atexit handler: Registered\n");
    printf("   - Total handlers: %d\n", g_runtime_ctx->cleanup_handler_count);
    
    return true;
}

// 注册单个清理处理器
void register_cleanup_handler(void (*handler)(void)) {
    if (g_runtime_ctx->cleanup_handler_count < 16) {
        g_runtime_ctx->cleanup_handlers[g_runtime_ctx->cleanup_handler_count++] = handler;
    }
}

// 处理段错误
void handle_segmentation_fault(int signal) {
    ExceptionInfo* exc = malloc(sizeof(ExceptionInfo));
    exc->signal_number = signal;
    exc->signal_name = strdup("SIGSEGV");
    exc->error_message = strdup("Segmentation fault");
    exc->fault_address = NULL; // 在真实实现中会获取故障地址
    exc->is_recoverable = false;
    exc->previous = g_runtime_ctx->current_exception;
    g_runtime_ctx->current_exception = exc;
    
    if (g_runtime_ctx->runtime_log) {
        fprintf(g_runtime_ctx->runtime_log, "FATAL: Segmentation fault\n");
        fflush(g_runtime_ctx->runtime_log);
    }
    
    printf("💥 FATAL: Segmentation fault detected\n");
    printf("Program will be terminated to prevent corruption\n");
    
    // 跳转到全局异常处理器
    if (g_runtime_ctx->global_exception_handler) {
        longjmp(*g_runtime_ctx->global_exception_handler, signal);
    }
    
    exit(1);
}

// 处理浮点错误
void handle_floating_point_error(int signal) {
    ExceptionInfo* exc = malloc(sizeof(ExceptionInfo));
    exc->signal_number = signal;
    exc->signal_name = strdup("SIGFPE");
    exc->error_message = strdup("Floating point error");
    exc->fault_address = NULL;
    exc->is_recoverable = true;
    exc->previous = g_runtime_ctx->current_exception;
    g_runtime_ctx->current_exception = exc;
    
    printf("⚠️  Floating point error detected\n");
    
    // 对于浮点错误，尝试恢复
    if (g_runtime_ctx->global_exception_handler) {
        longjmp(*g_runtime_ctx->global_exception_handler, signal);
    }
}

// 通用异常处理器
void handle_exception(int signal) {
    ExceptionInfo* exc = malloc(sizeof(ExceptionInfo));
    exc->signal_number = signal;
    
    switch (signal) {
        case SIGABRT:
            exc->signal_name = strdup("SIGABRT");
            exc->error_message = strdup("Program aborted");
            exc->is_recoverable = false;
            break;
        case SIGTERM:
            exc->signal_name = strdup("SIGTERM");
            exc->error_message = strdup("Termination request");
            exc->is_recoverable = true;
            break;
        case SIGINT:
            exc->signal_name = strdup("SIGINT");
            exc->error_message = strdup("Interrupt signal");
            exc->is_recoverable = true;
            break;
        default:
            exc->signal_name = strdup("UNKNOWN");
            exc->error_message = strdup("Unknown signal");
            exc->is_recoverable = false;
            break;
    }
    
    exc->previous = g_runtime_ctx->current_exception;
    g_runtime_ctx->current_exception = exc;
    
    printf("🚨 Signal %d (%s) received: %s\n", 
           signal, exc->signal_name, exc->error_message);
    
    if (exc->is_recoverable) {
        printf("   Attempting graceful shutdown...\n");
        runtime_exit_handler();
        exit(0);
    } else {
        printf("   Fatal error - immediate termination\n");
        exit(1);
    }
}

// 系统调用包装器实现
int c99bin_open(const char* pathname, int flags) {
    // 在setjmp/longjmp上下文中安全的文件打开
    if (g_runtime_ctx->runtime_log) {
        fprintf(g_runtime_ctx->runtime_log, "SYSCALL: open(%s, %d)\n", pathname, flags);
        fflush(g_runtime_ctx->runtime_log);
    }
    
    return open(pathname, flags);
}

ssize_t c99bin_write(int fd, const void* buf, size_t count) {
    // setjmp-aware的写入操作
    if (g_runtime_ctx->runtime_log && fd != fileno(g_runtime_ctx->runtime_log)) {
        fprintf(g_runtime_ctx->runtime_log, "SYSCALL: write(%d, %p, %zu)\n", fd, buf, count);
        fflush(g_runtime_ctx->runtime_log);
    }
    
    return write(fd, buf, count);
}

// 运行时退出处理器
void runtime_exit_handler(void) {
    printf("\n🧹 C99Bin Runtime System Cleanup\n");
    printf("================================\n");
    
    // 执行所有注册的清理处理器
    for (int i = g_runtime_ctx->cleanup_handler_count - 1; i >= 0; i--) {
        if (g_runtime_ctx->cleanup_handlers[i]) {
            g_runtime_ctx->cleanup_handlers[i]();
        }
    }
    
    if (g_runtime_ctx->runtime_log) {
        fprintf(g_runtime_ctx->runtime_log, "Runtime system cleanup completed\n");
        fclose(g_runtime_ctx->runtime_log);
    }
    
    printf("✅ Runtime cleanup completed\n");
}

// 获取运行时上下文
RuntimeContext* get_runtime_context(void) {
    return g_runtime_ctx;
}

// 清理运行时系统
void cleanup_runtime_system(void) {
    if (g_runtime_ctx) {
        // 清理程序信息
        if (g_runtime_ctx->program) {
            ProgramInfo* prog = g_runtime_ctx->program;
            for (int i = 0; i < prog->argc; i++) {
                free(prog->argv[i]);
            }
            free(prog->argv);
            
            if (prog->envp) {
                int i = 0;
                while (prog->envp[i]) {
                    free(prog->envp[i]);
                    i++;
                }
                free(prog->envp);
            }
            
            free(prog->program_name);
            free(prog->working_directory);
            free(prog);
        }
        
        // 清理异常信息
        ExceptionInfo* exc = g_runtime_ctx->current_exception;
        while (exc) {
            ExceptionInfo* prev = exc->previous;
            free(exc->signal_name);
            free(exc->error_message);
            free(exc);
            exc = prev;
        }
        
        // 清理系统调用表
        if (g_runtime_ctx->syscall_table) {
            for (int i = 0; i < g_runtime_ctx->syscall_count; i++) {
                free(g_runtime_ctx->syscall_table[i].syscall_name);
            }
            free(g_runtime_ctx->syscall_table);
        }
        
        if (g_runtime_ctx->global_exception_handler) {
            free(g_runtime_ctx->global_exception_handler);
        }
        
        if (g_runtime_ctx->runtime_log) {
            fclose(g_runtime_ctx->runtime_log);
        }
        
        free(g_runtime_ctx);
        g_runtime_ctx = NULL;
    }
}