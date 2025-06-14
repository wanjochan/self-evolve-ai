#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TASM编译器 - 将TASM源码编译为Runtime格式
文件格式:
0-3:   魔数 (TASM)
4-5:   版本号 (1.0)
6-7:   保留字段
8-11:  入口点
12-15: 段数量
16-31: 段表 (每个段16字节)
32+:   段数据
"""

import os
import sys
from enum import Enum, IntFlag
from dataclasses import dataclass
from typing import List, Optional, Dict, Union, Tuple
from tasm_common import (
    SectionType, SectionFlags, Opcode, Section, BinaryFile,
    Label, Instruction, Register
)

class SectionType(Enum):
    """段类型"""
    CODE = 1    # 代码段
    DATA = 2    # 数据段
    RELOC = 3   # 重定位段

class SectionFlags(IntFlag):
    """段标志"""
    NONE = 0
    READABLE = 1    # 可读
    WRITABLE = 2    # 可写
    EXECUTABLE = 4  # 可执行

@dataclass
class Label:
    """标签"""
    name: str
    section: str = ''
    offset: int = 0

@dataclass
class Section:
    """段"""
    type: SectionType
    data: bytes
    flags: SectionFlags
    
    @property
    def size(self) -> int:
        """段大小(字节)"""
        # 对齐到4字节边界
        return (len(self.data) + 3) & ~3

class Opcode(Enum):
    """操作码"""
    MOV = 0x01    # 数据移动
    ADD = 0x10    # 加法
    SUB = 0x11    # 减法
    MUL = 0x12    # 乘法
    DIV = 0x13    # 除法
    CMP = 0x30    # 比较
    JMP = 0x40    # 无条件跳转
    JE = 0x41     # 相等跳转
    JNE = 0x42    # 不等跳转
    SYSCALL = 0x60 # 系统调用
    READ_CODE = 0x70  # 读取代码段
    WRITE_CODE = 0x71 # 写入代码段
    VERIFY_CODE = 0x72 # 验证代码段
    BACKUP_CODE = 0x73 # 备份代码段
    RESTORE_CODE = 0x74 # 恢复代码段
    MUTATE_CODE = 0x75 # 变异代码段
    HLT = 0xFF    # 停机

@dataclass
class BinaryFile:
    """二进制文件"""
    MAGIC = b'TASM'
    VERSION = (1, 0)  # Major.Minor
    
    sections: List[Section]
    entry_point: int = 0
    
    def __init__(self):
        self.sections = []
    
    def add_section(self, section: Section):
        """添加段"""
        self.sections.append(section)
    
    def to_bytes(self) -> bytes:
        """生成二进制数据"""
        # 计算段表偏移
        header_size = 16  # 文件头大小
        section_table_size = 16 * len(self.sections)  # 段表大小
        data_offset = header_size + section_table_size
        
        # 生成文件头
        header = bytearray()
        header.extend(self.MAGIC)  # Magic
        header.extend(bytes([self.VERSION[0], self.VERSION[1], 0, 0]))  # Version + Reserved
        header.extend(self.entry_point.to_bytes(4, 'little'))  # Entry point
        header.extend(len(self.sections).to_bytes(4, 'little'))  # Number of sections
        
        # 生成段表
        section_table = bytearray()
        current_offset = data_offset
        
        for section in self.sections:
            # 段类型
            section_table.extend(section.type.value.to_bytes(4, 'little'))
            # 段偏移
            section_table.extend(current_offset.to_bytes(4, 'little'))
            # 段大小
            section_table.extend(section.size.to_bytes(4, 'little'))
            # 段标志
            section_table.extend(section.flags.value.to_bytes(4, 'little'))
            # 更新偏移
            current_offset += section.size
        
        # 生成段数据
        section_data = bytearray()
        for section in self.sections:
            # 添加段数据
            section_data.extend(section.data)
            # 添加对齐填充
            padding_size = section.size - len(section.data)
            section_data.extend(bytes(padding_size))
        
        # 组合所有部分
        return bytes(header + section_table + section_data)

class Instruction:
    """指令"""
    def __init__(self, opcode: Opcode, operands: List[Union[int, str, Register, Tuple[str, Union[int, Register, Tuple[Register, int], Tuple[Register, Register], Tuple[Register, Register, int]]]]], size: int = 3):
        self.opcode = opcode
        self.operands = operands
        # 根据指令类型设置大小
        if opcode in [Opcode.JMP, Opcode.JE, Opcode.JNE, Opcode.JL, Opcode.JLE, 
                     Opcode.JG, Opcode.JGE, Opcode.JZ, Opcode.JNZ]:
            # 跳转指令：1字节操作码 + 4字节目标地址
            self.size = 5
        elif opcode == Opcode.SYSCALL:
            # 系统调用指令：1字节操作码 + 1字节系统调用号
            self.size = 2
        elif opcode in [Opcode.NOP, Opcode.HLT]:
            # 无操作数指令：1字节操作码
            self.size = 1
        else:
            # 默认大小：1字节操作码 + 1字节目标寄存器 + 1字节源操作数
            self.size = 3
        self.offset = 0
    
    def encode(self, labels: Dict[str, Label] = None) -> bytes:
        """编码为字节序列
        指令格式:
        跳转指令:
            1字节: 操作码
            4字节: 目标地址
        双操作数指令:
            1字节: 操作码
            1字节: 目标寄存器
            1字节: 源操作数
        单操作数指令:
            1字节: 操作码
            1字节: 操作数
        无操作数指令:
            1字节: 操作码
        """
        data = bytearray()
        data.append(self.opcode.value)  # 操作码
        
        # 根据指令类型添加操作数
        if self.opcode in [Opcode.JMP, Opcode.JE, Opcode.JNE, Opcode.JL, Opcode.JLE, 
                          Opcode.JG, Opcode.JGE, Opcode.JZ, Opcode.JNZ]:
            # 跳转指令
            if not self.operands:
                raise ValueError(f"跳转指令需要目标地址: {self.opcode.name}")
            target = self.operands[0]
            if isinstance(target, str):  # 标签
                if labels is None:
                    raise ValueError("未提供标签字典")
                if target not in labels:
                    raise ValueError(f"未定义的标签: {target}")
                target = labels[target].offset
            elif isinstance(target, Register):  # 寄存器
                target = int(target)
            data.extend(target.to_bytes(4, 'little'))  # 目标地址
            
        elif self.opcode == Opcode.SYSCALL:
            # 系统调用指令
            if not self.operands:
                raise ValueError(f"系统调用指令需要系统调用号: {self.opcode.name}")
            syscall_num = self.operands[0]
            if isinstance(syscall_num, int):
                data.append(syscall_num)  # 系统调用号
            else:
                raise ValueError(f"系统调用号必须是整数: {syscall_num}")
            
        elif self.opcode in [Opcode.MOV, Opcode.ADD, Opcode.SUB, Opcode.MUL, Opcode.DIV,
                            Opcode.CMP, Opcode.TEST]:
            # 双操作数指令
            if len(self.operands) < 2:
                raise ValueError(f"双操作数指令需要两个操作数: {self.opcode.name}")
            
            # 目标操作数
            dst = self.operands[0]
            if isinstance(dst, Register):
                data.append(int(dst))  # 寄存器
            elif isinstance(dst, tuple) and dst[0] == 'mem':
                data.append(0xFF)  # 内存标志
                if isinstance(dst[1], int):
                    data.append(dst[1] & 0xFF)  # 内存地址
                else:
                    data.append(int(dst[1]) & 0xFF)  # 寄存器地址
            else:
                raise ValueError(f"无效的目标操作数: {dst}")
            
            # 源操作数
            src = self.operands[1]
            if isinstance(src, Register):
                data.append(int(src))  # 寄存器
            elif isinstance(src, tuple) and src[0] == 'mem':
                data.append(0xFF)  # 内存标志
                if isinstance(src[1], int):
                    data.append(src[1] & 0xFF)  # 内存地址
                else:
                    data.append(int(src[1]) & 0xFF)  # 寄存器地址
            elif isinstance(src, int):
                data.append(src & 0xFF)  # 立即数
            else:
                raise ValueError(f"无效的源操作数: {src}")
            
        elif self.opcode in [Opcode.PUSH, Opcode.POP, Opcode.INC, Opcode.DEC]:
            # 单操作数指令
            if not self.operands:
                raise ValueError(f"单操作数指令需要一个操作数: {self.opcode.name}")
            operand = self.operands[0]
            if isinstance(operand, Register):
                data.append(int(operand))  # 寄存器
            elif isinstance(operand, int):
                data.append(operand & 0xFF)  # 立即数
            else:
                raise ValueError(f"无效的操作数: {operand}")
            
        return bytes(data)

def parse_register(reg: str) -> Register:
    """解析寄存器
    
    Args:
        reg: 寄存器名称或索引
        
    Returns:
        寄存器枚举值
        
    Raises:
        ValueError: 当寄存器名称无效时
    """
    try:
        # 尝试直接解析为数字
        index = int(reg)
        if 0 <= index <= 8:
            return Register(index)
        raise ValueError(f"寄存器索引超出范围(0-8): {index}")
    except ValueError:
        # 尝试解析寄存器名称
        reg = reg.lower()
        if not reg.startswith('r'):
            raise ValueError(f"无效的寄存器名称: {reg}")
        try:
            index = int(reg[1:])
            if 0 <= index <= 8:
                return Register(index)
            raise ValueError(f"寄存器索引超出范围(0-8): {index}")
        except (ValueError, IndexError):
            raise ValueError(f"无效的寄存器名称: {reg}")

def parse_number(value: str) -> int:
    """解析数字
    
    Args:
        value: 数字字符串
        
    Returns:
        解析后的数字
        
    Raises:
        ValueError: 当数字格式无效时
    """
    value = value.lower()
    if value.startswith('0x'):
        return int(value[2:], 16)
    elif value.startswith('0b'):
        return int(value[2:], 2)
    else:
        return int(value)

def parse_data_directive(line: str) -> Optional[bytes]:
    """解析数据定义指令
    
    支持的格式:
    db "string"  - 字符串(自动添加结尾的0)
    db 1,2,3     - 字节序列
    dw 1,2,3     - 字序列(2字节)
    dd 1,2,3     - 双字序列(4字节)
    dq 1,2,3     - 四字序列(8字节)
    """
    line = line.strip()
    if not line:
        return None
        
    # 检查是否是数据定义指令
    parts = line.split(None, 1)
    if not parts or len(parts) < 2:
        return None
        
    directive = parts[0].lower()
    if directive not in ('db', 'dw', 'dd', 'dq'):
        return None
        
    data = parts[1].strip()
    result = bytearray()
    
    # 处理字符串
    if data.startswith('"') and data.endswith('"'):
        string = data[1:-1]  # 去掉引号
        result.extend(string.encode('utf-8'))
        result.append(0)  # 添加结尾的0
        
        # 添加对齐填充
        if directive != 'db':  # 如果不是db，需要对齐
            align = {'dw': 2, 'dd': 4, 'dq': 8}[directive]
            padding = (align - (len(result) % align)) % align
            result.extend(bytes(padding))
            
        return bytes(result)
    
    # 处理数字序列
    try:
        numbers = [int(x.strip()) for x in data.split(',')]
        size = {'db': 1, 'dw': 2, 'dd': 4, 'dq': 8}[directive]
        
        for num in numbers:
            result.extend(num.to_bytes(size, 'little', signed=(num < 0)))
            
        return bytes(result)
    except ValueError as e:
        raise ValueError(f"无效的数据值: {data}") from e

def parse_memory_operand(operand: str) -> Tuple[str, Union[int, Register]]:
    """解析内存操作数
    格式:
    [立即数]
    [寄存器]
    """
    # 去除方括号
    operand = operand.strip('[]')
    
    # 尝试解析为立即数
    try:
        value = parse_number(operand)
        return ('mem', value)
    except ValueError:
        pass
    
    # 尝试解析为寄存器
    try:
        reg = parse_register(operand)
        return ('mem', reg)
    except ValueError:
        pass
    
    raise ValueError(f'无效的内存操作数: {operand}')

def parse_instruction(line: str) -> Optional[Union[str, Label, Instruction]]:
    """解析指令"""
    # 移除注释
    line = line.split(';', 1)[0].strip()
    if not line:
        return None
        
    # 段声明
    if line.startswith('section'):
        return line
        
    # 标签定义
    if line.endswith(':'):
        name = line[:-1].strip()
        return Label(name)
        
    # 指令
    parts = line.split()
    if not parts:
        return None
        
    op = parts[0].lower()
    args = []
    
    # 解析参数
    if len(parts) > 1:
        args = [arg.strip(',') for arg in parts[1:]]
    
    # 匹配指令
    if op == 'mov':
        return Instruction(Opcode.MOV, args)
    elif op == 'add':
        return Instruction(Opcode.ADD, args)
    elif op == 'sub':
        return Instruction(Opcode.SUB, args)
    elif op == 'mul':
        return Instruction(Opcode.MUL, args)
    elif op == 'div':
        return Instruction(Opcode.DIV, args)
    elif op == 'cmp':
        return Instruction(Opcode.CMP, args)
    elif op == 'jmp':
        return Instruction(Opcode.JMP, args)
    elif op == 'je':
        return Instruction(Opcode.JE, args)
    elif op == 'jne':
        return Instruction(Opcode.JNE, args)
    elif op == 'syscall':
        return Instruction(Opcode.SYSCALL, args)
    elif op == 'read_code':
        return Instruction(Opcode.READ_CODE, args)
    elif op == 'write_code':
        return Instruction(Opcode.WRITE_CODE, args)
    elif op == 'verify_code':
        return Instruction(Opcode.VERIFY_CODE, args)
    elif op == 'backup_code':
        return Instruction(Opcode.BACKUP_CODE, args)
    elif op == 'restore_code':
        return Instruction(Opcode.RESTORE_CODE, args)
    elif op == 'mutate_code':
        return Instruction(Opcode.MUTATE_CODE, args)
    elif op == 'hlt':
        return Instruction(Opcode.HLT, args)
    
    return None

def compile_runtime(source_file: str, output_file: str):
    """编译TASM源码为Runtime格式"""
    # 读取源文件
    print(f"读取源文件: {source_file}")
    with open(source_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()
        
    # 第一遍扫描：收集标签
    print("第一遍扫描：收集标签")
    labels = {}
    current_section = None
    current_offset = 0
    
    for line in lines:
        result = parse_instruction(line)
        if result is None:
            continue
            
        if isinstance(result, str):
            # 段声明
            if result.startswith('section'):
                current_section = result.split(None, 1)[1].strip()
                print(f"进入段: {current_section}")
                current_offset = 0
            continue
            
        if isinstance(result, Label):
            # 标签定义
            result.section = current_section
            result.offset = current_offset
            labels[result.name] = result
            print(f"定义标签: {result.name} -> {result.offset}")
            continue
            
        # 指令
        current_offset += result.size
        
    # 第二遍扫描：生成代码
    print("\n第二遍扫描：生成代码")
    binary = BinaryFile()
    current_section = None
    current_data = bytearray()
    
    for line in lines:
        result = parse_instruction(line)
        if result is None:
            continue
            
        if isinstance(result, str):
            # 段声明
            if result.startswith('section'):
                # 保存当前段
                if current_section is not None and current_data:
                    section_type = SectionType[current_section]
                    flags = SectionFlags.READABLE
                    if section_type == SectionType.CODE:
                        flags |= SectionFlags.EXECUTABLE
                    elif section_type == SectionType.DATA:
                        flags |= SectionFlags.WRITABLE
                    binary.add_section(Section(section_type, bytes(current_data), flags))
                    print(f"添加段: {current_section} ({len(current_data)} 字节)")
                
                # 开始新段
                current_section = result.split(None, 1)[1].strip()
                current_data = bytearray()
                print(f"进入段: {current_section}")
            continue
            
        if isinstance(result, Label):
            # 标签已在第一遍处理
            continue
            
        if isinstance(result, str) and result.startswith('db'):
            # 数据定义
            data = parse_data_directive(result)
            if data:
                current_data.extend(data)
                print(f"添加数据: {len(data)} 字节")
            continue
            
        # 指令
        try:
            encoded = result.encode(labels)
            current_data.extend(encoded)
            print(f"编码指令: {result.opcode.name} -> {' '.join(f'{b:02x}' for b in encoded)}")
        except Exception as e:
            print(f"编码错误: {e}")
            raise
            
    # 保存最后一个段
    if current_section is not None and current_data:
        section_type = SectionType[current_section]
        flags = SectionFlags.READABLE
        if section_type == SectionType.CODE:
            flags |= SectionFlags.EXECUTABLE
        elif section_type == SectionType.DATA:
            flags |= SectionFlags.WRITABLE
        binary.add_section(Section(section_type, bytes(current_data), flags))
        print(f"添加段: {current_section} ({len(current_data)} 字节)")
    
    # 写入输出文件
    print(f"\n写入输出文件: {output_file}")
    with open(output_file, 'wb') as f:
        f.write(binary.to_bytes())
    print("编译完成")

def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: tasm_compile_to_runtime.py <source_file> [output_file]")
        sys.exit(1)
        
    # 编译
    try:
        compile_runtime(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else sys.argv[1] + '.bin')
    except Exception as e:
        print(f"编译错误: {e}")
        sys.exit(1)
        
    print(f"编译成功: {sys.argv[2] if len(sys.argv) > 2 else sys.argv[1] + '.bin'}")

if __name__ == '__main__':
    main() 