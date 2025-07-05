#!/usr/bin/env python3
"""
分析VSCode窗口内容
基于UI元素检测结果推断窗口中的具体内容和状态
"""

import json
from collections import defaultdict

def analyze_vscode_content(json_file):
    """分析VSCode窗口内容"""
    with open(json_file, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    elements = data.get('elements', [])
    window_info = data.get('window', {})
    
    print("🔍 VSCode窗口内容分析")
    print("=" * 60)
    print(f"窗口标题: {window_info.get('title', 'Unknown')}")
    print(f"窗口ID: {window_info.get('hwnd', 'Unknown')}")
    print(f"进程ID: {window_info.get('pid', 'Unknown')}")
    print(f"检测到 {len(elements)} 个UI元素")
    print()
    
    # 按区域分组分析
    regions = analyze_by_regions(elements)
    
    # 分析各个区域
    analyze_title_bar(regions['title_bar'])
    analyze_activity_bar(regions['activity_bar'])
    analyze_tab_bar(regions['tab_bar'])
    analyze_main_content(regions['main_content'])
    analyze_terminal_area(regions['terminal_area'])
    analyze_status_bar(regions['status_bar'])
    
    # 推断当前状态
    infer_current_state(elements)

def analyze_by_regions(elements):
    """按区域分组UI元素"""
    regions = {
        'title_bar': [],      # y < 35
        'activity_bar': [],   # x < 50
        'tab_bar': [],        # 35 <= y < 100
        'main_content': [],   # 100 <= y < 650, x >= 50
        'terminal_area': [],  # 650 <= y < 950
        'status_bar': []      # y >= 950
    }
    
    for element in elements:
        pos = element['position']
        y = pos['y1']
        x = pos['x1']
        
        if y < 35:
            regions['title_bar'].append(element)
        elif y >= 950:
            regions['status_bar'].append(element)
        elif x < 50:
            regions['activity_bar'].append(element)
        elif 35 <= y < 100:
            regions['tab_bar'].append(element)
        elif 650 <= y < 950:
            regions['terminal_area'].append(element)
        else:
            regions['main_content'].append(element)
    
    return regions

def analyze_title_bar(elements):
    """分析标题栏"""
    print("📋 标题栏区域 (Title Bar)")
    print("-" * 30)
    if not elements:
        print("  未检测到标题栏元素")
        return
    
    buttons = [e for e in elements if e['type'] == 'button']
    tabs = [e for e in elements if e['type'] == 'tab']
    
    print(f"  检测到 {len(elements)} 个元素:")
    print(f"    - {len(buttons)} 个按钮 (窗口控制按钮)")
    print(f"    - {len(tabs)} 个标签页")
    
    if tabs:
        tab = tabs[0]
        pos = tab['position']
        print(f"    - 标签页区域: ({pos['x1']}, {pos['y1']}) 到 ({pos['x2']}, {pos['y2']})")
        print(f"    - 标签页宽度: {pos['x2'] - pos['x1']}px (可能有多个文件打开)")
    print()

def analyze_activity_bar(elements):
    """分析活动栏"""
    print("🎯 活动栏区域 (Activity Bar)")
    print("-" * 30)
    if not elements:
        print("  未检测到活动栏元素")
        return
    
    buttons = [e for e in elements if e['type'] == 'button']
    icons = [e for e in elements if e['type'] == 'icon']
    
    print(f"  检测到 {len(elements)} 个元素:")
    print(f"    - {len(buttons)} 个按钮")
    print(f"    - {len(icons)} 个图标")
    
    # 按Y坐标排序，推断功能
    sorted_elements = sorted(elements, key=lambda x: x['position']['y1'])
    
    functions = [
        "文件资源管理器", "搜索", "源代码管理", "运行和调试", 
        "扩展", "设置", "更多选项", "终端"
    ]
    
    print("  推断的功能按钮:")
    for i, element in enumerate(sorted_elements[:len(functions)]):
        pos = element['position']
        func_name = functions[i] if i < len(functions) else f"功能{i+1}"
        print(f"    - {func_name}: ({pos['x1']}, {pos['y1']}) - {element['type']}")
    print()

def analyze_tab_bar(elements):
    """分析标签页栏"""
    print("📑 标签页栏区域 (Tab Bar)")
    print("-" * 30)
    if not elements:
        print("  未检测到标签页元素")
        return
    
    buttons = [e for e in elements if e['type'] == 'button']
    print(f"  检测到 {len(elements)} 个元素:")
    print(f"    - {len(buttons)} 个标签页按钮")
    
    # 按X坐标排序
    sorted_tabs = sorted(buttons, key=lambda x: x['position']['x1'])
    
    print("  打开的文件标签页:")
    for i, tab in enumerate(sorted_tabs):
        pos = tab['position']
        width = pos['x2'] - pos['x1']
        print(f"    - 标签页 {i+1}: 位置({pos['x1']}, {pos['y1']}) 宽度:{width}px")
    print()

def analyze_main_content(elements):
    """分析主内容区域"""
    print("📝 主内容区域 (Editor Area)")
    print("-" * 30)
    if not elements:
        print("  未检测到主内容区域元素")
        return
    
    text_elements = [e for e in elements if e['type'] == 'text']
    links = [e for e in elements if e['type'] == 'link']
    icons = [e for e in elements if e['type'] == 'icon']
    
    print(f"  检测到 {len(elements)} 个元素:")
    print(f"    - {len(text_elements)} 个文本区域")
    print(f"    - {len(links)} 个链接")
    print(f"    - {len(icons)} 个图标")
    
    # 查找大的文本区域（可能是编辑器内容）
    large_text = [e for e in text_elements if (e['position']['x2'] - e['position']['x1']) * (e['position']['y2'] - e['position']['y1']) > 50000]
    
    if large_text:
        print("  主编辑器区域:")
        for text in large_text:
            pos = text['position']
            area = (pos['x2'] - pos['x1']) * (pos['y2'] - pos['y1'])
            print(f"    - 大文本区域: ({pos['x1']}, {pos['y1']}) 到 ({pos['x2']}, {pos['y2']}) 面积:{area}px²")
    
    if links:
        print("  可点击链接:")
        for link in links:
            pos = link['position']
            print(f"    - 链接: ({pos['x1']}, {pos['y1']}) 到 ({pos['x2']}, {pos['y2']})")
    print()

def analyze_terminal_area(elements):
    """分析终端区域"""
    print("💻 终端区域 (Terminal Area)")
    print("-" * 30)
    if not elements:
        print("  未检测到终端区域元素")
        return
    
    buttons = [e for e in elements if e['type'] == 'button']
    icons = [e for e in elements if e['type'] == 'icon']
    text_elements = [e for e in elements if e['type'] == 'text']
    
    print(f"  检测到 {len(elements)} 个元素:")
    print(f"    - {len(buttons)} 个按钮 (终端控制)")
    print(f"    - {len(icons)} 个图标")
    print(f"    - {len(text_elements)} 个文本 (终端输出)")
    
    if text_elements:
        print("  终端输出内容:")
        for text in text_elements:
            pos = text['position']
            print(f"    - 文本行: ({pos['x1']}, {pos['y1']}) 宽度:{pos['x2'] - pos['x1']}px")
    print()

def analyze_status_bar(elements):
    """分析状态栏"""
    print("📊 状态栏区域 (Status Bar)")
    print("-" * 30)
    if not elements:
        print("  未检测到状态栏元素")
        return
    
    text_elements = [e for e in elements if e['type'] == 'text']
    buttons = [e for e in elements if e['type'] == 'button']
    
    print(f"  检测到 {len(elements)} 个元素:")
    print(f"    - {len(text_elements)} 个文本信息")
    print(f"    - {len(buttons)} 个按钮")
    
    # 按X坐标排序
    sorted_elements = sorted(elements, key=lambda x: x['position']['x1'])
    
    print("  状态栏信息 (从左到右):")
    for i, element in enumerate(sorted_elements):
        pos = element['position']
        width = pos['x2'] - pos['x1']
        element_type = element['type']
        print(f"    - {element_type} {i+1}: 位置({pos['x1']}, {pos['y1']}) 宽度:{width}px")
    print()

def infer_current_state(elements):
    """推断当前VSCode状态"""
    print("🔮 当前状态推断")
    print("-" * 30)
    
    # 统计各类型元素
    type_counts = defaultdict(int)
    for element in elements:
        type_counts[element['type']] += 1
    
    # 分析窗口状态
    has_terminal = any(650 <= e['position']['y1'] < 950 for e in elements)
    has_large_content = any((e['position']['x2'] - e['position']['x1']) * (e['position']['y2'] - e['position']['y1']) > 50000 for e in elements if e['type'] == 'text')
    
    print("  界面状态:")
    print(f"    - 终端面板: {'打开' if has_terminal else '关闭'}")
    print(f"    - 主编辑器: {'有内容' if has_large_content else '空白或少量内容'}")
    print(f"    - 活动栏按钮: {type_counts['button']} 个")
    print(f"    - 可交互图标: {type_counts['icon']} 个")
    print(f"    - 文本区域: {type_counts['text']} 个")
    print(f"    - 链接元素: {type_counts['link']} 个")
    
    # 推断工作状态
    if has_terminal and has_large_content:
        print("  推断状态: 🔥 活跃开发中 (编辑器和终端都在使用)")
    elif has_large_content:
        print("  推断状态: 📝 代码编辑中 (主要在编辑器中工作)")
    elif has_terminal:
        print("  推断状态: 💻 终端操作中 (主要在终端中工作)")
    else:
        print("  推断状态: 🏠 待机状态 (界面相对空闲)")

if __name__ == "__main__":
    analyze_vscode_content('current_vscode_analysis.json')
