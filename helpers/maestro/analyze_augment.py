#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import json
from pathlib import Path
import argparse
from PIL import Image

def extract_augment_dialog(screenshot_path, output_dir=None):
    """从VSCode截图中提取augment对话区域"""
    if not os.path.exists(screenshot_path):
        print(f"截图文件不存在: {screenshot_path}")
        return None
        
    # 加载截图
    img = Image.open(screenshot_path)
    
    # 定义可能的augment对话区域
    # 这些坐标是基于之前分析的结果
    dialog_areas = [
        (496, 283, 839, 598),  # 元素47
    ]
    
    results = []
    
    # 提取每个可能的区域
    for i, area in enumerate(dialog_areas):
        cropped = img.crop(area)
        
        # 保存裁剪后的图像
        if output_dir:
            os.makedirs(output_dir, exist_ok=True)
            output_path = os.path.join(output_dir, f"augment_dialog_{i}.png")
            cropped.save(output_path)
            print(f"保存对话区域 {i} 到: {output_path}")
            
        results.append({
            "area": area,
            "size": (area[2] - area[0], area[3] - area[1]),
            "image_path": output_path if output_dir else None
        })
    
    return results

def analyze_vscode_window(json_path, output_dir=None):
    """分析VSCode窗口中的augment对话内容"""
    if not os.path.exists(json_path):
        print(f"JSON文件不存在: {json_path}")
        return
        
    # 加载JSON数据
    with open(json_path, 'r') as f:
        data = json.load(f)
    
    # 查找截图路径
    screenshot_path = data.get("screenshot_path")
    
    # 如果JSON中没有截图路径，尝试在当前目录查找
    if not screenshot_path or not os.path.exists(screenshot_path):
        # 从窗口标题构造可能的截图文件名
        window_title = data.get("window", {}).get("title", "")
        if window_title:
            possible_screenshot = f"{window_title.replace(' ', '_')}_screenshot.png"
            if os.path.exists(possible_screenshot):
                screenshot_path = possible_screenshot
                print(f"找到截图文件: {screenshot_path}")
            else:
                # 查找当前目录下的所有png文件
                png_files = [f for f in os.listdir('.') if f.endswith('_screenshot.png')]
                if png_files:
                    screenshot_path = png_files[0]
                    print(f"使用找到的截图文件: {screenshot_path}")
    
    if not screenshot_path or not os.path.exists(screenshot_path):
        print(f"截图文件不存在，尝试查找当前目录下的截图文件...")
        # 查找当前目录下的所有png文件
        png_files = [f for f in os.listdir('.') if f.endswith('.png')]
        if png_files:
            screenshot_path = png_files[0]
            print(f"使用找到的截图文件: {screenshot_path}")
        else:
            print("没有找到任何截图文件")
            return
    
    # 提取augment对话区域
    dialog_areas = extract_augment_dialog(screenshot_path, output_dir)
    
    if not dialog_areas:
        print("无法提取对话区域")
        return
    
    # 分析UI元素
    elements = data.get("elements", [])
    
    # 查找可能的augment对话相关元素
    augment_elements = []
    
    # 根据位置和类型筛选可能的augment对话元素
    for elem in elements:
        elem_type = elem.get("type")
        position = elem.get("position", {})
        x1, y1 = position.get("x1", 0), position.get("y1", 0)
        x2, y2 = position.get("x2", 0), position.get("y2", 0)
        
        # 检查元素是否在对话区域内
        for area in dialog_areas:
            if (x1 >= area["area"][0] and y1 >= area["area"][1] and 
                x2 <= area["area"][2] and y2 <= area["area"][3]):
                augment_elements.append(elem)
                break
    
    # 输出分析结果
    print(f"\n找到 {len(augment_elements)} 个可能的augment对话元素:")
    for i, elem in enumerate(augment_elements):
        elem_type = elem.get("type")
        position = elem.get("position", {})
        x1, y1 = position.get("x1", 0), position.get("y1", 0)
        x2, y2 = position.get("x2", 0), position.get("y2", 0)
        width, height = x2 - x1, y2 - y1
        confidence = elem.get("confidence", 0)
        
        print(f"元素 {i+1}: 类型={elem_type}, 位置=({x1}, {y1}, {x2}, {y2}), 大小={width}x{height}px, 置信度={confidence:.2f}")
    
    # 保存分析结果
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, "augment_analysis.json")
        with open(output_path, 'w') as f:
            json.dump({
                "dialog_areas": dialog_areas,
                "augment_elements": augment_elements
            }, f, indent=2)
        print(f"\n分析结果已保存到: {output_path}")

def main():
    parser = argparse.ArgumentParser(description="分析VSCode窗口中的augment对话内容")
    parser.add_argument("json_file", help="VSCode窗口分析结果的JSON文件")
    parser.add_argument("-o", "--output-dir", help="输出目录")
    
    args = parser.parse_args()
    
    analyze_vscode_window(args.json_file, args.output_dir)

if __name__ == "__main__":
    main() 