#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// 最简单的PE文件生成测试
int main() {
    FILE* file = fopen("tests/minimal_test.exe", "wb");
    if (!file) {
        printf("Cannot create file\n");
        return 1;
    }
    
    // DOS头（最小化）
    unsigned char dos_header[64] = {0};
    dos_header[0] = 'M'; dos_header[1] = 'Z';  // MZ signature
    *(uint32_t*)(dos_header + 60) = 64;        // PE header offset
    fwrite(dos_header, 64, 1, file);
    
    // PE头
    fwrite("PE\0\0", 4, 1, file);              // PE signature
    
    // COFF头
    uint16_t machine = 0x014C;                 // i386
    uint16_t sections = 1;
    uint32_t timestamp = 0;
    uint32_t ptr_symbols = 0;
    uint32_t num_symbols = 0;
    uint16_t opt_header_size = 224;            // PE32 optional header size
    uint16_t characteristics = 0x0102;         // EXECUTABLE_IMAGE | 32BIT_MACHINE
    
    fwrite(&machine, 2, 1, file);
    fwrite(&sections, 2, 1, file);
    fwrite(&timestamp, 4, 1, file);
    fwrite(&ptr_symbols, 4, 1, file);
    fwrite(&num_symbols, 4, 1, file);
    fwrite(&opt_header_size, 2, 1, file);
    fwrite(&characteristics, 2, 1, file);
    
    // PE32 Optional Header
    unsigned char opt_header[224] = {0};
    *(uint16_t*)(opt_header + 0) = 0x010B;     // PE32 magic
    *(uint32_t*)(opt_header + 16) = 0x1000;    // entry point
    *(uint32_t*)(opt_header + 20) = 0x1000;    // base of code
    *(uint32_t*)(opt_header + 24) = 0x2000;    // base of data
    *(uint32_t*)(opt_header + 28) = 0x400000;  // image base
    *(uint32_t*)(opt_header + 32) = 0x1000;    // section alignment
    *(uint32_t*)(opt_header + 36) = 0x200;     // file alignment
    *(uint16_t*)(opt_header + 40) = 4;         // major OS version
    *(uint16_t*)(opt_header + 48) = 4;         // major subsystem version
    *(uint32_t*)(opt_header + 56) = 0x2000;    // size of image
    *(uint32_t*)(opt_header + 60) = 0x200;     // size of headers
    *(uint16_t*)(opt_header + 68) = 3;         // subsystem (console)
    fwrite(opt_header, 224, 1, file);
    
    // Section header
    unsigned char section[40] = {0};
    memcpy(section, ".text\0\0\0", 8);         // name
    *(uint32_t*)(section + 8) = 10;            // virtual size
    *(uint32_t*)(section + 12) = 0x1000;       // virtual address
    *(uint32_t*)(section + 16) = 0x200;        // size of raw data
    *(uint32_t*)(section + 20) = 0x200;        // pointer to raw data
    *(uint32_t*)(section + 36) = 0x60000020;   // characteristics
    fwrite(section, 40, 1, file);
    
    // 填充到文件对齐
    size_t current = ftell(file);
    while (current < 0x200) {
        fputc(0, file);
        current++;
    }
    
    // 代码段：最简单的返回123的程序
    unsigned char code[] = {
        0xB8, 0x7B, 0x00, 0x00, 0x00,  // mov eax, 123
        0xC3                            // ret
    };
    fwrite(code, sizeof(code), 1, file);
    
    // 填充到节对齐
    current = ftell(file);
    while (current < 0x400) {
        fputc(0, file);
        current++;
    }
    
    fclose(file);
    printf("Generated minimal PE file: tests/minimal_test.exe\n");
    return 0;
}
