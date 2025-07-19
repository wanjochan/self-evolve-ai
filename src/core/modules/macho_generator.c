/**
 * macho_generator.c - macOS Mach-O File Format Generator
 *
 * Generates macOS Mach-O executable files for C99Bin compiler
 * Supports ARM64 (Apple Silicon) and x86_64 architectures
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Mach-O file format structures
#pragma pack(push, 1)

// Mach-O header (64-bit)
typedef struct {
    uint32_t magic;        // MH_MAGIC_64 (0xfeedfacf)
    uint32_t cputype;      // CPU type
    uint32_t cpusubtype;   // CPU subtype
    uint32_t filetype;     // File type
    uint32_t ncmds;        // Number of load commands
    uint32_t sizeofcmds;   // Size of load commands
    uint32_t flags;        // Flags
    uint32_t reserved;     // Reserved (64-bit only)
} mach_header_64;

// Load command header
typedef struct {
    uint32_t cmd;          // Command type
    uint32_t cmdsize;      // Command size
} load_command;

// Segment command (64-bit)
typedef struct {
    uint32_t cmd;          // LC_SEGMENT_64
    uint32_t cmdsize;      // Command size
    char segname[16];      // Segment name
    uint64_t vmaddr;       // Virtual memory address
    uint64_t vmsize;       // Virtual memory size
    uint64_t fileoff;      // File offset
    uint64_t filesize;     // File size
    uint32_t maxprot;      // Maximum protection
    uint32_t initprot;     // Initial protection
    uint32_t nsects;       // Number of sections
    uint32_t flags;        // Flags
} segment_command_64;

// Section (64-bit)
typedef struct {
    char sectname[16];     // Section name
    char segname[16];      // Segment name
    uint64_t addr;         // Virtual address
    uint64_t size;         // Size in bytes
    uint32_t offset;       // File offset
    uint32_t align;        // Alignment (power of 2)
    uint32_t reloff;       // Relocation entries offset
    uint32_t nreloc;       // Number of relocation entries
    uint32_t flags;        // Flags
    uint32_t reserved1;    // Reserved
    uint32_t reserved2;    // Reserved
    uint32_t reserved3;    // Reserved (64-bit only)
} section_64;

// Thread command
typedef struct {
    uint32_t cmd;          // LC_UNIXTHREAD
    uint32_t cmdsize;      // Command size
    uint32_t flavor;       // Thread state flavor
    uint32_t count;        // Thread state count
} thread_command;

// ARM64 thread state
typedef struct {
    uint64_t x[29];        // General purpose registers x0-x28
    uint64_t fp;           // Frame pointer x29
    uint64_t lr;           // Link register x30
    uint64_t sp;           // Stack pointer
    uint64_t pc;           // Program counter
    uint32_t cpsr;         // Current program status register
    uint32_t __pad;        // Padding
} arm_thread_state64_t;

// x86_64 thread state
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
    uint16_t cs;
    uint16_t fs;
    uint16_t gs;
    uint16_t __pad;
} x86_thread_state64_t;

#pragma pack(pop)

// Mach-O constants
#define MH_MAGIC_64         0xfeedfacf
#define MH_EXECUTE          0x2

// CPU types
#define CPU_TYPE_ARM64      0x0100000c
#define CPU_TYPE_X86_64     0x01000007

// CPU subtypes
#define CPU_SUBTYPE_ARM64_ALL   0
#define CPU_SUBTYPE_X86_64_ALL  3

// Load command types
#define LC_SEGMENT_64       0x19
#define LC_UNIXTHREAD       0x5

// Thread flavors
#define ARM_THREAD_STATE64  6
#define x86_THREAD_STATE64  4

// Thread state counts
#define ARM_THREAD_STATE64_COUNT  68
#define x86_THREAD_STATE64_COUNT  42

// Protection flags
#define VM_PROT_READ        0x01
#define VM_PROT_WRITE       0x02
#define VM_PROT_EXECUTE     0x04

// Section types
#define S_REGULAR           0x0
#define S_ATTR_PURE_INSTRUCTIONS 0x80000000
#define S_ATTR_SOME_INSTRUCTIONS 0x40000000

// Mach-O Generator state
typedef struct {
    FILE* output_file;
    char* output_path;
    
    // Mach-O header
    mach_header_64 header;
    
    // Load commands
    segment_command_64 text_segment;
    section_64 text_section;
    segment_command_64 data_segment;
    section_64 data_section;
    thread_command thread_cmd;
    
    // Thread state (union for different architectures)
    union {
        arm_thread_state64_t arm64_state;
        x86_thread_state64_t x86_64_state;
    } thread_state;
    
    // Raw data buffers
    uint8_t* code_section;
    size_t code_size;
    uint8_t* data_section_data;
    size_t data_size;
    
    // Architecture
    int is_arm64;
    uint32_t cpu_type;
    uint32_t cpu_subtype;
    
    // Layout
    uint64_t text_vmaddr;
    uint64_t data_vmaddr;
    uint64_t entry_point;
    
    // Statistics
    size_t total_size;
    int symbols_added;
    
} MachO_Generator;

// Global generator instance
static MachO_Generator* g_macho_gen = NULL;

// Initialize Mach-O generator
int macho_generator_initialize(const char* target_platform) {
    printf("🔨 Mach-O Generator: Initializing for %s\n", target_platform);
    
    if (g_macho_gen) {
        printf("⚠️  Mach-O Generator: Already initialized\n");
        return 0;
    }
    
    g_macho_gen = calloc(1, sizeof(MachO_Generator));
    if (!g_macho_gen) {
        printf("❌ Mach-O Generator: Memory allocation failed\n");
        return -1;
    }
    
    // Determine architecture
    if (strstr(target_platform, "arm64") || strstr(target_platform, "aarch64")) {
        g_macho_gen->is_arm64 = 1;
        g_macho_gen->cpu_type = CPU_TYPE_ARM64;
        g_macho_gen->cpu_subtype = CPU_SUBTYPE_ARM64_ALL;
        printf("   🎯 Target: macOS ARM64 (Apple Silicon)\n");
    } else if (strstr(target_platform, "x64") || strstr(target_platform, "x86_64")) {
        g_macho_gen->is_arm64 = 0;
        g_macho_gen->cpu_type = CPU_TYPE_X86_64;
        g_macho_gen->cpu_subtype = CPU_SUBTYPE_X86_64_ALL;
        printf("   🎯 Target: macOS x86_64 (Intel)\n");
    } else {
        printf("❌ Mach-O Generator: Unsupported platform %s\n", target_platform);
        free(g_macho_gen);
        g_macho_gen = NULL;
        return -1;
    }
    
    // Initialize Mach-O header
    g_macho_gen->header.magic = MH_MAGIC_64;
    g_macho_gen->header.cputype = g_macho_gen->cpu_type;
    g_macho_gen->header.cpusubtype = g_macho_gen->cpu_subtype;
    g_macho_gen->header.filetype = MH_EXECUTE;
    g_macho_gen->header.ncmds = 0;
    g_macho_gen->header.sizeofcmds = 0;
    g_macho_gen->header.flags = 0;
    g_macho_gen->header.reserved = 0;
    
    // Set up virtual memory layout
    g_macho_gen->text_vmaddr = 0x100000000ULL;  // Standard text segment address
    g_macho_gen->data_vmaddr = 0x100004000ULL;  // Data segment follows text
    
    printf("✅ Mach-O Generator: Initialization complete\n");
    return 0;
}

// Add section
int macho_generator_add_section(const char* name, const void* data, size_t size) {
    if (!g_macho_gen) {
        printf("❌ Mach-O Generator: Not initialized\n");
        return -1;
    }
    
    printf("📝 Mach-O Generator: Adding section '%s' (%zu bytes)\n", name, size);
    
    if (strcmp(name, ".text") == 0 || strcmp(name, "__text") == 0) {
        // Code section
        g_macho_gen->code_section = malloc(size);
        if (g_macho_gen->code_section) {
            memcpy(g_macho_gen->code_section, data, size);
            g_macho_gen->code_size = size;
            
            // Set up TEXT segment
            g_macho_gen->text_segment.cmd = LC_SEGMENT_64;
            g_macho_gen->text_segment.cmdsize = sizeof(segment_command_64) + sizeof(section_64);
            strcpy(g_macho_gen->text_segment.segname, "__TEXT");
            g_macho_gen->text_segment.vmaddr = g_macho_gen->text_vmaddr;
            g_macho_gen->text_segment.vmsize = (size + 0xfff) & ~0xfff;  // Page align
            g_macho_gen->text_segment.fileoff = 0;
            g_macho_gen->text_segment.filesize = g_macho_gen->text_segment.vmsize;
            g_macho_gen->text_segment.maxprot = VM_PROT_READ | VM_PROT_EXECUTE;
            g_macho_gen->text_segment.initprot = VM_PROT_READ | VM_PROT_EXECUTE;
            g_macho_gen->text_segment.nsects = 1;
            g_macho_gen->text_segment.flags = 0;
            
            // Set up __text section
            strcpy(g_macho_gen->text_section.sectname, "__text");
            strcpy(g_macho_gen->text_section.segname, "__TEXT");
            g_macho_gen->text_section.addr = g_macho_gen->text_vmaddr;
            g_macho_gen->text_section.size = size;
            g_macho_gen->text_section.offset = 0;  // Will be updated in generate()
            g_macho_gen->text_section.align = 2;   // 4-byte alignment
            g_macho_gen->text_section.reloff = 0;
            g_macho_gen->text_section.nreloc = 0;
            g_macho_gen->text_section.flags = S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS;
            g_macho_gen->text_section.reserved1 = 0;
            g_macho_gen->text_section.reserved2 = 0;
            g_macho_gen->text_section.reserved3 = 0;
            
            printf("   ✅ TEXT section configured\n");
        }
    } else if (strcmp(name, ".data") == 0 || strcmp(name, "__data") == 0) {
        // Data section
        g_macho_gen->data_section_data = malloc(size);
        if (g_macho_gen->data_section_data) {
            memcpy(g_macho_gen->data_section_data, data, size);
            g_macho_gen->data_size = size;
            
            // Set up DATA segment
            g_macho_gen->data_segment.cmd = LC_SEGMENT_64;
            g_macho_gen->data_segment.cmdsize = sizeof(segment_command_64) + sizeof(section_64);
            strcpy(g_macho_gen->data_segment.segname, "__DATA");
            g_macho_gen->data_segment.vmaddr = g_macho_gen->data_vmaddr;
            g_macho_gen->data_segment.vmsize = (size + 0xfff) & ~0xfff;  // Page align
            g_macho_gen->data_segment.fileoff = 0;  // Will be updated in generate()
            g_macho_gen->data_segment.filesize = g_macho_gen->data_segment.vmsize;
            g_macho_gen->data_segment.maxprot = VM_PROT_READ | VM_PROT_WRITE;
            g_macho_gen->data_segment.initprot = VM_PROT_READ | VM_PROT_WRITE;
            g_macho_gen->data_segment.nsects = 1;
            g_macho_gen->data_segment.flags = 0;
            
            // Set up __data section
            strcpy(g_macho_gen->data_section.sectname, "__data");
            strcpy(g_macho_gen->data_section.segname, "__DATA");
            g_macho_gen->data_section.addr = g_macho_gen->data_vmaddr;
            g_macho_gen->data_section.size = size;
            g_macho_gen->data_section.offset = 0;  // Will be updated in generate()
            g_macho_gen->data_section.align = 3;   // 8-byte alignment
            g_macho_gen->data_section.reloff = 0;
            g_macho_gen->data_section.nreloc = 0;
            g_macho_gen->data_section.flags = S_REGULAR;
            g_macho_gen->data_section.reserved1 = 0;
            g_macho_gen->data_section.reserved2 = 0;
            g_macho_gen->data_section.reserved3 = 0;
            
            printf("   ✅ DATA section configured\n");
        }
    } else {
        printf("   ⚠️  Section '%s' not supported, ignoring\n", name);
        return 0;
    }
    
    return 0;
}

// Add symbol
int macho_generator_add_symbol(const char* name, size_t offset) {
    if (!g_macho_gen) {
        printf("❌ Mach-O Generator: Not initialized\n");
        return -1;
    }
    
    printf("🔗 Mach-O Generator: Adding symbol '%s' at offset 0x%zx\n", name, offset);
    
    // For Mach-O files, we primarily care about the entry point
    if (strcmp(name, "main") == 0 || strcmp(name, "_start") == 0 || strcmp(name, "_main") == 0) {
        g_macho_gen->entry_point = g_macho_gen->text_vmaddr + offset;
        printf("   🎯 Entry point set to 0x%llx\n", g_macho_gen->entry_point);
    }
    
    g_macho_gen->symbols_added++;
    return 0;
}

// Page align
static uint64_t page_align(uint64_t value) {
    return (value + 0xfff) & ~0xfff;
}

// Generate Mach-O file
int macho_generator_generate(const char* output_path) {
    if (!g_macho_gen) {
        printf("❌ Mach-O Generator: Not initialized\n");
        return -1;
    }
    
    printf("🔨 Mach-O Generator: Generating %s\n", output_path);
    
    g_macho_gen->output_path = strdup(output_path);
    g_macho_gen->output_file = fopen(output_path, "wb");
    if (!g_macho_gen->output_file) {
        printf("❌ Mach-O Generator: Cannot create output file %s\n", output_path);
        return -1;
    }
    
    // Calculate layout
    size_t header_size = sizeof(mach_header_64);
    size_t load_commands_size = 0;
    
    // Count load commands
    if (g_macho_gen->code_section) {
        load_commands_size += sizeof(segment_command_64) + sizeof(section_64);
        g_macho_gen->header.ncmds++;
    }
    if (g_macho_gen->data_section_data) {
        load_commands_size += sizeof(segment_command_64) + sizeof(section_64);
        g_macho_gen->header.ncmds++;
    }
    
    // Add thread command
    if (g_macho_gen->is_arm64) {
        load_commands_size += sizeof(thread_command) + sizeof(arm_thread_state64_t);
    } else {
        load_commands_size += sizeof(thread_command) + sizeof(x86_thread_state64_t);
    }
    g_macho_gen->header.ncmds++;
    
    g_macho_gen->header.sizeofcmds = load_commands_size;
    
    // Calculate file offsets
    uint64_t current_offset = page_align(header_size + load_commands_size);
    
    if (g_macho_gen->code_section) {
        g_macho_gen->text_segment.fileoff = current_offset;
        g_macho_gen->text_section.offset = current_offset;
        current_offset += page_align(g_macho_gen->code_size);
    }
    
    if (g_macho_gen->data_section_data) {
        g_macho_gen->data_segment.fileoff = current_offset;
        g_macho_gen->data_section.offset = current_offset;
        current_offset += page_align(g_macho_gen->data_size);
    }
    
    // Set up thread command
    g_macho_gen->thread_cmd.cmd = LC_UNIXTHREAD;
    
    if (g_macho_gen->is_arm64) {
        g_macho_gen->thread_cmd.cmdsize = sizeof(thread_command) + sizeof(arm_thread_state64_t);
        g_macho_gen->thread_cmd.flavor = ARM_THREAD_STATE64;
        g_macho_gen->thread_cmd.count = ARM_THREAD_STATE64_COUNT;
        
        // Initialize ARM64 thread state
        memset(&g_macho_gen->thread_state.arm64_state, 0, sizeof(arm_thread_state64_t));
        g_macho_gen->thread_state.arm64_state.pc = g_macho_gen->entry_point;
        g_macho_gen->thread_state.arm64_state.sp = 0x7fff5fbff000ULL;  // Initial stack pointer
    } else {
        g_macho_gen->thread_cmd.cmdsize = sizeof(thread_command) + sizeof(x86_thread_state64_t);
        g_macho_gen->thread_cmd.flavor = x86_THREAD_STATE64;
        g_macho_gen->thread_cmd.count = x86_THREAD_STATE64_COUNT;
        
        // Initialize x86_64 thread state
        memset(&g_macho_gen->thread_state.x86_64_state, 0, sizeof(x86_thread_state64_t));
        g_macho_gen->thread_state.x86_64_state.rip = g_macho_gen->entry_point;
        g_macho_gen->thread_state.x86_64_state.rsp = 0x7fff5fbff000ULL;  // Initial stack pointer
    }
    
    printf("   📊 Header size: %zu bytes\n", header_size);
    printf("   📊 Load commands size: %zu bytes\n", load_commands_size);
    printf("   📊 TEXT section offset: 0x%llx\n", g_macho_gen->text_segment.fileoff);
    if (g_macho_gen->data_section_data) {
        printf("   📊 DATA section offset: 0x%llx\n", g_macho_gen->data_segment.fileoff);
    }
    printf("   🎯 Entry point: 0x%llx\n", g_macho_gen->entry_point);
    
    // Write Mach-O header
    fwrite(&g_macho_gen->header, sizeof(mach_header_64), 1, g_macho_gen->output_file);
    
    // Write load commands
    if (g_macho_gen->code_section) {
        fwrite(&g_macho_gen->text_segment, sizeof(segment_command_64), 1, g_macho_gen->output_file);
        fwrite(&g_macho_gen->text_section, sizeof(section_64), 1, g_macho_gen->output_file);
    }
    
    if (g_macho_gen->data_section_data) {
        fwrite(&g_macho_gen->data_segment, sizeof(segment_command_64), 1, g_macho_gen->output_file);
        fwrite(&g_macho_gen->data_section, sizeof(section_64), 1, g_macho_gen->output_file);
    }
    
    // Write thread command
    fwrite(&g_macho_gen->thread_cmd, sizeof(thread_command), 1, g_macho_gen->output_file);
    if (g_macho_gen->is_arm64) {
        fwrite(&g_macho_gen->thread_state.arm64_state, sizeof(arm_thread_state64_t), 1, g_macho_gen->output_file);
    } else {
        fwrite(&g_macho_gen->thread_state.x86_64_state, sizeof(x86_thread_state64_t), 1, g_macho_gen->output_file);
    }
    
    // Pad to first section
    long current_pos = ftell(g_macho_gen->output_file);
    while (current_pos < g_macho_gen->text_segment.fileoff) {
        fputc(0, g_macho_gen->output_file);
        current_pos++;
    }
    
    // Write section data
    if (g_macho_gen->code_section) {
        printf("   💾 Writing TEXT section data (%zu bytes)\n", g_macho_gen->code_size);
        fwrite(g_macho_gen->code_section, g_macho_gen->code_size, 1, g_macho_gen->output_file);
        
        // Pad to page boundary
        current_pos = ftell(g_macho_gen->output_file);
        uint64_t text_end = g_macho_gen->text_segment.fileoff + g_macho_gen->text_segment.filesize;
        while (current_pos < text_end) {
            fputc(0, g_macho_gen->output_file);
            current_pos++;
        }
    }
    
    if (g_macho_gen->data_section_data) {
        // Pad to data section offset
        current_pos = ftell(g_macho_gen->output_file);
        while (current_pos < g_macho_gen->data_segment.fileoff) {
            fputc(0, g_macho_gen->output_file);
            current_pos++;
        }
        
        printf("   💾 Writing DATA section data (%zu bytes)\n", g_macho_gen->data_size);
        fwrite(g_macho_gen->data_section_data, g_macho_gen->data_size, 1, g_macho_gen->output_file);
        
        // Pad to page boundary
        current_pos = ftell(g_macho_gen->output_file);
        uint64_t data_end = g_macho_gen->data_segment.fileoff + g_macho_gen->data_segment.filesize;
        while (current_pos < data_end) {
            fputc(0, g_macho_gen->output_file);
            current_pos++;
        }
    }
    
    g_macho_gen->total_size = ftell(g_macho_gen->output_file);
    fclose(g_macho_gen->output_file);
    g_macho_gen->output_file = NULL;
    
    printf("✅ Mach-O Generator: File generated successfully\n");
    printf("   📊 File size: %zu bytes\n", g_macho_gen->total_size);
    printf("   📊 Load commands: %u\n", g_macho_gen->header.ncmds);
    printf("   📊 Symbols: %d\n", g_macho_gen->symbols_added);
    printf("   🎯 Entry point: 0x%llx\n", g_macho_gen->entry_point);
    
    return 0;
}

// Cleanup Mach-O generator
void macho_generator_cleanup(void) {
    if (!g_macho_gen) {
        return;
    }
    
    printf("🧹 Mach-O Generator: Cleaning up\n");
    
    if (g_macho_gen->output_file) {
        fclose(g_macho_gen->output_file);
    }
    
    if (g_macho_gen->output_path) {
        free(g_macho_gen->output_path);
    }
    
    if (g_macho_gen->code_section) {
        free(g_macho_gen->code_section);
    }
    
    if (g_macho_gen->data_section_data) {
        free(g_macho_gen->data_section_data);
    }
    
    free(g_macho_gen);
    g_macho_gen = NULL;
    
    printf("✅ Mach-O Generator: Cleanup complete\n");
}

// Test function
int macho_generator_test(void) {
    printf("🧪 Mach-O Generator: Running basic test\n");
    
    // Initialize for macOS ARM64
    if (macho_generator_initialize("macos-arm64") != 0) {
        return -1;
    }
    
    // Create minimal ARM64 code that exits with code 42
    uint8_t test_code[] = {
        // mov x0, #42
        0x40, 0x05, 0x80, 0xd2,  
        // mov x16, #1 (exit syscall)
        0x30, 0x00, 0x80, 0xd2,  
        // svc #0x80 (system call)
        0x01, 0x10, 0x00, 0xd4   
    };
    
    // Add sections
    if (macho_generator_add_section("__text", test_code, sizeof(test_code)) != 0) {
        macho_generator_cleanup();
        return -1;
    }
    
    // Add entry point symbol
    if (macho_generator_add_symbol("_main", 0) != 0) {
        macho_generator_cleanup();
        return -1;
    }
    
    // Generate test file
    if (macho_generator_generate("test_macho_output") != 0) {
        macho_generator_cleanup();
        return -1;
    }
    
    macho_generator_cleanup();
    
    printf("✅ Mach-O Generator: Test completed successfully\n");
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

ExecutableGenerator macho_generator = {
    .initialize = macho_generator_initialize,
    .add_section = macho_generator_add_section,
    .add_symbol = macho_generator_add_symbol,
    .generate = macho_generator_generate,
    .cleanup = macho_generator_cleanup
};