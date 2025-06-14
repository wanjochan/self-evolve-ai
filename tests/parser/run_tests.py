#!/usr/bin/env python3
"""
语法分析器测试运行器
用于自动化运行所有语法分析器相关的测试用例
"""

import os
import sys
import time
from pathlib import Path
from typing import List, Dict, Any

# 添加项目根目录到Python路径
project_root = Path(__file__).parent.parent.parent
sys.path.append(str(project_root))

from tasm_common import Parser, CompileError, Node

class ParserTestRunner:
    def __init__(self):
        self.test_dir = Path(__file__).parent
        self.total_tests = 0
        self.passed_tests = 0
        self.failed_tests = 0
        self.skipped_tests = 0
        self.performance_metrics = {}
        
    def run_parser_test(self, source: str, filename: str) -> Node:
        """运行语法分析器测试"""
        parser = Parser(source, filename)
        return parser.parse()
        
    def run_positive_test(self, test_file: Path):
        """运行期望成功的测试"""
        print(f"\n运行正向测试: {test_file.name}")
        try:
            with open(test_file, 'r', encoding='utf-8') as f:
                source = f.read()
            ast = self.run_parser_test(source, str(test_file))
            
            # 验证基本语法结构
            has_section = False
            has_label = False
            has_instruction = False
            has_data_def = False
            has_expression = False
            
            # 遍历AST验证各种节点类型
            def validate_node(node: Node):
                nonlocal has_section, has_label, has_instruction
                nonlocal has_data_def, has_expression
                
                if node.type == 'SECTION':
                    has_section = True
                elif node.type == 'LABEL':
                    has_label = True
                elif node.type == 'INSTRUCTION':
                    has_instruction = True
                elif node.type in ['DB', 'DW', 'DD', 'DQ']:
                    has_data_def = True
                elif node.type == 'EXPRESSION':
                    has_expression = True
                    
                for child in node.children:
                    validate_node(child)
                    
            validate_node(ast)
            
            # 检查是否覆盖了所有基本语法结构
            if not has_section:
                print("✗ 警告: 未测试段声明")
            if not has_label:
                print("✗ 警告: 未测试标签")
            if not has_instruction:
                print("✗ 警告: 未测试指令")
            if not has_data_def:
                print("✗ 警告: 未测试数据定义")
            if not has_expression:
                print("✗ 警告: 未测试表达式")
                
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
            ast = self.run_parser_test(source, str(test_file))
            print("✗ 测试失败: 期望出现语法错误但分析成功了")
            self.failed_tests += 1
        except CompileError as e:
            print(f"✓ 测试通过: 正确捕获到预期的语法错误")
            print(f"  错误信息: {e}")
            self.passed_tests += 1
        except Exception as e:
            print(f"✗ 测试异常: {e}")
            self.failed_tests += 1
            
    def run_error_recovery_test(self, test_file: Path):
        """运行错误恢复测试"""
        print(f"\n运行错误恢复测试: {test_file.name}")
        try:
            with open(test_file, 'r', encoding='utf-8') as f:
                source = f.read()
            
            # 分析源代码中的错误和有效部分
            lines = source.split('\n')
            error_lines = [i+1 for i, line in enumerate(lines) if '# 应该能正确识别' not in line and 
                          ('未闭合' in line or 'illegal' in line or 'invalid' in line or 
                           '非法' in line or '错误' in line)]
            valid_lines = [i+1 for i, line in enumerate(lines) if '# 应该能正确识别' in line]
            
            # 创建语法分析器并尝试分析整个文件
            parser = Parser(source, str(test_file))
            nodes = []
            error_count = 0
            valid_node_count = 0
            
            # 尝试解析所有语句，记录错误
            while True:
                try:
                    node = parser.parse_statement()
                    if not node:
                        break
                    nodes.append(node)
                    
                    # 检查是否在有效行附近找到了节点
                    if any(abs(node.line - valid_line) <= 1 for valid_line in valid_lines):
                        valid_node_count += 1
                        
                except CompileError as e:
                    error_count += 1
                    print(f"  捕获到预期的语法错误: {e}")
                    # 模拟错误恢复 - 跳过当前语句
                    parser.synchronize()
            
            # 验证结果
            if error_count == 0:
                print("✗ 测试失败: 未捕获到任何预期的语法错误")
                self.failed_tests += 1
                return
                
            if valid_node_count == 0:
                print("✗ 测试失败: 未能识别任何有效的语法结构")
                self.failed_tests += 1
                return
                
            recovery_rate = valid_node_count / (valid_node_count + error_count)
            print(f"✓ 错误恢复测试通过")
            print(f"  捕获错误数: {error_count}")
            print(f"  识别有效语法结构数: {valid_node_count}")
            print(f"  恢复率: {recovery_rate:.2%}")
            self.passed_tests += 1
            
        except Exception as e:
            print(f"✗ 测试异常: {e}")
            self.failed_tests += 1
    
    def run_performance_test(self, test_file: Path):
        """运行性能测试"""
        print(f"\n运行性能测试: {test_file.name}")
        try:
            with open(test_file, 'r', encoding='utf-8') as f:
                source = f.read()
            
            # 记录开始时间和内存使用
            start_time = time.time()
            
            # 运行语法分析
            ast = self.run_parser_test(source, str(test_file))
            
            # 计算性能指标
            end_time = time.time()
            elapsed_time = end_time - start_time
            nodes_per_second = len(str(ast).split('\n')) / elapsed_time if elapsed_time > 0 else 0
            chars_per_second = len(source) / elapsed_time if elapsed_time > 0 else 0
            
            # 记录性能指标
            self.performance_metrics[test_file.name] = {
                'nodes': len(str(ast).split('\n')),
                'chars': len(source),
                'time': elapsed_time,
                'nodes_per_second': nodes_per_second,
                'chars_per_second': chars_per_second
            }
            
            print(f"✓ 性能测试完成")
            print(f"  处理字符数: {len(source)}")
            print(f"  生成AST节点数: {len(str(ast).split('\n'))}")
            print(f"  处理时间: {elapsed_time:.4f}秒")
            print(f"  处理速度: {nodes_per_second:.2f}节点/秒, {chars_per_second:.2f}字符/秒")
            self.passed_tests += 1
            
        except Exception as e:
            print(f"✗ 测试异常: {e}")
            self.failed_tests += 1
    
    def run_all_tests(self):
        """运行所有测试用例"""
        print("开始运行语法分析器测试...")
        
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
        
        # 运行错误恢复测试
        recovery_test = self.test_dir / "test_error_recovery.tasm"
        if recovery_test.exists():
            self.total_tests += 1
            self.run_error_recovery_test(recovery_test)
        
        # 运行性能测试
        performance_test = self.test_dir / "test_performance.tasm"
        if performance_test.exists():
            self.total_tests += 1
            self.run_performance_test(performance_test)
            
        # 打印测试结果统计
        print("\n测试结果汇总:")
        print(f"总测试数: {self.total_tests}")
        print(f"通过数: {self.passed_tests}")
        print(f"失败数: {self.failed_tests}")
        print(f"跳过数: {self.skipped_tests}")
        
        # 打印性能测试结果
        if self.performance_metrics:
            print("\n性能测试结果:")
            for test_name, metrics in self.performance_metrics.items():
                print(f"  {test_name}:")
                print(f"    处理时间: {metrics['time']:.4f}秒")
                print(f"    处理速度: {metrics['nodes_per_second']:.2f}节点/秒")
                print(f"              {metrics['chars_per_second']:.2f}字符/秒")
        
        # 返回测试是否全部通过
        return self.failed_tests == 0

def main():
    runner = ParserTestRunner()
    success = runner.run_all_tests()
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main() 