/**
 * evolver0_elf.inc.c - ELF可执行文件生成模块
 * 被 evolver0.c 包含使用
 */

#ifndef EVOLVER0_ELF_INC_C
#define EVOLVER0_ELF_INC_C

#include <stdint.h>
#include <sys/stat.h>

// ====================================
// ELF 文件格式定义
// ====================================

#define ELF_MAGIC 0x464C457F  // 0x7F, 'E', 'L', 'F'

// ELF 类型
#define ELFCLASS32  1
#define ELFCLASS64  2

// 数据编码
#define ELFDATA2LSB 1  // Little-endian
#define ELFDATA2MSB 2  // Big-endian

// 文件类型
#define ET_EXEC     2  // 可执行文件
#define ET_DYN      3  // 共享对象

// 机器类型
#define EM_386      3  // Intel 80386
#define EM_X86_64  62  // AMD x86-64

// 段类型
#define PT_LOAD     1  // 可加载段

// 段标志
#define PF_X        0x1  // 可执行
#define PF_W        0x2  // 可写
#define PF_R        0x4  // 可读

// ====================================
// ELF 结构体定义
// ====================================

// ELF 标识
typedef struct {
    uint8_t  magic[4];      // 0x7F, 'E', 'L', 'F'
    uint8_t  class;         // 32/64位
    uint8_t  data;          // 字节序
    uint8_t  version;       // ELF版本
    uint8_t  osabi;         // OS/ABI
    uint8_t  abiversion;    // ABI版本
    uint8_t  pad[7];        // 填充
} Elf_Ident;

// 64位 ELF 头
typedef struct {
    Elf_Ident ident;        // ELF标识
    uint16_t  type;         // 文件类型
    uint16_t  machine;      // 机器类型
    uint32_t  version;      // 文件版本
    uint64_t  entry;        // 入口地址
    uint64_t  phoff;        // 程序头偏移
    uint64_t  shoff;        // 节头偏移
    uint32_t  flags;        // 处理器标志
    uint16_t  ehsize;       // ELF头大小
    uint16_t  phentsize;    // 程序头项大小
    uint16_t  phnum;        // 程序头项数量
    uint16_t  shentsize;    // 节头项大小
    uint16_t  shnum;        // 节头项数量
    uint16_t  shstrndx;     // 字符串表索引
} Elf64_Ehdr;

// 64位程序头
typedef struct {
    uint32_t  type;         // 段类型
    uint32_t  flags;        // 段标志
    uint64_t  offset;       // 文件偏移
    uint64_t  vaddr;        // 虚拟地址
    uint64_t  paddr;        // 物理地址
    uint64_t  filesz;       // 文件大小
    uint64_t  memsz;        // 内存大小
    uint64_t  align;        // 对齐
} Elf64_Phdr;

// 32位 ELF 头
typedef struct {
    Elf_Ident ident;        // ELF标识
    uint16_t  type;         // 文件类型
    uint16_t  machine;      // 机器类型
    uint32_t  version;      // 文件版本
    uint32_t  entry;        // 入口地址
    uint32_t  phoff;        // 程序头偏移
    uint32_t  shoff;        // 节头偏移
    uint32_t  flags;        // 处理器标志
    uint16_t  ehsize;       // ELF头大小
    uint16_t  phentsize;    // 程序头项大小
    uint16_t  phnum;        // 程序头项数量
    uint16_t  shentsize;    // 节头项大小
    uint16_t  shnum;        // 节头项数量
    uint16_t  shstrndx;     // 字符串表索引
} Elf32_Ehdr;

// 32位程序头
typedef struct {
    uint32_t  type;         // 段类型
    uint32_t  offset;       // 文件偏移
    uint32_t  vaddr;        // 虚拟地址
    uint32_t  paddr;        // 物理地址
    uint32_t  filesz;       // 文件大小
    uint32_t  memsz;        // 内存大小
    uint32_t  flags;        // 段标志
    uint32_t  align;        // 对齐
} Elf32_Phdr;

// ====================================
// ELF 生成器
// ====================================

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
    
    // 目标信息
    int bits;       // 32 或 64
    int machine;    // EM_386 或 EM_X86_64
    
    // 段信息
    struct {
        uint64_t vaddr;
        uint64_t offset;
        uint64_t size;
        uint32_t flags;
    } segments[16];
    int segment_count;
    
} ElfBuilder;

// ====================================
// 辅助函数
// ====================================

static void init_elf_builder(ElfBuilder *builder, int bits) {
    builder->data = (uint8_t*)malloc(4096);
    builder->size = 0;
    builder->capacity = 4096;
    builder->bits = bits;
    builder->machine = (bits == 64) ? EM_X86_64 : EM_386;
    builder->segment_count = 0;
}

static void free_elf_builder(ElfBuilder *builder) {
    free(builder->data);
}

static void elf_write_bytes(ElfBuilder *builder, const void *data, size_t size) {
    while (builder->size + size > builder->capacity) {
        builder->capacity *= 2;
        builder->data = (uint8_t*)realloc(builder->data, builder->capacity);
    }
    
    memcpy(builder->data + builder->size, data, size);
    builder->size += size;
}

static void elf_write_u8(ElfBuilder *builder, uint8_t value) {
    elf_write_bytes(builder, &value, 1);
}

static void elf_write_u16(ElfBuilder *builder, uint16_t value) {
    elf_write_bytes(builder, &value, 2);
}

static void elf_write_u32(ElfBuilder *builder, uint32_t value) {
    elf_write_bytes(builder, &value, 4);
}

static void elf_write_u64(ElfBuilder *builder, uint64_t value) {
    elf_write_bytes(builder, &value, 8);
}

static void elf_align(ElfBuilder *builder, size_t alignment) {
    while (builder->size % alignment != 0) {
        elf_write_u8(builder, 0);
    }
}

// ====================================
// 64位 ELF 生成
// ====================================

static void generate_elf64_header(ElfBuilder *builder, uint64_t entry_point) {
    Elf64_Ehdr ehdr = {0};
    
    // ELF 标识
    ehdr.ident.magic[0] = 0x7F;
    ehdr.ident.magic[1] = 'E';
    ehdr.ident.magic[2] = 'L';
    ehdr.ident.magic[3] = 'F';
    ehdr.ident.class = ELFCLASS64;
    ehdr.ident.data = ELFDATA2LSB;
    ehdr.ident.version = 1;
    ehdr.ident.osabi = 0;  // System V
    
    // ELF 头
    ehdr.type = ET_EXEC;
    ehdr.machine = EM_X86_64;
    ehdr.version = 1;
    ehdr.entry = entry_point;
    ehdr.phoff = sizeof(Elf64_Ehdr);
    ehdr.shoff = 0;  // 无节头表
    ehdr.flags = 0;
    ehdr.ehsize = sizeof(Elf64_Ehdr);
    ehdr.phentsize = sizeof(Elf64_Phdr);
    ehdr.phnum = builder->segment_count;
    ehdr.shentsize = 0;
    ehdr.shnum = 0;
    ehdr.shstrndx = 0;
    
    elf_write_bytes(builder, &ehdr, sizeof(ehdr));
}

static void generate_elf64_program_headers(ElfBuilder *builder) {
    for (int i = 0; i < builder->segment_count; i++) {
        Elf64_Phdr phdr = {0};
        
        phdr.type = PT_LOAD;
        phdr.flags = builder->segments[i].flags;
        phdr.offset = builder->segments[i].offset;
        phdr.vaddr = builder->segments[i].vaddr;
        phdr.paddr = builder->segments[i].vaddr;
        phdr.filesz = builder->segments[i].size;
        phdr.memsz = builder->segments[i].size;
        phdr.align = 0x1000;  // 页对齐
        
        elf_write_bytes(builder, &phdr, sizeof(phdr));
    }
}

// ====================================
// 32位 ELF 生成
// ====================================

static void generate_elf32_header(ElfBuilder *builder, uint32_t entry_point) {
    Elf32_Ehdr ehdr = {0};
    
    // ELF 标识
    ehdr.ident.magic[0] = 0x7F;
    ehdr.ident.magic[1] = 'E';
    ehdr.ident.magic[2] = 'L';
    ehdr.ident.magic[3] = 'F';
    ehdr.ident.class = ELFCLASS32;
    ehdr.ident.data = ELFDATA2LSB;
    ehdr.ident.version = 1;
    ehdr.ident.osabi = 0;  // System V
    
    // ELF 头
    ehdr.type = ET_EXEC;
    ehdr.machine = EM_386;
    ehdr.version = 1;
    ehdr.entry = entry_point;
    ehdr.phoff = sizeof(Elf32_Ehdr);
    ehdr.shoff = 0;  // 无节头表
    ehdr.flags = 0;
    ehdr.ehsize = sizeof(Elf32_Ehdr);
    ehdr.phentsize = sizeof(Elf32_Phdr);
    ehdr.phnum = builder->segment_count;
    ehdr.shentsize = 0;
    ehdr.shnum = 0;
    ehdr.shstrndx = 0;
    
    elf_write_bytes(builder, &ehdr, sizeof(ehdr));
}

static void generate_elf32_program_headers(ElfBuilder *builder) {
    for (int i = 0; i < builder->segment_count; i++) {
        Elf32_Phdr phdr = {0};
        
        phdr.type = PT_LOAD;
        phdr.offset = builder->segments[i].offset;
        phdr.vaddr = builder->segments[i].vaddr;
        phdr.paddr = builder->segments[i].vaddr;
        phdr.filesz = builder->segments[i].size;
        phdr.memsz = builder->segments[i].size;
        phdr.flags = builder->segments[i].flags;
        phdr.align = 0x1000;  // 页对齐
        
        elf_write_bytes(builder, &phdr, sizeof(phdr));
    }
}

// ====================================
// 公共接口
// ====================================

// 创建简单的 ELF 可执行文件（单段）
static int create_elf_executable(const char *filename, const uint8_t *code, size_t code_size, int bits) {
    ElfBuilder builder;
    init_elf_builder(&builder, bits);
    
    // 计算布局
    uint64_t base_addr = (bits == 64) ? 0x400000 : 0x08048000;
    size_t header_size = (bits == 64) ? 
        sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) :
        sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr);
    
    // 添加代码段
    builder.segments[0].vaddr = base_addr;
    builder.segments[0].offset = 0;
    builder.segments[0].size = header_size + code_size;
    builder.segments[0].flags = PF_R | PF_X;  // 可读可执行
    builder.segment_count = 1;
    
    // 生成 ELF 头
    if (bits == 64) {
        generate_elf64_header(&builder, base_addr + header_size);
        generate_elf64_program_headers(&builder);
    } else {
        generate_elf32_header(&builder, base_addr + header_size);
        generate_elf32_program_headers(&builder);
    }
    
    // 添加代码
    elf_write_bytes(&builder, code, code_size);
    
    // 写入文件
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "无法创建文件: %s\n", filename);
        free_elf_builder(&builder);
        return -1;
    }
    
    fwrite(builder.data, builder.size, 1, f);
    fclose(f);
    
    // 设置可执行权限
    if (chmod(filename, 0755) != 0) {
        fprintf(stderr, "无法设置可执行权限: %s\n", filename);
        free_elf_builder(&builder);
        return -1;
    }
    
    free_elf_builder(&builder);
    return 0;
}

// 创建包含代码段和数据段的 ELF 可执行文件
static int create_elf_executable_with_data(
    const char *filename, 
    const uint8_t *code, size_t code_size,
    const uint8_t *data, size_t data_size,
    int bits
) {
    ElfBuilder builder;
    init_elf_builder(&builder, bits);
    
    // 计算布局
    uint64_t base_addr = (bits == 64) ? 0x400000 : 0x08048000;
    size_t header_size = (bits == 64) ? 
        sizeof(Elf64_Ehdr) + 2 * sizeof(Elf64_Phdr) :
        sizeof(Elf32_Ehdr) + 2 * sizeof(Elf32_Phdr);
    
    // 对齐
    size_t code_aligned = (code_size + 0xFFF) & ~0xFFF;
    
    // 代码段
    builder.segments[0].vaddr = base_addr;
    builder.segments[0].offset = 0;
    builder.segments[0].size = header_size + code_size;
    builder.segments[0].flags = PF_R | PF_X;  // 可读可执行
    
    // 数据段
    builder.segments[1].vaddr = base_addr + 0x1000 + code_aligned;
    builder.segments[1].offset = header_size + code_aligned;
    builder.segments[1].size = data_size;
    builder.segments[1].flags = PF_R | PF_W;  // 可读可写
    
    builder.segment_count = 2;
    
    // 生成 ELF 头
    if (bits == 64) {
        generate_elf64_header(&builder, base_addr + header_size);
        generate_elf64_program_headers(&builder);
    } else {
        generate_elf32_header(&builder, base_addr + header_size);
        generate_elf32_program_headers(&builder);
    }
    
    // 添加代码
    elf_write_bytes(&builder, code, code_size);
    
    // 对齐到页边界
    elf_align(&builder, 0x1000);
    
    // 添加数据
    if (data && data_size > 0) {
        elf_write_bytes(&builder, data, data_size);
    }
    
    // 写入文件
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "无法创建文件: %s\n", filename);
        free_elf_builder(&builder);
        return -1;
    }
    
    fwrite(builder.data, builder.size, 1, f);
    fclose(f);
    
    // 设置可执行权限
    if (chmod(filename, 0755) != 0) {
        fprintf(stderr, "无法设置可执行权限: %s\n", filename);
        free_elf_builder(&builder);
        return -1;
    }
    
    free_elf_builder(&builder);
    return 0;
}

// ====================================
// 调试辅助
// ====================================

static void dump_elf_info(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return;
    }
    
    // 读取 ELF 标识
    Elf_Ident ident;
    if (fread(&ident, sizeof(ident), 1, f) != 1) {
        fprintf(stderr, "无法读取ELF标识\n");
        fclose(f);
        return;
    }
    
    // 验证魔数
    if (ident.magic[0] != 0x7F || ident.magic[1] != 'E' || 
        ident.magic[2] != 'L' || ident.magic[3] != 'F') {
        fprintf(stderr, "不是有效的ELF文件\n");
        fclose(f);
        return;
    }
    
    printf("=== ELF 文件信息: %s ===\n", filename);
    printf("类型: %d位\n", ident.class == ELFCLASS64 ? 64 : 32);
    printf("字节序: %s\n", ident.data == ELFDATA2LSB ? "小端" : "大端");
    printf("版本: %d\n", ident.version);
    printf("OS/ABI: %d\n", ident.osabi);
    
    fseek(f, 0, SEEK_SET);
    
    if (ident.class == ELFCLASS64) {
        Elf64_Ehdr ehdr;
        if (fread(&ehdr, sizeof(ehdr), 1, f) == 1) {
            printf("文件类型: %d\n", ehdr.type);
            printf("机器类型: %d\n", ehdr.machine);
            printf("入口点: 0x%lx\n", ehdr.entry);
            printf("程序头数量: %d\n", ehdr.phnum);
        }
    } else {
        Elf32_Ehdr ehdr;
        if (fread(&ehdr, sizeof(ehdr), 1, f) == 1) {
            printf("文件类型: %d\n", ehdr.type);
            printf("机器类型: %d\n", ehdr.machine);
            printf("入口点: 0x%x\n", ehdr.entry);
            printf("程序头数量: %d\n", ehdr.phnum);
        }
    }
    
    printf("======================\n");
    fclose(f);
}

#endif // EVOLVER0_ELF_INC_C