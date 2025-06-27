/**
 * minimal_pe_generator.c - 最小可行PE文件生成器
 * 
 * 生成一个最简单但可运行的Windows PE可执行文件
 * 用于验证我们的PE格式生成是否正确
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 生成最小的PE文件，返回42
int generate_minimal_pe(const char* output_file) {
    FILE* f = fopen(output_file, "wb");
    if (!f) return 1;
    
    // 最小PE文件的机器码：mov eax, 42; ret
    unsigned char code[] = {
        0xB8, 0x2A, 0x00, 0x00, 0x00,  // mov eax, 42
        0xC3                            // ret
    };
    
    // DOS头
    unsigned char dos_header[64] = {
        'M', 'Z',                       // DOS签名
        0x90, 0x00,                     // 最后页字节数
        0x03, 0x00,                     // 页数
        0x00, 0x00,                     // 重定位项
        0x04, 0x00,                     // 头部段数
        0x00, 0x00,                     // 最小额外段
        0xFF, 0xFF,                     // 最大额外段
        0x00, 0x00,                     // 初始SS
        0xB8, 0x00,                     // 初始SP
        0x00, 0x00,                     // 校验和
        0x00, 0x00,                     // 初始IP
        0x00, 0x00,                     // 初始CS
        0x40, 0x00,                     // 重定位表偏移
        0x00, 0x00,                     // 覆盖号
    };
    // 填充剩余的DOS头
    memset(dos_header + 28, 0, 36);
    // PE头偏移
    *(uint32_t*)(dos_header + 60) = 0x80;
    
    fwrite(dos_header, 1, 64, f);
    
    // DOS存根（填充到0x80）
    unsigned char stub[32] = {0};
    fwrite(stub, 1, 32, f);
    
    // PE签名
    fwrite("PE\0\0", 1, 4, f);
    
    // COFF文件头
    unsigned char coff_header[20] = {
        0x4C, 0x01,                     // 机器类型 (i386)
        0x01, 0x00,                     // 节数量
        0x00, 0x00, 0x00, 0x00,         // 时间戳
        0x00, 0x00, 0x00, 0x00,         // 符号表偏移
        0x00, 0x00, 0x00, 0x00,         // 符号数量
        0xE0, 0x00,                     // 可选头大小
        0x02, 0x01                      // 特征
    };
    fwrite(coff_header, 1, 20, f);
    
    // PE32可选头
    unsigned char optional_header[224] = {
        0x0B, 0x01,                     // 魔数 (PE32)
        0x0E, 0x00,                     // 链接器版本
    };
    *(uint32_t*)(optional_header + 4) = sizeof(code);      // 代码大小
    *(uint32_t*)(optional_header + 8) = 0;                 // 初始化数据大小
    *(uint32_t*)(optional_header + 12) = 0;                // 未初始化数据大小
    *(uint32_t*)(optional_header + 16) = 0x1000;           // 入口点地址
    *(uint32_t*)(optional_header + 20) = 0x1000;           // 代码基址
    *(uint32_t*)(optional_header + 24) = 0x1000;           // 数据基址
    *(uint32_t*)(optional_header + 28) = 0x400000;         // 镜像基址
    *(uint32_t*)(optional_header + 32) = 0x1000;           // 节对齐
    *(uint32_t*)(optional_header + 36) = 0x200;            // 文件对齐
    *(uint16_t*)(optional_header + 40) = 6;                // 操作系统版本
    *(uint16_t*)(optional_header + 42) = 0;
    *(uint16_t*)(optional_header + 44) = 0;                // 镜像版本
    *(uint16_t*)(optional_header + 46) = 0;
    *(uint16_t*)(optional_header + 48) = 6;                // 子系统版本
    *(uint16_t*)(optional_header + 50) = 0;
    *(uint32_t*)(optional_header + 52) = 0;                // Win32版本
    *(uint32_t*)(optional_header + 56) = 0x2000;           // 镜像大小
    *(uint32_t*)(optional_header + 60) = 0x200;            // 头部大小
    *(uint32_t*)(optional_header + 64) = 0;                // 校验和
    *(uint16_t*)(optional_header + 68) = 3;                // 子系统 (CONSOLE)
    *(uint16_t*)(optional_header + 70) = 0;                // DLL特征
    *(uint32_t*)(optional_header + 72) = 0x100000;         // 栈保留大小
    *(uint32_t*)(optional_header + 76) = 0x1000;           // 栈提交大小
    *(uint32_t*)(optional_header + 80) = 0x100000;         // 堆保留大小
    *(uint32_t*)(optional_header + 84) = 0x1000;           // 堆提交大小
    *(uint32_t*)(optional_header + 88) = 0;                // 加载器标志
    *(uint32_t*)(optional_header + 92) = 16;               // 数据目录数量
    
    // 数据目录（16个条目，每个8字节）
    memset(optional_header + 96, 0, 128);
    
    fwrite(optional_header, 1, 224, f);
    
    // 节表 (.text节)
    unsigned char section_header[40] = {
        '.', 't', 'e', 'x', 't', 0, 0, 0,  // 节名
    };
    *(uint32_t*)(section_header + 8) = sizeof(code);       // 虚拟大小
    *(uint32_t*)(section_header + 12) = 0x1000;            // 虚拟地址
    *(uint32_t*)(section_header + 16) = 0x200;             // 原始数据大小
    *(uint32_t*)(section_header + 20) = 0x200;             // 原始数据偏移
    *(uint32_t*)(section_header + 24) = 0;                 // 重定位偏移
    *(uint32_t*)(section_header + 28) = 0;                 // 行号偏移
    *(uint16_t*)(section_header + 32) = 0;                 // 重定位数量
    *(uint16_t*)(section_header + 34) = 0;                 // 行号数量
    *(uint32_t*)(section_header + 36) = 0x60000020;        // 特征
    
    fwrite(section_header, 1, 40, f);
    
    // 填充到文件对齐边界 (0x200)
    long current_pos = ftell(f);
    while (current_pos < 0x200) {
        fputc(0, f);
        current_pos++;
    }
    
    // 写入代码
    fwrite(code, 1, sizeof(code), f);
    
    // 填充到文件对齐边界
    current_pos = ftell(f);
    while (current_pos < 0x400) {
        fputc(0, f);
        current_pos++;
    }
    
    fclose(f);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <output.exe>\n", argv[0]);
        return 1;
    }
    
    printf("Generating minimal PE file: %s\n", argv[1]);
    
    if (generate_minimal_pe(argv[1]) == 0) {
        printf("✅ Minimal PE file generated successfully\n");
        return 0;
    } else {
        printf("❌ Failed to generate PE file\n");
        return 1;
    }
}
