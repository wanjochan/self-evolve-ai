/**
 * evolver0_pe.inc.c - PE可执行文件生成模块
 * 被 evolver0.c 包含使用
 */

#ifndef EVOLVER0_PE_INC_C
#define EVOLVER0_PE_INC_C

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ====================================
// PE 文件格式定义
// ====================================

#define PE_MAGIC 0x00004550  // "PE\0\0"
#define DOS_MAGIC 0x5A4D     // "MZ"

// 机器类型
#define IMAGE_FILE_MACHINE_I386  0x014c
#define IMAGE_FILE_MACHINE_AMD64 0x8664

// 文件特性
#define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002
#define IMAGE_FILE_32BIT_MACHINE    0x0100
#define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020

// 可选头部魔数
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b

// 子系统
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3

// 段特性
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ    0x40000000
#define IMAGE_SCN_MEM_WRITE   0x80000000
#define IMAGE_SCN_CNT_CODE    0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040

// ====================================
// PE 结构体定义
// ====================================

// DOS头
typedef struct {
    uint16_t e_magic;    // DOS魔数 (MZ)
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;   // PE头偏移
} IMAGE_DOS_HEADER;

// PE文件头
typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

// 数据目录
typedef struct {
    uint32_t VirtualAddress;
    uint32_t Size;
} IMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

// 32位可选头
typedef struct {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32;

// 64位可选头
typedef struct {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64;

// 节头
typedef struct {
    char     Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER;

// NT头 (32位)
typedef struct {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32;

// NT头 (64位)
typedef struct {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;

// ====================================
// PE 生成器
// ====================================

typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
    
    // 目标信息
    int bits;       // 32 或 64
    int machine;    // IMAGE_FILE_MACHINE_I386 或 IMAGE_FILE_MACHINE_AMD64
    
    // 段信息
    struct {
        char name[8];
        uint32_t virtual_addr;
        uint32_t virtual_size;
        uint32_t raw_data_ptr;
        uint32_t raw_data_size;
        uint32_t characteristics;
    } sections[16];
    int section_count;
    
    // 入口点
    uint32_t entry_point;
    
} PeBuilder;

// ====================================
// 辅助函数
// ====================================

static void init_pe_builder(PeBuilder *builder, int bits) {
    builder->data = (uint8_t*)malloc(4096);
    builder->size = 0;
    builder->capacity = 4096;
    builder->bits = bits;
    builder->machine = (bits == 64) ? IMAGE_FILE_MACHINE_AMD64 : IMAGE_FILE_MACHINE_I386;
    builder->section_count = 0;
    builder->entry_point = 0;
}

static void free_pe_builder(PeBuilder *builder) {
    free(builder->data);
}

static void pe_write_bytes(PeBuilder *builder, const void *data, size_t size) {
    while (builder->size + size > builder->capacity) {
        builder->capacity *= 2;
        builder->data = (uint8_t*)realloc(builder->data, builder->capacity);
    }
    
    memcpy(builder->data + builder->size, data, size);
    builder->size += size;
}

static void pe_write_u8(PeBuilder *builder, uint8_t value) {
    pe_write_bytes(builder, &value, 1);
}

static void pe_write_u16(PeBuilder *builder, uint16_t value) {
    pe_write_bytes(builder, &value, 2);
}

static void pe_write_u32(PeBuilder *builder, uint32_t value) {
    pe_write_bytes(builder, &value, 4);
}

static void pe_write_u64(PeBuilder *builder, uint64_t value) {
    pe_write_bytes(builder, &value, 8);
}

static void pe_align(PeBuilder *builder, size_t alignment) {
    while (builder->size % alignment != 0) {
        pe_write_u8(builder, 0);
    }
}

// ====================================
// DOS头生成
// ====================================

static void generate_dos_header(PeBuilder *builder) {
    // DOS头
    IMAGE_DOS_HEADER dos_header = {0};
    dos_header.e_magic = DOS_MAGIC;
    dos_header.e_lfanew = 0x40; // PE头偏移，通常为0x40或更大
    
    pe_write_bytes(builder, &dos_header, sizeof(dos_header));
    
    // DOS存根程序
    const char *stub_msg = "This program cannot be run in DOS mode.\r\n$";
    pe_write_bytes(builder, stub_msg, strlen(stub_msg));
    
    // 填充到e_lfanew
    pe_align(builder, dos_header.e_lfanew);
}

// ====================================
// 32位PE头生成
// ====================================

static void generate_pe32_headers(PeBuilder *builder, uint32_t entry_point) {
    // PE签名
    pe_write_u32(builder, PE_MAGIC);
    
    // 文件头
    IMAGE_FILE_HEADER file_header = {0};
    file_header.Machine = IMAGE_FILE_MACHINE_I386;
    file_header.NumberOfSections = builder->section_count;
    file_header.TimeDateStamp = 0;
    file_header.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
    file_header.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE;
    
    pe_write_bytes(builder, &file_header, sizeof(file_header));
    
    // 计算映像大小
    uint32_t image_size = 0;
    for (int i = 0; i < builder->section_count; i++) {
        uint32_t section_end = builder->sections[i].virtual_addr + builder->sections[i].virtual_size;
        if (section_end > image_size) {
            image_size = section_end;
        }
    }
    
    // 可选头
    IMAGE_OPTIONAL_HEADER32 opt_header = {0};
    opt_header.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    opt_header.MajorLinkerVersion = 1;
    opt_header.MinorLinkerVersion = 0;
    opt_header.SizeOfCode = 0; // 由节填充
    opt_header.SizeOfInitializedData = 0; // 由节填充
    opt_header.SizeOfUninitializedData = 0;
    opt_header.AddressOfEntryPoint = entry_point;
    opt_header.BaseOfCode = 0x1000; // 通常代码段从0x1000开始
    opt_header.BaseOfData = 0x2000; // 通常数据段从0x2000开始
    opt_header.ImageBase = 0x400000; // 默认基址
    opt_header.SectionAlignment = 0x1000; // 内存中节对齐
    opt_header.FileAlignment = 0x200; // 文件中节对齐
    opt_header.MajorOperatingSystemVersion = 5;
    opt_header.MinorOperatingSystemVersion = 0;
    opt_header.MajorImageVersion = 0;
    opt_header.MinorImageVersion = 0;
    opt_header.MajorSubsystemVersion = 5;
    opt_header.MinorSubsystemVersion = 0;
    opt_header.Win32VersionValue = 0;
    opt_header.SizeOfImage = image_size;
    opt_header.SizeOfHeaders = 0x400; // 通常头部大小为0x400
    opt_header.CheckSum = 0;
    opt_header.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    opt_header.DllCharacteristics = 0;
    opt_header.SizeOfStackReserve = 0x100000;
    opt_header.SizeOfStackCommit = 0x1000;
    opt_header.SizeOfHeapReserve = 0x100000;
    opt_header.SizeOfHeapCommit = 0x1000;
    opt_header.LoaderFlags = 0;
    opt_header.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    
    // 填充代码和数据大小
    for (int i = 0; i < builder->section_count; i++) {
        if (builder->sections[i].characteristics & IMAGE_SCN_CNT_CODE) {
            opt_header.SizeOfCode += builder->sections[i].raw_data_size;
        } else if (builder->sections[i].characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
            opt_header.SizeOfInitializedData += builder->sections[i].raw_data_size;
        }
    }
    
    pe_write_bytes(builder, &opt_header, sizeof(opt_header));
}

// ====================================
// 64位PE头生成
// ====================================

static void generate_pe64_headers(PeBuilder *builder, uint32_t entry_point) {
    // PE签名
    pe_write_u32(builder, PE_MAGIC);
    
    // 文件头
    IMAGE_FILE_HEADER file_header = {0};
    file_header.Machine = IMAGE_FILE_MACHINE_AMD64;
    file_header.NumberOfSections = builder->section_count;
    file_header.TimeDateStamp = 0;
    file_header.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
    file_header.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE;
    
    pe_write_bytes(builder, &file_header, sizeof(file_header));
    
    // 计算映像大小
    uint32_t image_size = 0;
    for (int i = 0; i < builder->section_count; i++) {
        uint32_t section_end = builder->sections[i].virtual_addr + builder->sections[i].virtual_size;
        if (section_end > image_size) {
            image_size = section_end;
        }
    }
    
    // 可选头
    IMAGE_OPTIONAL_HEADER64 opt_header = {0};
    opt_header.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    opt_header.MajorLinkerVersion = 1;
    opt_header.MinorLinkerVersion = 0;
    opt_header.SizeOfCode = 0; // 由节填充
    opt_header.SizeOfInitializedData = 0; // 由节填充
    opt_header.SizeOfUninitializedData = 0;
    opt_header.AddressOfEntryPoint = entry_point;
    opt_header.BaseOfCode = 0x1000; // 通常代码段从0x1000开始
    opt_header.ImageBase = 0x140000000; // 默认基址
    opt_header.SectionAlignment = 0x1000; // 内存中节对齐
    opt_header.FileAlignment = 0x200; // 文件中节对齐
    opt_header.MajorOperatingSystemVersion = 5;
    opt_header.MinorOperatingSystemVersion = 0;
    opt_header.MajorImageVersion = 0;
    opt_header.MinorImageVersion = 0;
    opt_header.MajorSubsystemVersion = 5;
    opt_header.MinorSubsystemVersion = 0;
    opt_header.Win32VersionValue = 0;
    opt_header.SizeOfImage = image_size;
    opt_header.SizeOfHeaders = 0x400; // 通常头部大小为0x400
    opt_header.CheckSum = 0;
    opt_header.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    opt_header.DllCharacteristics = 0;
    opt_header.SizeOfStackReserve = 0x100000;
    opt_header.SizeOfStackCommit = 0x1000;
    opt_header.SizeOfHeapReserve = 0x100000;
    opt_header.SizeOfHeapCommit = 0x1000;
    opt_header.LoaderFlags = 0;
    opt_header.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    
    // 填充代码和数据大小
    for (int i = 0; i < builder->section_count; i++) {
        if (builder->sections[i].characteristics & IMAGE_SCN_CNT_CODE) {
            opt_header.SizeOfCode += builder->sections[i].raw_data_size;
        } else if (builder->sections[i].characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
            opt_header.SizeOfInitializedData += builder->sections[i].raw_data_size;
        }
    }
    
    pe_write_bytes(builder, &opt_header, sizeof(opt_header));
}

// ====================================
// 节头生成
// ====================================

static void generate_section_headers(PeBuilder *builder) {
    for (int i = 0; i < builder->section_count; i++) {
        IMAGE_SECTION_HEADER section = {0};
        memcpy(section.Name, builder->sections[i].name, 8);
        section.VirtualSize = builder->sections[i].virtual_size;
        section.VirtualAddress = builder->sections[i].virtual_addr;
        section.SizeOfRawData = builder->sections[i].raw_data_size;
        section.PointerToRawData = builder->sections[i].raw_data_ptr;
        section.Characteristics = builder->sections[i].characteristics;
        
        pe_write_bytes(builder, &section, sizeof(section));
    }
}

// ====================================
// PE文件生成
// ====================================

static int create_pe_executable(const char *filename, const uint8_t *code, size_t code_size, int bits) {
    PeBuilder builder;
    init_pe_builder(&builder, bits);
    
    // 添加代码段
    strcpy(builder.sections[0].name, ".text");
    builder.sections[0].virtual_addr = 0x1000;
    builder.sections[0].virtual_size = code_size;
    builder.sections[0].raw_data_ptr = 0x400; // 通常从0x400开始
    builder.sections[0].raw_data_size = code_size;
    builder.sections[0].characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
    builder.section_count = 1;
    
    // 设置入口点
    builder.entry_point = 0x1000; // 代码段起始地址
    
    // 生成DOS头
    generate_dos_header(&builder);
    
    // 生成PE头
    if (bits == 32) {
        generate_pe32_headers(&builder, builder.entry_point);
    } else {
        generate_pe64_headers(&builder, builder.entry_point);
    }
    
    // 生成节头
    generate_section_headers(&builder);
    
    // 对齐到文件对齐值
    pe_align(&builder, 0x200);
    
    // 写入代码段
    pe_write_bytes(&builder, code, code_size);
    
    // 写入文件
    FILE *file = fopen(filename, "wb");
    if (!file) {
        free_pe_builder(&builder);
        return 0;
    }
    
    fwrite(builder.data, 1, builder.size, file);
    fclose(file);
    
    free_pe_builder(&builder);
    return 1;
}

// ====================================
// 对外接口
// ====================================

static int write_pe_file(const char *filename, unsigned char *code, size_t code_size) {
    return create_pe_executable(filename, code, code_size, 32);
}

static int write_pe64_file(const char *filename, unsigned char *code, size_t code_size) {
    return create_pe_executable(filename, code, code_size, 64);
}

#endif // EVOLVER0_PE_INC_C 