#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
词法分析器调试工具
"""

import sys
from tasm_common import Lexer, TokenType

def debug_lexer(filename):
    """调试词法分析器"""
    print(f"调试文件: {filename}")
    
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            source = f.read()
        
        lexer = Lexer(source, filename)
        tokens = []
        
        print("\n词法分析结果:")
        print("-" * 60)
        print(f"{'类型':<15} {'值':<20} {'行':<5} {'列':<5}")
        print("-" * 60)
        
        while True:
            token = lexer.get_next_token()
            tokens.append(token)
            print(f"{token.type.name:<15} {token.value:<20} {token.line:<5} {token.column:<5}")
            
            if token.type == TokenType.EOF:
                break
        
        print("-" * 60)
        print(f"总计词法单元数: {len(tokens)}")
        
        # 统计各类型词法单元数量
        token_types = {}
        for token in tokens:
            if token.type not in token_types:
                token_types[token.type] = 0
            token_types[token.type] += 1
        
        print("\n词法单元类型统计:")
        for token_type, count in token_types.items():
            print(f"{token_type.name:<15}: {count}")
        
    except Exception as e:
        print(f"错误: {e}")
        return False
    
    return True

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("用法: python debug_lexer.py <文件名>")
        sys.exit(1)
    
    success = debug_lexer(sys.argv[1])
    sys.exit(0 if success else 1)
