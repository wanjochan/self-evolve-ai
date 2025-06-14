#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TASM编译器 - 将TASM源码编译为IR
"""

import os
import sys
from enum import Enum
from dataclasses import dataclass
from typing import List, Dict, Set, Optional, Union, Tuple
from tasm_common import (
    SectionType, SectionFlags, Opcode, Section, BinaryFile,
    Label, Instruction, Register
)

class IRType(Enum):
    """IR类型"""
    VOID = 0    # 空类型
    I8 = 1      # 8位整数
    I16 = 2     # 16位整数
    I32 = 3     # 32位整数
    I64 = 4     # 64位整数
    PTR = 5     # 指针
    ARRAY = 6   # 数组

@dataclass
class IRValue:
    """IR值"""
    type: IRType
    name: str = ''
    value: Union[int, str, bytes, None] = None
    size: Optional[int] = None  # 数组大小

@dataclass
class IRInstruction:
    """IR指令"""
    opcode: str
    dest: Optional[IRValue] = None
    src1: Optional[IRValue] = None
    src2: Optional[IRValue] = None

@dataclass
class IRBasicBlock:
    """IR基本块"""
    name: str
    instructions: List[IRInstruction]
    predecessors: Set[str]
    successors: Set[str]

@dataclass
class IRFunction:
    """IR函数"""
    name: str
    blocks: Dict[str, IRBasicBlock]
    entry_block: str
    exit_block: str

class IRModule:
    """IR模块"""
    
    def __init__(self):
        self.functions: Dict[str, IRFunction] = {}
        self.globals: Dict[str, IRValue] = {}
        self.entry_point: str = ''
        
    def add_function(self, func: IRFunction):
        """添加函数"""
        self.functions[func.name] = func
        
    def add_global(self, name: str, value: IRValue):
        """添加全局变量"""
        self.globals[name] = value
        
    def set_entry_point(self, name: str):
        """设置入口点"""
        if name not in self.functions:
            raise ValueError(f"入口函数不存在: {name}")
        self.entry_point = name

class IRBuilder:
    """IR构建器"""
    
    def __init__(self):
        self.module = IRModule()
        self.current_function: Optional[IRFunction] = None
        self.current_block: Optional[IRBasicBlock] = None
        self.temp_counter = 0
        self.block_counter = 0
        self.current_section = None
        self.entry_point = None
        self.current_label = None
        
    def create_temp(self, type: IRType) -> IRValue:
        """创建临时变量"""
        name = f"%{self.temp_counter}"
        self.temp_counter += 1
        return IRValue(type=type, name=name)
        
    def create_block(self) -> str:
        """创建基本块"""
        name = f"bb{self.block_counter}"
        self.block_counter += 1
        return name
        
    def create_function(self, name: str):
        """创建函数"""
        # 创建入口和出口基本块
        entry_block = self.create_block()
        exit_block = self.create_block()
        
        # 创建函数
        self.current_function = IRFunction(
            name=name,
            blocks={},
            entry_block=entry_block,
            exit_block=exit_block
        )
        
        # 添加到模块
        self.module.add_function(self.current_function)
        
        # 设置当前基本块为入口块
        self.set_block(entry_block)
        
    def set_block(self, name: str):
        """设置当前基本块"""
        if name not in self.current_function.blocks:
            # 创建新的基本块
            self.current_function.blocks[name] = IRBasicBlock(
                name=name,
                instructions=[],
                predecessors=set(),
                successors=set()
            )
            
            # 如果有前一个基本块，建立连接
            if self.current_block:
                # 获取前一个基本块的最后一条指令
                last_inst = None
                if self.current_block.instructions:
                    last_inst = self.current_block.instructions[-1]
                
                # 如果前一个基本块没有终结指令，添加一个跳转
                if not last_inst or last_inst.opcode not in ['jmp', 'je', 'jne', 'jl', 'jle', 'jg', 'jge', 'jz', 'jnz', 'ret']:
                    self.current_block.instructions.append(IRInstruction(
                        opcode='jmp',
                        dest=IRValue(type=IRType.VOID, value=name)
                    ))
                
                # 更新前驱和后继关系
                self.current_block.successors.add(name)
                self.current_function.blocks[name].predecessors.add(self.current_block.name)
        
        self.current_block = self.current_function.blocks[name]
        
    def add_instruction(self, inst: IRInstruction):
        """添加指令"""
        if self.current_block is None:
            if self.current_function is None:
                raise ValueError("未设置当前函数")
            # 创建新的基本块
            block_name = self.create_block()
            self.set_block(block_name)
        
        # 检查是否需要创建新的基本块
        if self.current_block.instructions:
            last_inst = self.current_block.instructions[-1]
            if last_inst.opcode in ['jmp', 'je', 'jne', 'jl', 'jle', 'jg', 'jge', 'jz', 'jnz', 'ret']:
                # 前一条指令是终结指令，创建新的基本块
                block_name = self.create_block()
                self.set_block(block_name)
        
        # 添加指令
        self.current_block.instructions.append(inst)
        
        # 如果是跳转指令，更新基本块关系
        if inst.opcode in ['jmp', 'je', 'jne', 'jl', 'jle', 'jg', 'jge', 'jz', 'jnz']:
            target = inst.dest.value
            # 创建目标基本块（如果不存在）
            if target not in self.current_function.blocks:
                self.current_function.blocks[target] = IRBasicBlock(
                    name=target,
                    instructions=[],
                    predecessors=set([self.current_block.name]),
                    successors=set()
                )
            # 更新前驱和后继关系
            self.current_block.successors.add(target)
            self.current_function.blocks[target].predecessors.add(self.current_block.name)
            
            # 如果是条件跳转，还需要添加fall-through边
            if inst.opcode != 'jmp':
                next_block = self.create_block()
                self.current_block.successors.add(next_block)
                self.current_function.blocks[next_block].predecessors.add(self.current_block.name)
                self.set_block(next_block)
        
    def build_ir(self, source: str) -> IRModule:
        """构建IR
        
        Args:
            source: TASM源代码
            
        Returns:
            IR模块
        """
        # 初始化
        self.module = IRModule()
        self.current_function = None
        self.current_block = None
        self.temp_counter = 0
        self.block_counter = 0
        self.current_section = None
        self.entry_point = None
        self.current_label = None
        
        # 第一遍扫描：收集函数和标签
        for line in source.splitlines():
            # 去除注释
            if '#' in line:
                line = line[:line.index('#')]
            line = line.strip()
            if not line:
                continue
                
            # 解析全局标签
            if line.startswith('.global'):
                parts = line.split(None, 1)
                if len(parts) < 2:
                    raise ValueError(f"无效的全局标签: {line}")
                self.entry_point = parts[1]
                continue
                
            # 解析标签
            if line.endswith(':'):
                label = line[:-1].strip()
                if self.current_section == '.text':
                    # 在代码段中的标签是函数
                    self.create_function(label)
                else:
                    # 在其他段中的标签只是普通标签
                    self.current_label = label
                continue
                
            # 解析段声明
            if line.startswith('.section'):
                self.current_section = line.split(None, 1)[1].strip()
                continue
        
        # 如果没有找到入口点，使用第一个函数
        if self.entry_point is None and self.module.functions:
            self.entry_point = next(iter(self.module.functions.keys()))
        
        # 设置入口点
        if self.entry_point:
            if self.entry_point not in self.module.functions:
                # 创建入口函数
                self.create_function(self.entry_point)
        
        # 第二遍扫描：处理指令和数据
        self.current_section = None
        self.current_function = None
        self.current_block = None
        
        for line in source.splitlines():
            # 去除注释
            if '#' in line:
                line = line[:line.index('#')]
            line = line.strip()
            if not line:
                continue
                
            # 解析段声明
            if line.startswith('.section'):
                self.current_section = line.split(None, 1)[1].strip()
                continue
                
            # 解析常量定义
            if line.startswith('.const'):
                parts = line.split(None, 2)
                if len(parts) < 3:
                    raise ValueError(f"无效的常量定义: {line}")
                name = parts[1]
                value = parts[2]
                
                # 字符串常量
                if value.startswith('"') and value.endswith('"'):
                    value = value[1:-1].encode('utf-8')
                    self.module.add_global(name, IRValue(
                        type=IRType.ARRAY,
                        value=value,
                        size=len(value)
                    ))
                    continue
                
                # 数值常量
                try:
                    if value.startswith('0x'):
                        value = int(value, 16)
                    else:
                        value = int(value)
                    self.module.add_global(name, IRValue(
                        type=IRType.I64,
                        value=value
                    ))
                except ValueError:
                    raise ValueError(f"无效的常量值: {value}")
                continue
                
            # 解析数据定义
            if line.startswith('.data'):
                parts = line.split(None, 3)
                if len(parts) < 3:
                    raise ValueError(f"无效的数据定义: {line}")
                name = parts[1]
                value = parts[2]
                
                try:
                    if value.startswith('0x'):
                        value = int(value, 16)
                    else:
                        value = int(value)
                    self.module.add_global(name, IRValue(
                        type=IRType.I64,
                        value=value
                    ))
                except ValueError:
                    raise ValueError(f"无效的数据值: {value}")
                continue
                
            # 解析BSS定义
            if line.startswith('.bss'):
                parts = line.split(None, 3)
                if len(parts) < 3:
                    raise ValueError(f"无效的BSS定义: {line}")
                name = parts[1]
                size = parts[2]
                
                try:
                    if size.startswith('0x'):
                        size = int(size, 16)
                    else:
                        size = int(size)
                    self.module.add_global(name, IRValue(
                        type=IRType.ARRAY,
                        size=size
                    ))
                except ValueError:
                    raise ValueError(f"无效的BSS大小: {size}")
                continue
                
            # 解析全局标签
            if line.startswith('.global'):
                continue
                
            # 解析标签
            if line.endswith(':'):
                label = line[:-1].strip()
                if self.current_section == '.text':
                    # 切换到新函数
                    if label in self.module.functions:
                        self.current_function = self.module.functions[label]
                        self.current_block = None
                    else:
                        self.create_function(label)
                continue
                
            # 解析指令
            if self.current_section != '.text':
                continue
                
            # 分割指令和操作数
            parts = line.split(None, 1)
            if not parts:
                continue
                
            opcode = parts[0].upper()
            operands = []
            if len(parts) > 1:
                # 处理逗号分隔的操作数
                operands = [op.strip() for op in parts[1].split(',')]
            
            # 数据移动指令
            if opcode == 'MOV':
                # MOV dst, src
                if len(operands) != 2:
                    raise ValueError(f"MOV指令需要两个操作数: {line}")
                    
                dst = self.parse_operand(operands[0])
                src = self.parse_operand(operands[1])
                
                self.add_instruction(IRInstruction(
                    opcode='mov',
                    dest=dst,
                    src1=src
                ))
                
            elif opcode == 'LEA':
                # LEA dst, [src]
                if len(operands) != 2:
                    raise ValueError(f"LEA指令需要两个操作数: {line}")
                    
                dst = self.parse_operand(operands[0])
                src = operands[1]
                if not (src.startswith('[') and src.endswith(']')):
                    raise ValueError(f"LEA指令的源操作数必须是内存引用: {line}")
                src = self.parse_operand(src[1:-1])
                
                self.add_instruction(IRInstruction(
                    opcode='lea',
                    dest=dst,
                    src1=src
                ))
                
            # 算术和逻辑指令
            elif opcode == 'XOR':
                # XOR dst, src
                if len(operands) != 2:
                    raise ValueError(f"XOR指令需要两个操作数: {line}")
                    
                dst = self.parse_operand(operands[0])
                src = self.parse_operand(operands[1])
                
                self.add_instruction(IRInstruction(
                    opcode='xor',
                    dest=dst,
                    src1=dst,
                    src2=src
                ))
                
            elif opcode == 'TEST':
                # TEST src1, src2
                if len(operands) != 2:
                    raise ValueError(f"TEST指令需要两个操作数: {line}")
                    
                src1 = self.parse_operand(operands[0])
                src2 = self.parse_operand(operands[1])
                
                self.add_instruction(IRInstruction(
                    opcode='test',
                    src1=src1,
                    src2=src2
                ))
                
            elif opcode == 'CMP':
                # CMP src1, src2
                if len(operands) != 2:
                    raise ValueError(f"CMP指令需要两个操作数: {line}")
                    
                src1 = self.parse_operand(operands[0])
                src2 = self.parse_operand(operands[1])
                
                self.add_instruction(IRInstruction(
                    opcode='cmp',
                    src1=src1,
                    src2=src2
                ))
                
            # 控制流指令
            elif opcode == 'JMP':
                # JMP target
                if len(operands) != 1:
                    raise ValueError(f"JMP指令需要一个操作数: {line}")
                    
                target = operands[0]
                self.add_instruction(IRInstruction(
                    opcode='jmp',
                    dest=IRValue(type=IRType.VOID, value=target)
                ))
                
            elif opcode in ['JL', 'JLE', 'JG', 'JGE', 'JZ', 'JNZ']:
                # Jcc target
                if len(operands) != 1:
                    raise ValueError(f"{opcode}指令需要一个操作数: {line}")
                    
                target = operands[0]
                self.add_instruction(IRInstruction(
                    opcode=opcode.lower(),
                    dest=IRValue(type=IRType.VOID, value=target)
                ))
                
            elif opcode == 'CALL':
                # CALL target
                if len(operands) != 1:
                    raise ValueError(f"CALL指令需要一个操作数: {line}")
                    
                target = operands[0]
                self.add_instruction(IRInstruction(
                    opcode='call',
                    dest=IRValue(type=IRType.VOID, value=target)
                ))
                
            elif opcode == 'RET':
                # RET
                if operands:
                    raise ValueError(f"RET指令不需要操作数: {line}")
                    
                self.add_instruction(IRInstruction(
                    opcode='ret'
                ))
                
            # 栈操作指令
            elif opcode == 'PUSH':
                # PUSH src
                if len(operands) != 1:
                    raise ValueError(f"PUSH指令需要一个操作数: {line}")
                    
                src = self.parse_operand(operands[0])
                self.add_instruction(IRInstruction(
                    opcode='push',
                    src1=src
                ))
                
            elif opcode == 'POP':
                # POP dst
                if len(operands) != 1:
                    raise ValueError(f"POP指令需要一个操作数: {line}")
                    
                dst = self.parse_operand(operands[0])
                self.add_instruction(IRInstruction(
                    opcode='pop',
                    dest=dst
                ))
                
            else:
                raise ValueError(f"未知的指令: {opcode}")
                
        return self.module
        
    def parse_operand(self, operand: str) -> IRValue:
        """解析操作数
        
        Args:
            operand: 操作数字符串
            
        Returns:
            IR值
        """
        # 寄存器
        if operand.startswith('r') or operand in ['sp', 'pc']:
            reg = operand.upper()
            return IRValue(
                type=IRType.I64,
                name=f"reg_{reg.lower()}"
            )
            
        # 立即数
        try:
            if operand.startswith('0x'):
                value = int(operand, 16)
            else:
                value = int(operand)
            return IRValue(
                type=IRType.I64,
                value=value
            )
        except ValueError:
            pass
            
        # 内存引用
        if operand.startswith('[') and operand.endswith(']'):
            addr = operand[1:-1]
            if addr in self.module.globals:
                return IRValue(
                    type=IRType.PTR,
                    value=addr
                )
            else:
                try:
                    if addr.startswith('0x'):
                        value = int(addr, 16)
                    else:
                        value = int(addr)
                    return IRValue(
                        type=IRType.PTR,
                        value=value
                    )
                except ValueError:
                    pass
            
        # 标签/变量
        return IRValue(
            type=IRType.PTR,
            value=operand
        )

def compile_to_ir(source_file: str, output_file: str):
    """编译TASM源码为IR
    
    Args:
        source_file: 源文件路径
        output_file: 输出文件路径
    """
    # 读取源文件
    with open(source_file, 'r', encoding='utf-8') as f:
        source = f.read()
        
    # 构建IR
    builder = IRBuilder()
    module = builder.build_ir(source)
    
    # 输出IR
    with open(output_file, 'w', encoding='utf-8') as f:
        # 输出全局变量
        for name, value in module.globals.items():
            f.write(f"@{name} = global {value.type.name}")
            if value.value is not None:
                if isinstance(value.value, bytes):
                    f.write(f" \"{value.value.decode('utf-8')}\"")
                else:
                    f.write(f" {value.value}")
            if value.size is not None:
                f.write(f" [{value.size}]")
            f.write('\n')
        f.write('\n')
        
        # 输出函数
        for func in module.functions.values():
            f.write(f"define {func.name}() {{\n")
            
            # 输出基本块
            for block in func.blocks.values():
                f.write(f"{block.name}:\n")
                
                # 输出指令
                for inst in block.instructions:
                    # 缩进
                    f.write('    ')
                    
                    # 目标操作数
                    if inst.dest:
                        f.write(f"{inst.dest.name} = ")
                    
                    # 操作码
                    f.write(inst.opcode)
                    
                    # 源操作数
                    if inst.src1:
                        if inst.src1.value is not None:
                            f.write(f" {inst.src1.value}")
                        else:
                            f.write(f" {inst.src1.name}")
                    
                    if inst.src2:
                        if inst.src2.value is not None:
                            f.write(f", {inst.src2.value}")
                        else:
                            f.write(f", {inst.src2.name}")
                    
                    f.write('\n')
                
                f.write('\n')
            
            f.write('}\n\n')

def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: tasm_compile_to_ir.py <source_file> [output_file]")
        sys.exit(1)
        
    source_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else source_file + '.ir'
    
    try:
        compile_to_ir(source_file, output_file)
        print(f"编译成功: {output_file}")
    except Exception as e:
        print(f"编译错误: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main() 