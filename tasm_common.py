#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TASM编译器 - 共享的类和函数
"""

import os
import sys
from enum import Enum, IntFlag
from dataclasses import dataclass
from typing import List, Optional, Dict, Union, Tuple

class CompileError(Exception):
    """编译错误"""
    def __init__(self, message: str, line: int = 0, column: int = 0, file: str = ""):
        self.message = message
        self.line = line
        self.column = column
        self.file = file
        super().__init__(self._format_message())
        
    def _format_message(self) -> str:
        """格式化错误信息"""
        if self.file and self.line > 0:
            if self.column > 0:
                return f"{self.file}:{self.line}:{self.column}: {self.message}"
            return f"{self.file}:{self.line}: {self.message}"
        return self.message

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

class Register(Enum):
    """寄存器"""
    R0 = 0  # 程序基址寄存器
    R1 = 1  # 程序大小寄存器
    R2 = 2  # 寄存器基址寄存器
    R3 = 3  # 程序计数器 (PC)
    R4 = 4  # 临时计算寄存器
    R5 = 5  # 临时计算寄存器
    R6 = 6  # 临时计算寄存器
    R7 = 7  # 返回值寄存器
    SP = 8  # 栈指针
    PC = 3  # 程序计数器 (同 R3)
    
    def __str__(self):
        """返回寄存器名称"""
        return self.name
        
    def __int__(self):
        """返回寄存器编号"""
        return self.value
        
    def __and__(self, other):
        """位与运算"""
        return int(self) & other
        
    def __or__(self, other):
        """位或运算"""
        return int(self) | other

class Opcode(Enum):
    """指令操作码"""
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

@dataclass
class Instruction:
    """指令"""
    opcode: Opcode
    operands: List[Union[int, str, Register]]
    size: int = 3  # 默认每条指令3字节
    offset: int = 0
    
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
            elif isinstance(target, Register):  # 寄存器
                target = int(target)
            data.extend(target.to_bytes(4, 'little'))  # 目标地址
            
        elif self.opcode in [Opcode.MOV, Opcode.ADD, Opcode.SUB, Opcode.MUL, Opcode.DIV,
                            Opcode.CMP, Opcode.TEST]:
            # 双操作数指令
            if len(self.operands) < 2:
                raise ValueError(f"双操作数指令需要两个操作数: {self.opcode.name}")
            # 目标寄存器
            if isinstance(self.operands[0], Register):
                data.append(int(self.operands[0]))
            else:
                raise ValueError(f"目标操作数必须是寄存器: {self.operands[0]}")
            # 源操作数
            if isinstance(self.operands[1], Register):
                data.append(int(self.operands[1]))
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
                data.append(int(self.operands[0]))
            else:
                raise ValueError(f"操作数必须是寄存器: {self.operands[0]}")
                
        elif self.opcode == Opcode.SYSCALL:
            # 系统调用指令
            if not self.operands:
                raise ValueError(f"系统调用指令需要一个操作数: {self.opcode.name}")
            if isinstance(self.operands[0], int):
                if self.operands[0] > 255:
                    raise ValueError(f"系统调用号超出范围(0-255): {self.operands[0]}")
                data.append(self.operands[0])
                data.append(0)  # 填充字节
            else:
                raise ValueError(f"系统调用号必须是立即数: {self.operands[0]}")
                
        elif self.opcode in [Opcode.NOP, Opcode.HLT]:
            # 无操作数指令
            pass
            
        else:
            raise ValueError(f"未知的指令: {self.opcode.name}")
            
        # 如果指令长度不足，用0填充
        while len(data) < self.size:
            data.append(0)
            
        return bytes(data)

class BinaryFile:
    """二进制文件"""
    MAGIC = b'TASM'
    VERSION = (1, 0)  # Major.Minor
    
    def __init__(self):
        self.sections = []
        self.entry_point = 0
    
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

class TokenType(Enum):
    """词法单元类型"""
    # 特殊标记
    EOF = 'EOF'           # 文件结束
    NEWLINE = 'NEWLINE'   # 换行
    COMMENT = 'COMMENT'   # 注释
    
    # 标点符号
    COLON = ':'          # 冒号
    COMMA = ','          # 逗号
    LBRACKET = '['      # 左方括号
    RBRACKET = ']'      # 右方括号
    PLUS = '+'          # 加号
    MINUS = '-'         # 减号
    STAR = '*'          # 星号
    
    # 关键字
    SECTION = 'section'  # 段声明
    DB = 'db'           # 字节定义
    DW = 'dw'           # 字定义
    DD = 'dd'           # 双字定义
    DQ = 'dq'           # 四字定义
    GLOBAL = 'global'   # 全局标签
    EXTERN = 'extern'   # 外部标签
    
    # 标识符和常量
    IDENTIFIER = 'IDENTIFIER'  # 标识符
    NUMBER = 'NUMBER'         # 数字
    STRING = 'STRING'         # 字符串
    REGISTER = 'REGISTER'     # 寄存器

@dataclass
class Token:
    """词法单元"""
    type: TokenType
    value: str
    line: int
    column: int
    
    def __str__(self):
        return f"{self.type.name}({self.value})"

class Lexer:
    """词法分析器"""
    def __init__(self, source: str, filename: str = ""):
        self.source = source
        self.filename = filename
        self.pos = 0
        self.line = 1
        self.column = 1
        self.current_char = self.source[0] if source else None
        
    def error(self, message: str) -> None:
        """抛出词法错误"""
        raise CompileError(message, self.line, self.column, self.filename)
        
    def advance(self) -> None:
        """读取下一个字符"""
        self.pos += 1
        if self.pos >= len(self.source):
            self.current_char = None
        else:
            self.current_char = self.source[self.pos]
            if self.current_char == '\n':
                self.line += 1
                self.column = 1
            else:
                self.column += 1
                
    def peek(self) -> Optional[str]:
        """预读下一个字符"""
        peek_pos = self.pos + 1
        if peek_pos >= len(self.source):
            return None
        return self.source[peek_pos]
        
    def skip_whitespace(self) -> None:
        """跳过空白字符"""
        while self.current_char and self.current_char.isspace():
            self.advance()
            
    def skip_comment(self) -> None:
        """跳过注释"""
        # 单行注释
        if self.current_char == ';':
            while self.current_char and self.current_char != '\n':
                self.advance()
            return
            
        # 多行注释
        if self.current_char == '/' and self.peek() == '*':
            self.advance()  # 跳过 /
            self.advance()  # 跳过 *
            while self.current_char:
                if self.current_char == '*' and self.peek() == '/':
                    self.advance()  # 跳过 *
                    self.advance()  # 跳过 /
                    return
                self.advance()
            self.error("未闭合的多行注释")
            
    def read_number(self) -> Token:
        """读取数字"""
        # 记录起始位置
        start_line = self.line
        start_column = self.column
        result = ''
        
        # 处理十六进制
        if self.current_char == '0' and self.peek() and self.peek().lower() == 'x':
            result += '0x'
            self.advance()  # 跳过 0
            self.advance()  # 跳过 x
            while self.current_char and (self.current_char.isdigit() or 
                  self.current_char.lower() in 'abcdef'):
                result += self.current_char
                self.advance()
            if len(result) <= 2:
                self.error("无效的十六进制数")
            return Token(TokenType.NUMBER, result, start_line, start_column)
            
        # 处理二进制
        if self.current_char == '0' and self.peek() and self.peek().lower() == 'b':
            result += '0b'
            self.advance()  # 跳过 0
            self.advance()  # 跳过 b
            while self.current_char and self.current_char in '01':
                result += self.current_char
                self.advance()
            if len(result) <= 2:
                self.error("无效的二进制数")
            return Token(TokenType.NUMBER, result, start_line, start_column)
            
        # 处理十进制
        while self.current_char and self.current_char.isdigit():
            result += self.current_char
            self.advance()
            
        return Token(TokenType.NUMBER, result, start_line, start_column)
        
    def read_string(self) -> Token:
        """读取字符串"""
        # 记录起始位置
        start_line = self.line
        start_column = self.column
        quote = self.current_char  # 引号类型 (" 或 ')
        result = ''
        self.advance()  # 跳过引号
        
        while self.current_char and self.current_char != quote:
            if self.current_char == '\\':
                self.advance()
                if not self.current_char:
                    self.error("字符串未闭合")
                # 处理转义字符
                if self.current_char in 'nrt\\':
                    result += '\\' + self.current_char
                else:
                    self.error(f"无效的转义字符: \\{self.current_char}")
            else:
                result += self.current_char
            self.advance()
            
        if not self.current_char:
            self.error("字符串未闭合")
            
        self.advance()  # 跳过结束引号
        return Token(TokenType.STRING, result, start_line, start_column)
        
    def read_identifier(self) -> Token:
        """读取标识符"""
        # 记录起始位置
        start_line = self.line
        start_column = self.column
        result = ''
        
        while self.current_char and (self.current_char.isalnum() or 
              self.current_char == '_'):
            result += self.current_char
            self.advance()
            
        # 检查是否是关键字
        upper = result.upper()
        if upper in ['SECTION', 'DB', 'DW', 'DD', 'DQ', 'GLOBAL', 'EXTERN']:
            return Token(TokenType[upper], result, start_line, start_column)
            
        # 检查是否是寄存器
        if upper in ['RAX', 'RBX', 'RCX', 'RDX', 'RSI', 'RDI', 'RSP', 'RBP',
                    'R8', 'R9', 'R10', 'R11', 'R12', 'R13', 'R14', 'R15']:
            return Token(TokenType.REGISTER, result, start_line, start_column)
            
        return Token(TokenType.IDENTIFIER, result, start_line, start_column)
        
    def get_next_token(self) -> Token:
        """获取下一个词法单元"""
        while self.current_char:
            # 跳过空白字符
            if self.current_char.isspace():
                self.skip_whitespace()
                continue
                
            # 处理注释
            if self.current_char == ';' or (self.current_char == '/' and 
               self.peek() == '*'):
                self.skip_comment()
                continue
                
            # 记录当前位置
            line = self.line
            column = self.column
            
            # 处理标点符号
            if self.current_char == ':':
                self.advance()
                return Token(TokenType.COLON, ':', line, column)
            elif self.current_char == ',':
                self.advance()
                return Token(TokenType.COMMA, ',', line, column)
            elif self.current_char == '[':
                self.advance()
                return Token(TokenType.LBRACKET, '[', line, column)
            elif self.current_char == ']':
                self.advance()
                return Token(TokenType.RBRACKET, ']', line, column)
            elif self.current_char == '+':
                self.advance()
                return Token(TokenType.PLUS, '+', line, column)
            elif self.current_char == '-':
                self.advance()
                return Token(TokenType.MINUS, '-', line, column)
            elif self.current_char == '*':
                self.advance()
                return Token(TokenType.STAR, '*', line, column)
                
            # 处理数字
            if self.current_char.isdigit():
                return self.read_number()
                
            # 处理字符串
            if self.current_char in '"\'':
                return self.read_string()
                
            # 处理标识符和关键字
            if self.current_char.isalpha() or self.current_char == '_':
                return self.read_identifier()
                
            self.error(f"无效的字符: {self.current_char}")
            
        # 文件结束
        return Token(TokenType.EOF, '', self.line, self.column) 