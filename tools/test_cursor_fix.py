#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
测试Cursor中文输入修复
"""

import sys
import os
sys.path.append(os.path.dirname(__file__))

from uictrl import UIController

def test_cursor_chat():
    """测试Cursor聊天功能"""
    controller = UIController()
    controller.verbose = True
    
    print("=== 测试Cursor聊天功能 ===")
    
    # 查找IDE窗口
    print("1. 查找IDE窗口...")
    ide_windows = controller.find_ide_windows()
    print(f"发现 {len(ide_windows)} 个IDE窗口:")
    for window in ide_windows:
        print(f"  - {window['title']} (ID: {window['id']})")
    
    # 测试中文输入
    print("\n2. 测试中文输入...")
    result = controller.cursor_chat("你好，这是一个测试")
    print(f"结果: {result}")
    
    return result

def test_direct_input():
    """测试直接输入功能"""
    controller = UIController()
    controller.verbose = True
    
    print("\n=== 测试直接输入功能 ===")
    
    # 查找Cursor窗口
    ide_windows = controller.find_ide_windows()
    cursor_window = None
    for window in ide_windows:
        if 'cursor' in window['title'].lower():
            cursor_window = window
            break
    
    if not cursor_window:
        print("未找到Cursor窗口")
        return
    
    print(f"找到Cursor窗口: {cursor_window['title']}")
    
    # 测试输入中文
    result = controller.type_text(cursor_window['id'], "测试中文输入")
    print(f"输入结果: {result}")

if __name__ == "__main__":
    print("开始测试修复后的Cursor中文输入功能...")
    
    try:
        # 测试聊天功能
        test_cursor_chat()
        
        # 测试直接输入
        test_direct_input()
        
        print("\n测试完成!")
        
    except Exception as e:
        print(f"测试失败: {e}")
        import traceback
        traceback.print_exc() 