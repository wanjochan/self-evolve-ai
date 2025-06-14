# PE文件格式规格说明（Portable Executable）

https://learn.microsoft.com/en-us/windows/win32/debug/pe-format

## 概述

PE (Portable Executable) 是Windows平台的可执行文件格式，用于.exe、.dll、.sys等文件类型。"Portable Executable"名称表明该格式不是特定于任何CPU架构的。

### 重要概念

| 概念 | 描述 |
|------|------|
| 属性证书 | 用于将可验证声明与镜像关联的证书。最常用的是软件制造商的签名，用于验证文件的完整性。 |
| 日期/时间戳 | PE文件中用于不同目的的时间标记。通常使用C运行时库的时间函数格式。如果值为0或0xFFFFFFFF，则不代表实际时间。 |
| 文件指针 | 在链接器（对于对象文件）或加载器（对于镜像文件）处理之前，项目在文件本身中的位置。即磁盘上存储的文件中的位置。 |
| 相对虚拟地址(RVA) | 在镜像文件中，这是项目加载到内存后的地址减去镜像文件的基地址。RVA几乎总是与其在磁盘上的文件位置不同。 |
| 节(Section) | PE或COFF文件中的基本代码或数据单位。例如，所有代码可以组合在一个节中，或每个函数可以占用自己的节。 |
| 虚拟地址(VA) | 与RVA相同，但不减去镜像文件的基地址。由于Windows为每个进程创建独立的VA空间，VA应被视为普通地址。 |

## PE文件整体结构

```
+---------------------------+
| DOS Header (64 bytes)     |  <- 以"MZ"开头
+---------------------------+
| DOS Stub Program          |  <- 显示"This program cannot be run in DOS mode"
+---------------------------+
| PE Header                 |  <- 以"PE\0\0"开头
| ├─ PE Signature (4 bytes) |
| ├─ COFF Header (20 bytes) |
| └─ Optional Header        |  <- PE32: 224字节, PE32+: 240字节
+---------------------------+
| Section Headers           |  <- 每个节40字节
+---------------------------+
| Section Data              |  <- 实际代码和数据
| ├─ .text (代码)           |
| ├─ .data (数据)           |
| ├─ .rdata (只读数据)      |
| └─ .rsrc (资源)           |
+---------------------------+
```

## 1. DOS Header (IMAGE_DOS_HEADER)

**偏移**: 0x00, **大小**: 64字节

```c
typedef struct _IMAGE_DOS_HEADER {
    WORD   e_magic;          // 0x00: "MZ" (0x5A4D)
    WORD   e_cblp;           // 0x02: 最后页的字节数
    WORD   e_cp;             // 0x04: 页数
    WORD   e_crlc;           // 0x06: 重定位数
    WORD   e_cparhdr;        // 0x08: 头部段落数
    WORD   e_minalloc;       // 0x0A: 最小额外段落
    WORD   e_maxalloc;       // 0x0C: 最大额外段落
    WORD   e_ss;             // 0x0E: 初始SS值
    WORD   e_sp;             // 0x10: 初始SP值
    WORD   e_csum;           // 0x12: 校验和
    WORD   e_ip;             // 0x14: 初始IP值
    WORD   e_cs;             // 0x16: 初始CS值
    WORD   e_lfarlc;         // 0x18: 重定位表地址
    WORD   e_ovno;           // 0x1A: 覆盖号
    WORD   e_res[4];         // 0x1C: 保留字段
    WORD   e_oemid;          // 0x24: OEM标识
    WORD   e_oeminfo;        // 0x26: OEM信息
    WORD   e_res2[10];       // 0x28: 保留字段
    LONG   e_lfanew;         // 0x3C: PE头偏移 ⭐️ 关键字段
} IMAGE_DOS_HEADER;
```

**关键字段**:
- `e_magic`: 必须是"MZ" (0x5A4D)
- `e_lfanew`: 指向PE头的文件偏移

## 2. PE Signature

**偏移**: DOS Header的e_lfanew指定的位置  
**大小**: 4字节  
**内容**: "PE\0\0" (0x50450000)

## 3. COFF Header (IMAGE_FILE_HEADER)

**偏移**: PE Signature之后, **大小**: 20字节

```c
typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;              // 0x00: 机器类型
    WORD    NumberOfSections;     // 0x02: 节数量 ⭐️
    DWORD   TimeDateStamp;        // 0x04: 时间戳
    DWORD   PointerToSymbolTable; // 0x08: 符号表指针(通常为0)
    DWORD   NumberOfSymbols;      // 0x0C: 符号数(通常为0)
    WORD    SizeOfOptionalHeader; // 0x10: 可选头大小 ⭐️
    WORD    Characteristics;      // 0x12: 文件特征 ⭐️
} IMAGE_FILE_HEADER;
```

**Machine值**:
- `0x014c`: i386 (x86)
- `0x8664`: AMD64 (x86-64)
- `0x01c0`: ARM
- `0xaa64`: ARM64

**Characteristics标志**:
- `0x0002`: IMAGE_FILE_EXECUTABLE_IMAGE - 可执行文件
- `0x0100`: IMAGE_FILE_32BIT_MACHINE - 32位机器
- `0x2000`: IMAGE_FILE_DLL - 动态链接库

## 4. Optional Header (IMAGE_OPTIONAL_HEADER)

### 4.1 标准字段

```c
typedef struct _IMAGE_OPTIONAL_HEADER {
    WORD    Magic;                    // 0x00: 魔数 ⭐️
    BYTE    MajorLinkerVersion;       // 0x02: 链接器主版本
    BYTE    MinorLinkerVersion;       // 0x03: 链接器副版本
    DWORD   SizeOfCode;               // 0x04: 代码段大小
    DWORD   SizeOfInitializedData;    // 0x08: 初始化数据大小
    DWORD   SizeOfUninitializedData;  // 0x0C: 未初始化数据大小
    DWORD   AddressOfEntryPoint;      // 0x10: 入口点地址 ⭐️
    DWORD   BaseOfCode;               // 0x14: 代码基址
    // PE32才有下面这个字段，PE32+没有
    DWORD   BaseOfData;               // 0x18: 数据基址(仅PE32)
    
    // Windows特定字段
    ULONGLONG ImageBase;              // 0x1C/0x18: 镜像基址 ⭐️
    DWORD   SectionAlignment;         // 0x24/0x20: 内存对齐 ⭐️
    DWORD   FileAlignment;            // 0x28/0x24: 文件对齐 ⭐️
    WORD    MajorOperatingSystemVersion; // 0x2C/0x28: OS主版本
    WORD    MinorOperatingSystemVersion; // 0x2E/0x2A: OS副版本
    WORD    MajorImageVersion;        // 0x30/0x2C: 镜像主版本
    WORD    MinorImageVersion;        // 0x32/0x2E: 镜像副版本
    WORD    MajorSubsystemVersion;    // 0x34/0x30: 子系统主版本
    WORD    MinorSubsystemVersion;    // 0x36/0x32: 子系统副版本
    DWORD   Win32VersionValue;        // 0x38/0x34: Win32版本(保留)
    DWORD   SizeOfImage;              // 0x3C/0x38: 镜像大小 ⭐️
    DWORD   SizeOfHeaders;            // 0x40/0x3C: 头部大小 ⭐️
    DWORD   CheckSum;                 // 0x44/0x40: 校验和
    WORD    Subsystem;                // 0x48/0x44: 子系统 ⭐️
    WORD    DllCharacteristics;       // 0x4A/0x46: DLL特征
    ULONGLONG SizeOfStackReserve;     // 0x4C/0x48: 栈保留大小
    ULONGLONG SizeOfStackCommit;      // 0x54/0x50: 栈提交大小
    ULONGLONG SizeOfHeapReserve;      // 0x5C/0x58: 堆保留大小
    ULONGLONG SizeOfHeapCommit;       // 0x64/0x60: 堆提交大小
    DWORD   LoaderFlags;              // 0x6C/0x68: 加载器标志(保留)
    DWORD   NumberOfRvaAndSizes;      // 0x70/0x6C: 数据目录数量 ⭐️
    // 数据目录紧随其后
} IMAGE_OPTIONAL_HEADER;
```

**Magic值**:
- `0x10b`: PE32 (32位)
- `0x20b`: PE32+ (64位)

**Subsystem值**:
- `1`: NATIVE - 本机驱动
- `2`: WINDOWS_GUI - 图形界面应用
- `3`: WINDOWS_CUI - 控制台应用

### 4.2 数据目录 (Data Directories)

每个条目8字节，包含RVA和大小：

```c
typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;    // RVA地址
    DWORD   Size;              // 大小
} IMAGE_DATA_DIRECTORY;
```

**标准数据目录索引**:
```
0  - Export Table          导出表
1  - Import Table          导入表 ⭐️
2  - Resource Table        资源表
3  - Exception Table       异常表
4  - Certificate Table     证书表
5  - Base Relocation Table 重定位表
6  - Debug                 调试信息
7  - Architecture          架构(保留)
8  - Global Ptr            全局指针
9  - TLS Table             线程本地存储
10 - Load Config Table     加载配置表
11 - Bound Import          绑定导入
12 - IAT                   导入地址表 ⭐️
13 - Delay Import Descriptor 延迟导入
14 - CLR Runtime Header    .NET运行时头
15 - Reserved              保留
```

## 5. Section Headers

每个节头40字节：

```c
typedef struct _IMAGE_SECTION_HEADER {
    BYTE    Name[8];               // 0x00: 节名称
    DWORD   VirtualSize;           // 0x08: 内存中大小 ⭐️
    DWORD   VirtualAddress;        // 0x0C: 内存中地址(RVA) ⭐️
    DWORD   SizeOfRawData;         // 0x10: 文件中大小 ⭐️
    DWORD   PointerToRawData;      // 0x14: 文件中偏移 ⭐️
    DWORD   PointerToRelocations;  // 0x18: 重定位偏移
    DWORD   PointerToLinenumbers;  // 0x1C: 行号偏移
    WORD    NumberOfRelocations;   // 0x20: 重定位数量
    WORD    NumberOfLinenumbers;   // 0x22: 行号数量
    DWORD   Characteristics;       // 0x24: 节特征 ⭐️
} IMAGE_SECTION_HEADER;
```

**常见节名**:
- `.text`: 代码段
- `.data`: 初始化数据
- `.rdata`: 只读数据
- `.bss`: 未初始化数据
- `.rsrc`: 资源
- `.reloc`: 重定位信息

**Characteristics标志**:
- `0x00000020`: IMAGE_SCN_CNT_CODE - 包含代码
- `0x00000040`: IMAGE_SCN_CNT_INITIALIZED_DATA - 包含初始化数据
- `0x00000080`: IMAGE_SCN_CNT_UNINITIALIZED_DATA - 包含未初始化数据
- `0x20000000`: IMAGE_SCN_MEM_EXECUTE - 可执行
- `0x40000000`: IMAGE_SCN_MEM_READ - 可读
- `0x80000000`: IMAGE_SCN_MEM_WRITE - 可写

## 6. 地址计算

**重要概念**:
- **RVA (Relative Virtual Address)**: 相对于ImageBase的虚拟地址
- **VA (Virtual Address)**: 绝对虚拟地址 = ImageBase + RVA
- **File Offset**: 文件中的物理偏移

**RVA转文件偏移算法**:
```
1. 找到包含该RVA的节
2. section_offset = RVA - section.VirtualAddress
3. file_offset = section.PointerToRawData + section_offset
```

## 7. 导入表结构

导入表用于加载外部DLL和函数：

```c
typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    DWORD   OriginalFirstThunk;    // 导入名称表RVA
    DWORD   TimeDateStamp;         // 时间戳
    DWORD   ForwarderChain;        // 转发器链
    DWORD   Name;                  // DLL名称RVA ⭐️
    DWORD   FirstThunk;            // 导入地址表RVA ⭐️
} IMAGE_IMPORT_DESCRIPTOR;
```

## 8. 最小可用PE文件示例

一个能运行的最小PE文件包含：
1. DOS Header (64字节)
2. PE Signature (4字节)
3. COFF Header (20字节)
4. Optional Header (224/240字节)
5. 至少一个Section Header (40字节)
6. Section Data (代码/数据)

## 9. 常见错误和注意事项

1. **对齐要求**: 
   - 文件中数据必须按FileAlignment对齐
   - 内存中数据必须按SectionAlignment对齐

2. **必要字段验证**:
   - DOS Header的e_magic必须是"MZ"
   - PE Signature必须是"PE\0\0"
   - Optional Header的Magic必须是0x10b或0x20b

3. **地址计算**:
   - RVA不能超过SizeOfImage
   - 文件偏移不能超过文件大小

## 10. Authenticode PE镜像哈希

Authenticode签名用于验证PE镜像文件的相关部分未被修改。为实现这一目标，Authenticode签名包含PE镜像哈希。

### 10.1 什么是Authenticode PE镜像哈希？

Authenticode PE镜像哈希（简称文件哈希）类似于文件校验和，它产生一个与文件完整性相关的小值。与校验和不同的是：
- 校验和由简单算法生成，主要用于检测内存故障
- 文件哈希不仅可以检测文件损坏，而且很难修改文件使其具有与原始文件相同的哈希值
- 文件哈希可以检测病毒、黑客或特洛伊木马程序引入的有意和微妙的修改

在Authenticode签名中，文件哈希使用只有文件签名者知道的私钥进行数字签名。软件使用者可以通过以下步骤验证文件完整性：
1. 计算文件的哈希值
2. 将其与Authenticode数字签名中包含的签名哈希值进行比较
3. 如果文件哈希不匹配，说明PE镜像哈希覆盖的部分已被修改

### 10.2 PE镜像哈希计算范围

不是所有镜像文件数据都包含在PE镜像哈希计算中。以下区域被排除：

1. **文件校验和字段**
   - 位于可选头的Windows特定字段中
   - 此校验和包括整个文件（包括文件中的任何属性证书）
   - 插入Authenticode签名后，校验和很可能与原始值不同

2. **属性证书相关信息**
   - 可选头数据目录中的证书表字段
   - 证书表字段指向的证书表和相应证书
   - 这些区域的排除允许重新签名或添加时间戳

3. **最后一个节之后的信息**
   - 通常包含调试信息
   - 调试信息可以视为调试器的建议性信息
   - 不影响可执行程序的实际完整性
   - 可以在产品发布后移除以节省磁盘空间

### 10.3 PE镜像哈希计算过程

1. 按地址范围对节表中指定的节进行排序
2. 对结果字节序列进行哈希处理，跳过排除范围
3. 节内的调试信息不能在不使Authenticode签名无效的情况下移除

## 11. 参考资料

本文档基于以下权威资料编写：

### 官方文档
- **Microsoft PE Format Specification**  
  https://learn.microsoft.com/en-us/windows/win32/debug/pe-format

- **Windows Authenticode PE Signature Format**  
  https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#appendix-a-calculating-authenticode-pe-image-hash

### 技术书籍
- **Windows Internals, 7th Edition** by Pavel Yosifovich, Mark Russinovich, David Solomon, Alex Ionescu
- **Practical Malware Analysis** by Michael Sikorski and Andrew Honig

### 工具和调试器
- **PE-bear** - PE文件分析工具  
  https://github.com/hasherezade/pe-bear-releases

- **CFF Explorer** - PE编辑器  
  https://ntcore.com/?page_id=388

- **Dependency Walker** - DLL依赖分析  
  http://www.dependencywalker.com/

---