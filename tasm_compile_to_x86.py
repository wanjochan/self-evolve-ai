#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TASM编译器 - 将IR编译为x86-64机器码
"""

import os
import sys
from enum import Enum
from dataclasses import dataclass
from typing import List, Dict, Set, Optional, Union, Tuple

class X86Register(Enum):
    """x86-64寄存器"""
    # 通用寄存器
    RAX = 0  # 累加器
    RCX = 1  # 计数器
    RDX = 2  # 数据
    RBX = 3  # 基址
    RSP = 4  # 栈指针
    RBP = 5  # 基址指针
    RSI = 6  # 源索引
    RDI = 7  # 目标索引
    # 扩展寄存器
    R8  = 8  # 通用寄存器
    R9  = 9  # 通用寄存器
    R10 = 10 # 通用寄存器
    R11 = 11 # 通用寄存器
    R12 = 12 # 通用寄存器
    R13 = 13 # 通用寄存器
    R14 = 14 # 通用寄存器
    R15 = 15 # 通用寄存器

class X86Scale(Enum):
    """内存寻址的比例因子"""
    S1 = 0  # *1
    S2 = 1  # *2
    S4 = 2  # *4
    S8 = 3  # *8

@dataclass
class X86Instruction:
    """x86-64指令"""
    opcode: bytes
    modrm: Optional[int] = None
    sib: Optional[int] = None
    displacement: Optional[int] = None
    immediate: Optional[int] = None
    
    def encode(self) -> bytes:
        """编码指令为字节序列"""
        result = bytearray(self.opcode)
        print(f"Encoding instruction:")
        print(f"  Opcode: {[hex(b) for b in result]}")
        
        if self.modrm is not None:
            result.append(self.modrm)
            print(f"  ModRM: {hex(self.modrm)}")
            
        if self.sib is not None:
            result.append(self.sib)
            print(f"  SIB: {hex(self.sib)}")
            
        if self.displacement is not None:
            if -128 <= self.displacement <= 127:
                # 8位位移
                result.append(self.displacement & 0xFF)
                print(f"  Disp8: {hex(self.displacement & 0xFF)}")
            else:
                # 32位位移
                disp_bytes = self.displacement.to_bytes(4, 'little', signed=True)
                result.extend(disp_bytes)
                print(f"  Disp32: {[hex(b) for b in disp_bytes]}")
                
        if self.immediate is not None:
            # 64位立即数
            imm_bytes = self.immediate.to_bytes(8, 'little', signed=True)
            result.extend(imm_bytes)
            print(f"  Imm64: {[hex(b) for b in imm_bytes]}")
            
        print(f"  Final: {[hex(b) for b in result]}\n")
        return bytes(result)

class X86Builder:
    """x86-64构建器"""
    
    def __init__(self):
        self.code = bytearray()
        self.data = bytearray()
        self.labels: Dict[str, int] = {}
        self.fixups: List[Tuple[int, str]] = []
        
    def add_instruction(self, inst: X86Instruction):
        """添加指令"""
        self.code.extend(inst.encode())
        
    def add_label(self, name: str):
        """添加标签"""
        self.labels[name] = len(self.code)
        
    def add_fixup(self, name: str):
        """添加修复项"""
        self.fixups.append((len(self.code), name))
        self.code.extend(bytes([0] * 4))  # 占位
        
    def fix_labels(self):
        """修复标签引用"""
        for offset, name in self.fixups:
            if name not in self.labels:
                raise ValueError(f"未定义的标签: {name}")
            target = self.labels[name]
            rel = target - (offset + 4)  # 相对偏移
            self.code[offset:offset+4] = rel.to_bytes(4, 'little', signed=True)
            
    def mov_reg_reg(self, dst: X86Register, src: X86Register):
        """MOV reg, reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x48, 0x89]),  # REX.W + MOV
            modrm=0xC0 | (src.value << 3) | dst.value
        ))
        
    def mov_reg_imm(self, dst: X86Register, imm: int):
        """MOV reg, imm"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x48, 0xB8 | dst.value]),  # REX.W + MOV
            immediate=imm
        ))
        
    def mov_reg_mem(self, dst: X86Register, base: X86Register, disp: Optional[int] = None):
        """MOV reg, [base+disp]"""
        if disp is None:
            # 无偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([0x48, 0x8B]),  # REX.W + MOV
                modrm=0x00 | (dst.value << 3) | base.value
            ))
        elif -128 <= disp <= 127:
            # 8位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([0x48, 0x8B]),  # REX.W + MOV
                modrm=0x40 | (dst.value << 3) | base.value,
                displacement=disp
            ))
        else:
            # 32位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([0x48, 0x8B]),  # REX.W + MOV
                modrm=0x80 | (dst.value << 3) | base.value,
                displacement=disp
            ))
            
    def mov_mem_reg(self, base: X86Register, src: X86Register, disp: Optional[int] = None):
        """MOV [base+disp], reg"""
        if disp is None:
            # 无偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([0x48, 0x89]),  # REX.W + MOV
                modrm=0x00 | (src.value << 3) | base.value
            ))
        elif -128 <= disp <= 127:
            # 8位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([0x48, 0x89]),  # REX.W + MOV
                modrm=0x40 | (src.value << 3) | base.value,
                displacement=disp
            ))
        else:
            # 32位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([0x48, 0x89]),  # REX.W + MOV
                modrm=0x80 | (src.value << 3) | base.value,
                displacement=disp
            ))
            
    def lea_reg_mem(self, dst: X86Register, base: X86Register, disp: Optional[int] = None):
        """LEA reg, [base+disp]"""
        if disp is None:
            # 无偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([0x48, 0x8D]),  # REX.W + LEA
                modrm=0x00 | (dst.value << 3) | base.value
            ))
        elif -128 <= disp <= 127:
            # 8位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([0x48, 0x8D]),  # REX.W + LEA
                modrm=0x40 | (dst.value << 3) | base.value,
                displacement=disp
            ))
        else:
            # 32位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([0x48, 0x8D]),  # REX.W + LEA
                modrm=0x80 | (dst.value << 3) | base.value,
                displacement=disp
            ))
            
    def add_reg_reg(self, dst: X86Register, src: X86Register):
        """ADD reg, reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x48, 0x01]),  # REX.W + ADD
            modrm=0xC0 | (src.value << 3) | dst.value
        ))
        
    def sub_reg_reg(self, dst: X86Register, src: X86Register):
        """SUB reg, reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x48, 0x29]),  # REX.W + SUB
            modrm=0xC0 | (src.value << 3) | dst.value
        ))
        
    def xor_reg_reg(self, dst: X86Register, src: X86Register):
        """XOR reg, reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x48, 0x31]),  # REX.W + XOR
            modrm=0xC0 | (src.value << 3) | dst.value
        ))
        
    def test_reg_reg(self, src1: X86Register, src2: X86Register):
        """TEST reg, reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x48, 0x85]),  # REX.W + TEST
            modrm=0xC0 | (src2.value << 3) | src1.value
        ))
        
    def cmp_reg_reg(self, src1: X86Register, src2: X86Register):
        """CMP reg, reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x48, 0x39]),  # REX.W + CMP
            modrm=0xC0 | (src2.value << 3) | src1.value
        ))
        
    def mul_reg(self, src: X86Register):
        """MUL reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x48, 0xF7]),  # REX.W + MUL
            modrm=0xE0 | src.value
        ))
        
    def div_reg(self, src: X86Register):
        """DIV reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x48, 0xF7]),  # REX.W + DIV
            modrm=0xF0 | src.value
        ))
        
    def push_reg(self, reg: X86Register):
        """PUSH reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x50 | reg.value])
        ))
        
    def pop_reg(self, reg: X86Register):
        """POP reg"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x58 | reg.value])
        ))
        
    def call_rel32(self, target: str):
        """CALL rel32"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0xE8])
        ))
        self.add_fixup(target)
        
    def jmp_rel32(self, target: str):
        """JMP rel32"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0xE9])
        ))
        self.add_fixup(target)
        
    def jl_rel32(self, target: str):
        """JL rel32"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x0F, 0x8C])
        ))
        self.add_fixup(target)
        
    def jle_rel32(self, target: str):
        """JLE rel32"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x0F, 0x8E])
        ))
        self.add_fixup(target)
        
    def jg_rel32(self, target: str):
        """JG rel32"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x0F, 0x8F])
        ))
        self.add_fixup(target)
        
    def jge_rel32(self, target: str):
        """JGE rel32"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x0F, 0x8D])
        ))
        self.add_fixup(target)
        
    def jz_rel32(self, target: str):
        """JZ rel32"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x0F, 0x84])
        ))
        self.add_fixup(target)
        
    def jnz_rel32(self, target: str):
        """JNZ rel32"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0x0F, 0x85])
        ))
        self.add_fixup(target)
        
    def ret(self):
        """RET"""
        self.add_instruction(X86Instruction(
            opcode=bytes([0xC3])
        ))

    def _encode_sib(self, scale: X86Scale, index: X86Register, base: X86Register) -> int:
        """编码SIB字节
        
        SIB字节格式：
        7 6 | 5 4 3 | 2 1 0
        Scale | Index | Base
        
        Scale: 00=1, 01=2, 10=4, 11=8
        Index: 寄存器编号 (000-111)
        Base: 寄存器编号 (000-111)
        
        注意：
        1. 对于R8-R15寄存器，需要在REX前缀中设置相应的位
        2. 比例因子必须是1、2、4或8
        3. 不能使用RSP作为索引寄存器
        """
        # 检查是否使用了RSP作为索引寄存器
        if index == X86Register.RSP:
            raise ValueError("不能使用RSP作为索引寄存器")
            
        # 获取寄存器的低3位
        index_val = index.value & 0x7
        base_val = base.value & 0x7
        
        return (scale.value << 6) | (index_val << 3) | base_val
        
    def mov_reg_mem_sib(self, dst: X86Register, base: X86Register, 
                       index: X86Register, scale: X86Scale,
                       disp: Optional[int] = None):
        """MOV reg, [base + index*scale + disp]"""
        # 计算REX前缀
        rex_w = 0x48  # 64位操作
        rex_r = 0x04 if dst.value >= 8 else 0x00  # 目标寄存器是R8-R15
        rex_x = 0x02 if index.value >= 8 else 0x00  # 索引寄存器是R8-R15
        rex_b = 0x01 if base.value >= 8 else 0x00  # 基址寄存器是R8-R15
        rex = rex_w | rex_r | rex_x | rex_b
        
        if disp is None:
            # 无偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([rex, 0x8B]),  # REX + MOV
                modrm=0x04 | ((dst.value & 0x7) << 3),  # ModRM with SIB
                sib=self._encode_sib(scale, index, base)
            ))
        elif -128 <= disp <= 127:
            # 8位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([rex, 0x8B]),  # REX + MOV
                modrm=0x44 | ((dst.value & 0x7) << 3),  # ModRM with SIB
                sib=self._encode_sib(scale, index, base),
                displacement=disp
            ))
        else:
            # 32位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([rex, 0x8B]),  # REX + MOV
                modrm=0x84 | ((dst.value & 0x7) << 3),  # ModRM with SIB
                sib=self._encode_sib(scale, index, base),
                displacement=disp
            ))
            
    def mov_mem_sib_reg(self, base: X86Register, index: X86Register,
                       scale: X86Scale, src: X86Register,
                       disp: Optional[int] = None):
        """MOV [base + index*scale + disp], reg"""
        # 计算REX前缀
        rex_w = 0x48  # 64位操作
        rex_r = 0x04 if src.value >= 8 else 0x00  # 源寄存器是R8-R15
        rex_x = 0x02 if index.value >= 8 else 0x00  # 索引寄存器是R8-R15
        rex_b = 0x01 if base.value >= 8 else 0x00  # 基址寄存器是R8-R15
        rex = rex_w | rex_r | rex_x | rex_b
        
        if disp is None:
            # 无偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([rex, 0x89]),  # REX + MOV
                modrm=0x04 | ((src.value & 0x7) << 3),  # ModRM with SIB
                sib=self._encode_sib(scale, index, base)
            ))
        elif -128 <= disp <= 127:
            # 8位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([rex, 0x89]),  # REX + MOV
                modrm=0x44 | ((src.value & 0x7) << 3),  # ModRM with SIB
                sib=self._encode_sib(scale, index, base),
                displacement=disp
            ))
        else:
            # 32位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([rex, 0x89]),  # REX + MOV
                modrm=0x84 | ((src.value & 0x7) << 3),  # ModRM with SIB
                sib=self._encode_sib(scale, index, base),
                displacement=disp
            ))
            
    def lea_reg_mem_sib(self, dst: X86Register, base: X86Register,
                       index: X86Register, scale: X86Scale,
                       disp: Optional[int] = None):
        """LEA reg, [base + index*scale + disp]"""
        # 计算REX前缀
        rex_w = 0x48  # 64位操作
        rex_r = 0x04 if dst.value >= 8 else 0x00  # 目标寄存器是R8-R15
        rex_x = 0x02 if index.value >= 8 else 0x00  # 索引寄存器是R8-R15
        rex_b = 0x01 if base.value >= 8 else 0x00  # 基址寄存器是R8-R15
        rex = rex_w | rex_r | rex_x | rex_b
        
        if disp is None:
            # 无偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([rex, 0x8D]),  # REX + LEA
                modrm=0x04 | ((dst.value & 0x7) << 3),  # ModRM with SIB
                sib=self._encode_sib(scale, index, base)
            ))
        elif -128 <= disp <= 127:
            # 8位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([rex, 0x8D]),  # REX + LEA
                modrm=0x44 | ((dst.value & 0x7) << 3),  # ModRM with SIB
                sib=self._encode_sib(scale, index, base),
                displacement=disp
            ))
        else:
            # 32位偏移
            self.add_instruction(X86Instruction(
                opcode=bytes([rex, 0x8D]),  # REX + LEA
                modrm=0x84 | ((dst.value & 0x7) << 3),  # ModRM with SIB
                sib=self._encode_sib(scale, index, base),
                displacement=disp
            ))

class X86PeepholeOptimizer:
    """x86代码窥孔优化器"""
    
    def __init__(self):
        self.patterns = [
            self._optimize_redundant_mov,
            self._optimize_push_pop,
            self._optimize_test_jz,
            self._optimize_add_sub,
            self._optimize_lea_complex,
            self._optimize_jump_chain,
            self._optimize_memory_access  # 添加内存访问优化
        ]
        
    def optimize(self, instructions: List[X86Instruction]) -> List[X86Instruction]:
        """优化指令序列"""
        optimized = True
        result = instructions.copy()
        
        while optimized:
            optimized = False
            for i in range(len(result)):
                for pattern in self.patterns:
                    if i + pattern.window_size <= len(result):
                        window = result[i:i+pattern.window_size]
                        if new_insts := pattern(window):
                            result[i:i+pattern.window_size] = new_insts
                            optimized = True
                            break
                if optimized:
                    break
                    
        return result
        
    def _optimize_redundant_mov(self, window: List[X86Instruction]) -> Optional[List[X86Instruction]]:
        """优化冗余的MOV指令
        
        模式1：
        MOV reg1, reg2
        MOV reg2, reg1
        ->
        MOV reg1, reg2
        
        模式2：
        MOV reg1, imm
        MOV reg2, reg1
        ->
        MOV reg1, imm
        """
        if len(window) < 2:
            return None
            
        inst1, inst2 = window[0], window[1]
        
        # 检查是否都是MOV指令
        if not (inst1.opcode[1] in [0x89, 0x8B] and inst2.opcode[1] in [0x89, 0x8B]):
            return None
            
        # 获取源和目标寄存器
        dst1 = (inst1.modrm >> 3) & 0x7
        src1 = inst1.modrm & 0x7
        dst2 = (inst2.modrm >> 3) & 0x7
        src2 = inst2.modrm & 0x7
        
        # 检查模式1
        if dst1 == src2 and src1 == dst2:
            return [inst1]
            
        # 检查模式2
        if inst1.immediate is not None and dst1 == src2:
            return [inst1]
            
        return None
        
    def _optimize_push_pop(self, window: List[X86Instruction]) -> Optional[List[X86Instruction]]:
        """优化PUSH/POP序列
        
        模式1：
        PUSH reg1
        POP reg1
        ->
        (删除)
        
        模式2：
        PUSH reg1
        PUSH reg2
        POP reg1
        POP reg2
        ->
        MOV reg2, reg1
        """
        if len(window) < 2:
            return None
            
        # 检查模式1
        if (window[0].opcode[0] & 0xF8 == 0x50 and  # PUSH reg
            window[1].opcode[0] & 0xF8 == 0x58):    # POP reg
            reg1 = window[0].opcode[0] & 0x7
            reg2 = window[1].opcode[0] & 0x7
            if reg1 == reg2:
                return []
                
        # 检查模式2
        if len(window) >= 4:
            if (window[0].opcode[0] & 0xF8 == 0x50 and  # PUSH reg1
                window[1].opcode[0] & 0xF8 == 0x50 and  # PUSH reg2
                window[2].opcode[0] & 0xF8 == 0x58 and  # POP reg1
                window[3].opcode[0] & 0xF8 == 0x58):    # POP reg2
                reg1 = window[0].opcode[0] & 0x7
                reg2 = window[1].opcode[0] & 0x7
                pop1 = window[2].opcode[0] & 0x7
                pop2 = window[3].opcode[0] & 0x7
                if reg1 == pop2 and reg2 == pop1:
                    return [X86Instruction(
                        opcode=bytes([0x48, 0x89]),  # MOV
                        modrm=0xC0 | (reg2 << 3) | reg1
                    )]
                    
        return None
        
    def _optimize_test_jz(self, window: List[X86Instruction]) -> Optional[List[X86Instruction]]:
        """优化TEST/JZ序列
        
        模式1：
        TEST reg, reg
        JZ label
        ->
        CMP reg, 0
        JZ label
        
        模式2：
        TEST reg, -1
        JZ label
        ->
        CMP reg, 0
        JZ label
        """
        if len(window) < 2:
            return None
            
        inst1, inst2 = window[0], window[1]
        
        # 检查是否是TEST+JZ
        if not (inst1.opcode[1] == 0x85 and  # TEST
               inst2.opcode[0] == 0x0F and inst2.opcode[1] == 0x84):  # JZ
            return None
            
        # 获取TEST的源和目标寄存器
        dst = (inst1.modrm >> 3) & 0x7
        src = inst1.modrm & 0x7
        
        # 如果是同一个寄存器或者立即数-1
        if dst == src or inst1.immediate == -1:
            return [
                X86Instruction(
                    opcode=bytes([0x48, 0x83]),  # CMP
                    modrm=0xF8 | dst,  # CMP reg, imm8
                    immediate=0
                ),
                inst2
            ]
            
        return None
        
    def _optimize_add_sub(self, window: List[X86Instruction]) -> Optional[List[X86Instruction]]:
        """优化ADD/SUB序列
        
        模式1：
        ADD reg, imm1
        SUB reg, imm2
        ->
        ADD reg, (imm1-imm2)  # 如果imm1>imm2
        SUB reg, (imm2-imm1)  # 如果imm2>imm1
        
        模式2：
        ADD reg, 0
        ->
        (删除)
        """
        if len(window) < 2:
            return None
            
        inst1, inst2 = window[0], window[1]
        
        # 检查ADD+SUB
        if (inst1.opcode[1] == 0x81 and  # ADD reg, imm32
            inst2.opcode[1] == 0x81 and  # SUB reg, imm32
            (inst1.modrm & 0x38) == 0x00 and  # ADD
            (inst2.modrm & 0x38) == 0x28):    # SUB
            reg1 = inst1.modrm & 0x7
            reg2 = inst2.modrm & 0x7
            if reg1 == reg2:
                imm1 = inst1.immediate
                imm2 = inst2.immediate
                if imm1 > imm2:
                    return [X86Instruction(
                        opcode=bytes([0x48, 0x81]),  # ADD
                        modrm=0xC0 | reg1,
                        immediate=imm1 - imm2
                    )]
                else:
                    return [X86Instruction(
                        opcode=bytes([0x48, 0x81]),  # SUB
                        modrm=0xE8 | reg1,
                        immediate=imm2 - imm1
                    )]
                    
        # 检查ADD reg, 0
        if (inst1.opcode[1] == 0x81 and  # ADD reg, imm32
            (inst1.modrm & 0x38) == 0x00 and  # ADD
            inst1.immediate == 0):
            return []
            
        return None
        
    def _optimize_lea_complex(self, window: List[X86Instruction]) -> Optional[List[X86Instruction]]:
        """优化复杂的LEA指令
        
        模式1：
        LEA reg1, [reg2 + reg2*1]
        ->
        LEA reg1, [reg2*2]
        
        模式2：
        LEA reg1, [reg2 + reg2*2]
        ->
        LEA reg1, [reg2*3]
        """
        if len(window) < 1:
            return None
            
        inst = window[0]
        
        # 检查是否是LEA指令
        if inst.opcode[1] != 0x8D:
            return None
            
        # 检查是否使用SIB字节
        if (inst.modrm & 0x07) != 0x04:
            return None
            
        # 获取SIB字节的各个字段
        scale = inst.sib >> 6
        index = (inst.sib >> 3) & 0x7
        base = inst.sib & 0x7
        
        # 检查模式1：[reg + reg*1]
        if scale == 0 and base == index:
            return [X86Instruction(
                opcode=inst.opcode,
                modrm=inst.modrm,
                sib=(1 << 6) | (base << 3) | 0x05  # scale=2, no base
            )]
            
        # 检查模式2：[reg + reg*2]
        if scale == 1 and base == index:
            return [X86Instruction(
                opcode=inst.opcode,
                modrm=inst.modrm,
                sib=(2 << 6) | (base << 3) | 0x05  # scale=4, no base
            )]
            
        return None

    def _optimize_jump_chain(self, window: List[X86Instruction]) -> Optional[List[X86Instruction]]:
        """优化条件跳转链
        
        模式1：
        JZ label1
        JMP label2
        label1:
        JMP label3
        ->
        JNZ label2
        JMP label3
        
        模式2：
        JZ label1
        ...
        label1:
        JMP label2
        ->
        JZ label2
        ...
        
        模式3：
        JMP label1
        label1:
        JMP label2
        ->
        JMP label2
        """
        if len(window) < 2:
            return None
            
        inst1, inst2 = window[0], window[1]
        
        # 检查模式1：条件跳转后跟无条件跳转
        if (inst1.opcode[0] == 0x0F and inst1.opcode[1] in [0x84, 0x85, 0x8C, 0x8D, 0x8E, 0x8F] and  # Jcc
            inst2.opcode[0] == 0xE9):  # JMP
            # 反转条件跳转
            new_opcode = bytes([0x0F, self._invert_condition_code(inst1.opcode[1])])
            return [
                X86Instruction(
                    opcode=new_opcode,
                    immediate=inst2.immediate
                ),
                inst1
            ]
            
        # 检查模式2：跳转到跳转
        if ((inst1.opcode[0] == 0x0F and inst1.opcode[1] in [0x84, 0x85, 0x8C, 0x8D, 0x8E, 0x8F]) or  # Jcc
            inst1.opcode[0] == 0xE9) and  # JMP
           inst2.label and inst2.opcode[0] == 0xE9:  # labeled JMP
            if inst1.immediate == inst2.label:
                return [X86Instruction(
                    opcode=inst1.opcode,
                    immediate=inst2.immediate
                )]
                
        # 检查模式3：无条件跳转链
        if (inst1.opcode[0] == 0xE9 and  # JMP
            inst2.label and inst2.opcode[0] == 0xE9):  # labeled JMP
            if inst1.immediate == inst2.label:
                return [X86Instruction(
                    opcode=bytes([0xE9]),  # JMP
                    immediate=inst2.immediate
                )]
                
        return None
        
    def _invert_condition_code(self, cc: int) -> int:
        """反转条件码"""
        invert_map = {
            0x84: 0x85,  # JZ -> JNZ
            0x85: 0x84,  # JNZ -> JZ
            0x8C: 0x8D,  # JL -> JGE
            0x8D: 0x8C,  # JGE -> JL
            0x8E: 0x8F,  # JLE -> JG
            0x8F: 0x8E   # JG -> JLE
        }
        return invert_map.get(cc, cc)

    def _optimize_memory_access(self, window: List[X86Instruction]) -> Optional[List[X86Instruction]]:
        """优化内存访问指令
        
        模式1：
        MOV reg1, [mem]
        MOV [mem], reg1
        ->
        MOV reg1, [mem]
        
        模式2：
        MOV [mem], reg1
        MOV reg2, [mem]
        ->
        MOV [mem], reg1
        MOV reg2, reg1
        
        模式3：
        LEA reg1, [mem]
        MOV reg2, [reg1]
        ->
        MOV reg2, [mem]
        
        模式4：
        MOV reg1, imm
        MOV [mem], reg1
        ->
        MOV DWORD PTR [mem], imm
        """
        if len(window) < 2:
            return None
            
        inst1, inst2 = window[0], window[1]
        
        # 检查模式1：读后写相同位置
        if (inst1.opcode[1] == 0x8B and  # MOV reg, [mem]
            inst2.opcode[1] == 0x89):    # MOV [mem], reg
            # 获取源和目标寄存器
            dst1 = (inst1.modrm >> 3) & 0x7
            src2 = (inst2.modrm >> 3) & 0x7
            # 检查内存操作数是否相同
            if (dst1 == src2 and
                inst1.modrm & 0x07 == inst2.modrm & 0x07 and
                inst1.sib == inst2.sib and
                inst1.displacement == inst2.displacement):
                return [inst1]
                
        # 检查模式2：写后读相同位置
        if (inst1.opcode[1] == 0x89 and  # MOV [mem], reg1
            inst2.opcode[1] == 0x8B):    # MOV reg2, [mem]
            src1 = (inst1.modrm >> 3) & 0x7
            dst2 = (inst2.modrm >> 3) & 0x7
            # 检查内存操作数是否相同
            if (inst1.modrm & 0x07 == inst2.modrm & 0x07 and
                inst1.sib == inst2.sib and
                inst1.displacement == inst2.displacement):
                return [
                    inst1,
                    X86Instruction(
                        opcode=bytes([0x48, 0x89]),  # MOV reg2, reg1
                        modrm=0xC0 | (src1 << 3) | dst2
                    )
                ]
                
        # 检查模式3：LEA后跟MOV
        if (inst1.opcode[1] == 0x8D and  # LEA reg1, [mem]
            inst2.opcode[1] == 0x8B):    # MOV reg2, [reg1]
            dst1 = (inst1.modrm >> 3) & 0x7
            dst2 = (inst2.modrm >> 3) & 0x7
            base = inst2.modrm & 0x07
            if dst1 == base and not inst2.sib:
                return [X86Instruction(
                    opcode=bytes([0x48, 0x8B]),  # MOV reg2, [mem]
                    modrm=(inst1.modrm & 0x07) | (dst2 << 3),
                    sib=inst1.sib,
                    displacement=inst1.displacement
                )]
                
        # 检查模式4：立即数存储
        if (inst1.opcode[1] == 0xC7 and  # MOV reg1, imm
            inst2.opcode[1] == 0x89):    # MOV [mem], reg1
            src1 = inst1.modrm & 0x07
            src2 = (inst2.modrm >> 3) & 0x7
            if src1 == src2:
                return [X86Instruction(
                    opcode=bytes([0x48, 0xC7]),  # MOV QWORD PTR [mem], imm
                    modrm=inst2.modrm & 0x07,
                    sib=inst2.sib,
                    displacement=inst2.displacement,
                    immediate=inst1.immediate
                )]
                
        return None

class IRToX86Translator:
    """IR到x86指令的转换器"""
    
    def __init__(self):
        self.builder = X86Builder()
        self.current_function = None
        self.basic_blocks = {}  # 基本块映射
        self.register_map = {}  # 虚拟寄存器映射
        self.optimizer = X86PeepholeOptimizer()
        
    def translate_function(self, func: IRFunction) -> List[X86Instruction]:
        """转换IR函数到x86指令序列"""
        instructions = []
        
        # 生成函数序言
        instructions.extend(self._generate_prologue(func))
        
        # 转换每个基本块
        for block in func.blocks:
            instructions.extend(self._translate_block(block))
            
        # 生成函数结语
        instructions.extend(self._generate_epilogue(func))
        
        # 应用窥孔优化
        optimized = self.optimizer.optimize(instructions)
        
        return optimized
        
    def _translate_block(self, block: IRBasicBlock) -> List[X86Instruction]:
        """转换IR基本块到x86指令序列"""
        instructions = []
        
        # 添加块标签
        if block.label:
            instructions.append(X86Instruction(
                label=block.label
            ))
            
        # 转换每条IR指令
        for ir_inst in block.instructions:
            x86_insts = self._translate_instruction(ir_inst)
            instructions.extend(x86_insts)
            
            # 对每个指令窗口应用窥孔优化
            if len(instructions) >= 2:
                window = instructions[-2:]
                optimized = self.optimizer.optimize(window)
                if len(optimized) < len(window):
                    instructions[-2:] = optimized
                    
        return instructions
        
    def _translate_instruction(self, ir_inst: IRInstruction) -> List[X86Instruction]:
        """转换单条IR指令到x86指令序列"""
        instructions = super()._translate_instruction(ir_inst)
        
        # 对生成的指令序列应用窥孔优化
        if len(instructions) >= 2:
            optimized = self.optimizer.optimize(instructions)
            if len(optimized) < len(instructions):
                return optimized
                
        return instructions

    def _save_callee_saved_registers(self):
        """保存被调用者保存的寄存器"""
        for reg in [X86Register.RBX, X86Register.RBP, X86Register.R12,
                   X86Register.R13, X86Register.R14, X86Register.R15]:
            self.builder.push_reg(reg)
            
    def _restore_callee_saved_registers(self):
        """恢复被调用者保存的寄存器"""
        for reg in [X86Register.R15, X86Register.R14, X86Register.R13,
                   X86Register.R12, X86Register.RBP, X86Register.RBX]:
            self.builder.pop_reg(reg)
            
    def _setup_stack_frame(self):
        """设置栈帧"""
        # 保存旧的帧指针
        self.builder.push_reg(X86Register.RBP)
        # 设置新的帧指针
        self.builder.mov_reg_reg(X86Register.RBP, X86Register.RSP)
        # 为局部变量分配空间
        if self.current_function.stack_size > 0:
            self.builder.sub_reg_imm(X86Register.RSP, self.current_function.stack_size)
            
    def _cleanup_stack_frame(self):
        """清理栈帧"""
        # 恢复栈指针
        self.builder.mov_reg_reg(X86Register.RSP, X86Register.RBP)
        # 恢复旧的帧指针
        self.builder.pop_reg(X86Register.RBP)
        
    def _translate_mov(self, inst):
        """转换MOV指令"""
        dst = self._get_operand_location(inst.dst)
        src = self._get_operand_location(inst.src)
        
        if isinstance(dst, X86Register) and isinstance(src, X86Register):
            self.builder.mov_reg_reg(dst, src)
        elif isinstance(dst, X86Register) and isinstance(src, int):
            self.builder.mov_reg_imm(dst, src)
        elif isinstance(dst, X86Register) and isinstance(src, tuple):
            base, disp = src
            self.builder.mov_reg_mem(dst, base, disp)
        elif isinstance(dst, tuple) and isinstance(src, X86Register):
            base, disp = dst
            self.builder.mov_mem_reg(base, src, disp)
        else:
            raise ValueError(f"不支持的MOV操作数组合: {dst}, {src}")
            
    def _translate_lea(self, inst):
        """转换LEA指令"""
        dst = self._get_operand_location(inst.dst)
        src = self._get_operand_location(inst.src)
        
        if isinstance(dst, X86Register) and isinstance(src, tuple):
            base, disp = src
            self.builder.lea_reg_mem(dst, base, disp)
        else:
            raise ValueError(f"不支持的LEA操作数组合: {dst}, {src}")
            
    def _get_operand_location(self, operand):
        """获取操作数的位置（寄存器、内存地址或立即数）"""
        if operand.type == "register":
            return self._allocate_register(operand.value)
        elif operand.type == "memory":
            base = self._allocate_register(operand.base)
            return (base, operand.displacement)
        elif operand.type == "immediate":
            return operand.value
        else:
            raise ValueError(f"未知的操作数类型: {operand.type}")
            
    def _allocate_register(self, virtual_reg):
        """分配物理寄存器（简单版本）"""
        if virtual_reg not in self.register_map:
            # 这里使用简单的寄存器分配策略
            # 在实际实现中，应该使用更复杂的寄存器分配算法
            available_regs = [
                X86Register.RAX, X86Register.RCX, X86Register.RDX,
                X86Register.R8, X86Register.R9, X86Register.R10, X86Register.R11
            ]
            for reg in available_regs:
                if reg not in self.register_map.values():
                    self.register_map[virtual_reg] = reg
                    break
            else:
                raise ValueError("没有可用的物理寄存器")
        return self.register_map[virtual_reg]

def compile_to_x86(ir_file: str, output_file: str):
    """编译IR为x86-64机器码
    
    Args:
        ir_file: IR文件路径
        output_file: 输出文件路径
    """
    # 读取IR文件
    with open(ir_file, 'r', encoding='utf-8') as f:
        ir = f.read()
        
    # 创建x86构建器
    builder = X86Builder()
    
    # 解析IR
    current_function = None
    current_block = None
    
    for line in ir.splitlines():
        line = line.strip()
        if not line:
            continue
            
        # 解析函数定义
        if line.startswith('define'):
            current_function = line.split()[1].split('(')[0]
            builder.add_label(current_function)
            continue
            
        # 解析基本块
        if line.endswith(':'):
            current_block = line[:-1]
            builder.add_label(current_block)
            continue
            
        # 解析指令
        if not line.startswith(' '):
            continue
            
        parts = line.strip().split()
        if not parts:
            continue
            
        # 解析赋值
        dest = None
        if '=' in line:
            dest = line.split('=')[0].strip()
            parts = line.split('=')[1].strip().split()
            
        opcode = parts[0]
        operands = parts[1:]
        
        # 生成x86指令
        if opcode == 'mov':
            # mov dst, src
            if len(operands) != 1:
                raise ValueError(f"mov指令需要一个源操作数: {line}")
                
            src = operands[0]
            if src.startswith('reg_'):
                # 寄存器到寄存器
                src_reg = X86Register[src[4:].upper()]
                dst_reg = X86Register[dest[4:].upper()]
                builder.mov_reg_reg(dst_reg, src_reg)
            else:
                # 立即数到寄存器
                try:
                    imm = int(src)
                    dst_reg = X86Register[dest[4:].upper()]
                    builder.mov_reg_imm(dst_reg, imm)
                except ValueError:
                    raise ValueError(f"无效的立即数: {src}")
                    
        elif opcode == 'add':
            # add dst, src1, src2
            if len(operands) != 2:
                raise ValueError(f"add指令需要两个源操作数: {line}")
                
            src1 = operands[0]
            src2 = operands[1]
            
            if not src1.startswith('reg_') or not src2.startswith('reg_'):
                raise ValueError(f"add指令的操作数必须是寄存器: {line}")
                
            src1_reg = X86Register[src1[4:].upper()]
            src2_reg = X86Register[src2[4:].upper()]
            dst_reg = X86Register[dest[4:].upper()]
            
            builder.mov_reg_reg(dst_reg, src1_reg)
            builder.add_reg_reg(dst_reg, src2_reg)
            
        elif opcode == 'sub':
            # sub dst, src1, src2
            if len(operands) != 2:
                raise ValueError(f"sub指令需要两个源操作数: {line}")
                
            src1 = operands[0]
            src2 = operands[1]
            
            if not src1.startswith('reg_') or not src2.startswith('reg_'):
                raise ValueError(f"sub指令的操作数必须是寄存器: {line}")
                
            src1_reg = X86Register[src1[4:].upper()]
            src2_reg = X86Register[src2[4:].upper()]
            dst_reg = X86Register[dest[4:].upper()]
            
            builder.mov_reg_reg(dst_reg, src1_reg)
            builder.sub_reg_reg(dst_reg, src2_reg)
            
        elif opcode == 'mul':
            # mul dst, src1, src2
            if len(operands) != 2:
                raise ValueError(f"mul指令需要两个源操作数: {line}")
                
            src1 = operands[0]
            src2 = operands[1]
            
            if not src1.startswith('reg_') or not src2.startswith('reg_'):
                raise ValueError(f"mul指令的操作数必须是寄存器: {line}")
                
            src1_reg = X86Register[src1[4:].upper()]
            src2_reg = X86Register[src2[4:].upper()]
            dst_reg = X86Register[dest[4:].upper()]
            
            builder.mov_reg_reg(X86Register.RAX, src1_reg)
            builder.mul_reg(src2_reg)
            builder.mov_reg_reg(dst_reg, X86Register.RAX)
            
        elif opcode == 'div':
            # div dst, src1, src2
            if len(operands) != 2:
                raise ValueError(f"div指令需要两个源操作数: {line}")
                
            src1 = operands[0]
            src2 = operands[1]
            
            if not src1.startswith('reg_') or not src2.startswith('reg_'):
                raise ValueError(f"div指令的操作数必须是寄存器: {line}")
                
            src1_reg = X86Register[src1[4:].upper()]
            src2_reg = X86Register[src2[4:].upper()]
            dst_reg = X86Register[dest[4:].upper()]
            
            builder.mov_reg_reg(X86Register.RAX, src1_reg)
            builder.mov_reg_imm(X86Register.RDX, 0)  # 清零高位
            builder.div_reg(src2_reg)
            builder.mov_reg_reg(dst_reg, X86Register.RAX)
            
        elif opcode == 'call':
            # call func
            if len(operands) != 1:
                raise ValueError(f"call指令需要一个操作数: {line}")
                
            target = operands[0]
            builder.call_rel32(target)
            
        elif opcode == 'jmp':
            # jmp target
            if len(operands) != 1:
                raise ValueError(f"jmp指令需要一个操作数: {line}")
                
            target = operands[0]
            builder.jmp_rel32(target)
            
        elif opcode == 'ret':
            # ret [value]
            if operands:
                value = operands[0]
                if value.startswith('reg_'):
                    src_reg = X86Register[value[4:].upper()]
                    builder.mov_reg_reg(X86Register.RAX, src_reg)
            builder.ret()
            
        else:
            raise ValueError(f"未知的指令: {opcode}")
            
    # 修复标签引用
    builder.fix_labels()
    
    # 写入输出文件
    with open(output_file, 'wb') as f:
        f.write(builder.code)

def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: tasm_compile_to_x86.py <ir_file> [output_file]")
        sys.exit(1)
        
    ir_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else ir_file + '.x86'
    
    try:
        compile_to_x86(ir_file, output_file)
        print(f"编译成功: {output_file}")
    except Exception as e:
        print(f"编译错误: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main() 