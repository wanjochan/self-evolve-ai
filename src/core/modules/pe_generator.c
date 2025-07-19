/**
 * pe_generator.c - Windows PE File Format Generator
 *
 * Generates Windows PE32/PE32+ executable files for C99Bin compiler
 * Supports both x86_32 and x64 architectures
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

// PE file format structures
#pragma pack(push, 1)

// DOS Header
typedef struct {
    uint16_t e_magic;      // 0x5A4D "MZ"
    uint16_t e_cblp;       // Bytes on last page
    uint16_t e_cp;         // Pages in file
    uint16_t e_crlc;       // Relocations
    uint16_t e_cparhdr;    // Size of header in paragraphs
    uint16_t e_minalloc;   // Minimum extra paragraphs needed
    uint16_t e_maxalloc;   // Maximum extra paragraphs needed
    uint16_t e_ss;         // Initial relative SS value
    uint16_t e_sp;         // Initial SP value
    uint16_t e_csum;       // Checksum
    uint16_t e_ip;         // Initial IP value
    uint16_t e_cs;         // Initial relative CS value
    uint16_t e_lfarlc;     // File address of relocation table
    uint16_t e_ovno;       // Overlay number
    uint16_t e_res[4];     // Reserved words
    uint16_t e_oemid;      // OEM identifier
    uint16_t e_oeminfo;    // OEM information
    uint16_t e_res2[10];   // Reserved words
    uint32_t e_lfanew;     // File address of new exe header
} DOS_Header;

// PE Header
typedef struct {
    uint32_t signature;    // 0x00004550 "PE\0\0"
    uint16_t machine;      // Machine type
    uint16_t num_sections; // Number of sections
    uint32_t timestamp;    // Time/date stamp
    uint32_t ptr_to_sym_table; // Pointer to symbol table
    uint32_t num_symbols;  // Number of symbols
    uint16_t size_optional_header; // Size of optional header
    uint16_t characteristics; // Characteristics
} PE_Header;

// Optional Header (PE32+)
typedef struct {
    uint16_t magic;        // 0x020b for PE32+, 0x010b for PE32
    uint8_t  major_linker_version;
    uint8_t  minor_linker_version;
    uint32_t size_of_code;
    uint32_t size_of_initialized_data;
    uint32_t size_of_uninitialized_data;
    uint32_t address_of_entry_point;
    uint32_t base_of_code;
    uint64_t image_base;   // 64-bit for PE32+
    uint32_t section_alignment;
    uint32_t file_alignment;
    uint16_t major_os_version;
    uint16_t minor_os_version;
    uint16_t major_image_version;
    uint16_t minor_image_version;
    uint16_t major_subsystem_version;
    uint16_t minor_subsystem_version;
    uint32_t win32_version_value;
    uint32_t size_of_image;
    uint32_t size_of_headers;
    uint32_t checksum;
    uint16_t subsystem;
    uint16_t dll_characteristics;
    uint64_t size_of_stack_reserve;
    uint64_t size_of_stack_commit;
    uint64_t size_of_heap_reserve;
    uint64_t size_of_heap_commit;
    uint32_t loader_flags;
    uint32_t number_of_rva_and_sizes;
    // Data directories follow...
} Optional_Header_PE32Plus;

// Section Header
typedef struct {
    char     name[8];      // Section name
    uint32_t virtual_size; // Virtual size
    uint32_t virtual_address; // Virtual address
    uint32_t size_of_raw_data; // Size of raw data
    uint32_t ptr_to_raw_data; // Pointer to raw data
    uint32_t ptr_to_relocs; // Pointer to relocations
    uint32_t ptr_to_line_numbers; // Pointer to line numbers
    uint16_t num_relocs;   // Number of relocations
    uint16_t num_line_numbers; // Number of line numbers
    uint32_t characteristics; // Characteristics
} Section_Header;

#pragma pack(pop)

// PE Generator state
typedef struct {
    FILE* output_file;
    char* output_path;
    
    // PE structure
    DOS_Header dos_header;
    PE_Header pe_header;
    Optional_Header_PE32Plus opt_header;
    
    // Sections
    Section_Header sections[16];
    int section_count;
    
    // Raw data buffers
    uint8_t* code_section;
    size_t code_size;
    uint8_t* data_section;
    size_t data_size;
    
    // Architecture
    int is_64bit;
    uint16_t machine_type;
    
    // Entry point
    uint32_t entry_point_rva;
    
    // Statistics
    size_t total_size;
    int symbols_added;
    
} PE_Generator;

// Global generator instance
static PE_Generator* g_pe_gen = NULL;

// Machine types
#define IMAGE_FILE_MACHINE_I386     0x014c  // x86
#define IMAGE_FILE_MACHINE_AMD64    0x8664  // x64

// Characteristics
#define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002
#define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020
#define IMAGE_FILE_DEBUG_STRIPPED   0x0200

// Section characteristics
#define IMAGE_SCN_CNT_CODE          0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040
#define IMAGE_SCN_MEM_EXECUTE       0x20000000
#define IMAGE_SCN_MEM_READ          0x40000000
#define IMAGE_SCN_MEM_WRITE         0x80000000

// Subsystems
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3  // Console application

// Initialize PE generator
int pe_generator_initialize(const char* target_platform) {
    printf("🔨 PE Generator: Initializing for %s\n", target_platform);
    
    if (g_pe_gen) {
        printf("⚠️  PE Generator: Already initialized\n");
        return 0;
    }
    
    g_pe_gen = calloc(1, sizeof(PE_Generator));
    if (!g_pe_gen) {
        printf("❌ PE Generator: Memory allocation failed\n");
        return -1;
    }
    
    // Determine architecture
    if (strstr(target_platform, "x64") || strstr(target_platform, "amd64")) {
        g_pe_gen->is_64bit = 1;
        g_pe_gen->machine_type = IMAGE_FILE_MACHINE_AMD64;
        printf("   🎯 Target: Windows x64\n");
    } else if (strstr(target_platform, "x86") || strstr(target_platform, "i386")) {
        g_pe_gen->is_64bit = 0;
        g_pe_gen->machine_type = IMAGE_FILE_MACHINE_I386;
        printf("   🎯 Target: Windows x86\n");
    } else {
        printf("❌ PE Generator: Unsupported platform %s\n", target_platform);
        free(g_pe_gen);
        g_pe_gen = NULL;
        return -1;
    }
    
    // Initialize DOS header
    g_pe_gen->dos_header.e_magic = 0x5A4D;  // "MZ"
    g_pe_gen->dos_header.e_cblp = 0x90;
    g_pe_gen->dos_header.e_cp = 0x03;
    g_pe_gen->dos_header.e_cparhdr = 0x04;
    g_pe_gen->dos_header.e_maxalloc = 0xFFFF;
    g_pe_gen->dos_header.e_sp = 0xB8;
    g_pe_gen->dos_header.e_lfarlc = 0x40;
    g_pe_gen->dos_header.e_lfanew = 0x80;  // PE header starts at 0x80
    
    // Initialize PE header
    g_pe_gen->pe_header.signature = 0x00004550;  // "PE\0\0"
    g_pe_gen->pe_header.machine = g_pe_gen->machine_type;
    g_pe_gen->pe_header.timestamp = (uint32_t)time(NULL);
    g_pe_gen->pe_header.size_optional_header = sizeof(Optional_Header_PE32Plus);
    g_pe_gen->pe_header.characteristics = IMAGE_FILE_EXECUTABLE_IMAGE;
    
    if (g_pe_gen->is_64bit) {
        g_pe_gen->pe_header.characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
    }
    
    // Initialize optional header
    g_pe_gen->opt_header.magic = g_pe_gen->is_64bit ? 0x020b : 0x010b;
    g_pe_gen->opt_header.major_linker_version = 14;
    g_pe_gen->opt_header.minor_linker_version = 0;
    g_pe_gen->opt_header.image_base = g_pe_gen->is_64bit ? 0x140000000ULL : 0x400000;
    g_pe_gen->opt_header.section_alignment = 0x1000;
    g_pe_gen->opt_header.file_alignment = 0x200;
    g_pe_gen->opt_header.major_os_version = 6;
    g_pe_gen->opt_header.minor_os_version = 0;
    g_pe_gen->opt_header.major_subsystem_version = 6;
    g_pe_gen->opt_header.minor_subsystem_version = 0;
    g_pe_gen->opt_header.subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    g_pe_gen->opt_header.size_of_stack_reserve = 0x100000;
    g_pe_gen->opt_header.size_of_stack_commit = 0x1000;
    g_pe_gen->opt_header.size_of_heap_reserve = 0x100000;
    g_pe_gen->opt_header.size_of_heap_commit = 0x1000;
    g_pe_gen->opt_header.number_of_rva_and_sizes = 16;
    
    printf("✅ PE Generator: Initialization complete\n");
    return 0;
}

// Add code section
int pe_generator_add_section(const char* name, const void* data, size_t size) {
    if (!g_pe_gen) {
        printf("❌ PE Generator: Not initialized\n");
        return -1;
    }
    
    if (g_pe_gen->section_count >= 16) {
        printf("❌ PE Generator: Too many sections\n");
        return -1;
    }
    
    printf("📝 PE Generator: Adding section '%s' (%zu bytes)\n", name, size);
    
    Section_Header* section = &g_pe_gen->sections[g_pe_gen->section_count];
    
    // Set section name (max 8 chars)
    strncpy(section->name, name, 8);
    section->name[7] = '\0';
    
    // Set section properties
    section->virtual_size = size;
    section->size_of_raw_data = (size + g_pe_gen->opt_header.file_alignment - 1) & 
                               ~(g_pe_gen->opt_header.file_alignment - 1);
    
    if (strcmp(name, ".text") == 0) {
        // Code section
        section->characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
        g_pe_gen->code_section = malloc(size);
        if (g_pe_gen->code_section) {
            memcpy(g_pe_gen->code_section, data, size);
            g_pe_gen->code_size = size;
            g_pe_gen->opt_header.size_of_code = section->size_of_raw_data;
            g_pe_gen->opt_header.base_of_code = 0x1000;  // Will be updated in generate()
        }
    } else if (strcmp(name, ".data") == 0) {
        // Data section  
        section->characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        g_pe_gen->data_section = malloc(size);
        if (g_pe_gen->data_section) {
            memcpy(g_pe_gen->data_section, data, size);
            g_pe_gen->data_size = size;
            g_pe_gen->opt_header.size_of_initialized_data = section->size_of_raw_data;
        }
    } else {
        // Generic section
        section->characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
    }
    
    g_pe_gen->section_count++;
    printf("   ✅ Section '%s' added (index: %d)\n", name, g_pe_gen->section_count - 1);
    
    return 0;
}

// Add symbol (simplified - PE symbols are complex)
int pe_generator_add_symbol(const char* name, size_t offset) {
    if (!g_pe_gen) {
        printf("❌ PE Generator: Not initialized\n");
        return -1;
    }
    
    printf("🔗 PE Generator: Adding symbol '%s' at offset 0x%zx\n", name, offset);
    
    // For PE files, we primarily care about the entry point
    if (strcmp(name, "main") == 0 || strcmp(name, "_start") == 0) {
        g_pe_gen->entry_point_rva = (uint32_t)offset + 0x1000;  // .text section RVA
        printf("   🎯 Entry point set to RVA 0x%x\n", g_pe_gen->entry_point_rva);
    }
    
    g_pe_gen->symbols_added++;
    return 0;
}

// Calculate aligned size
static uint32_t align_to(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

// Generate PE file
int pe_generator_generate(const char* output_path) {
    if (!g_pe_gen) {
        printf("❌ PE Generator: Not initialized\n");
        return -1;
    }
    
    printf("🔨 PE Generator: Generating %s\n", output_path);
    
    g_pe_gen->output_path = strdup(output_path);
    g_pe_gen->output_file = fopen(output_path, "wb");
    if (!g_pe_gen->output_file) {
        printf("❌ PE Generator: Cannot create output file %s\n", output_path);
        return -1;
    }
    
    // Calculate section layout
    uint32_t current_rva = 0x1000;  // First section starts at 0x1000
    uint32_t current_file_offset = align_to(sizeof(DOS_Header) + 
                                          g_pe_gen->dos_header.e_lfanew - sizeof(DOS_Header) +
                                          sizeof(PE_Header) + 
                                          sizeof(Optional_Header_PE32Plus) +
                                          g_pe_gen->section_count * sizeof(Section_Header),
                                          g_pe_gen->opt_header.file_alignment);
    
    // Update section addresses
    for (int i = 0; i < g_pe_gen->section_count; i++) {
        Section_Header* section = &g_pe_gen->sections[i];
        
        section->virtual_address = current_rva;
        section->ptr_to_raw_data = current_file_offset;
        
        printf("   📍 Section '%s': RVA=0x%x, File=0x%x, Size=%u\n",
               section->name, section->virtual_address, 
               section->ptr_to_raw_data, section->size_of_raw_data);
        
        current_rva = align_to(current_rva + section->virtual_size, 
                              g_pe_gen->opt_header.section_alignment);
        current_file_offset = align_to(current_file_offset + section->size_of_raw_data,
                                      g_pe_gen->opt_header.file_alignment);
    }
    
    // Update optional header
    g_pe_gen->opt_header.address_of_entry_point = g_pe_gen->entry_point_rva;
    g_pe_gen->opt_header.size_of_image = current_rva;
    g_pe_gen->opt_header.size_of_headers = align_to(sizeof(DOS_Header) + 
                                                   g_pe_gen->dos_header.e_lfanew - sizeof(DOS_Header) +
                                                   sizeof(PE_Header) + 
                                                   sizeof(Optional_Header_PE32Plus) +
                                                   g_pe_gen->section_count * sizeof(Section_Header),
                                                   g_pe_gen->opt_header.file_alignment);
    
    // Update PE header
    g_pe_gen->pe_header.num_sections = g_pe_gen->section_count;
    
    // Write DOS header
    fwrite(&g_pe_gen->dos_header, sizeof(DOS_Header), 1, g_pe_gen->output_file);
    
    // Write DOS stub (simple stub that prints error message)
    const char dos_stub[] = 
        "\x0e\x1f\xba\x0e\x00\xb4\x09\xcd\x21\xb8\x01\x4c\xcd\x21"
        "This program cannot be run in DOS mode.\r\r\n$";
    
    fwrite(dos_stub, sizeof(dos_stub) - 1, 1, g_pe_gen->output_file);
    
    // Pad to PE header offset
    long current_pos = ftell(g_pe_gen->output_file);
    while (current_pos < g_pe_gen->dos_header.e_lfanew) {
        fputc(0, g_pe_gen->output_file);
        current_pos++;
    }
    
    // Write PE header
    fwrite(&g_pe_gen->pe_header, sizeof(PE_Header), 1, g_pe_gen->output_file);
    
    // Write optional header
    fwrite(&g_pe_gen->opt_header, sizeof(Optional_Header_PE32Plus), 1, g_pe_gen->output_file);
    
    // Write section headers
    for (int i = 0; i < g_pe_gen->section_count; i++) {
        fwrite(&g_pe_gen->sections[i], sizeof(Section_Header), 1, g_pe_gen->output_file);
    }
    
    // Pad to first section
    current_pos = ftell(g_pe_gen->output_file);
    uint32_t first_section_offset = g_pe_gen->sections[0].ptr_to_raw_data;
    while (current_pos < first_section_offset) {
        fputc(0, g_pe_gen->output_file);
        current_pos++;
    }
    
    // Write section data
    for (int i = 0; i < g_pe_gen->section_count; i++) {
        Section_Header* section = &g_pe_gen->sections[i];
        
        printf("   💾 Writing section '%s' data\n", section->name);
        
        if (strcmp(section->name, ".text") == 0 && g_pe_gen->code_section) {
            fwrite(g_pe_gen->code_section, g_pe_gen->code_size, 1, g_pe_gen->output_file);
        } else if (strcmp(section->name, ".data") == 0 && g_pe_gen->data_section) {
            fwrite(g_pe_gen->data_section, g_pe_gen->data_size, 1, g_pe_gen->output_file);
        }
        
        // Pad section to alignment
        current_pos = ftell(g_pe_gen->output_file);
        uint32_t section_end = section->ptr_to_raw_data + section->size_of_raw_data;
        while (current_pos < section_end) {
            fputc(0, g_pe_gen->output_file);
            current_pos++;
        }
    }
    
    g_pe_gen->total_size = ftell(g_pe_gen->output_file);
    fclose(g_pe_gen->output_file);
    g_pe_gen->output_file = NULL;
    
    printf("✅ PE Generator: File generated successfully\n");
    printf("   📊 File size: %zu bytes\n", g_pe_gen->total_size);
    printf("   📊 Sections: %d\n", g_pe_gen->section_count);
    printf("   📊 Symbols: %d\n", g_pe_gen->symbols_added);
    printf("   🎯 Entry point: RVA 0x%x\n", g_pe_gen->entry_point_rva);
    
    return 0;
}

// Cleanup PE generator
void pe_generator_cleanup(void) {
    if (!g_pe_gen) {
        return;
    }
    
    printf("🧹 PE Generator: Cleaning up\n");
    
    if (g_pe_gen->output_file) {
        fclose(g_pe_gen->output_file);
    }
    
    if (g_pe_gen->output_path) {
        free(g_pe_gen->output_path);
    }
    
    if (g_pe_gen->code_section) {
        free(g_pe_gen->code_section);
    }
    
    if (g_pe_gen->data_section) {
        free(g_pe_gen->data_section);
    }
    
    free(g_pe_gen);
    g_pe_gen = NULL;
    
    printf("✅ PE Generator: Cleanup complete\n");
}

// Test function
int pe_generator_test(void) {
    printf("🧪 PE Generator: Running basic test\n");
    
    // Initialize for Windows x64
    if (pe_generator_initialize("windows-x64") != 0) {
        return -1;
    }
    
    // Create minimal x64 code that exits with code 42
    uint8_t test_code[] = {
        0x48, 0xc7, 0xc1, 0x2a, 0x00, 0x00, 0x00,  // mov rcx, 42
        0x48, 0x83, 0xec, 0x28,                      // sub rsp, 40
        0xff, 0x15, 0x02, 0x00, 0x00, 0x00,          // call [ExitProcess]
        0xeb, 0xfe,                                  // jmp $ (infinite loop)
        // ExitProcess import address (placeholder)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    // Add sections
    if (pe_generator_add_section(".text", test_code, sizeof(test_code)) != 0) {
        pe_generator_cleanup();
        return -1;
    }
    
    // Add entry point symbol
    if (pe_generator_add_symbol("_start", 0) != 0) {
        pe_generator_cleanup();
        return -1;
    }
    
    // Generate test file
    if (pe_generator_generate("test_pe_output.exe") != 0) {
        pe_generator_cleanup();
        return -1;
    }
    
    pe_generator_cleanup();
    
    printf("✅ PE Generator: Test completed successfully\n");
    return 0;
}

// Export the generator interface
typedef struct {
    int (*initialize)(const char* target_platform);
    int (*add_section)(const char* name, const void* data, size_t size);
    int (*add_symbol)(const char* name, size_t offset);
    int (*generate)(const char* output_path);
    void (*cleanup)(void);
} ExecutableGenerator;

ExecutableGenerator pe_generator = {
    .initialize = pe_generator_initialize,
    .add_section = pe_generator_add_section,
    .add_symbol = pe_generator_add_symbol,
    .generate = pe_generator_generate,
    .cleanup = pe_generator_cleanup
};