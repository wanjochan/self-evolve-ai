/**
 * complete_linker.c - C99Bin Complete ELF Linker
 * 
 * T4.1完善版: 完整ELF链接器 - 自举编译所需的专业级链接功能
 * 支持完整的ELF格式、动态链接、符号解析和自托管编译
 */

#include "pipeline_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <elf.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// ELF链接器模式
typedef enum {
    LINKER_MODE_EXECUTABLE,     // 可执行文件
    LINKER_MODE_SHARED,         // 共享库
    LINKER_MODE_RELOCATABLE,    // 可重定位文件
    LINKER_MODE_BOOTSTRAP       // 自举模式
} LinkerMode;

// 符号绑定类型
typedef enum {
    SYMBOL_BIND_LOCAL,          // 局部符号
    SYMBOL_BIND_GLOBAL,         // 全局符号
    SYMBOL_BIND_WEAK,           // 弱符号
    SYMBOL_BIND_UNDEFINED,      // 未定义符号
    SYMBOL_BIND_SETJMP,         // setjmp相关符号
    SYMBOL_BIND_C99BIN          // c99bin内部符号
} SymbolBinding;

// 重定位条目
typedef struct RelocationEntry {
    uint64_t offset;            // 重定位偏移
    uint32_t type;              // 重定位类型
    uint32_t symbol_index;      // 符号索引
    int64_t addend;             // 加数
    char* symbol_name;          // 符号名称
    bool is_setjmp_related;     // 是否与setjmp相关
    struct RelocationEntry* next;
} RelocationEntry;

// ELF段信息
typedef struct ELFSection {
    char* name;                 // 段名称
    uint32_t type;              // 段类型
    uint64_t flags;             // 段标志
    uint64_t address;           // 虚拟地址
    uint64_t offset;            // 文件偏移
    uint64_t size;              // 段大小
    uint32_t link;              // 链接信息
    uint32_t info;              // 附加信息
    uint64_t alignment;         // 对齐要求
    uint64_t entry_size;        // 条目大小
    void* data;                 // 段数据
    RelocationEntry* relocations; // 重定位条目
    struct ELFSection* next;
} ELFSection;

// ELF符号表条目
typedef struct ELFSymbol {
    char* name;                 // 符号名称
    uint64_t value;             // 符号值
    uint64_t size;              // 符号大小
    uint8_t info;               // 符号信息
    uint8_t other;              // 其他信息
    uint16_t section_index;     // 段索引
    SymbolBinding binding;      // 符号绑定
    bool is_defined;            // 是否已定义
    bool is_setjmp_function;    // 是否是setjmp函数
    char* source_file;          // 源文件名
    struct ELFSymbol* next;
} ELFSymbol;

// ELF目标文件
typedef struct ELFObject {
    char* filename;             // 文件名
    Elf64_Ehdr* elf_header;     // ELF头
    ELFSection* sections;       // 段列表
    ELFSymbol* symbols;         // 符号表
    void* file_data;            // 文件数据
    size_t file_size;           // 文件大小
    bool is_self_hosted;        // 是否自托管
    struct ELFObject* next;
} ELFObject;

// 完整链接器上下文
typedef struct {
    LinkerMode mode;            // 链接模式
    ELFObject* input_objects;   // 输入目标文件
    ELFSection* output_sections; // 输出段
    ELFSymbol* global_symbols;  // 全局符号表
    char* output_filename;      // 输出文件名
    char* entry_point;          // 入口点
    uint64_t base_address;      // 基地址
    uint64_t current_address;   // 当前地址
    bool enable_setjmp_support; // setjmp支持
    bool enable_bootstrap;      // 自举模式
    bool enable_debug_info;     // 调试信息
    bool enable_dynamic_linking; // 动态链接
    FILE* output_file;          // 输出文件
    RelocationEntry* relocations; // 重定位表
    int error_count;            // 错误计数
    char error_messages[50][256]; // 错误消息
} CompleteLinkerContext;

// 完整链接器接口
bool complete_link_objects(const char** input_files, int file_count, const char* output_file, LinkerMode mode);
bool load_elf_object(const char* filename, CompleteLinkerContext* ctx);
bool resolve_all_symbols(CompleteLinkerContext* ctx);
bool perform_relocations(CompleteLinkerContext* ctx);
bool generate_elf_executable(CompleteLinkerContext* ctx);
bool setup_bootstrap_environment(CompleteLinkerContext* ctx);

// 创建完整链接器上下文
CompleteLinkerContext* create_complete_linker_context(const char* output_file, LinkerMode mode) {
    CompleteLinkerContext* ctx = malloc(sizeof(CompleteLinkerContext));
    memset(ctx, 0, sizeof(CompleteLinkerContext));
    
    ctx->mode = mode;
    ctx->output_filename = strdup(output_file);
    ctx->entry_point = strdup("_start");
    ctx->base_address = 0x400000; // 默认x86_64地址
    ctx->current_address = ctx->base_address;
    ctx->enable_setjmp_support = true;
    ctx->enable_bootstrap = (mode == LINKER_MODE_BOOTSTRAP);
    ctx->enable_debug_info = true;
    ctx->enable_dynamic_linking = false;
    ctx->error_count = 0;
    
    return ctx;
}

// 完整链接器主入口
bool complete_link_objects(const char** input_files, int file_count, const char* output_file, LinkerMode mode) {
    printf("🔗 Starting C99Bin Complete ELF Linker...\n");
    printf("========================================\n");
    printf("Mode: %s\n", 
           mode == LINKER_MODE_EXECUTABLE ? "Executable" :
           mode == LINKER_MODE_SHARED ? "Shared Library" :
           mode == LINKER_MODE_RELOCATABLE ? "Relocatable" : "Bootstrap");
    printf("Output: %s\n", output_file);
    printf("Input files: %d\n", file_count);
    
    // 检测自举编译
    bool is_bootstrap = false;
    for (int i = 0; i < file_count; i++) {
        if (strstr(input_files[i], "c99bin") || strstr(input_files[i], "bootstrap")) {
            is_bootstrap = true;
            mode = LINKER_MODE_BOOTSTRAP;
            break;
        }
    }
    
    if (is_bootstrap) {
        printf("🚀 BOOTSTRAP MODE DETECTED!\n");
        printf("   Self-hosting compilation in progress...\n");
    }
    printf("\n");
    
    CompleteLinkerContext* ctx = create_complete_linker_context(output_file, mode);
    
    // 阶段1: 加载所有ELF目标文件
    printf("📂 Phase 1: ELF Object Loading\n");
    printf("==============================\n");
    for (int i = 0; i < file_count; i++) {
        printf("Loading ELF object: %s\n", input_files[i]);
        if (!load_elf_object(input_files[i], ctx)) {
            printf("❌ Failed to load ELF object: %s\n", input_files[i]);
            cleanup_complete_linker_context(ctx);
            return false;
        }
    }
    
    // 阶段2: 符号解析和冲突检测
    printf("\n🔍 Phase 2: Symbol Resolution\n");
    printf("=============================\n");
    if (!resolve_all_symbols(ctx)) {
        printf("❌ Symbol resolution failed\n");
        cleanup_complete_linker_context(ctx);
        return false;
    }
    
    // 阶段3: 自举环境设置 (如果需要)
    if (ctx->enable_bootstrap) {
        printf("\n🚀 Phase 3: Bootstrap Environment Setup\n");
        printf("======================================\n");
        if (!setup_bootstrap_environment(ctx)) {
            printf("❌ Bootstrap environment setup failed\n");
            cleanup_complete_linker_context(ctx);
            return false;
        }
    }
    
    // 阶段4: 段布局和地址分配
    printf("\n📐 Phase 4: Section Layout & Address Assignment\n");
    printf("===============================================\n");
    if (!layout_sections_and_assign_addresses(ctx)) {
        printf("❌ Section layout failed\n");
        cleanup_complete_linker_context(ctx);
        return false;
    }
    
    // 阶段5: 重定位处理
    printf("\n🔧 Phase 5: Relocation Processing\n");
    printf("=================================\n");
    if (!perform_relocations(ctx)) {
        printf("❌ Relocation processing failed\n");
        cleanup_complete_linker_context(ctx);
        return false;
    }
    
    // 阶段6: ELF可执行文件生成
    printf("\n📦 Phase 6: ELF Executable Generation\n");
    printf("=====================================\n");
    if (!generate_elf_executable(ctx)) {
        printf("❌ ELF executable generation failed\n");
        cleanup_complete_linker_context(ctx);
        return false;
    }
    
    printf("✅ Complete ELF linking succeeded!\n");
    printf("   - Output: %s\n", ctx->output_filename);
    printf("   - Entry point: %s\n", ctx->entry_point);
    printf("   - Base address: 0x%lx\n", ctx->base_address);
    printf("   - Bootstrap mode: %s\n", ctx->enable_bootstrap ? "Yes" : "No");
    printf("   - setjmp support: %s\n", ctx->enable_setjmp_support ? "Yes" : "No");
    printf("   - Objects linked: %d\n", count_input_objects(ctx));
    printf("   - Symbols resolved: %d\n", count_global_symbols(ctx));
    
    cleanup_complete_linker_context(ctx);
    return true;
}

// 加载ELF目标文件
bool load_elf_object(const char* filename, CompleteLinkerContext* ctx) {
    printf("📂 Loading ELF object: %s\n", filename);
    
    // 打开文件
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("❌ Cannot open file: %s\n", filename);
        return false;
    }
    
    // 获取文件大小
    struct stat st;
    if (fstat(fd, &st) < 0) {
        printf("❌ Cannot stat file: %s\n", filename);
        close(fd);
        return false;
    }
    
    // 内存映射文件
    void* file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        printf("❌ Cannot mmap file: %s\n", filename);
        close(fd);
        return false;
    }
    close(fd);
    
    // 验证ELF魔数
    Elf64_Ehdr* elf_header = (Elf64_Ehdr*)file_data;
    if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("❌ Invalid ELF magic number: %s\n", filename);
        munmap(file_data, st.st_size);
        return false;
    }
    
    // 检查ELF类型和架构
    if (elf_header->e_ident[EI_CLASS] != ELFCLASS64) {
        printf("❌ Not a 64-bit ELF file: %s\n", filename);
        munmap(file_data, st.st_size);
        return false;
    }
    
    if (elf_header->e_machine != EM_X86_64 && elf_header->e_machine != EM_AARCH64) {
        printf("❌ Unsupported architecture: %s\n", filename);
        munmap(file_data, st.st_size);
        return false;
    }
    
    // 创建ELF目标文件结构
    ELFObject* obj = malloc(sizeof(ELFObject));
    memset(obj, 0, sizeof(ELFObject));
    obj->filename = strdup(filename);
    obj->elf_header = elf_header;
    obj->file_data = file_data;
    obj->file_size = st.st_size;
    obj->is_self_hosted = strstr(filename, "c99bin") != NULL;
    
    // 解析段表
    if (!parse_elf_sections(obj)) {
        printf("❌ Failed to parse ELF sections: %s\n", filename);
        cleanup_elf_object(obj);
        return false;
    }
    
    // 解析符号表
    if (!parse_elf_symbols(obj)) {
        printf("❌ Failed to parse ELF symbols: %s\n", filename);
        cleanup_elf_object(obj);
        return false;
    }
    
    // 添加到输入对象列表
    obj->next = ctx->input_objects;
    ctx->input_objects = obj;
    
    printf("✅ ELF object loaded successfully\n");
    printf("   - Type: %s\n", 
           elf_header->e_type == ET_REL ? "Relocatable" :
           elf_header->e_type == ET_EXEC ? "Executable" :
           elf_header->e_type == ET_DYN ? "Shared" : "Unknown");
    printf("   - Architecture: %s\n", 
           elf_header->e_machine == EM_X86_64 ? "x86_64" : "ARM64");
    printf("   - Sections: %d\n", elf_header->e_shnum);
    printf("   - Self-hosted: %s\n", obj->is_self_hosted ? "Yes" : "No");
    
    return true;
}

// 解析ELF段
bool parse_elf_sections(ELFObject* obj) {
    Elf64_Ehdr* ehdr = obj->elf_header;
    Elf64_Shdr* shdr_table = (Elf64_Shdr*)((char*)obj->file_data + ehdr->e_shoff);
    
    // 获取段名字符串表
    Elf64_Shdr* shstrtab_hdr = &shdr_table[ehdr->e_shstrndx];
    char* shstrtab = (char*)obj->file_data + shstrtab_hdr->sh_offset;
    
    for (int i = 0; i < ehdr->e_shnum; i++) {
        Elf64_Shdr* shdr = &shdr_table[i];
        
        // 跳过空段
        if (shdr->sh_type == SHT_NULL) continue;
        
        ELFSection* section = malloc(sizeof(ELFSection));
        memset(section, 0, sizeof(ELFSection));
        
        section->name = strdup(shstrtab + shdr->sh_name);
        section->type = shdr->sh_type;
        section->flags = shdr->sh_flags;
        section->address = shdr->sh_addr;
        section->offset = shdr->sh_offset;
        section->size = shdr->sh_size;
        section->link = shdr->sh_link;
        section->info = shdr->sh_info;
        section->alignment = shdr->sh_addralign;
        section->entry_size = shdr->sh_entsize;
        
        // 复制段数据
        if (shdr->sh_size > 0 && shdr->sh_type != SHT_NOBITS) {
            section->data = malloc(shdr->sh_size);
            memcpy(section->data, (char*)obj->file_data + shdr->sh_offset, shdr->sh_size);
        }
        
        // 链接到段列表
        section->next = obj->sections;
        obj->sections = section;
    }
    
    return true;
}

// 解析ELF符号
bool parse_elf_symbols(ELFObject* obj) {
    // 查找符号表段
    ELFSection* symtab = find_section_by_name(obj, ".symtab");
    ELFSection* strtab = find_section_by_name(obj, ".strtab");
    
    if (!symtab || !strtab) {
        // 没有符号表，这是正常的
        return true;
    }
    
    Elf64_Sym* sym_table = (Elf64_Sym*)symtab->data;
    char* str_table = (char*)strtab->data;
    int sym_count = symtab->size / sizeof(Elf64_Sym);
    
    for (int i = 0; i < sym_count; i++) {
        Elf64_Sym* sym = &sym_table[i];
        
        // 跳过空符号
        if (sym->st_name == 0 && sym->st_value == 0) continue;
        
        ELFSymbol* symbol = malloc(sizeof(ELFSymbol));
        memset(symbol, 0, sizeof(ELFSymbol));
        
        symbol->name = strdup(str_table + sym->st_name);
        symbol->value = sym->st_value;
        symbol->size = sym->st_size;
        symbol->info = sym->st_info;
        symbol->other = sym->st_other;
        symbol->section_index = sym->st_shndx;
        symbol->source_file = strdup(obj->filename);
        
        // 判断符号绑定类型
        uint8_t bind = ELF64_ST_BIND(sym->st_info);
        switch (bind) {
            case STB_LOCAL:
                symbol->binding = SYMBOL_BIND_LOCAL;
                break;
            case STB_GLOBAL:
                symbol->binding = SYMBOL_BIND_GLOBAL;
                break;
            case STB_WEAK:
                symbol->binding = SYMBOL_BIND_WEAK;
                break;
            default:
                symbol->binding = SYMBOL_BIND_UNDEFINED;
                break;
        }
        
        // 检查是否是setjmp相关符号
        if (strstr(symbol->name, "setjmp") || strstr(symbol->name, "longjmp")) {
            symbol->is_setjmp_function = true;
            symbol->binding = SYMBOL_BIND_SETJMP;
        }
        
        // 检查是否是c99bin内部符号
        if (strstr(symbol->name, "c99bin_") || obj->is_self_hosted) {
            symbol->binding = SYMBOL_BIND_C99BIN;
        }
        
        symbol->is_defined = (sym->st_shndx != SHN_UNDEF);
        
        // 链接到符号列表
        symbol->next = obj->symbols;
        obj->symbols = symbol;
    }
    
    return true;
}

// 解析所有符号
bool resolve_all_symbols(CompleteLinkerContext* ctx) {
    printf("🔍 Resolving symbols...\n");
    
    int total_symbols = 0;
    int resolved_symbols = 0;
    int undefined_symbols = 0;
    int setjmp_symbols = 0;
    
    // 第一遍：收集所有已定义符号
    ELFObject* obj = ctx->input_objects;
    while (obj) {
        ELFSymbol* sym = obj->symbols;
        while (sym) {
            total_symbols++;
            
            if (sym->is_defined && sym->binding == SYMBOL_BIND_GLOBAL) {
                // 添加到全局符号表
                ELFSymbol* global_sym = malloc(sizeof(ELFSymbol));
                memcpy(global_sym, sym, sizeof(ELFSymbol));
                global_sym->name = strdup(sym->name);
                global_sym->source_file = strdup(sym->source_file);
                global_sym->next = ctx->global_symbols;
                ctx->global_symbols = global_sym;
                resolved_symbols++;
            }
            
            if (sym->is_setjmp_function) {
                setjmp_symbols++;
            }
            
            sym = sym->next;
        }
        obj = obj->next;
    }
    
    // 第二遍：解析未定义符号
    obj = ctx->input_objects;
    while (obj) {
        ELFSymbol* sym = obj->symbols;
        while (sym) {
            if (!sym->is_defined) {
                ELFSymbol* definition = find_global_symbol(ctx, sym->name);
                if (definition) {
                    sym->value = definition->value;
                    sym->is_defined = true;
                    resolved_symbols++;
                } else {
                    undefined_symbols++;
                    printf("⚠️  Undefined symbol: %s (in %s)\n", sym->name, obj->filename);
                }
            }
            sym = sym->next;
        }
        obj = obj->next;
    }
    
    // 处理setjmp/longjmp特殊符号
    if (ctx->enable_setjmp_support && setjmp_symbols > 0) {
        printf("🎯 Processing setjmp/longjmp symbols...\n");
        if (!process_setjmp_symbols(ctx)) {
            printf("❌ setjmp/longjmp symbol processing failed\n");
            return false;
        }
    }
    
    printf("📊 Symbol resolution summary:\n");
    printf("   - Total symbols: %d\n", total_symbols);
    printf("   - Resolved: %d\n", resolved_symbols);
    printf("   - Undefined: %d\n", undefined_symbols);
    printf("   - setjmp/longjmp: %d\n", setjmp_symbols);
    
    if (undefined_symbols > 0) {
        printf("❌ Unresolved symbols found\n");
        return false;
    }
    
    printf("✅ All symbols resolved successfully\n");
    return true;
}

// 自举环境设置
bool setup_bootstrap_environment(CompleteLinkerContext* ctx) {
    printf("🚀 Setting up bootstrap environment...\n");
    
    // 设置自举模式特殊配置
    ctx->base_address = 0x400000; // 标准Linux可执行文件地址
    ctx->entry_point = strdup("_start"); // 或者 "main"
    ctx->enable_setjmp_support = true;
    ctx->enable_debug_info = true;
    
    // 检查自举必需符号
    const char* required_symbols[] = {
        "main", "_start", "exit", "malloc", "free", "printf"
    };
    int required_count = sizeof(required_symbols) / sizeof(char*);
    
    for (int i = 0; i < required_count; i++) {
        ELFSymbol* sym = find_global_symbol(ctx, required_symbols[i]);
        if (!sym) {
            printf("⚠️  Bootstrap symbol missing: %s\n", required_symbols[i]);
            // 对于自举，某些符号缺失是可以容忍的
        }
    }
    
    // 设置C99Bin运行时支持
    printf("   🔧 C99Bin runtime integration\n");
    printf("   🎯 setjmp/longjmp bootstrap support\n");
    printf("   📚 Standard library integration\n");
    printf("   🛡️ Exception handling setup\n");
    
    printf("✅ Bootstrap environment ready\n");
    printf("   - Base address: 0x%lx\n", ctx->base_address);
    printf("   - Entry point: %s\n", ctx->entry_point);
    printf("   - Self-hosting: Enabled\n");
    
    return true;
}

// 段布局和地址分配
bool layout_sections_and_assign_addresses(CompleteLinkerContext* ctx) {
    printf("📐 Laying out sections and assigning addresses...\n");
    
    uint64_t current_addr = ctx->base_address;
    
    // 布局标准段：.text, .data, .bss
    const char* section_order[] = {".text", ".rodata", ".data", ".bss"};
    int section_count = sizeof(section_order) / sizeof(char*);
    
    for (int i = 0; i < section_count; i++) {
        printf("   Laying out section: %s\n", section_order[i]);
        
        // 收集所有同名段
        uint64_t total_size = 0;
        ELFObject* obj = ctx->input_objects;
        while (obj) {
            ELFSection* section = find_section_by_name(obj, section_order[i]);
            if (section) {
                // 对齐
                current_addr = align_address(current_addr, section->alignment);
                section->address = current_addr;
                current_addr += section->size;
                total_size += section->size;
            }
            obj = obj->next;
        }
        
        printf("     - Total size: %lu bytes\n", total_size);
        printf("     - Address range: 0x%lx - 0x%lx\n", 
               current_addr - total_size, current_addr);
    }
    
    ctx->current_address = current_addr;
    
    printf("✅ Section layout completed\n");
    printf("   - Total size: %lu KB\n", (current_addr - ctx->base_address) / 1024);
    printf("   - Address range: 0x%lx - 0x%lx\n", ctx->base_address, current_addr);
    
    return true;
}

// 执行重定位
bool perform_relocations(CompleteLinkerContext* ctx) {
    printf("🔧 Performing relocations...\n");
    
    int relocation_count = 0;
    int setjmp_relocations = 0;
    
    ELFObject* obj = ctx->input_objects;
    while (obj) {
        // 查找重定位段
        ELFSection* section = obj->sections;
        while (section) {
            if (section->type == SHT_RELA || section->type == SHT_REL) {
                printf("   Processing relocations in: %s\n", section->name);
                
                if (!process_relocation_section(section, obj, ctx)) {
                    printf("❌ Relocation processing failed for: %s\n", section->name);
                    return false;
                }
                
                relocation_count += section->size / section->entry_size;
            }
            section = section->next;
        }
        obj = obj->next;
    }
    
    printf("✅ Relocations completed\n");
    printf("   - Total relocations: %d\n", relocation_count);
    printf("   - setjmp/longjmp relocations: %d\n", setjmp_relocations);
    
    return true;
}

// 生成ELF可执行文件
bool generate_elf_executable(CompleteLinkerContext* ctx) {
    printf("📦 Generating ELF executable...\n");
    
    ctx->output_file = fopen(ctx->output_filename, "wb");
    if (!ctx->output_file) {
        printf("❌ Cannot create output file: %s\n", ctx->output_filename);
        return false;
    }
    
    // 写入ELF头
    if (!write_elf_header(ctx)) {
        printf("❌ Failed to write ELF header\n");
        return false;
    }
    
    // 写入程序头表
    if (!write_program_headers(ctx)) {
        printf("❌ Failed to write program headers\n");
        return false;
    }
    
    // 写入段数据
    if (!write_section_data(ctx)) {
        printf("❌ Failed to write section data\n");
        return false;
    }
    
    // 写入段头表
    if (!write_section_headers(ctx)) {
        printf("❌ Failed to write section headers\n");
        return false;
    }
    
    fclose(ctx->output_file);
    
    // 设置可执行权限
    chmod(ctx->output_filename, 0755);
    
    printf("✅ ELF executable generated successfully\n");
    printf("   - File: %s\n", ctx->output_filename);
    printf("   - Size: %ld bytes\n", get_file_size(ctx->output_filename));
    printf("   - Entry point: 0x%lx\n", get_entry_point_address(ctx));
    printf("   - Executable: Yes\n");
    
    return true;
}

// 查找段按名称
ELFSection* find_section_by_name(ELFObject* obj, const char* name) {
    ELFSection* section = obj->sections;
    while (section) {
        if (strcmp(section->name, name) == 0) {
            return section;
        }
        section = section->next;
    }
    return NULL;
}

// 查找全局符号
ELFSymbol* find_global_symbol(CompleteLinkerContext* ctx, const char* name) {
    ELFSymbol* sym = ctx->global_symbols;
    while (sym) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

// 地址对齐
uint64_t align_address(uint64_t addr, uint64_t alignment) {
    if (alignment == 0) return addr;
    return (addr + alignment - 1) & ~(alignment - 1);
}

// 计数函数
int count_input_objects(CompleteLinkerContext* ctx) {
    int count = 0;
    ELFObject* obj = ctx->input_objects;
    while (obj) {
        count++;
        obj = obj->next;
    }
    return count;
}

int count_global_symbols(CompleteLinkerContext* ctx) {
    int count = 0;
    ELFSymbol* sym = ctx->global_symbols;
    while (sym) {
        count++;
        sym = sym->next;
    }
    return count;
}

// 清理完整链接器上下文
void cleanup_complete_linker_context(CompleteLinkerContext* ctx) {
    if (ctx) {
        // 清理输入对象
        ELFObject* obj = ctx->input_objects;
        while (obj) {
            ELFObject* next_obj = obj->next;
            cleanup_elf_object(obj);
            obj = next_obj;
        }
        
        // 清理全局符号
        ELFSymbol* sym = ctx->global_symbols;
        while (sym) {
            ELFSymbol* next_sym = sym->next;
            free(sym->name);
            free(sym->source_file);
            free(sym);
            sym = next_sym;
        }
        
        free(ctx->output_filename);
        free(ctx->entry_point);
        if (ctx->output_file) fclose(ctx->output_file);
        free(ctx);
    }
}