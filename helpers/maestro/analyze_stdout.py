#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import re
import subprocess
import json

def run_maestro_command(window_title="Visual Studio Code"):
    """运行maestro_cli.py命令并获取输出"""
    cmd = ["python", "maestro_cli.py", "detail", window_title]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        return result.stdout
    except Exception as e:
        print(f"执行命令失败: {e}")
        return None

def parse_elements_from_output(output):
    """从输出中解析UI元素信息"""
    if not output:
        return []
        
    # 使用正则表达式匹配元素信息
    pattern = r"元素 (\d+): 类型=(\w+), 位置=\((\d+), (\d+), (\d+), (\d+)\), 置信度=([\d\.]+)"
    matches = re.findall(pattern, output)
    
    elements = []
    for match in matches:
        element_id, element_type, x1, y1, x2, y2, confidence = match
        elements.append({
            "id": int(element_id),
            "type": element_type,
            "position": {
                "x1": int(x1),
                "y1": int(y1),
                "x2": int(x2),
                "y2": int(y2)
            },
            "confidence": float(confidence)
        })
    
    return elements

def find_augment_dialog_elements(elements):
    """查找可能的augment对话元素"""
    # 查找大型文本区域，可能是对话内容
    text_elements = [e for e in elements if e["type"] == "text"]
    large_text_elements = []
    
    for elem in text_elements:
        width = elem["position"]["x2"] - elem["position"]["x1"]
        height = elem["position"]["y2"] - elem["position"]["y1"]
        area = width * height
        
        # 如果面积大于一定阈值，可能是对话区域
        if area > 10000:  # 调整阈值
            large_text_elements.append({
                **elem,
                "width": width,
                "height": height,
                "area": area
            })
    
    # 按面积排序
    large_text_elements.sort(key=lambda e: e["area"], reverse=True)
    
    return large_text_elements

def analyze_vscode_window():
    """分析VSCode窗口中的augment对话内容"""
    # 运行命令获取输出
    output = run_maestro_command()
    
    if not output:
        print("无法获取命令输出")
        return
    
    # 解析元素信息
    elements = parse_elements_from_output(output)
    
    if not elements:
        print("未检测到UI元素")
        return
    
    # 查找可能的augment对话元素
    dialog_elements = find_augment_dialog_elements(elements)
    
    # 输出分析结果
    print(f"\n找到 {len(elements)} 个UI元素，其中 {len(dialog_elements)} 个可能是augment对话区域:")
    
    for i, elem in enumerate(dialog_elements):
        elem_id = elem["id"]
        elem_type = elem["type"]
        x1, y1 = elem["position"]["x1"], elem["position"]["y1"]
        x2, y2 = elem["position"]["x2"], elem["position"]["y2"]
        width, height = elem["width"], elem["height"]
        confidence = elem["confidence"]
        
        print(f"对话区域 {i+1} (元素 {elem_id}): 类型={elem_type}, 位置=({x1}, {y1}, {x2}, {y2}), 大小={width}x{height}px, 置信度={confidence:.2f}")
    
    # 查找按钮元素，可能是对话中的操作按钮
    button_elements = [e for e in elements if e["type"] == "button"]
    
    print(f"\n找到 {len(button_elements)} 个按钮元素，可能是对话中的操作按钮:")
    
    for i, elem in enumerate(button_elements[:5]):  # 只显示前5个
        elem_id = elem["id"]
        x1, y1 = elem["position"]["x1"], elem["position"]["y1"]
        x2, y2 = elem["position"]["x2"], elem["position"]["y2"]
        width = x2 - x1
        height = y2 - y1
        confidence = elem["confidence"]
        
        print(f"按钮 {i+1} (元素 {elem_id}): 位置=({x1}, {y1}, {x2}, {y2}), 大小={width}x{height}px, 置信度={confidence:.2f}")

if __name__ == "__main__":
    analyze_vscode_window() 