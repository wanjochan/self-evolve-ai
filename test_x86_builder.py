import unittest
from tasm_compile_to_x86 import X86Builder, X86Register, X86Scale

class TestX86Builder(unittest.TestCase):
    def setUp(self):
        self.builder = X86Builder()
        
    def test_mov_reg_mem(self):
        """测试从内存加载到寄存器的指令编码"""
        # MOV rax, [rbx]
        self.builder.mov_reg_mem(X86Register.RAX, X86Register.RBX)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8B, 0x03]))
        
        # MOV rcx, [rdx+0x12]
        self.builder = X86Builder()  # 重置
        self.builder.mov_reg_mem(X86Register.RCX, X86Register.RDX, 0x12)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8B, 0x4A, 0x12]))
        
        # MOV rsi, [rdi+0x1234]
        self.builder = X86Builder()
        self.builder.mov_reg_mem(X86Register.RSI, X86Register.RDI, 0x1234)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8B, 0xB7, 0x34, 0x12, 0x00, 0x00]))
        
    def test_mov_mem_reg(self):
        """测试存储寄存器到内存的指令编码"""
        # MOV [rbx], rax
        self.builder.mov_mem_reg(X86Register.RBX, X86Register.RAX)
        self.assertEqual(self.builder.code, bytes([0x48, 0x89, 0x03]))
        
        # MOV [rdx+0x12], rcx
        self.builder = X86Builder()
        self.builder.mov_mem_reg(X86Register.RDX, X86Register.RCX, 0x12)
        self.assertEqual(self.builder.code, bytes([0x48, 0x89, 0x4A, 0x12]))
        
    def test_lea_reg_mem(self):
        """测试LEA指令编码"""
        # LEA rax, [rbx]
        self.builder.lea_reg_mem(X86Register.RAX, X86Register.RBX)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8D, 0x03]))
        
        # LEA rcx, [rdx+0x12]
        self.builder = X86Builder()
        self.builder.lea_reg_mem(X86Register.RCX, X86Register.RDX, 0x12)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8D, 0x4A, 0x12]))
        
    def test_logical_ops(self):
        """测试逻辑运算指令编码"""
        # XOR rax, rbx
        self.builder.xor_reg_reg(X86Register.RAX, X86Register.RBX)
        self.assertEqual(self.builder.code, bytes([0x48, 0x31, 0xD8]))
        
        # TEST rcx, rdx
        self.builder = X86Builder()
        self.builder.test_reg_reg(X86Register.RCX, X86Register.RDX)
        self.assertEqual(self.builder.code, bytes([0x48, 0x85, 0xD1]))
        
        # CMP rsi, rdi
        self.builder = X86Builder()
        self.builder.cmp_reg_reg(X86Register.RSI, X86Register.RDI)
        self.assertEqual(self.builder.code, bytes([0x48, 0x39, 0xFE]))
        
    def test_conditional_jumps(self):
        """测试条件跳转指令编码"""
        # 创建一个标签
        self.builder.add_label("target")
        
        # JL target
        self.builder.jl_rel32("target")
        self.assertEqual(self.builder.code[0:2], bytes([0x0F, 0x8C]))
        
        # JLE target
        self.builder = X86Builder()
        self.builder.add_label("target")
        self.builder.jle_rel32("target")
        self.assertEqual(self.builder.code[0:2], bytes([0x0F, 0x8E]))
        
        # JZ target
        self.builder = X86Builder()
        self.builder.add_label("target")
        self.builder.jz_rel32("target")
        self.assertEqual(self.builder.code[0:2], bytes([0x0F, 0x84]))
        
        # JNZ target
        self.builder = X86Builder()
        self.builder.add_label("target")
        self.builder.jnz_rel32("target")
        self.assertEqual(self.builder.code[0:2], bytes([0x0F, 0x85]))

    def test_mov_reg_mem_sib(self):
        """测试带SIB字节的内存加载指令"""
        # MOV rax, [rbx + rcx*1]
        self.builder.mov_reg_mem_sib(X86Register.RAX, X86Register.RBX,
                                   X86Register.RCX, X86Scale.S1)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8B, 0x04, 0x0B]))
        
        # MOV rdx, [rsi + rdi*2 + 0x12]
        self.builder = X86Builder()
        self.builder.mov_reg_mem_sib(X86Register.RDX, X86Register.RSI,
                                   X86Register.RDI, X86Scale.S2, 0x12)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8B, 0x54, 0x7E, 0x12]))
        
        # MOV r8, [r9 + r10*4 + 0x1234]
        self.builder = X86Builder()
        self.builder.mov_reg_mem_sib(X86Register.R8, X86Register.R9,
                                   X86Register.R10, X86Scale.S4, 0x1234)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8B, 0x84, 0x91, 0x34, 0x12, 0x00, 0x00]))
        
    def test_mov_mem_sib_reg(self):
        """测试带SIB字节的内存存储指令"""
        # MOV [rbx + rcx*1], rax
        self.builder.mov_mem_sib_reg(X86Register.RBX, X86Register.RCX,
                                   X86Scale.S1, X86Register.RAX)
        self.assertEqual(self.builder.code, bytes([0x48, 0x89, 0x04, 0x0B]))
        
        # MOV [rsi + rdi*2 + 0x12], rdx
        self.builder = X86Builder()
        self.builder.mov_mem_sib_reg(X86Register.RSI, X86Register.RDI,
                                   X86Scale.S2, X86Register.RDX, 0x12)
        self.assertEqual(self.builder.code, bytes([0x48, 0x89, 0x54, 0x7E, 0x12]))
        
    def test_lea_reg_mem_sib(self):
        """测试带SIB字节的LEA指令"""
        # LEA rax, [rbx + rcx*8]
        self.builder.lea_reg_mem_sib(X86Register.RAX, X86Register.RBX,
                                   X86Register.RCX, X86Scale.S8)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8D, 0x04, 0xCB]))
        
        # LEA rdx, [rsi + rdi*4 + 0x12]
        self.builder = X86Builder()
        self.builder.lea_reg_mem_sib(X86Register.RDX, X86Register.RSI,
                                   X86Register.RDI, X86Scale.S4, 0x12)
        self.assertEqual(self.builder.code, bytes([0x48, 0x8D, 0x54, 0xBE, 0x12]))

if __name__ == '__main__':
    unittest.main() 