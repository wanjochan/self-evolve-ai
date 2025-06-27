# ELF文件格式规格说明（Executable and Linkable Format）

## 参考资料

- [Tool Interface Standard (TIS) Executable and Linking Format (ELF) Specification](https://refspecs.linuxfoundation.org/elf/elf.pdf)
- [System V Application Binary Interface](https://www.sco.com/developers/gabi/latest/contents.html)
- [Oracle Linker and Libraries Guide](https://docs.oracle.com/cd/E19683-01/817-3677/index.html)

## 概述

ELF (Executable and Linkable Format) 是UNIX和类UNIX系统（如Linux、FreeBSD等）上使用的标准二进制文件格式，用于可执行文件、目标代码、共享库和核心转储。ELF设计灵活，可扩展，并且与平台无关。

### 重要概念

| 概念 | 描述 |
|------|------|
| 节(Section) | 包含程序数据的最小单位，如代码、数据、符号表等。链接视图中的基本单位。 |
| 段(Segment) | 由一个或多个节组成的内存单元，用于程序执行。执行视图中的基本单位。 |
| 虚拟地址(VA) | 程序被加载到内存后的地址。 |
| 文件偏移 | 数据在ELF文件中的位置。 |
| 重定位 | 修改代码或数据中的地址引用，使其指向正确的运行时位置。 |
| 符号 | 代表程序中的函数、变量或其他实体的名称。 |

## ELF文件的两种视图

ELF文件有两种视图：

1. **链接视图**：由节(sections)组成，用于静态链接
2. **执行视图**：由段(segments)组成，用于程序执行

```
 +----------------------+
 | ELF Header           |  共享
 +----------------------+
 | Program Header Table |  执行视图
 +----------------------+
 | Section 1            |
 | Section 2            |  链接视图
 | ...                  |
 | Section n            |
 +----------------------+
 | Section Header Table |  链接视图
 +----------------------+
```

## 1. ELF Header (Ehdr)

**位置**: 文件开始, **大小**: 32位系统52字节, 64位系统64字节

```c
typedef struct {
    unsigned char e_ident[16];     // 0x00: ELF标识信息 ⭐️
    Elf32_Half    e_type;          // 0x10: 文件类型
    Elf32_Half    e_machine;       // 0x12: 机器类型
    Elf32_Word    e_version;       // 0x14: ELF版本
    Elf32_Addr    e_entry;         // 0x18: 程序入口点 ⭐️
    Elf32_Off     e_phoff;         // 0x1C: 程序头表偏移 ⭐️
    Elf32_Off     e_shoff;         // 0x20: 节头表偏移 ⭐️
    Elf32_Word    e_flags;         // 0x24: 处理器特定标志
    Elf32_Half    e_ehsize;        // 0x28: ELF头大小
    Elf32_Half    e_phentsize;     // 0x2A: 程序头表项大小
    Elf32_Half    e_phnum;         // 0x2C: 程序头表项数量 ⭐️
    Elf32_Half    e_shentsize;     // 0x2E: 节头表项大小
    Elf32_Half    e_shnum;         // 0x30: 节头表项数量 ⭐️
    Elf32_Half    e_shstrndx;      // 0x32: 节名称字符串表索引
} Elf32_Ehdr;
```

64位版本 (Elf64_Ehdr) 中的地址和偏移字段为8字节。

### e_ident[16] 字段详解

```
e_ident[0..3]   = { 0x7F, 'E', 'L', 'F' }  // 魔数
e_ident[4]      = 文件类 (1=32位, 2=64位)
e_ident[5]      = 数据编码 (1=小端, 2=大端)
e_ident[6]      = 版本 (通常为1)
e_ident[7]      = OS ABI (0=System V, 3=Linux等)
e_ident[8]      = ABI版本
e_ident[9..15]  = 填充字节(未使用)
```

### e_type 文件类型

- `ET_NONE (0)`: 未知类型
- `ET_REL (1)`: 可重定位文件(目标文件)
- `ET_EXEC (2)`: 可执行文件
- `ET_DYN (3)`: 共享对象(动态库)
- `ET_CORE (4)`: 核心转储文件

### e_machine 机器类型

- `EM_386 (3)`: Intel 80386
- `EM_X86_64 (62)`: AMD x86-64
- `EM_ARM (40)`: ARM
- `EM_AARCH64 (183)`: ARM 64位

## 2. 程序头表 (Program Header Table)

程序头表描述了执行视图中的段(segments)。每个表项描述一个段或系统准备程序执行所需的其他信息。

**32位结构**:
```c
typedef struct {
    Elf32_Word    p_type;          // 0x00: 段类型
    Elf32_Off     p_offset;        // 0x04: 段在文件中的偏移 ⭐️
    Elf32_Addr    p_vaddr;         // 0x08: 段的虚拟地址 ⭐️
    Elf32_Addr    p_paddr;         // 0x0C: 段的物理地址
    Elf32_Word    p_filesz;        // 0x10: 段在文件中的大小 ⭐️
    Elf32_Word    p_memsz;         // 0x14: 段在内存中的大小 ⭐️
    Elf32_Word    p_flags;         // 0x18: 段标志
    Elf32_Word    p_align;         // 0x1C: 段对齐
} Elf32_Phdr;
```

**64位结构**:
```c
typedef struct {
    Elf64_Word    p_type;          // 0x00: 段类型
    Elf64_Word    p_flags;         // 0x04: 段标志
    Elf64_Off     p_offset;        // 0x08: 段在文件中的偏移 ⭐️
    Elf64_Addr    p_vaddr;         // 0x10: 段的虚拟地址 ⭐️
    Elf64_Addr    p_paddr;         // 0x18: 段的物理地址
    Elf64_Xword   p_filesz;        // 0x20: 段在文件中的大小 ⭐️
    Elf64_Xword   p_memsz;         // 0x28: 段在内存中的大小 ⭐️
    Elf64_Xword   p_align;         // 0x30: 段对齐
} Elf64_Phdr;
```

### p_type 段类型

- `PT_NULL (0)`: 未使用
- `PT_LOAD (1)`: 可加载段 ⭐️
- `PT_DYNAMIC (2)`: 动态链接信息
- `PT_INTERP (3)`: 程序解释器路径
- `PT_NOTE (4)`: 辅助信息
- `PT_SHLIB (5)`: 保留
- `PT_PHDR (6)`: 程序头表本身
- `PT_TLS (7)`: 线程局部存储模板

### p_flags 段标志

- `PF_X (1)`: 可执行
- `PF_W (2)`: 可写
- `PF_R (4)`: 可读

## 3. 节头表 (Section Header Table)

节头表描述了文件中所有的节(sections)。每个节占用文件中的一个连续区域（可能为空）。

**32位结构**:
```c
typedef struct {
    Elf32_Word    sh_name;         // 0x00: 节名称(字符串表索引)
    Elf32_Word    sh_type;         // 0x04: 节类型
    Elf32_Word    sh_flags;        // 0x08: 节标志
    Elf32_Addr    sh_addr;         // 0x0C: 内存中的地址
    Elf32_Off     sh_offset;       // 0x10: 文件中的偏移 ⭐️
    Elf32_Word    sh_size;         // 0x14: 节大小 ⭐️
    Elf32_Word    sh_link;         // 0x18: 链接到其他节
    Elf32_Word    sh_info;         // 0x1C: 附加信息
    Elf32_Word    sh_addralign;    // 0x20: 地址对齐
    Elf32_Word    sh_entsize;      // 0x24: 表项大小
} Elf32_Shdr;
```

**64位结构**:
```c
typedef struct {
    Elf64_Word    sh_name;         // 0x00: 节名称(字符串表索引)
    Elf64_Word    sh_type;         // 0x04: 节类型
    Elf64_Xword   sh_flags;        // 0x08: 节标志
    Elf64_Addr    sh_addr;         // 0x10: 内存中的地址
    Elf64_Off     sh_offset;       // 0x18: 文件中的偏移 ⭐️
    Elf64_Xword   sh_size;         // 0x20: 节大小 ⭐️
    Elf64_Word    sh_link;         // 0x28: 链接到其他节
    Elf64_Word    sh_info;         // 0x2C: 附加信息
    Elf64_Xword   sh_addralign;    // 0x30: 地址对齐
    Elf64_Xword   sh_entsize;      // 0x38: 表项大小
} Elf64_Shdr;
```

### sh_type 节类型

- `SHT_NULL (0)`: 未使用
- `SHT_PROGBITS (1)`: 程序定义信息
- `SHT_SYMTAB (2)`: 符号表
- `SHT_STRTAB (3)`: 字符串表
- `SHT_RELA (4)`: 带加数的重定位表项
- `SHT_HASH (5)`: 符号哈希表
- `SHT_DYNAMIC (6)`: 动态链接信息
- `SHT_NOTE (7)`: 注释信息
- `SHT_NOBITS (8)`: 不占文件空间的节
- `SHT_REL (9)`: 不带加数的重定位表项
- `SHT_DYNSYM (11)`: 动态链接符号表

### sh_flags 节标志

- `SHF_WRITE (0x1)`: 可写
- `SHF_ALLOC (0x2)`: 占用内存
- `SHF_EXECINSTR (0x4)`: 包含可执行代码
- `SHF_MERGE (0x10)`: 可合并
- `SHF_STRINGS (0x20)`: 包含以空字符结尾的字符串
- `SHF_INFO_LINK (0x40)`: sh_info包含节索引
- `SHF_TLS (0x400)`: 包含线程局部变量

## 4. 常见节(Sections)

### 特殊节

- `.text`: 程序代码
- `.data`: 已初始化的数据
- `.bss`: 未初始化的数据（不占文件空间）
- `.rodata`: 只读数据
- `.symtab`: 符号表
- `.strtab`: 字符串表
- `.shstrtab`: 节名称字符串表
- `.dynamic`: 动态链接信息
- `.got`: 全局偏移表
- `.plt`: 过程链接表
- `.rel.*/.rela.*`: 重定位信息

## 5. 动态链接

动态链接信息存储在`.dynamic`节中，由一系列结构组成：

```c
typedef struct {
    Elf32_Sword   d_tag;           // 类型标记
    union {
        Elf32_Word d_val;          // 整数值
        Elf32_Addr d_ptr;          // 地址值
    } d_un;
} Elf32_Dyn;
```

### 常见的d_tag值

- `DT_NEEDED`: 所需共享库名称
- `DT_SYMTAB`: 符号表地址
- `DT_STRTAB`: 字符串表地址
- `DT_RELA`: 重定位表地址
- `DT_HASH`: 符号哈希表地址
- `DT_SONAME`: 共享目标名称
- `DT_RPATH`: 库搜索路径

## 6. 符号表

符号表包含程序中定义和引用的符号信息：

**32位结构**:
```c
typedef struct {
    Elf32_Word    st_name;         // 符号名(字符串表索引)
    Elf32_Addr    st_value;        // 符号值
    Elf32_Word    st_size;         // 符号大小
    unsigned char st_info;         // 符号类型和绑定
    unsigned char st_other;        // 未使用
    Elf32_Half    st_shndx;        // 相关节索引
} Elf32_Sym;
```

### st_info 符号类型和绑定

- 绑定(高4位):
  - `STB_LOCAL (0)`: 局部符号
  - `STB_GLOBAL (1)`: 全局符号
  - `STB_WEAK (2)`: 弱符号

- 类型(低4位):
  - `STT_NOTYPE (0)`: 未定义类型
  - `STT_OBJECT (1)`: 数据对象
  - `STT_FUNC (2)`: 函数
  - `STT_SECTION (3)`: 节
  - `STT_FILE (4)`: 源文件名

## 7. 重定位表

重定位表用于修改代码或数据中的地址引用：

```c
typedef struct {
    Elf32_Addr    r_offset;        // 重定位位置
    Elf32_Word    r_info;          // 符号索引和类型
} Elf32_Rel;

typedef struct {
    Elf32_Addr    r_offset;        // 重定位位置
    Elf32_Word    r_info;          // 符号索引和类型
    Elf32_Sword   r_addend;        // 常数加数
} Elf32_Rela;
```

## 8. ELF文件解析步骤

1. 读取并验证ELF头(e_ident魔数)
2. 确定文件类型(e_type)
3. 对于可执行文件:
   - 读取程序头表(Program Headers)
   - 加载PT_LOAD类型的段到内存
   - 跳转到程序入口点(e_entry)
4. 对于目标文件:
   - 读取节头表(Section Headers)
   - 处理重定位和符号解析

## 9. 地址计算

**文件偏移转虚拟地址**:
1. 找到包含该偏移的段
2. VA = p_vaddr + (offset - p_offset)

**虚拟地址转文件偏移**:
1. 找到包含该VA的段
2. offset = p_offset + (VA - p_vaddr)

## 10. 工具

- `readelf`: 显示ELF文件信息
- `objdump`: 反汇编和显示ELF文件内容
- `nm`: 列出ELF文件中的符号
- `ldd`: 显示共享库依赖关系
- `strip`: 移除ELF文件中的符号和调试信息 