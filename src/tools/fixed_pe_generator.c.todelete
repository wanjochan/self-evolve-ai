/**
 * fixed_pe_generator.c - 修复的PE文件生成器
 * 
 * 基于深入的PE格式研究，生成正确的最小Windows PE可执行文件
 * 参考: https://www.bigmessowires.com/2015/10/08/a-handmade-executable-file/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 生成最小但正确的PE文件
int generate_fixed_pe(const char* output_file, unsigned char* code, size_t code_size) {
    FILE* f = fopen(output_file, "wb");
    if (!f) return 1;
    
    // 确保代码大小至少为4字节
    if (code_size < 4) {
        code_size = 4;
        code = (unsigned char*)"\xB8\x2A\x00\x00\x00\xC3"; // mov eax, 42; ret
    }
    
    unsigned char exe[512] = {0}; // 初始化为0
    
    // 1. DOS头 (64字节)
    exe[0] = 'M'; exe[1] = 'Z';                    // DOS签名
    exe[2] = 0x90; exe[3] = 0x00;                  // 最后页字节数
    exe[4] = 0x03; exe[5] = 0x00;                  // 页数
    exe[6] = 0x00; exe[7] = 0x00;                  // 重定位项
    exe[8] = 0x04; exe[9] = 0x00;                  // 头部段数
    exe[10] = 0x00; exe[11] = 0x00;                // 最小额外段
    exe[12] = 0xFF; exe[13] = 0xFF;                // 最大额外段
    exe[14] = 0x00; exe[15] = 0x00;                // 初始SS
    exe[16] = 0xB8; exe[17] = 0x00;                // 初始SP
    exe[18] = 0x00; exe[19] = 0x00;                // 校验和
    exe[20] = 0x00; exe[21] = 0x00;                // 初始IP
    exe[22] = 0x00; exe[23] = 0x00;                // 初始CS
    exe[24] = 0x40; exe[25] = 0x00;                // 重定位表偏移
    // 填充到60字节
    *(uint32_t*)(exe + 60) = 64;                   // PE头偏移
    
    // 2. PE签名 (4字节)
    exe[64] = 'P'; exe[65] = 'E'; exe[66] = 0; exe[67] = 0;
    
    // 3. COFF文件头 (20字节)
    *(uint16_t*)(exe + 68) = 0x14C;                // 机器类型 (i386)
    *(uint16_t*)(exe + 70) = 1;                    // 节数量
    *(uint32_t*)(exe + 72) = 0;                    // 时间戳
    *(uint32_t*)(exe + 76) = 0;                    // 符号表偏移
    *(uint32_t*)(exe + 80) = 0;                    // 符号数量
    *(uint16_t*)(exe + 84) = 224;                  // 可选头大小
    *(uint16_t*)(exe + 86) = 0x103;                // 特征
    
    // 4. 可选头 (224字节)
    *(uint16_t*)(exe + 88) = 0x10B;                // 魔数 (PE32)
    *(uint8_t*)(exe + 90) = 0x0E;                  // 链接器主版本
    *(uint8_t*)(exe + 91) = 0x00;                  // 链接器次版本
    *(uint32_t*)(exe + 92) = code_size;            // 代码大小
    *(uint32_t*)(exe + 96) = 0;                    // 初始化数据大小
    *(uint32_t*)(exe + 100) = 0;                   // 未初始化数据大小
    *(uint32_t*)(exe + 104) = 0x1000;              // 入口点地址 (RVA)
    *(uint32_t*)(exe + 108) = 0x1000;              // 代码基址
    *(uint32_t*)(exe + 112) = 0x1000;              // 数据基址
    *(uint32_t*)(exe + 116) = 0x400000;            // 镜像基址
    *(uint32_t*)(exe + 120) = 0x1000;              // 节对齐 (4096)
    *(uint32_t*)(exe + 124) = 0x200;               // 文件对齐 (512)
    *(uint16_t*)(exe + 128) = 6;                   // 操作系统主版本
    *(uint16_t*)(exe + 130) = 0;                   // 操作系统次版本
    *(uint16_t*)(exe + 132) = 0;                   // 镜像主版本
    *(uint16_t*)(exe + 134) = 0;                   // 镜像次版本
    *(uint16_t*)(exe + 136) = 4;                   // 子系统主版本
    *(uint16_t*)(exe + 138) = 0;                   // 子系统次版本
    *(uint32_t*)(exe + 140) = 0;                   // Win32版本
    *(uint32_t*)(exe + 144) = 0x2000;              // 镜像大小
    *(uint32_t*)(exe + 148) = 0x200;               // 头部大小
    *(uint32_t*)(exe + 152) = 0;                   // 校验和
    *(uint16_t*)(exe + 156) = 3;                   // 子系统 (CONSOLE)
    *(uint16_t*)(exe + 158) = 0;                   // DLL特征
    *(uint32_t*)(exe + 160) = 0x100000;            // 栈保留大小
    *(uint32_t*)(exe + 164) = 0x1000;              // 栈提交大小
    *(uint32_t*)(exe + 168) = 0x100000;            // 堆保留大小
    *(uint32_t*)(exe + 172) = 0x1000;              // 堆提交大小
    *(uint32_t*)(exe + 176) = 0;                   // 加载器标志
    *(uint32_t*)(exe + 180) = 16;                  // 数据目录数量
    
    // 5. 数据目录 (16个条目，每个8字节，全部为0)
    // 已经通过memset初始化为0
    
    // 6. 节表 (.text节，40字节)
    memcpy(exe + 312, ".text\0\0\0", 8);           // 节名
    *(uint32_t*)(exe + 320) = code_size;           // 虚拟大小
    *(uint32_t*)(exe + 324) = 0x1000;              // 虚拟地址
    *(uint32_t*)(exe + 328) = code_size;           // 原始数据大小
    *(uint32_t*)(exe + 332) = 0x200;               // 原始数据偏移
    *(uint32_t*)(exe + 336) = 0;                   // 重定位偏移
    *(uint32_t*)(exe + 340) = 0;                   // 行号偏移
    *(uint16_t*)(exe + 344) = 0;                   // 重定位数量
    *(uint16_t*)(exe + 346) = 0;                   // 行号数量
    *(uint32_t*)(exe + 348) = 0x60000020;          // 特征 (CODE|EXECUTE|READ)
    
    // 写入头部 (填充到512字节)
    fwrite(exe, 1, 512, f);
    
    // 7. 代码段 (在文件偏移0x200处)
    fwrite(code, 1, code_size, f);
    
    // 填充到512字节边界
    size_t padding = (512 - (code_size % 512)) % 512;
    for (size_t i = 0; i < padding; i++) {
        fputc(0, f);
    }
    
    fclose(f);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <output.exe>\n", argv[0]);
        return 1;
    }
    
    printf("Generating fixed PE file: %s\n", argv[1]);
    
    // 简单的测试代码：mov eax, 42; ret
    unsigned char test_code[] = {
        0xB8, 0x2A, 0x00, 0x00, 0x00,  // mov eax, 42
        0xC3                            // ret
    };
    
    if (generate_fixed_pe(argv[1], test_code, sizeof(test_code)) == 0) {
        printf("✅ Fixed PE file generated successfully\n");
        printf("📊 File should be approximately 1024 bytes\n");
        return 0;
    } else {
        printf("❌ Failed to generate PE file\n");
        return 1;
    }
}
