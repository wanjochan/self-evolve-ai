#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int main() {
    // 测试生成的机器码
    // 这是一个返回42的函数：
    // push rbp
    // mov rbp, rsp
    // sub rsp, 64
    // mov eax, 42
    // mov rsp, rbp
    // pop rbp
    // ret
    
    unsigned char code[] = {
        0x55,                   // push rbp
        0x48, 0x89, 0xE5,      // mov rbp, rsp
        0x48, 0x81, 0xEC, 0x40, 0x00, 0x00, 0x00,  // sub rsp, 64
        0xB8, 0x2A, 0x00, 0x00, 0x00,  // mov eax, 42
        0x48, 0x89, 0xEC,      // mov rsp, rbp
        0x5D,                   // pop rbp
        0xC3                    // ret
    };
    
    size_t code_size = sizeof(code);
    
    // 分配可执行内存
    void *mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (mem == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
    // 复制代码
    memcpy(mem, code, code_size);
    
    // 执行代码
    typedef int (*func_t)(void);
    func_t func = (func_t)mem;
    int result = func();
    
    printf("结果: %d\n", result);
    
    // 清理
    munmap(mem, 4096);
    
    return 0;
}