# Mach-O文件格式规格说明（Mach Object File Format）

## 参考资料

- [Apple Developer: Mach-O Programming Topics](https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/MachOTopics/0-Introduction/introduction.html)
- [Apple Open Source: Mach-O Headers](https://opensource.apple.com/source/xnu/xnu-7195.81.3/EXTERNAL_HEADERS/mach-o/loader.h.auto.html)
- [OS X ABI Mach-O File Format Reference](https://github.com/aidansteele/osx-abi-macho-file-format-reference)

## 概述

Mach-O（Mach Object）是Apple操作系统（macOS、iOS、watchOS和tvOS）上使用的原生可执行文件格式。它源自NeXTSTEP操作系统，设计用于支持多种CPU架构，并提供高效的动态链接和加载。

### 重要概念

| 概念 | 描述 |
|------|------|
| 胖二进制(Fat Binary) | 包含多种CPU架构代码的单一文件，允许同一程序在不同架构上运行。 |
| 段(Segment) | Mach-O文件中的主要划分，包含具有相似属性的节。 |
| 节(Section) | 包含特定类型代码或数据的段的子划分。 |
| 符号(Symbol) | 代表函数、变量或其他程序实体的名称。 |
| 加载命令(Load Command) | 指导动态链接器如何设置和加载可执行文件的指令。 |
| 重定位(Relocation) | 修改代码或数据中的地址引用，使其指向正确的运行时位置。 |

## Mach-O文件整体结构

```
+---------------------------+
| Mach-O Header             |  <- 标识文件类型和目标架构
+---------------------------+
| Load Commands             |  <- 描述文件布局和链接特性
+---------------------------+
| Segments & Sections       |  <- 实际代码和数据
| ├─ __TEXT (代码)          |
| ├─ __DATA (数据)          |
| ├─ __LINKEDIT (链接信息)  |
| └─ 其他段...              |
+---------------------------+
```

对于胖二进制(Fat Binary)，结构稍有不同：

```
+---------------------------+
| Fat Header                |  <- 标识为胖二进制
+---------------------------+
| Fat Arch 1 (架构1描述)    |  <- 第一种架构的信息
+---------------------------+
| Fat Arch 2 (架构2描述)    |  <- 第二种架构的信息
+---------------------------+
| ...                       |
+---------------------------+
| Mach-O 1 (架构1的完整文件)|  <- 第一种架构的完整Mach-O文件
+---------------------------+
| Mach-O 2 (架构2的完整文件)|  <- 第二种架构的完整Mach-O文件
+---------------------------+
| ...                       |
+---------------------------+
```

## 1. Mach-O Header

**偏移**: 0x00, **大小**: 32位系统28字节, 64位系统32字节

```c
struct mach_header {
    uint32_t    magic;          // 0x00: 魔数 ⭐️
    cpu_type_t  cputype;        // 0x04: CPU类型 ⭐️
    cpu_subtype_t cpusubtype;   // 0x08: CPU子类型
    uint32_t    filetype;       // 0x0C: 文件类型 ⭐️
    uint32_t    ncmds;          // 0x10: 加载命令数量 ⭐️
    uint32_t    sizeofcmds;     // 0x14: 加载命令大小
    uint32_t    flags;          // 0x18: 标志
    uint32_t    reserved;       // 0x1C: 保留(仅64位)
};
```

64位版本称为`struct mach_header_64`，末尾增加了一个保留字段。

### magic 魔数值

- `MH_MAGIC (0xfeedface)`: 32位小端序
- `MH_CIGAM (0xcefaedfe)`: 32位大端序
- `MH_MAGIC_64 (0xfeedfacf)`: 64位小端序
- `MH_CIGAM_64 (0xcffaedfe)`: 64位大端序

### cputype CPU类型

- `CPU_TYPE_I386 (7)`: Intel x86
- `CPU_TYPE_X86_64 (16777223)`: Intel x86-64
- `CPU_TYPE_ARM (12)`: ARM
- `CPU_TYPE_ARM64 (16777228)`: ARM64

### filetype 文件类型

- `MH_OBJECT (1)`: 可重定位目标文件
- `MH_EXECUTE (2)`: 可执行文件
- `MH_DYLIB (6)`: 动态库
- `MH_DYLINKER (7)`: 动态链接器
- `MH_BUNDLE (8)`: 插件包
- `MH_DSYM (10)`: 调试符号文件

### flags 标志

- `MH_NOUNDEFS (0x1)`: 无未定义引用
- `MH_DYLDLINK (0x4)`: 由动态链接器加载
- `MH_PIE (0x200000)`: 位置无关可执行文件
- `MH_TWOLEVEL (0x80)`: 两级命名空间

## 2. Fat Header (胖二进制头)

用于表示包含多种架构的二进制文件：

```c
struct fat_header {
    uint32_t    magic;          // 0x00: 魔数 ⭐️
    uint32_t    nfat_arch;      // 0x04: 架构数量
};
```

### magic 魔数值

- `FAT_MAGIC (0xcafebabe)`: 小端序
- `FAT_CIGAM (0xbebafeca)`: 大端序

紧随其后的是每个架构的描述：

```c
struct fat_arch {
    cpu_type_t  cputype;        // CPU类型
    cpu_subtype_t cpusubtype;   // CPU子类型
    uint32_t    offset;         // 该架构Mach-O文件的偏移
    uint32_t    size;           // 该架构Mach-O文件的大小
    uint32_t    align;          // 对齐值(2的幂)
};
```

## 3. 加载命令 (Load Commands)

紧随Mach-O头之后，加载命令指示动态链接器如何处理文件：

```c
struct load_command {
    uint32_t    cmd;            // 0x00: 命令类型
    uint32_t    cmdsize;        // 0x04: 命令大小
    // 命令特定数据...
};
```

### 常见加载命令类型

- `LC_SEGMENT/LC_SEGMENT_64 (0x1/0x19)`: 定义内存段
- `LC_SYMTAB (0x2)`: 符号表信息
- `LC_DYSYMTAB (0xb)`: 动态符号表信息
- `LC_LOAD_DYLIB (0xc)`: 加载动态库
- `LC_ID_DYLIB (0xd)`: 动态库标识
- `LC_LOAD_DYLINKER (0xe)`: 加载动态链接器
- `LC_UUID (0x1b)`: 唯一标识符
- `LC_MAIN (0x28)`: 主程序入口点
- `LC_ENCRYPTION_INFO (0x21)`: 加密信息
- `LC_CODE_SIGNATURE (0x1d)`: 代码签名

## 4. 段命令 (Segment Command)

定义文件的内存段：

**32位结构**:
```c
struct segment_command {
    uint32_t    cmd;            // 0x00: LC_SEGMENT
    uint32_t    cmdsize;        // 0x04: 命令大小
    char        segname[16];    // 0x08: 段名称
    uint32_t    vmaddr;         // 0x18: 虚拟内存地址 ⭐️
    uint32_t    vmsize;         // 0x1C: 虚拟内存大小 ⭐️
    uint32_t    fileoff;        // 0x20: 文件偏移 ⭐️
    uint32_t    filesize;       // 0x24: 文件大小 ⭐️
    vm_prot_t   maxprot;        // 0x28: 最大保护
    vm_prot_t   initprot;       // 0x2C: 初始保护
    uint32_t    nsects;         // 0x30: 节数量
    uint32_t    flags;          // 0x34: 标志
};
```

**64位结构** (`struct segment_command_64`) 类似，但地址和大小字段为64位。

### 常见段名称

- `__TEXT`: 包含代码和只读数据
- `__DATA`: 包含可写数据
- `__LINKEDIT`: 包含链接器使用的数据
- `__PAGEZERO`: 空段，捕获空指针引用
- `__OBJC`: Objective-C运行时数据

### 保护标志

- `VM_PROT_READ (0x1)`: 可读
- `VM_PROT_WRITE (0x2)`: 可写
- `VM_PROT_EXECUTE (0x4)`: 可执行

## 5. 节 (Section)

每个段可以包含多个节，每个节包含特定类型的代码或数据：

**32位结构**:
```c
struct section {
    char        sectname[16];   // 0x00: 节名称
    char        segname[16];    // 0x10: 所属段名称
    uint32_t    addr;           // 0x20: 内存地址 ⭐️
    uint32_t    size;           // 0x24: 大小 ⭐️
    uint32_t    offset;         // 0x28: 文件偏移 ⭐️
    uint32_t    align;          // 0x2C: 对齐(2的幂)
    uint32_t    reloff;         // 0x30: 重定位条目偏移
    uint32_t    nreloc;         // 0x34: 重定位条目数量
    uint32_t    flags;          // 0x38: 标志
    uint32_t    reserved1;      // 0x3C: 保留
    uint32_t    reserved2;      // 0x40: 保留
};
```

**64位结构** (`struct section_64`) 类似，但地址和大小字段为64位。

### 常见节名称

**__TEXT段中的节**:
- `__text`: 可执行代码
- `__stubs`: 间接符号存根
- `__stub_helper`: 动态链接器帮助代码
- `__cstring`: 常量字符串
- `__const`: 常量数据

**__DATA段中的节**:
- `__data`: 初始化数据
- `__bss`: 未初始化数据
- `__la_symbol_ptr`: 懒绑定符号指针
- `__nl_symbol_ptr`: 非懒绑定符号指针
- `__got`: 全局偏移表

## 6. 符号表命令 (Symbol Table Command)

定义符号表位置和大小：

```c
struct symtab_command {
    uint32_t    cmd;            // 0x00: LC_SYMTAB
    uint32_t    cmdsize;        // 0x04: 命令大小
    uint32_t    symoff;         // 0x08: 符号表偏移 ⭐️
    uint32_t    nsyms;          // 0x0C: 符号数量 ⭐️
    uint32_t    stroff;         // 0x10: 字符串表偏移 ⭐️
    uint32_t    strsize;        // 0x14: 字符串表大小 ⭐️
};
```

## 7. 符号表条目

符号表中的每个条目描述一个符号：

**32位结构**:
```c
struct nlist {
    uint32_t    n_strx;         // 字符串表索引
    uint8_t     n_type;         // 类型标志
    uint8_t     n_sect;         // 节索引
    int16_t     n_desc;         // 描述字段
    uint32_t    n_value;        // 符号值
};
```

**64位结构** (`struct nlist_64`) 类似，但n_value为64位。

### n_type 类型标志

- `N_UNDF (0x0)`: 未定义
- `N_ABS (0x2)`: 绝对地址
- `N_SECT (0xe)`: 在某节中定义
- `N_PBUD (0xc)`: 预绑定
- `N_INDR (0xa)`: 间接

额外标志位：
- `N_STAB (0xe0)`: 调试符号
- `N_PEXT (0x10)`: 私有外部符号
- `N_EXT (0x01)`: 外部符号

## 8. 动态符号表命令 (Dynamic Symbol Table Command)

提供动态链接所需的额外符号信息：

```c
struct dysymtab_command {
    uint32_t    cmd;            // 0x00: LC_DYSYMTAB
    uint32_t    cmdsize;        // 0x04: 命令大小
    uint32_t    ilocalsym;      // 0x08: 本地符号索引
    uint32_t    nlocalsym;      // 0x0C: 本地符号数量
    uint32_t    iextdefsym;     // 0x10: 外部定义符号索引
    uint32_t    nextdefsym;     // 0x14: 外部定义符号数量
    uint32_t    iundefsym;      // 0x18: 未定义符号索引
    uint32_t    nundefsym;      // 0x1C: 未定义符号数量
    // 更多字段...
};
```

## 9. 动态链接器命令 (Dylinker Command)

指定动态链接器路径：

```c
struct dylinker_command {
    uint32_t    cmd;            // 0x00: LC_LOAD_DYLINKER
    uint32_t    cmdsize;        // 0x04: 命令大小
    union lc_str name;          // 0x08: 动态链接器名称
};
```

## 10. 动态库命令 (Dylib Command)

指定需要加载的动态库：

```c
struct dylib_command {
    uint32_t    cmd;            // 0x00: LC_LOAD_DYLIB等
    uint32_t    cmdsize;        // 0x04: 命令大小
    struct dylib dylib;         // 0x08: 动态库信息
};

struct dylib {
    union lc_str  name;         // 动态库路径
    uint32_t timestamp;         // 时间戳
    uint32_t current_version;   // 当前版本
    uint32_t compatibility_version; // 兼容版本
};
```

## 11. 主程序命令 (Main Command)

指定程序入口点（替代传统的LC_UNIXTHREAD）：

```c
struct entry_point_command {
    uint32_t    cmd;            // 0x00: LC_MAIN
    uint32_t    cmdsize;        // 0x04: 命令大小
    uint64_t    entryoff;       // 0x08: 入口点文件偏移 ⭐️
    uint64_t    stacksize;      // 0x10: 初始栈大小
};
```

## 12. 代码签名命令 (Code Signature Command)

指定代码签名数据位置：

```c
struct linkedit_data_command {
    uint32_t    cmd;            // 0x00: LC_CODE_SIGNATURE
    uint32_t    cmdsize;        // 0x04: 命令大小
    uint32_t    dataoff;        // 0x08: 数据偏移
    uint32_t    datasize;       // 0x0C: 数据大小
};
```

## 13. 地址计算

**文件偏移转虚拟地址**:
1. 找到包含该偏移的段
2. VA = vmaddr + (offset - fileoff)

**虚拟地址转文件偏移**:
1. 找到包含该VA的段
2. offset = fileoff + (VA - vmaddr)

## 14. 动态链接过程

1. 加载器读取Mach-O头和加载命令
2. 将段映射到内存
3. 加载指定的动态链接器
4. 动态链接器加载依赖的动态库
5. 执行符号解析和重定位
6. 跳转到程序入口点

## 15. 工具

- `otool`: 显示Mach-O文件的各部分
- `nm`: 列出符号
- `lipo`: 操作胖二进制文件
- `codesign`: 代码签名工具
- `dyld_info`: 显示动态链接信息
- `install_name_tool`: 修改动态库路径
- `vtool`: 处理代码签名和加密
- `dwarfdump`: 显示DWARF调试信息 