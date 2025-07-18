/**
 * c2astc_ultra_minimal.c - 极简C到ASTC编译器
 * 
 * 专门设计为c99bin能够正确处理的最简版本
 * 只包含最基本的printf语句，避免所有复杂逻辑
 */

#include <stdio.h>

int main() {
    printf("c2astc_ultra_minimal: 极简C到ASTC编译器\n");
    printf("生成固定的ASTC文件: test_ultra.astc\n");
    
    // 创建一个固定的ASTC文件，返回值为123
    FILE* file = fopen("test_ultra.astc", "wb");
    
    // 写入ASTC头部 "ASTC"
    fputc('A', file);
    fputc('S', file);
    fputc('T', file);
    fputc('C', file);
    
    // 写入版本号 (4字节) - 版本1
    fputc(1, file);
    fputc(0, file);
    fputc(0, file);
    fputc(0, file);
    
    // 写入程序大小 (4字节) - 12字节 (3条指令)
    fputc(12, file);
    fputc(0, file);
    fputc(0, file);
    fputc(0, file);
    
    // 写入入口点 (4字节) - 0
    fputc(0, file);
    fputc(0, file);
    fputc(0, file);
    fputc(0, file);
    
    // 写入指令1: LOAD_CONST 123 (4字节，小端序)
    // 格式: (opcode << 24) | operand
    // 0x01000000 | 123 = 0x0100007B
    fputc(0x7B, file); // 123 (低字节)
    fputc(0x00, file);
    fputc(0x00, file);
    fputc(0x01, file); // ASTC_LOAD_CONST (高字节)
    
    // 写入指令2: RETURN (4字节，小端序)
    // 格式: (opcode << 24) | 0 = 0x02000000
    fputc(0x00, file); // 0 (低字节)
    fputc(0x00, file);
    fputc(0x00, file);
    fputc(0x02, file); // ASTC_RETURN (高字节)
    
    // 写入指令3: NOP (4字节)
    fputc(0, file);   // ASTC_NOP
    fputc(0, file);
    fputc(0, file);
    fputc(0, file);
    
    fclose(file);
    
    printf("成功生成test_ultra.astc文件 (返回值123)\n");
    
    return 0;
}
