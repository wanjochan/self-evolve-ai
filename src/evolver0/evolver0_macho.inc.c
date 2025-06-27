/**
 * evolver0_macho.inc.c - Mach-O可执行文件生成模块
 * 被 evolver0.c 包含使用
 */

#ifndef EVOLVER0_MACHO_INC_C
#define EVOLVER0_MACHO_INC_C

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ====================================
// Mach-O 文件格式定义
// ====================================

#define MH_MAGIC    0xfeedface  // 32位小端序
#define MH_CIGAM    0xcefaedfe  // 32位大端序
#define MH_MAGIC_64 0xfeedfacf  // 64位小端序
#define MH_CIGAM_64 0xcffaedfe  // 64位大端序

// CPU类型
#define CPU_TYPE_I386      7
#define CPU_TYPE_X86_64    (CPU_TYPE_I386 | 0x01000000)
#define CPU_TYPE_ARM       12
#define CPU_TYPE_ARM64     (CPU_TYPE_ARM | 0x01000000)

// 文件类型
#define MH_EXECUTE  2  // 可执行文件

// 加载命令类型
#define LC_SEGMENT      0x1  // 段命令 (32位)
#define LC_SEGMENT_64   0x19 // 段命令 (64位)
#define LC_UNIXTHREAD   0x5  // UNIX线程状态
#define LC_MAIN         0x28 // 主程序入口点

// 段标志
#define VM_PROT_READ    0x1
#define VM_PROT_WRITE   0x2
#define VM_PROT_EXECUTE 0x4

// x86_64线程状态
#define x86_THREAD_STATE64 4
#define x86_THREAD_STATE64_COUNT ((sizeof(x86_thread_state64_t)) / sizeof(uint32_t))

// ====================================
// Mach-O 结构体定义
// ====================================

// Mach-O头 (32位)
typedef struct {
    uint32_t magic;           // 魔数
    uint32_t cputype;         // CPU类型
    uint32_t cpusubtype;      // CPU子类型
    uint32_t filetype;        // 文件类型
    uint32_t ncmds;           // 加载命令数量
    uint32_t sizeofcmds;      // 加载命令大小
    uint32_t flags;           // 标志
} mach_header;

// Mach-O头 (64位)
typedef struct {
    uint32_t magic;           // 魔数
    uint32_t cputype;         // CPU类型
    uint32_t cpusubtype;      // CPU子类型
    uint32_t filetype;        // 文件类型
    uint32_t ncmds;           // 加载命令数量
    uint32_t sizeofcmds;      // 加载命令大小
    uint32_t flags;           // 标志
    uint32_t reserved;        // 保留
} mach_header_64;

// 加载命令
typedef struct {
    uint32_t cmd;             // 命令类型
    uint32_t cmdsize;         // 命令大小
} load_command;

// 段命令 (32位)
typedef struct {
    uint32_t cmd;             // LC_SEGMENT
    uint32_t cmdsize;         // 命令大小
    char     segname[16];     // 段名称
    uint32_t vmaddr;          // 虚拟内存地址
    uint32_t vmsize;          // 虚拟内存大小
    uint32_t fileoff;         // 文件偏移
    uint32_t filesize;        // 文件大小
    uint32_t maxprot;         // 最大保护
    uint32_t initprot;        // 初始保护
    uint32_t nsects;          // 节数量
    uint32_t flags;           // 标志
} segment_command;

// 段命令 (64位)
typedef struct {
    uint32_t cmd;             // LC_SEGMENT_64
    uint32_t cmdsize;         // 命令大小
    char     segname[16];     // 段名称
    uint64_t vmaddr;          // 虚拟内存地址
    uint64_t vmsize;          // 虚拟内存大小
    uint64_t fileoff;         // 文件偏移
    uint64_t filesize;        // 文件大小
    uint32_t maxprot;         // 最大保护
    uint32_t initprot;        // 初始保护
    uint32_t nsects;          // 节数量
    uint32_t flags;           // 标志
} segment_command_64;

// 节 (32位)
typedef struct {
    char     sectname[16];    // 节名称
    char     segname[16];     // 段名称
    uint32_t addr;            // 内存地址
    uint32_t size;            // 大小
    uint32_t offset;          // 文件偏移
    uint32_t align;           // 对齐(2的幂)
    uint32_t reloff;          // 重定位条目偏移
    uint32_t nreloc;          // 重定位条目数量
    uint32_t flags;           // 标志
    uint32_t reserved1;       // 保留
    uint32_t reserved2;       // 保留
} section;

// 节 (64位)
typedef struct {
    char     sectname[16];    // 节名称
    char     segname[16];     // 段名称
    uint64_t addr;            // 内存地址
    uint64_t size;            // 大小
    uint32_t offset;          // 文件偏移
    uint32_t align;           // 对齐(2的幂)
    uint32_t reloff;          // 重定位条目偏移
    uint32_t nreloc;          // 重定位条目数量
    uint32_t flags;           // 标志
    uint32_t reserved1;       // 保留
    uint32_t reserved2;       // 保留
    uint32_t reserved3;       // 保留 (64位)
} section_64;

// x86_64线程状态
typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t rflags;
    uint64_t cs;
    uint64_t fs;
    uint64_t gs;
} x86_thread_state64_t;

// UNIX线程命令
typedef struct {
    uint32_t cmd;             // LC_UNIXTHREAD
    uint32_t cmdsize;         // 命令大小
    uint32_t flavor;          // 线程状态类型
    uint32_t count;           // 线程状态大小
    x86_thread_state64_t state;  // 线程状态
} thread_command;

// 入口点命令
typedef struct {
    uint32_t cmd;             // LC_MAIN
    uint32_t cmdsize;         // 命令大小
    uint64_t entryoff;        // 入口点文件偏移
    uint64_t stacksize;       // 初始栈大小
} entry_point_command;

// ====================================
// Mach-O 生成器
// ====================================

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
    
    // 目标信息
    int bits;       // 32 或 64
    int cpu_type;   // CPU_TYPE_I386 或 CPU_TYPE_X86_64
    
    // 段信息
    struct {
        char segname[16];
        char sectname[16];
        uint64_t vmaddr;
        uint64_t vmsize;
        uint64_t fileoff;
        uint64_t filesize;
        uint32_t maxprot;
        uint32_t initprot;
    } segments[16];
    int segment_count;
    
    // 入口点
    uint64_t entry_point;
    
} MachOBuilder;

// ====================================
// 辅助函数
// ====================================

static void init_macho_builder(MachOBuilder *builder, int bits) {
    builder->data = (uint8_t*)malloc(4096);
    builder->size = 0;
    builder->capacity = 4096;
    builder->bits = bits;
    builder->cpu_type = (bits == 64) ? CPU_TYPE_X86_64 : CPU_TYPE_I386;
    builder->segment_count = 0;
    builder->entry_point = 0;
}

static void free_macho_builder(MachOBuilder *builder) {
    free(builder->data);
}

static void macho_write_bytes(MachOBuilder *builder, const void *data, size_t size) {
    while (builder->size + size > builder->capacity) {
        builder->capacity *= 2;
        builder->data = (uint8_t*)realloc(builder->data, builder->capacity);
    }
    
    memcpy(builder->data + builder->size, data, size);
    builder->size += size;
}

static void macho_write_u8(MachOBuilder *builder, uint8_t value) {
    macho_write_bytes(builder, &value, 1);
}

static void macho_write_u16(MachOBuilder *builder, uint16_t value) {
    macho_write_bytes(builder, &value, 2);
}

static void macho_write_u32(MachOBuilder *builder, uint32_t value) {
    macho_write_bytes(builder, &value, 4);
}

static void macho_write_u64(MachOBuilder *builder, uint64_t value) {
    macho_write_bytes(builder, &value, 8);
}

static void macho_align(MachOBuilder *builder, size_t alignment) {
    while (builder->size % alignment != 0) {
        macho_write_u8(builder, 0);
    }
}

// ====================================
// 64位Mach-O头生成
// ====================================

static void generate_macho64_header(MachOBuilder *builder, int cmd_count) {
    mach_header_64 header = {0};
    header.magic = MH_MAGIC_64;
    header.cputype = CPU_TYPE_X86_64;
    header.cpusubtype = 0x80000003; // CPU_SUBTYPE_X86_64_ALL | CPU_SUBTYPE_LIB64
    header.filetype = MH_EXECUTE;
    header.ncmds = cmd_count;
    header.sizeofcmds = 0; // 稍后填充
    header.flags = 0;
    header.reserved = 0;
    
    macho_write_bytes(builder, &header, sizeof(header));
}

// ====================================
// 64位段命令生成
// ====================================

static void generate_segment64_command(MachOBuilder *builder, int segment_index) {
    segment_command_64 segment = {0};
    segment.cmd = LC_SEGMENT_64;
    segment.cmdsize = sizeof(segment_command_64) + sizeof(section_64);
    strncpy(segment.segname, builder->segments[segment_index].segname, 16);
    segment.vmaddr = builder->segments[segment_index].vmaddr;
    segment.vmsize = builder->segments[segment_index].vmsize;
    segment.fileoff = builder->segments[segment_index].fileoff;
    segment.filesize = builder->segments[segment_index].filesize;
    segment.maxprot = builder->segments[segment_index].maxprot;
    segment.initprot = builder->segments[segment_index].initprot;
    segment.nsects = 1;
    segment.flags = 0;
    
    macho_write_bytes(builder, &segment, sizeof(segment));
    
    // 添加节
    section_64 section = {0};
    strncpy(section.sectname, builder->segments[segment_index].sectname, 16);
    strncpy(section.segname, builder->segments[segment_index].segname, 16);
    section.addr = builder->segments[segment_index].vmaddr;
    section.size = builder->segments[segment_index].vmsize;
    section.offset = builder->segments[segment_index].fileoff;
    section.align = 0;
    section.reloff = 0;
    section.nreloc = 0;
    section.flags = 0;
    section.reserved1 = 0;
    section.reserved2 = 0;
    section.reserved3 = 0;
    
    macho_write_bytes(builder, &section, sizeof(section));
}

// ====================================
// 64位入口点命令生成
// ====================================

static void generate_entry_point_command(MachOBuilder *builder) {
    entry_point_command entry = {0};
    entry.cmd = LC_MAIN;
    entry.cmdsize = sizeof(entry_point_command);
    entry.entryoff = builder->entry_point;
    entry.stacksize = 0;
    
    macho_write_bytes(builder, &entry, sizeof(entry));
}

// ====================================
// 64位线程状态命令生成
// ====================================

static void generate_thread_command(MachOBuilder *builder, uint64_t entry_point) {
    thread_command thread = {0};
    thread.cmd = LC_UNIXTHREAD;
    thread.cmdsize = sizeof(thread_command);
    thread.flavor = x86_THREAD_STATE64;
    thread.count = x86_THREAD_STATE64_COUNT;
    
    // 设置入口点
    thread.state.rip = entry_point;
    
    macho_write_bytes(builder, &thread, sizeof(thread));
}

// ====================================
// Mach-O文件生成
// ====================================

static int create_macho64_executable(const char *filename, const uint8_t *code, size_t code_size) {
    MachOBuilder builder;
    init_macho_builder(&builder, 64);
    
    // 计算头部大小
    size_t header_size = sizeof(mach_header_64);
    size_t segment_cmd_size = sizeof(segment_command_64) + sizeof(section_64);
    size_t entry_cmd_size = sizeof(entry_point_command);
    size_t cmds_size = segment_cmd_size + entry_cmd_size;
    size_t total_header_size = header_size + cmds_size;
    
    // 对齐到页大小
    size_t page_size = 0x1000;
    size_t aligned_header_size = (total_header_size + page_size - 1) & ~(page_size - 1);
    
    // 添加代码段
    strcpy(builder.segments[0].segname, "__TEXT");
    strcpy(builder.segments[0].sectname, "__text");
    builder.segments[0].vmaddr = 0x100000000;
    builder.segments[0].vmsize = code_size;
    builder.segments[0].fileoff = aligned_header_size;
    builder.segments[0].filesize = code_size;
    builder.segments[0].maxprot = VM_PROT_READ | VM_PROT_EXECUTE;
    builder.segments[0].initprot = VM_PROT_READ | VM_PROT_EXECUTE;
    builder.segment_count = 1;
    
    // 设置入口点
    builder.entry_point = aligned_header_size;
    
    // 生成Mach-O头
    generate_macho64_header(&builder, 2); // 2个命令: LC_SEGMENT_64 + LC_MAIN
    
    // 生成段命令
    generate_segment64_command(&builder, 0);
    
    // 生成入口点命令
    generate_entry_point_command(&builder);
    
    // 更新命令大小
    uint32_t cmds_size_actual = builder.size - sizeof(mach_header_64);
    memcpy(builder.data + offsetof(mach_header_64, sizeofcmds), &cmds_size_actual, sizeof(uint32_t));
    
    // 对齐到页大小
    macho_align(&builder, page_size);
    
    // 写入代码段
    macho_write_bytes(&builder, code, code_size);
    
    // 写入文件
    FILE *file = fopen(filename, "wb");
    if (!file) {
        free_macho_builder(&builder);
        return 0;
    }
    
    fwrite(builder.data, 1, builder.size, file);
    fclose(file);
    
    free_macho_builder(&builder);
    return 1;
}

// ====================================
// 对外接口
// ====================================

static int write_macho_file(const char *filename, unsigned char *code, size_t code_size) {
    return create_macho64_executable(filename, code, code_size);
}

#endif // EVOLVER0_MACHO_INC_C 