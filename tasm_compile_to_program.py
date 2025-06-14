#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TASM编译器 - 将TASM源码编译为Program格式(TIR)
文件格式:
0-3:   魔数 (TASM)
4-5:   主版本号 (uint16)
6-7:   次版本号 (uint16)
8-11:  入口点 (uint32)
12-15: 段数量 (uint32)
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
    Label, Instruction, Register, CompileError, Lexer, Token, TokenType
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
    # 数据移动指令
    NOP = 0x00    # 空操作
    MOV = 0x01    # 移动数据
    LOAD = 0x02   # 从内存加载
    STORE = 0x03  # 存储到内存
    PUSH = 0x04   # 压栈
    POP = 0x05    # 出栈
    LEA = 0x06    # 加载有效地址
    
    # 算术指令
    ADD = 0x10    # 加法
    SUB = 0x11    # 减法
    MUL = 0x12    # 乘法
    DIV = 0x13    # 除法
    MOD = 0x14    # 取模
    NEG = 0x15    # 取反
    INC = 0x16    # 自增
    DEC = 0x17    # 自减
    
    # 位运算指令
    AND = 0x20    # 与
    OR = 0x21     # 或
    XOR = 0x22    # 异或
    NOT = 0x23    # 取反
    SHL = 0x24    # 左移
    SHR = 0x25    # 右移
    
    # 比较指令
    CMP = 0x30    # 比较
    TEST = 0x31   # 测试
    
    # 跳转指令
    JMP = 0x40    # 无条件跳转
    JE = 0x41     # 相等跳转
    JNE = 0x42    # 不等跳转
    JL = 0x43     # 小于跳转
    JLE = 0x44    # 小于等于跳转
    JG = 0x45     # 大于跳转
    JGE = 0x46    # 大于等于跳转
    JZ = 0x47     # 为零跳转
    JNZ = 0x48    # 不为零跳转
    
    # 函数调用指令
    CALL = 0x50   # 调用函数
    RET = 0x51    # 函数返回
    
    # 系统调用指令
    SYSCALL = 0x60  # 系统调用
    INT = 0x61      # 中断
    
    # 其他指令
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
        header.extend(self.VERSION[0].to_bytes(2, 'little'))  # Major version
        header.extend(self.VERSION[1].to_bytes(2, 'little'))  # Minor version
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

    def get_section(self, section_type: SectionType):
        """根据段类型获取第一个匹配的段，如果不存在返回 None"""
        for section in self.sections:
            if section.type == section_type:
                return section
        return None

class Instruction:
    """指令"""
    def __init__(self, opcode: Opcode, operands: List[Union[int, str]], size: int = 3):
        self.opcode = opcode
        self.operands = operands
        self.size = size
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
            1字节: 源寄存器/立即数
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
            data.extend(target.to_bytes(4, 'little'))  # 目标地址
            
        elif self.opcode in [Opcode.MOV, Opcode.ADD, Opcode.SUB, Opcode.MUL, Opcode.DIV,
                            Opcode.CMP, Opcode.TEST]:
            # 双操作数指令
            if len(self.operands) < 2:
                raise ValueError(f"双操作数指令需要两个操作数: {self.opcode.name}")
            # 目标寄存器
            if isinstance(self.operands[0], Register):
                data.append(self.operands[0].value)
            else:
                raise ValueError(f"目标操作数必须是寄存器: {self.operands[0]}")
            # 源操作数
            if isinstance(self.operands[1], Register):
                data.append(self.operands[1].value)
            elif isinstance(self.operands[1], int):
                if self.operands[1] > 255:
                    raise ValueError(f"立即数超出范围(0-255): {self.operands[1]}")
                data.append(self.operands[1])
            else:
                raise ValueError(f"无效的源操作数: {self.operands[1]}")
                
        elif self.opcode in [Opcode.PUSH, Opcode.POP, Opcode.INC, Opcode.DEC]:
            # 单操作数指令
            if not self.operands:
                raise ValueError(f"单操作数指令需要一个操作数: {self.opcode.name}")
            if isinstance(self.operands[0], Register):
                data.append(self.operands[0].value)
            else:
                raise ValueError(f"操作数必须是寄存器: {self.operands[0]}")
                
        elif self.opcode in [Opcode.NOP, Opcode.HLT]:
            # 无操作数指令
            pass
            
        elif self.opcode == Opcode.SYSCALL:
            # 系统调用指令：1字节操作码 + 1字节系统调用号
            if not self.operands:
                raise ValueError(f"SYSCALL指令需要系统调用号: {self.opcode.name}")
            syscall_num = self.operands[0]
            if isinstance(syscall_num, str):
                syscall_num = int(syscall_num)
            if not isinstance(syscall_num, int) or syscall_num > 255:
                raise ValueError(f"无效的系统调用号: {syscall_num}")
            data.append(syscall_num)
            
        else:
            raise ValueError(f"未知的指令: {self.opcode.name}")
            
        return bytes(data)

def parse_register(reg: str) -> Register:
    """解析寄存器
    
    Args:
        reg: 寄存器名称
        
    Returns:
        寄存器枚举值
        
    Raises:
        ValueError: 当寄存器名称无效时
    """
    reg = reg.upper()
    
    # 处理特殊寄存器
    if reg == 'SP':
        return Register.SP
    if reg == 'PC':
        return Register.PC
    # 兼容常见x86寄存器名称，全部映射为通用寄存器R0（占位实现）
    if reg in ('RAX', 'RBX', 'RCX', 'RDX', 'RSI', 'RDI', 'RSP', 'RBP',
               'R8', 'R9', 'R10', 'R11', 'R12', 'R13', 'R14', 'R15'):
        return Register.R0
        
    # 处理通用寄存器
    if reg.startswith('R'):
        try:
            reg_num = int(reg[1:])
            if 0 <= reg_num <= 7:
                return Register(reg_num)
        except ValueError:
            pass
            
    raise ValueError(f"无效的寄存器: {reg}")

def parse_number(value: str) -> int:
    """解析数字
    
    Args:
        value: 数字字符串 (支持十进制和十六进制)
        
    Returns:
        解析后的数字
    """
    try:
        if value.startswith('0x'):
            return int(value[2:], 16)
        return int(value)
    except ValueError:
        raise ValueError(f"无效的数字: {value}")

def parse_data_directive(line: str) -> Optional[bytes]:
    """解析数据指令
    
    Args:
        line: 指令行
        
    Returns:
        解析后的数据，如果不是数据指令则返回None
    """
    line = line.strip()
    if line.startswith('db '):
        # 解析字节数据
        data = []
        for item in line[3:].split(','):
            item = item.strip()
            if item.startswith('"') and item.endswith('"'):
                # 字符串
                data.extend(item[1:-1].encode('utf-8'))
            else:
                # 数字
                data.append(parse_number(item) & 0xFF)
        return bytes(data)
    elif line.startswith('times '):
        # 重复数据
        parts = line[6:].split(' ')
        if len(parts) != 2:
            raise ValueError(f"无效的times指令: {line}")
        count = parse_number(parts[0])
        value = parse_number(parts[1])
        return bytes([value & 0xFF] * count)
    return None

def parse_instruction(line: str) -> Optional[Union[Instruction, Label, str]]:
    """解析指令行
    
    Args:
        line: 指令行
        
    Returns:
        解析结果，可能是指令、标签或注释
    """
    # 去除注释（; 或 # 开头）
    comment_pos = None
    for marker in (';', '#'):
        pos = line.find(marker)
        if pos != -1:
            comment_pos = pos if comment_pos is None else min(comment_pos, pos)
    if comment_pos is not None:
        line = line[:comment_pos].strip()

    # 处理整行以 # 开头的注释
    if not line:
        return None

    # 支持 .section 指令前缀
    if line.startswith('.section '):
        line = line[1:]

    # 检查是否是段声明
    if line.startswith('section '):
        return line
        
    # 检查是否是标签
    parts = line.strip().split()
    if parts and parts[0].endswith(':'):
        label_name = parts[0][:-1].strip()
        if len(parts) > 1:
            # 如果标签后面还有内容，返回内容部分
            return ' '.join(parts[1:])
        return Label(name=label_name)
        
    # 解析指令
    if not parts:
        return None

    # 忽略汇编伪指令 global / extern
    if parts[0].lower() in ('global', 'extern'):
        return None

    # 获取操作码
    opcode_str = parts[0].upper()
    try:
        opcode = Opcode[opcode_str]
    except KeyError:
        raise ValueError(f"无效的操作码: {opcode_str}")
        
    # 解析操作数
    operands = []
    if len(parts) > 1:
        operand_strs = ' '.join(parts[1:]).split(',')
        for operand_str in operand_strs:
            operand_str = operand_str.strip()
            
            # 去除开头的点前缀(如 .test_label)
            if operand_str.startswith('.'):
                operand_str = operand_str[1:]

            # 去除数据大小前缀(byte/word/dword/qword)以及方括号
            for prefix in ('byte', 'word', 'dword', 'qword'):
                if operand_str.lower().startswith(prefix):
                    operand_str = operand_str[len(prefix):].strip()
            # 去掉方括号
            if operand_str.startswith('[') and operand_str.endswith(']'):
                operand_str = operand_str[1:-1].strip()
            
            # 如果包含 + 或 -，可能是 寄存器 + 偏移，提取寄存器部分
            if '+' in operand_str or '-' in operand_str:
                operand_base = operand_str.split('+')[0].split('-')[0].strip()
            else:
                operand_base = operand_str

            # 检查是否是寄存器
            if operand_base.startswith('r') or operand_base.upper() in ('SP', 'PC'):
                try:
                    reg = parse_register(operand_base)
                    operands.append(reg)
                except ValueError as e:
                    raise ValueError(f"无效的寄存器: {operand_base}")
            
            # 检查是否是标签或数字
            else:
                # 尝试解析为数字
                try:
                    value = parse_number(operand_str)
                    operands.append(value)
                except ValueError:
                    # 如果不是数字，则作为标签处理
                    if operand_str.isidentifier():  # 检查是否是有效的标识符
                        operands.append(operand_str)  # 标签名
                    else:
                        raise ValueError(f"无效的操作数: {operand_str}")
    
    # 根据指令类型设置指令大小
    size = 3  # 默认大小
    if opcode in [Opcode.JMP, Opcode.JE, Opcode.JNE, Opcode.JL, Opcode.JLE, 
                  Opcode.JG, Opcode.JGE, Opcode.JZ, Opcode.JNZ]:
        size = 5  # 跳转指令：1字节操作码 + 4字节目标地址
    elif opcode in [Opcode.MOV, Opcode.ADD, Opcode.SUB, Opcode.MUL, Opcode.DIV,
                    Opcode.CMP, Opcode.TEST]:
        size = 3  # 双操作数指令：1字节操作码 + 1字节目标寄存器 + 1字节源操作数
    elif opcode in [Opcode.PUSH, Opcode.POP, Opcode.INC, Opcode.DEC]:
        size = 2  # 单操作数指令：1字节操作码 + 1字节操作数
    elif opcode == Opcode.SYSCALL:
        size = 2  # 系统调用指令：1字节操作码 + 1字节系统调用号
    elif opcode in [Opcode.NOP, Opcode.HLT]:
        size = 1  # 无操作数指令：1字节操作码
                
    return Instruction(opcode=opcode, operands=operands, size=size)

def compile_tasm_object(source: str) -> BinaryFile:
    """将TASM源码编译为 BinaryFile 对象（包含段、入口点等元数据）。"""
    # 复制 compile_tasm 的实现，但返回 BinaryFile 对象而不是 bytes

    # 初始化
    binary = BinaryFile()
    current_section = '.code'  # 默认为代码段
    section_data = bytearray()
    labels: Dict[str, Label] = {}  # 标签字典
    instructions: List[Instruction] = []  # 指令列表
    current_offset = 0  # 当前偏移

    # 第一遍扫描：收集标签和指令
    for line in source.splitlines():
        # 解析指令
        result = parse_instruction(line)

        if result is None:
            continue

        if isinstance(result, str):
            # 段声明
            if result.startswith('section '):
                if current_section:
                    # 添加当前段
                    section_type = SectionType.CODE if current_section == '.code' else SectionType.DATA
                    flags = SectionFlags.READABLE
                    if section_type == SectionType.CODE:
                        flags |= SectionFlags.EXECUTABLE
                    binary.add_section(Section(type=section_type, data=bytes(section_data), flags=flags))
                    section_data = bytearray()
                    current_offset = 0

                current_section = result.split()[1]
                continue

            # 数据指令
            if current_section == '.data':
                data = parse_data_directive(result)
                if data:
                    section_data.extend(data)
                    current_offset += len(data)
            continue

        if isinstance(result, Label):
            # 记录标签位置
            labels[result.name] = Label(name=result.name, section=current_section, offset=current_offset)
            continue

        if isinstance(result, Instruction):
            # 记录指令位置
            result.offset = current_offset
            instructions.append(result)
            # 更新偏移量
            if result.opcode in [Opcode.JMP, Opcode.JE, Opcode.JNE, Opcode.JL, Opcode.JLE,
                                 Opcode.JG, Opcode.JGE, Opcode.JZ, Opcode.JNZ]:
                current_offset += 5  # 跳转指令：1字节操作码 + 4字节目标地址
            elif result.opcode in [Opcode.MOV, Opcode.ADD, Opcode.SUB, Opcode.MUL, Opcode.DIV,
                                   Opcode.CMP, Opcode.TEST]:
                current_offset += 3  # 双操作数指令
            elif result.opcode in [Opcode.PUSH, Opcode.POP, Opcode.INC, Opcode.DEC]:
                current_offset += 2  # 单操作数指令
            elif result.opcode == Opcode.SYSCALL:
                current_offset += 2  # 系统调用指令
            elif result.opcode in [Opcode.NOP, Opcode.HLT]:
                current_offset += 1
            else:
                current_offset += 3  # 默认大小

    # 第二遍扫描：生成代码
    section_data = bytearray()
    for inst in instructions:
        # 编码指令
        data = inst.encode(labels)
        section_data.extend(data)

    # 添加最后一个段
    if current_section:
        section_type = SectionType.CODE if current_section == '.code' else SectionType.DATA
        flags = SectionFlags.READABLE
        if section_type == SectionType.CODE:
            flags |= SectionFlags.EXECUTABLE
        binary.add_section(Section(type=section_type, data=bytes(section_data), flags=flags))

    # TODO: 设置正确入口点（目前设置为0）
    binary.entry_point = 0

    return binary

# 保持原接口：返回 bytes
def compile_tasm(source: str) -> bytes:
    binary = compile_tasm_object(source)
    return binary.to_bytes()

def compile_program(source_file: str, output_file: str):
    """编译TASM程序
    
    Args:
        source_file: 源文件路径
        output_file: 输出文件路径
        
    Raises:
        FileNotFoundError: 源文件不存在
        IOError: 写入输出文件失败
        ValueError: 编译错误
    """
    # 读取源文件
    if not os.path.exists(source_file):
        raise FileNotFoundError(f"源文件不存在: {source_file}")
        
    with open(source_file, 'r', encoding='utf-8') as f:
        source = f.read()
        
    # 编译
    binary = compile_tasm(source)
    
    # 写入输出文件
    with open(output_file, 'wb') as f:
        f.write(binary)

def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: tasm_compile_to_program.py <source_file> [output_file]")
        sys.exit(1)
        
    # 读取源文件
    with open(sys.argv[1], 'r', encoding='utf-8') as f:
        source = f.read()
        
    # 编译
    try:
        binary = compile_tasm(source)
    except Exception as e:
        print(f"编译错误: {e}")
        sys.exit(1)
        
    # 写入输出文件
    output_file = sys.argv[2] if len(sys.argv) > 2 else sys.argv[1] + '.tir'
    with open(output_file, 'wb') as f:
        f.write(binary)
        
    print(f"编译成功: {output_file}")

if __name__ == '__main__':
    main() 