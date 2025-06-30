#!/usr/bin/env python
# -*- coding: utf-8 -*-

import win32gui

def enum_child_windows(parent_hwnd):
    """枚举子窗口"""
    children = []
    def callback(hwnd, _):
        if win32gui.IsWindowVisible(hwnd):
            text = win32gui.GetWindowText(hwnd)
            class_name = win32gui.GetClassName(hwnd)
            rect = win32gui.GetWindowRect(hwnd)
            children.append({
                'hwnd': hwnd,
                'text': text,
                'class': class_name,
                'rect': rect
            })
        return True
    
    win32gui.EnumChildWindows(parent_hwnd, callback, None)
    return children

def analyze_vscode_window(hwnd):
    """分析VSCode窗口"""
    print(f"分析VSCode窗口: {hwnd}")
    print(f"窗口标题: {win32gui.GetWindowText(hwnd)}")
    print(f"窗口类名: {win32gui.GetClassName(hwnd)}")
    
    # 获取窗口位置和大小
    rect = win32gui.GetWindowRect(hwnd)
    print(f"窗口位置: ({rect[0]}, {rect[1]}) 大小: {rect[2]-rect[0]}x{rect[3]-rect[1]}")
    
    # 枚举子窗口
    children = enum_child_windows(hwnd)
    print(f"\n找到 {len(children)} 个子窗口组件:")
    
    for i, child in enumerate(children):
        text = child['text'][:50] if child['text'] else "(无文本)"
        print(f"{i+1:2d}. 类名: {child['class']:20} 文本: {text}")
        if i >= 15:  # 限制显示数量
            print("    ... (更多组件)")
            break

if __name__ == "__main__":
    vscode_hwnd = 136055040
    analyze_vscode_window(vscode_hwnd) 