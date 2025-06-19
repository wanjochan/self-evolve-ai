#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <elf.h>

// 创建一个最小的ELF可执行文件，使用系统调用退出
int main() {
    FILE *f = fopen("test_minimal", "wb");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    // 使用系统调用的代码：
    // mov rax, 60  (sys_exit)
    // mov rdi, 42  (退出码)
    // syscall
    unsigned char code[] = {
        0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00,  // mov rax, 60
        0x48, 0xC7, 0xC7, 0x2A, 0x00, 0x00, 0x00,  // mov rdi, 42
        0x0F, 0x05                                   // syscall
    };
    
    size_t code_size = sizeof(code);
    
    // ELF头
    Elf64_Ehdr ehdr = {0};
    ehdr.e_ident[EI_MAG0] = ELFMAG0;
    ehdr.e_ident[EI_MAG1] = ELFMAG1;
    ehdr.e_ident[EI_MAG2] = ELFMAG2;
    ehdr.e_ident[EI_MAG3] = ELFMAG3;
    ehdr.e_ident[EI_CLASS] = ELFCLASS64;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;
    ehdr.e_ident[EI_OSABI] = ELFOSABI_SYSV;
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = 0x400078;  // 入口地址 = 0x400000 + sizeof(Ehdr) + sizeof(Phdr)
    ehdr.e_phoff = sizeof(Elf64_Ehdr);
    ehdr.e_shoff = 0;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Phdr);
    ehdr.e_phnum = 1;
    ehdr.e_shentsize = 0;
    ehdr.e_shnum = 0;
    ehdr.e_shstrndx = 0;
    
    // 程序头
    Elf64_Phdr phdr = {0};
    phdr.p_type = PT_LOAD;
    phdr.p_flags = PF_X | PF_R;
    phdr.p_offset = 0;
    phdr.p_vaddr = 0x400000;
    phdr.p_paddr = 0x400000;
    phdr.p_filesz = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + code_size;
    phdr.p_memsz = phdr.p_filesz;
    phdr.p_align = 0x1000;
    
    // 写入
    fwrite(&ehdr, sizeof(ehdr), 1, f);
    fwrite(&phdr, sizeof(phdr), 1, f);
    fwrite(code, code_size, 1, f);
    
    fclose(f);
    
    // 设置可执行权限
    chmod("test_minimal", 0755);
    
    printf("创建了 test_minimal\n");
    printf("入口地址: 0x%lx\n", ehdr.e_entry);
    printf("代码大小: %zu 字节\n", code_size);
    
    return 0;
}