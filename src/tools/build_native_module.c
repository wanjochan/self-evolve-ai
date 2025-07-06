/**
 * build_native_module.c - 构建原生模块工具
 * 
 * 这个工具用于将编译后的目标代码转换为.native模块格式。
 * 它使用native_module.c中的函数来创建和写入.native模块。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// 直接包含native模块的头文件
#include "../core/old/native.h"

// 定义架构和类型枚举（与native.h保持一致）
typedef enum {
    NATIVE_ARCH_X86_64 = 1,
    NATIVE_ARCH_ARM64 = 2,
    NATIVE_ARCH_X86_32 = 3
} NativeArchitecture;

typedef enum {
    NATIVE_TYPE_VM = 1,
    NATIVE_TYPE_LIBC = 2,
    NATIVE_TYPE_USER = 3
} NativeModuleType;

typedef enum {
    NATIVE_EXPORT_FUNCTION = 1,
    NATIVE_EXPORT_VARIABLE = 2,
    NATIVE_EXPORT_CONSTANT = 3
} NativeExportType;

// 直接使用native.c中的函数，无需动态加载
// 这些函数在编译时链接
extern NativeModule* native_module_create(NativeArchitecture arch, NativeModuleType type);
extern void native_module_free(NativeModule* module);
extern int native_module_set_code(NativeModule* module, const uint8_t* code, size_t size, uint32_t entry_point);
extern int native_module_add_export(NativeModule* module, const char* name, NativeExportType type, uint64_t offset, uint64_t size);
extern int native_module_write_file(const NativeModule* module, const char* filename);

// 简化的初始化函数
static int init_native_functions(void) {
    // 直接链接，无需初始化
    return 0;
}

// 读取文件到缓冲区
static uint8_t* read_file_to_buffer(const char* filename, size_t* size_out) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "无法打开文件: %s\n", filename);
        return NULL;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 分配缓冲区
    uint8_t* buffer = (uint8_t*)malloc(size);
    if (!buffer) {
        fprintf(stderr, "内存分配失败\n");
        fclose(file);
        return NULL;
    }
    
    // 读取文件内容
    size_t read_size = fread(buffer, 1, size, file);
    fclose(file);
    
    if (read_size != size) {
        fprintf(stderr, "读取文件失败: %s\n", filename);
        free(buffer);
        return NULL;
    }
    
    *size_out = size;
    return buffer;
}

// ELF文件头结构
typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

// ELF节头结构
typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

// ELF符号表项结构
typedef struct {
    uint32_t st_name;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;

// 从ELF文件中提取符号信息
static int extract_symbols_from_elf(NativeModule* module, const uint8_t* elf_data, size_t elf_size) {
    if (elf_size < sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "无效的ELF文件: 文件太小\n");
        return -1;
    }
    
    // 解析ELF头
    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)elf_data;
    
    // 检查ELF魔数
    if (ehdr->e_ident[0] != 0x7F || ehdr->e_ident[1] != 'E' || 
        ehdr->e_ident[2] != 'L' || ehdr->e_ident[3] != 'F') {
        fprintf(stderr, "无效的ELF文件: 魔数不匹配\n");
        return -1;
    }
    
    // 检查是否为64位ELF
    if (ehdr->e_ident[4] != 2) {
        fprintf(stderr, "仅支持64位ELF文件\n");
        return -1;
    }
    
    // 获取节头表
    if (ehdr->e_shoff == 0 || ehdr->e_shnum == 0) {
        fprintf(stderr, "ELF文件没有节头表\n");
        return -1;
    }
    
    const Elf64_Shdr* shdr_table = (const Elf64_Shdr*)(elf_data + ehdr->e_shoff);
    
    // 获取字符串表节
    if (ehdr->e_shstrndx >= ehdr->e_shnum) {
        fprintf(stderr, "无效的节头字符串表索引\n");
        return -1;
    }
    
    const Elf64_Shdr* shstrtab_hdr = &shdr_table[ehdr->e_shstrndx];
    const char* shstrtab = (const char*)(elf_data + shstrtab_hdr->sh_offset);
    
    // 查找符号表和字符串表
    const Elf64_Shdr* symtab_hdr = NULL;
    const Elf64_Shdr* strtab_hdr = NULL;
    
    for (int i = 0; i < ehdr->e_shnum; i++) {
        const Elf64_Shdr* shdr = &shdr_table[i];
        const char* name = shstrtab + shdr->sh_name;
        
        if (shdr->sh_type == 2 && strcmp(name, ".symtab") == 0) {
            symtab_hdr = shdr;
        } else if (shdr->sh_type == 3 && strcmp(name, ".strtab") == 0) {
            strtab_hdr = shdr;
        }
    }
    
    if (!symtab_hdr || !strtab_hdr) {
        fprintf(stderr, "ELF文件缺少符号表或字符串表\n");
        return -1;
    }
    
    // 获取符号表和字符串表
    const Elf64_Sym* symtab = (const Elf64_Sym*)(elf_data + symtab_hdr->sh_offset);
    const char* strtab = (const char*)(elf_data + strtab_hdr->sh_offset);
    int sym_count = symtab_hdr->sh_size / sizeof(Elf64_Sym);
    
    // 查找文本段和数据段
    const Elf64_Shdr* text_hdr = NULL;
    const Elf64_Shdr* data_hdr = NULL;
    const Elf64_Shdr* rodata_hdr = NULL;
    
    for (int i = 0; i < ehdr->e_shnum; i++) {
        const Elf64_Shdr* shdr = &shdr_table[i];
        const char* name = shstrtab + shdr->sh_name;
        
        if (strcmp(name, ".text") == 0) {
            text_hdr = shdr;
        } else if (strcmp(name, ".data") == 0) {
            data_hdr = shdr;
        } else if (strcmp(name, ".rodata") == 0) {
            rodata_hdr = shdr;
        }
    }
    
    // 提取符号信息
    int export_count = 0;
    for (int i = 0; i < sym_count; i++) {
        const Elf64_Sym* sym = &symtab[i];
        
        // 跳过未定义的符号
        if (sym->st_shndx == 0) {
            continue;
        }
        
        // 获取符号名称
        const char* name = strtab + sym->st_name;
        if (!name || name[0] == '\0') {
            continue;
        }
        
        // 确定符号类型
        NativeExportType type;
        uint64_t offset;
        
        // 获取符号所在节
        const Elf64_Shdr* sym_shdr = &shdr_table[sym->st_shndx];
        
        // 确定符号类型和偏移
        if (sym_shdr == text_hdr) {
            // 函数符号
            type = NATIVE_EXPORT_FUNCTION;
            offset = sym->st_value - text_hdr->sh_addr;
        } else if (sym_shdr == data_hdr) {
            // 变量符号
            type = NATIVE_EXPORT_VARIABLE;
            offset = sym->st_value - data_hdr->sh_addr + text_hdr->sh_size;
        } else if (sym_shdr == rodata_hdr) {
            // 常量符号
            type = NATIVE_EXPORT_CONSTANT;
            offset = sym->st_value - rodata_hdr->sh_addr + text_hdr->sh_size + data_hdr->sh_size;
        } else {
            // 其他节中的符号，跳过
            continue;
        }
        
        // 添加导出符号
        if (native_module_add_export(module, name, type, offset, sym->st_size) == 0) {
            export_count++;
            printf("添加导出符号: %s (类型=%d, 偏移=%llu, 大小=%llu)\n", 
                   name, type, (unsigned long long)offset, (unsigned long long)sym->st_size);
        }
    }
    
    printf("成功导出 %d 个符号\n", export_count);
    return export_count > 0 ? 0 : -1;
}

// 检测当前架构
static NativeArchitecture detect_architecture(void) {
    #if defined(__x86_64__) || defined(_M_X64)
        return NATIVE_ARCH_X86_64;
    #elif defined(__aarch64__) || defined(_M_ARM64)
        return NATIVE_ARCH_ARM64;
    #elif defined(__i386__) || defined(_M_IX86)
        return NATIVE_ARCH_X86_32;
    #else
        fprintf(stderr, "不支持的架构\n");
        return NATIVE_ARCH_X86_64; // 默认返回x86_64
    #endif
}

// 打印用法信息
static void print_usage(const char* program_name) {
    printf("用法: %s <输入文件> <输出文件> [选项]\n", program_name);
    printf("选项:\n");
    printf("  --arch=<架构>    指定目标架构 (x86_64, arm64, x86_32)\n");
    printf("  --type=<类型>    指定模块类型 (vm, libc, user)\n");
    printf("  --entry=<偏移>   指定入口点偏移 (默认为0)\n");
    printf("示例:\n");
    printf("  %s input.obj vm_x86_64.native --arch=x86_64 --type=vm\n", program_name);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    // 初始化native模块函数（现在是空操作）
    if (init_native_functions() != 0) {
        fprintf(stderr, "初始化native模块函数失败\n");
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    NativeArchitecture arch = detect_architecture();
    NativeModuleType type = NATIVE_TYPE_USER;
    uint32_t entry_point = 0;
    
    // 解析选项
    for (int i = 3; i < argc; i++) {
        if (strncmp(argv[i], "--arch=", 7) == 0) {
            const char* arch_str = argv[i] + 7;
            if (strcmp(arch_str, "x86_64") == 0) {
                arch = NATIVE_ARCH_X86_64;
            } else if (strcmp(arch_str, "arm64") == 0) {
                arch = NATIVE_ARCH_ARM64;
            } else if (strcmp(arch_str, "x86_32") == 0) {
                arch = NATIVE_ARCH_X86_32;
            } else {
                fprintf(stderr, "未知架构: %s\n", arch_str);
                return 1;
            }
        } else if (strncmp(argv[i], "--type=", 7) == 0) {
            const char* type_str = argv[i] + 7;
            if (strcmp(type_str, "vm") == 0) {
                type = NATIVE_TYPE_VM;
            } else if (strcmp(type_str, "libc") == 0) {
                type = NATIVE_TYPE_LIBC;
            } else if (strcmp(type_str, "user") == 0) {
                type = NATIVE_TYPE_USER;
            } else {
                fprintf(stderr, "未知模块类型: %s\n", type_str);
                return 1;
            }
        } else if (strncmp(argv[i], "--entry=", 8) == 0) {
            entry_point = (uint32_t)strtoul(argv[i] + 8, NULL, 0);
        } else {
            fprintf(stderr, "未知选项: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // 读取输入文件
    size_t code_size;
    uint8_t* code = read_file_to_buffer(input_file, &code_size);
    if (!code) {
        return 1;
    }
    
    // 创建模块
    NativeModule* module = native_module_create(arch, type);
    if (!module) {
        fprintf(stderr, "创建模块失败\n");
        free(code);
        return 1;
    }
    
    // 设置代码
    if (native_module_set_code(module, code, code_size, entry_point) != 0) {
        fprintf(stderr, "设置模块代码失败\n");
        native_module_free(module);
        free(code);
        return 1;
    }
    
    // 从ELF文件中提取符号信息
    if (extract_symbols_from_elf(module, code, code_size) != 0) {
        fprintf(stderr, "提取符号信息失败\n");
        native_module_free(module);
        free(code);
        return 1;
    }
    
    // 写入输出文件
    if (native_module_write_file(module, output_file) != 0) {
        fprintf(stderr, "写入模块文件失败\n");
        native_module_free(module);
        free(code);
        return 1;
    }
    
    printf("成功创建模块: %s\n", output_file);
    
    // 清理
    native_module_free(module);
    free(code);

    return 0;
} 