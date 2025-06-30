#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode窗口分析和测试工具
参考test_cursor_fix.py的方法
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__)))

from tools.uictrl import UIController
import win32gui
import json

def analyze_vscode_detailed():
    """详细分析VSCode窗口"""
    controller = UIController()
    controller.verbose = True
    
    print("=== VSCode详细分析 ===")
    
    # 查找IDE窗口
    print("1. 查找IDE窗口...")
    ide_windows = controller.find_ide_windows()
    print(f"发现 {len(ide_windows)} 个IDE窗口:")
    
    vscode_window = None
    for window in ide_windows:
        print(f"  - {window['title']} (ID: {window['id']})")
        if 'visual studio code' in window['title'].lower():
            vscode_window = window
    
    if not vscode_window:
        print("未找到VSCode窗口")
        return None
    
    print(f"\n2. 分析VSCode窗口: {vscode_window['title']}")
    
    # 获取窗口详细信息
    hwnd = int(vscode_window['id'])
    
    # 基本窗口信息
    window_info = {
        'id': vscode_window['id'],
        'title': vscode_window['title'],
        'class_name': win32gui.GetClassName(hwnd),
        'rect': win32gui.GetWindowRect(hwnd),
        'is_visible': win32gui.IsWindowVisible(hwnd),
        'is_enabled': win32gui.IsWindowEnabled(hwnd)
    }
    
    print("窗口基本信息:")
    print(f"  - 类名: {window_info['class_name']}")
    print(f"  - 位置: {window_info['rect']}")
    print(f"  - 可见: {window_info['is_visible']}")
    print(f"  - 启用: {window_info['is_enabled']}")
    
    # 分析子窗口
    children = []
    def enum_callback(child_hwnd, _):
        if win32gui.IsWindowVisible(child_hwnd):
            child_info = {
                'hwnd': child_hwnd,
                'class_name': win32gui.GetClassName(child_hwnd),
                'text': win32gui.GetWindowText(child_hwnd),
                'rect': win32gui.GetWindowRect(child_hwnd)
            }
            children.append(child_info)
        return True
    
    win32gui.EnumChildWindows(hwnd, enum_callback, None)
    
    print(f"\n3. 子窗口分析 (共{len(children)}个):")
    for i, child in enumerate(children):
        text = child['text'][:30] if child['text'] else "(无文本)"
        print(f"  {i+1:2d}. {child['class_name']:25} - {text}")
    
    return {
        'window_info': window_info,
        'children': children
    }

def test_vscode_interaction():
    """测试VSCode交互功能"""
    controller = UIController()
    controller.verbose = True
    
    print("\n=== 测试VSCode交互 ===")
    
    # 查找VSCode窗口
    ide_windows = controller.find_ide_windows()
    vscode_window = None
    for window in ide_windows:
        if 'visual studio code' in window['title'].lower():
            vscode_window = window
            break
    
    if not vscode_window:
        print("未找到VSCode窗口")
        return
    
    print(f"找到VSCode窗口: {vscode_window['title']}")
    
    # 测试截图功能
    print("\n1. 测试截图功能...")
    try:
        screenshot_result = controller.take_screenshot(vscode_window['id'], "vscode_test_screenshot.png")
        print(f"截图结果: {screenshot_result}")
    except Exception as e:
        print(f"截图失败: {e}")
    
    # 测试按键发送
    print("\n2. 测试按键发送...")
    try:
        # 发送Ctrl+Shift+P打开命令面板
        keys_result = controller.send_keys(vscode_window['id'], "ctrl+shift+p")
        print(f"按键结果: {keys_result}")
    except Exception as e:
        print(f"按键发送失败: {e}")

def analyze_vscode_ui_elements():
    """分析VSCode的UI元素布局"""
    print("\n=== VSCode UI元素布局分析 ===")
    
    # 查找VSCode窗口
    controller = UIController()
    ide_windows = controller.find_ide_windows()
    vscode_window = None
    for window in ide_windows:
        if 'visual studio code' in window['title'].lower():
            vscode_window = window
            break
    
    if not vscode_window:
        print("未找到VSCode窗口")
        return
    
    hwnd = int(vscode_window['id'])
    rect = win32gui.GetWindowRect(hwnd)
    width = rect[2] - rect[0]
    height = rect[3] - rect[1]
    
    print(f"VSCode窗口尺寸: {width}x{height}")
    
    # 基于VSCode典型布局推测区域
    ui_regions = {
        'title_bar': {'x': 0, 'y': 0, 'width': width, 'height': 30},
        'menu_bar': {'x': 0, 'y': 30, 'width': width, 'height': 25},
        'activity_bar': {'x': 0, 'y': 55, 'width': 48, 'height': height-85},
        'sidebar': {'x': 48, 'y': 55, 'width': 250, 'height': height-85},
        'editor_area': {'x': 298, 'y': 55, 'width': width-298, 'height': height-115},
        'status_bar': {'x': 0, 'y': height-30, 'width': width, 'height': 30}
    }
    
    print("推测的UI区域布局:")
    for region_name, region in ui_regions.items():
        print(f"  {region_name:12}: x={region['x']:3d}, y={region['y']:3d}, "
              f"w={region['width']:3d}, h={region['height']:3d}")
    
    return ui_regions

if __name__ == "__main__":
    print("开始VSCode详细分析...")
    
    try:
        # 详细分析
        analysis_result = analyze_vscode_detailed()
        
        # UI布局分析
        ui_layout = analyze_vscode_ui_elements()
        
        # 交互测试
        test_vscode_interaction()
        
        print("\n=== 分析完成 ===")
        
        if analysis_result:
            print(f"窗口信息已保存，共分析了 {len(analysis_result['children'])} 个子组件")
        
    except Exception as e:
        print(f"分析失败: {e}")
        import traceback
        traceback.print_exc() 