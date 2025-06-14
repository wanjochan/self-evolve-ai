#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TASM编译器 - 将TASM源码编译为本地可执行文件(.exe)
"""

import os
import sys
import struct
import platform
from typing import List, Dict, Tuple, Optional
from tasm_common import (
    SectionType, SectionFlags, Opcode, Section, BinaryFile,
    Label, Instruction, Register
)
from tasm_compile_to_program import compile_tasm_object
from dataclasses import dataclass

# PE文件格式常量
IMAGE_DOS_SIGNATURE = 0x5A4D      # MZ
IMAGE_NT_SIGNATURE = 0x00004550   # PE\0\0
IMAGE_FILE_MACHINE_AMD64 = 0x8664
IMAGE_FILE_EXECUTABLE_IMAGE = 0x0002
IMAGE_FILE_LARGE_ADDRESS_AWARE = 0x0020
IMAGE_SUBSYSTEM_WINDOWS_CUI = 0x0003
IMAGE_SCN_MEM_EXECUTE = 0x20000000
IMAGE_SCN_MEM_READ = 0x40000000
IMAGE_SCN_MEM_WRITE = 0x80000000

@dataclass
class PESection:
    """PE节信息简单封装，用于PEBuilder内部"""
    name: str
    data: bytes
    flags: int
    file_offset: int = 0
    virtual_address: int = 0

class PEBuilder:
    """PE文件构建器"""
    
    def __init__(self):
        self.sections: List[PESection] = []
        self.entry_point = 0
        self.image_base = 0x400000
        self.section_alignment = 0x1000
        self.file_alignment = 0x200
        self.size_of_headers = 0x400
        
    def add_section(self, name: str, data: bytes, flags: int):
        """添加节"""
        self.sections.append(PESection(
            name=name,
            data=data,
            flags=flags
        ))
        
    def build(self) -> bytes:
        """构建PE文件"""
        # 计算节的文件偏移和虚拟地址
        file_offset = self.size_of_headers
        virtual_address = self.section_alignment
        
        for section in self.sections:
            # 对齐到文件对齐边界
            file_offset = (file_offset + self.file_alignment - 1) & ~(self.file_alignment - 1)
            section.file_offset = file_offset
            file_offset += len(section.data)
            
            # 对齐到节对齐边界
            virtual_address = (virtual_address + self.section_alignment - 1) & ~(self.section_alignment - 1)
            section.virtual_address = virtual_address
            virtual_address += len(section.data)
            
        # 构建DOS头
        dos_header = bytearray([
            0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00,
            0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
            0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
        ])
        
        # DOS存根
        dos_stub = bytes([
            0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD,
            0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68,
            0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72,
            0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F,
            0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E,
            0x20, 0x69, 0x6E, 0x20, 0x44, 0x4F, 0x53, 0x20,
            0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A,
            0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        ])
        
        # PE头
        pe_header = struct.pack('<IHHIIIHHHHHHHHH',
            IMAGE_NT_SIGNATURE,        # Signature
            IMAGE_FILE_MACHINE_AMD64,  # Machine
            len(self.sections),        # NumberOfSections
            0,                         # TimeDateStamp
            0,                         # PointerToSymbolTable
            0,                         # NumberOfSymbols
            0xF0,                      # SizeOfOptionalHeader
            IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE,  # Characteristics
            0x20B,                     # Magic (PE32+)
            0,                         # MajorLinkerVersion
            0,                         # MinorLinkerVersion
            0,                         # SizeOfCode
            0,                         # SizeOfInitializedData
            0,                         # SizeOfUninitializedData
            self.entry_point           # AddressOfEntryPoint
        )
        
        # 可选头
        optional_header = struct.pack('<QIIIIHHHHHHHHHHQQQQII',
            self.image_base,           # ImageBase
            self.section_alignment,    # SectionAlignment
            self.file_alignment,       # FileAlignment
            6,                         # MajorOperatingSystemVersion
            0,                         # MinorOperatingSystemVersion
            0,                         # MajorImageVersion
            0,                         # MinorImageVersion
            6,                         # MajorSubsystemVersion
            0,                         # MinorSubsystemVersion
            0,                         # Win32VersionValue
            virtual_address,           # SizeOfImage
            self.size_of_headers,      # SizeOfHeaders
            0,                         # CheckSum
            IMAGE_SUBSYSTEM_WINDOWS_CUI,  # Subsystem
            0,                         # DllCharacteristics
            0x100000,                  # SizeOfStackReserve
            0x1000,                    # SizeOfStackCommit
            0x100000,                  # SizeOfHeapReserve
            0x1000,                    # SizeOfHeapCommit
            0,                         # LoaderFlags
            0x10                       # NumberOfRvaAndSizes
        )
        
        # 数据目录
        data_directory = bytes([0] * 128)
        
        # 节表
        section_table = bytearray()
        for section in self.sections:
            name = section.name.encode('ascii')[:8].ljust(8, b'\0')
            section_table.extend(struct.pack('<8sIIIIIIHHI',
                name,                      # Name
                len(section.data),         # VirtualSize
                section.virtual_address,   # VirtualAddress
                len(section.data),         # SizeOfRawData
                section.file_offset,       # PointerToRawData
                0,                         # PointerToRelocations
                0,                         # PointerToLinenumbers
                0,                         # NumberOfRelocations
                0,                         # NumberOfLinenumbers
                section.flags              # Characteristics
            ))
        
        # 构建完整的PE文件
        pe_file = bytearray()
        pe_file.extend(dos_header)
        pe_file.extend(dos_stub)
        pe_file.extend(pe_header)
        pe_file.extend(optional_header)
        pe_file.extend(data_directory)
        pe_file.extend(section_table)
        
        # 填充到节开始
        pe_file.extend(bytes([0] * (self.size_of_headers - len(pe_file))))
        
        # 添加节数据
        for section in self.sections:
            current_offset = len(pe_file)
            padding = section.file_offset - current_offset
            if padding > 0:
                pe_file.extend(bytes([0] * padding))
            pe_file.extend(section.data)
        
        return bytes(pe_file)

def compile_to_exe(source_file: str, output_file: str):
    """编译TASM源码为本地可执行文件
    
    Args:
        source_file: 源文件路径
        output_file: 输出文件路径
    """
    # 读取源文件
    with open(source_file, 'r', encoding='utf-8') as f:
        source = f.read()
    
    # 编译为 BinaryFile 对象
    binary = compile_tasm_object(source)
    
    # 创建PE构建器
    builder = PEBuilder()
    
    # 添加代码段
    code_section = binary.get_section(SectionType.CODE)
    if code_section:
        builder.add_section('.text', code_section.data,
            IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ)
    
    # 添加数据段
    data_section = binary.get_section(SectionType.DATA)
    if data_section:
        builder.add_section('.data', data_section.data,
            IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE)
    
    # 设置入口点
    builder.entry_point = binary.entry_point
    
    # 构建PE文件
    pe_data = builder.build()
    
    # 写入输出文件
    with open(output_file, 'wb') as f:
        f.write(pe_data)

def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: tasm_compile_to_exe.py <source_file> [output_file]")
        sys.exit(1)
        
    source_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else source_file + '.exe'
    
    try:
        compile_to_exe(source_file, output_file)
        print(f"编译成功: {output_file}")
    except Exception as e:
        print(f"编译错误: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main() 