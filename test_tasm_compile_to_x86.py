import unittest

class TestX86PeepholeOptimizer(unittest.TestCase):
    """测试x86窥孔优化器"""
    
    def setUp(self):
        self.optimizer = X86PeepholeOptimizer()
        
    def test_redundant_mov(self):
        """测试冗余MOV指令优化"""
        # 测试模式1：MOV reg1,reg2 + MOV reg2,reg1
        insts = [
            X86Instruction(opcode=bytes([0x48, 0x89]), modrm=0xC1),  # MOV rcx,rax
            X86Instruction(opcode=bytes([0x48, 0x89]), modrm=0xC8)   # MOV rax,rcx
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].opcode, insts[0].opcode)
        self.assertEqual(optimized[0].modrm, insts[0].modrm)
        
        # 测试模式2：MOV reg1,imm + MOV reg2,reg1
        insts = [
            X86Instruction(opcode=bytes([0x48, 0xC7]), modrm=0xC0, immediate=42),  # MOV rax,42
            X86Instruction(opcode=bytes([0x48, 0x89]), modrm=0xC1)                 # MOV rcx,rax
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].immediate, 42)
        
    def test_push_pop(self):
        """测试PUSH/POP序列优化"""
        # 测试模式1：PUSH reg1 + POP reg1
        insts = [
            X86Instruction(opcode=bytes([0x50])),  # PUSH rax
            X86Instruction(opcode=bytes([0x58]))   # POP rax
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 0)
        
        # 测试模式2：PUSH reg1 + PUSH reg2 + POP reg1 + POP reg2
        insts = [
            X86Instruction(opcode=bytes([0x50])),  # PUSH rax
            X86Instruction(opcode=bytes([0x51])),  # PUSH rcx
            X86Instruction(opcode=bytes([0x59])),  # POP rcx
            X86Instruction(opcode=bytes([0x58]))   # POP rax
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].opcode, bytes([0x48, 0x89]))  # MOV
        
    def test_test_jz(self):
        """测试TEST/JZ序列优化"""
        # 测试模式1：TEST reg,reg + JZ
        insts = [
            X86Instruction(opcode=bytes([0x48, 0x85]), modrm=0xC0),         # TEST rax,rax
            X86Instruction(opcode=bytes([0x0F, 0x84]), immediate=0x1234)    # JZ label
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 2)
        self.assertEqual(optimized[0].opcode, bytes([0x48, 0x83]))  # CMP
        self.assertEqual(optimized[0].immediate, 0)
        self.assertEqual(optimized[1].opcode, insts[1].opcode)
        
    def test_add_sub(self):
        """测试ADD/SUB序列优化"""
        # 测试模式1：ADD reg,imm1 + SUB reg,imm2
        insts = [
            X86Instruction(opcode=bytes([0x48, 0x81]), modrm=0xC0, immediate=100),  # ADD rax,100
            X86Instruction(opcode=bytes([0x48, 0x81]), modrm=0xE8, immediate=30)    # SUB rax,30
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].opcode, bytes([0x48, 0x81]))  # ADD
        self.assertEqual(optimized[0].immediate, 70)
        
        # 测试模式2：ADD reg,0
        insts = [
            X86Instruction(opcode=bytes([0x48, 0x81]), modrm=0xC0, immediate=0)  # ADD rax,0
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 0)
        
    def test_lea_complex(self):
        """测试LEA复杂寻址优化"""
        # 测试模式1：LEA reg1,[reg2 + reg2*1]
        insts = [
            X86Instruction(
                opcode=bytes([0x48, 0x8D]),
                modrm=0x04,
                sib=0x00 | (0 << 6) | (0 << 3) | 0  # [rax + rax*1]
            )
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].sib & 0xC0, 0x40)  # scale=2
        
        # 测试模式2：LEA reg1,[reg2 + reg2*2]
        insts = [
            X86Instruction(
                opcode=bytes([0x48, 0x8D]),
                modrm=0x04,
                sib=0x00 | (1 << 6) | (0 << 3) | 0  # [rax + rax*2]
            )
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].sib & 0xC0, 0x80)  # scale=4 

    def test_memory_access(self):
        """测试内存访问优化"""
        # 测试模式1：读后写相同位置
        insts = [
            X86Instruction(opcode=bytes([0x48, 0x8B]), modrm=0x45, displacement=8),  # MOV rax, [rbp+8]
            X86Instruction(opcode=bytes([0x48, 0x89]), modrm=0x45, displacement=8)   # MOV [rbp+8], rax
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].opcode, insts[0].opcode)
        self.assertEqual(optimized[0].modrm, insts[0].modrm)
        self.assertEqual(optimized[0].displacement, insts[0].displacement)
        
        # 测试模式2：写后读相同位置
        insts = [
            X86Instruction(opcode=bytes([0x48, 0x89]), modrm=0x45, displacement=8),  # MOV [rbp+8], rax
            X86Instruction(opcode=bytes([0x48, 0x8B]), modrm=0x4D, displacement=8)   # MOV rcx, [rbp+8]
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 2)
        self.assertEqual(optimized[0].opcode, insts[0].opcode)
        self.assertEqual(optimized[1].opcode, bytes([0x48, 0x89]))  # MOV rcx, rax
        self.assertEqual(optimized[1].modrm & 0x07, 1)  # rcx
        
        # 测试模式3：LEA后跟MOV
        insts = [
            X86Instruction(opcode=bytes([0x48, 0x8D]), modrm=0x45, displacement=8),  # LEA rax, [rbp+8]
            X86Instruction(opcode=bytes([0x48, 0x8B]), modrm=0x08)                   # MOV rcx, [rax]
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].opcode, bytes([0x48, 0x8B]))  # MOV
        self.assertEqual(optimized[0].modrm, 0x4D)  # [rbp+disp]
        self.assertEqual(optimized[0].displacement, 8)
        
        # 测试模式4：立即数存储
        insts = [
            X86Instruction(opcode=bytes([0x48, 0xC7]), modrm=0xC0, immediate=42),  # MOV rax, 42
            X86Instruction(opcode=bytes([0x48, 0x89]), modrm=0x45, displacement=8)  # MOV [rbp+8], rax
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].opcode, bytes([0x48, 0xC7]))  # MOV QWORD PTR
        self.assertEqual(optimized[0].modrm, 0x45)  # [rbp+disp]
        self.assertEqual(optimized[0].displacement, 8)
        self.assertEqual(optimized[0].immediate, 42)
        
        # 测试复杂内存访问模式
        insts = [
            X86Instruction(  # MOV rax, [rbp+rcx*4+8]
                opcode=bytes([0x48, 0x8B]),
                modrm=0x44,
                sib=0x8D,  # scale=2, index=rcx, base=rbp
                displacement=8
            ),
            X86Instruction(  # MOV [rbp+rcx*4+8], rax
                opcode=bytes([0x48, 0x89]),
                modrm=0x44,
                sib=0x8D,
                displacement=8
            )
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].opcode, insts[0].opcode)
        self.assertEqual(optimized[0].modrm, insts[0].modrm)
        self.assertEqual(optimized[0].sib, insts[0].sib)
        self.assertEqual(optimized[0].displacement, insts[0].displacement)

    def test_jump_chain(self):
        """测试跳转链优化"""
        # 测试模式1：条件跳转后跟无条件跳转
        insts = [
            X86Instruction(opcode=bytes([0x0F, 0x84]), immediate="label1"),  # JZ label1
            X86Instruction(opcode=bytes([0xE9]), immediate="label2"),       # JMP label2
            X86Instruction(label="label1", opcode=bytes([0xE9]), immediate="label3")  # label1: JMP label3
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 2)
        self.assertEqual(optimized[0].opcode, bytes([0x0F, 0x85]))  # JNZ
        self.assertEqual(optimized[0].immediate, "label2")
        self.assertEqual(optimized[1].immediate, "label3")
        
        # 测试模式2：跳转到跳转
        insts = [
            X86Instruction(opcode=bytes([0x0F, 0x84]), immediate="label1"),  # JZ label1
            X86Instruction(opcode=bytes([0x90])),  # NOP
            X86Instruction(label="label1", opcode=bytes([0xE9]), immediate="label2")  # label1: JMP label2
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 2)
        self.assertEqual(optimized[0].opcode, bytes([0x0F, 0x84]))  # JZ
        self.assertEqual(optimized[0].immediate, "label2")
        
        # 测试模式3：无条件跳转链
        insts = [
            X86Instruction(opcode=bytes([0xE9]), immediate="label1"),  # JMP label1
            X86Instruction(label="label1", opcode=bytes([0xE9]), immediate="label2")  # label1: JMP label2
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 1)
        self.assertEqual(optimized[0].opcode, bytes([0xE9]))  # JMP
        self.assertEqual(optimized[0].immediate, "label2")
        
        # 测试复杂条件跳转链
        insts = [
            X86Instruction(opcode=bytes([0x0F, 0x8C]), immediate="label1"),  # JL label1
            X86Instruction(opcode=bytes([0xE9]), immediate="label2"),       # JMP label2
            X86Instruction(label="label1", opcode=bytes([0xE9]), immediate="label3"),  # label1: JMP label3
            X86Instruction(label="label2", opcode=bytes([0xE9]), immediate="label4")   # label2: JMP label4
        ]
        optimized = self.optimizer.optimize(insts)
        self.assertEqual(len(optimized), 2)
        self.assertEqual(optimized[0].opcode, bytes([0x0F, 0x8D]))  # JGE
        self.assertEqual(optimized[0].immediate, "label4")  # 跳转到最终目标
        self.assertEqual(optimized[1].immediate, "label3") 