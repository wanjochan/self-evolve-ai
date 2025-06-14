#!/usr/bin/env python3
"""
词法分析器测试运行器
用于自动化运行所有词法分析器相关的测试用例
"""

import os
import sys
from pathlib import Path
from typing import List

# 添加项目根目录到Python路径
project_root = Path(__file__).parent.parent.parent
sys.path.append(str(project_root))

from tasm_common import Lexer, TokenType, CompileError

class LexerTestRunner:
    def __init__(self):
        self.test_dir = Path(__file__).parent
        self.total_tests = 0
        self.passed_tests = 0
        self.failed_tests = 0
        
    def run_lexer_test(self, source: str, filename: str) -> List[Token]:
        """运行词法分析器测试"""
        lexer = Lexer(source, filename)
        tokens = []
        while True:
            token = lexer.get_next_token()
            tokens.append(token)
            if token.type == TokenType.EOF:
                break
        return tokens
        
    def run_positive_test(self, test_file: Path):
        """运行期望成功的测试"""
        print(f"\n运行正向测试: {test_file.name}")
        try:
            with open(test_file, 'r', encoding='utf-8') as f:
                source = f.read()
            tokens = self.run_lexer_test(source, str(test_file))
            
            # 验证基本功能
            has_section = False
            has_label = False
            has_instruction = False
            has_register = False
            has_number = False
            has_string = False
            
            for token in tokens:
                if token.type == TokenType.SECTION:
                    has_section = True
                elif token.type == TokenType.IDENTIFIER and \
                     self.next_token_is(tokens, token, TokenType.COLON):
                    has_label = True
                elif token.type == TokenType.IDENTIFIER:
                    has_instruction = True
                elif token.type == TokenType.REGISTER:
                    has_register = True
                elif token.type == TokenType.NUMBER:
                    has_number = True
                elif token.type == TokenType.STRING:
                    has_string = True
                    
            # 检查是否覆盖了所有基本功能
            if not has_section:
                print("✗ 警告: 未测试段声明")
            if not has_label:
                print("✗ 警告: 未测试标签")
            if not has_instruction:
                print("✗ 警告: 未测试指令")
            if not has_register:
                print("✗ 警告: 未测试寄存器")
            if not has_number:
                print("✗ 警告: 未测试数字")
            if not has_string:
                print("✗ 警告: 未测试字符串")
                
            print("✓ 测试通过")
            self.passed_tests += 1
            
        except CompileError as e:
            print(f"✗ 测试失败: {e}")
            self.failed_tests += 1
        except Exception as e:
            print(f"✗ 测试异常: {e}")
            self.failed_tests += 1
            
    def run_negative_test(self, test_file: Path):
        """运行期望失败的测试"""
        print(f"\n运行错误处理测试: {test_file.name}")
        try:
            with open(test_file, 'r', encoding='utf-8') as f:
                source = f.read()
            tokens = self.run_lexer_test(source, str(test_file))
            print("✗ 测试失败: 期望出现词法错误但分析成功了")
            self.failed_tests += 1
        except CompileError as e:
            print(f"✓ 测试通过: 正确捕获到预期的词法错误")
            print(f"  错误信息: {e}")
            self.passed_tests += 1
        except Exception as e:
            print(f"✗ 测试异常: {e}")
            self.failed_tests += 1
            
    def next_token_is(self, tokens: List[Token], current: Token, 
                     expected_type: TokenType) -> bool:
        """检查下一个词法单元的类型"""
        try:
            idx = tokens.index(current)
            return idx + 1 < len(tokens) and tokens[idx + 1].type == expected_type
        except ValueError:
            return False
            
    def run_all_tests(self):
        """运行所有测试用例"""
        print("开始运行词法分析器测试...")
        
        # 运行基本功能测试（期望成功的测试）
        basic_test = self.test_dir / "test_basic.tasm"
        if basic_test.exists():
            self.total_tests += 1
            self.run_positive_test(basic_test)
            
        # 运行错误处理测试（期望失败的测试）
        error_test = self.test_dir / "test_error_handling.tasm"
        if error_test.exists():
            self.total_tests += 1
            self.run_negative_test(error_test)
            
        # 打印测试结果统计
        print("\n测试结果汇总:")
        print(f"总测试数: {self.total_tests}")
        print(f"通过数: {self.passed_tests}")
        print(f"失败数: {self.failed_tests}")
        
        # 返回测试是否全部通过
        return self.failed_tests == 0

def main():
    runner = LexerTestRunner()
    success = runner.run_all_tests()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main() 