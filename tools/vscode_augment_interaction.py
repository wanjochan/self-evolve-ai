#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode Augment对话框交互工具
检测左边的augment对话框，查找特定提示并自动回复
"""

import sys
import os
sys.path.append(os.path.join(os.path.dirname(__file__), 'tools'))

from uictrl import UIController
import win32gui
import time

def find_vscode_window():
    """查找VSCode窗口"""
    controller = UIController()
    ide_windows = controller.find_ide_windows()
    
    for window in ide_windows:
        if 'visual studio code' in window['title'].lower():
            return window
    return None

def analyze_augment_dialog():
    """分析VSCode的augment对话框"""
    vscode_window = find_vscode_window()
    if not vscode_window:
        print("未找到VSCode窗口")
        return False
    
    print(f"找到VSCode窗口: {vscode_window['title']}")
    
    # 获取截图进行分析
    controller = UIController()
    screenshot_result = controller.take_screenshot(vscode_window['id'], "vscode_augment_check.png")
    print(f"截图已保存: {screenshot_result}")
    
    # 基于VSCode布局，推测augment对话框位置
    # 通常在左侧sidebar区域，大约在x=48到x=298之间
    hwnd = int(vscode_window['id'])
    rect = win32gui.GetWindowRect(hwnd)
    window_width = rect[2] - rect[0]
    window_height = rect[3] - rect[1]
    
    # 推测的augment对话框区域（左侧sidebar）
    augment_region = {
        'x': 48,   # activity bar后面
        'y': 100,  # 菜单栏下方
        'width': 250,  # sidebar宽度
        'height': window_height - 200  # 留出状态栏空间
    }
    
    print(f"推测augment对话框区域: {augment_region}")
    return vscode_window, augment_region

def interact_with_augment_dialog():
    """与augment对话框交互"""
    result = analyze_augment_dialog()
    if not result:
        return False
    
    vscode_window, augment_region = result
    controller = UIController()
    
    print("\n=== 尝试与augment对话框交互 ===")
    
    # 首先确保VSCode窗口获得焦点
    print("1. 激活VSCode窗口...")
    try:
        hwnd = int(vscode_window['id'])
        win32gui.SetForegroundWindow(hwnd)
        time.sleep(0.5)
        print("VSCode窗口已激活")
    except Exception as e:
        print(f"激活窗口失败: {e}")
    
    # 点击augment对话框区域来确保焦点
    print("2. 点击augment对话框区域...")
    try:
        # 点击左侧sidebar中间位置
        click_x = augment_region['x'] + augment_region['width'] // 2
        click_y = augment_region['y'] + augment_region['height'] // 2
        
        # 使用相对于窗口的坐标点击
        result = controller.send_mouse_input(vscode_window['id'], 'click', click_x, click_y)
        print(f"点击结果: {result}")
        time.sleep(0.5)
    except Exception as e:
        print(f"点击失败: {e}")
    
    # 尝试在可能的输入区域输入"continue"
    print("3. 输入 'continue'...")
    try:
        # 先尝试直接输入
        type_result = controller.type_text(vscode_window['id'], "continue")
        print(f"输入结果: {type_result}")
        time.sleep(0.5)
        
        # 按回车键
        print("4. 按回车键...")
        enter_result = controller.send_keys(vscode_window['id'], "enter")
        print(f"回车结果: {enter_result}")
        
    except Exception as e:
        print(f"输入失败: {e}")
    
    # 再次截图确认结果
    print("5. 截图确认结果...")
    final_screenshot = controller.take_screenshot(vscode_window['id'], "vscode_after_interaction.png")
    print(f"最终截图: {final_screenshot}")
    
    return True

def smart_augment_interaction():
    """智能augment交互 - 尝试多种方法"""
    vscode_window = find_vscode_window()
    if not vscode_window:
        return False
    
    controller = UIController()
    print("=== 智能augment对话框交互 ===")
    
    # 方法1: 尝试使用Tab键导航到输入框
    print("方法1: 使用Tab键导航...")
    try:
        # 按几次Tab键来导航到可能的输入框
        for i in range(3):
            controller.send_keys(vscode_window['id'], "tab")
            time.sleep(0.2)
        
        # 输入continue
        controller.type_text(vscode_window['id'], "continue")
        time.sleep(0.3)
        controller.send_keys(vscode_window['id'], "enter")
        print("方法1完成")
        
    except Exception as e:
        print(f"方法1失败: {e}")
    
    # 方法2: 尝试点击左侧不同位置
    print("方法2: 点击左侧不同区域...")
    try:
        hwnd = int(vscode_window['id'])
        rect = win32gui.GetWindowRect(hwnd)
        
        # 尝试点击左侧几个不同的位置
        positions = [
            (150, 300),  # 左侧上方
            (150, 400),  # 左侧中间
            (150, 500),  # 左侧下方
        ]
        
        for x, y in positions:
            try:
                # 点击位置
                win32gui.SetCursorPos(rect[0] + x, rect[1] + y)
                time.sleep(0.1)
                controller.send_keys(vscode_window['id'], "click")
                time.sleep(0.2)
                
                # 尝试输入
                controller.type_text(vscode_window['id'], "continue")
                time.sleep(0.1)
                controller.send_keys(vscode_window['id'], "enter")
                time.sleep(0.3)
                
            except Exception as e:
                continue
        
        print("方法2完成")
        
    except Exception as e:
        print(f"方法2失败: {e}")
    
    # 截图确认最终状态
    final_screenshot = controller.take_screenshot(vscode_window['id'], "vscode_smart_interaction_result.png")
    print(f"最终状态截图: {final_screenshot}")
    
    return True

if __name__ == "__main__":
    print("开始VSCode Augment对话框交互...")
    
    try:
        # 首先分析对话框
        analyze_augment_dialog()
        
        # 尝试标准交互
        interact_with_augment_dialog()
        
        # 尝试智能交互
        smart_augment_interaction()
        
        print("\n交互完成！请检查截图确认结果。")
        
    except Exception as e:
        print(f"交互失败: {e}")
        import traceback
        traceback.print_exc() 