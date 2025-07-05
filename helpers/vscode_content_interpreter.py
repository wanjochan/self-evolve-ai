#!/usr/bin/env python3
"""
VSCode窗口内容智能解释器
基于UI元素检测结果，智能推断VSCode窗口中的具体内容和状态
"""

import json
from collections import defaultdict

def interpret_vscode_content(json_file):
    """智能解释VSCode窗口内容"""
    with open(json_file, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    elements = data.get('elements', [])
    window_info = data.get('window', {})
    
    print("🔍 VSCode窗口内容智能解释")
    print("=" * 60)
    print(f"窗口: {window_info.get('title', 'Unknown')}")
    print(f"检测到 {len(elements)} 个UI元素")
    print()
    
    # 分析窗口布局
    layout = analyze_layout(elements)
    
    # 推断具体内容
    content_analysis = infer_content(layout, elements)
    
    # 输出分析结果
    print_analysis_results(content_analysis)
    
    # 推断当前工作状态
    work_state = infer_work_state(content_analysis)
    print_work_state(work_state)

def analyze_layout(elements):
    """分析窗口布局结构"""
    layout = {
        'title_bar': {'elements': [], 'area': (0, 0, 960, 35)},
        'activity_bar': {'elements': [], 'area': (0, 35, 50, 950)},
        'sidebar': {'elements': [], 'area': (50, 35, 300, 950)},
        'editor_area': {'elements': [], 'area': (300, 35, 960, 650)},
        'terminal_panel': {'elements': [], 'area': (300, 650, 960, 950)},
        'status_bar': {'elements': [], 'area': (0, 950, 960, 1019)}
    }
    
    for element in elements:
        pos = element['position']
        x, y = pos['x1'], pos['y1']
        
        # 根据位置分配到相应区域
        if y < 35:
            layout['title_bar']['elements'].append(element)
        elif y >= 950:
            layout['status_bar']['elements'].append(element)
        elif x < 50:
            layout['activity_bar']['elements'].append(element)
        elif x < 300:
            layout['sidebar']['elements'].append(element)
        elif y < 650:
            layout['editor_area']['elements'].append(element)
        else:
            layout['terminal_panel']['elements'].append(element)
    
    return layout

def infer_content(layout, elements):
    """推断具体内容"""
    analysis = {}
    
    # 分析标题栏
    analysis['title_bar'] = analyze_title_bar(layout['title_bar']['elements'])
    
    # 分析活动栏
    analysis['activity_bar'] = analyze_activity_bar(layout['activity_bar']['elements'])
    
    # 分析侧边栏
    analysis['sidebar'] = analyze_sidebar(layout['sidebar']['elements'])
    
    # 分析编辑器区域
    analysis['editor_area'] = analyze_editor_area(layout['editor_area']['elements'])
    
    # 分析终端面板
    analysis['terminal_panel'] = analyze_terminal_panel(layout['terminal_panel']['elements'])
    
    # 分析状态栏
    analysis['status_bar'] = analyze_status_bar(layout['status_bar']['elements'])
    
    return analysis

def analyze_title_bar(elements):
    """分析标题栏内容"""
    tabs = [e for e in elements if e['type'] == 'tab']
    buttons = [e for e in elements if e['type'] == 'button']
    
    analysis = {
        'open_files': len(tabs),
        'window_controls': len(buttons),
        'has_tabs': len(tabs) > 0
    }
    
    if tabs:
        # 分析标签页宽度来推断文件数量
        tab = tabs[0]
        tab_width = tab['position']['x2'] - tab['position']['x1']
        if tab_width > 300:
            analysis['estimated_files'] = "多个文件打开"
        else:
            analysis['estimated_files'] = "1-2个文件"
    
    return analysis

def analyze_activity_bar(elements):
    """分析活动栏内容"""
    buttons = [e for e in elements if e['type'] == 'button']
    
    # 按Y坐标排序
    sorted_buttons = sorted(buttons, key=lambda x: x['position']['y1'])
    
    functions = [
        "文件资源管理器", "搜索", "源代码管理", "运行和调试", 
        "扩展", "设置", "更多功能"
    ]
    
    analysis = {
        'total_buttons': len(buttons),
        'available_functions': []
    }
    
    for i, button in enumerate(sorted_buttons):
        if i < len(functions):
            analysis['available_functions'].append({
                'name': functions[i],
                'position': button['position'],
                'confidence': button['confidence']
            })
    
    return analysis

def analyze_sidebar(elements):
    """分析侧边栏内容"""
    # 侧边栏通常在x=50-300区域
    analysis = {
        'is_open': len(elements) > 0,
        'content_type': 'unknown'
    }
    
    if elements:
        # 根据元素类型推断侧边栏内容
        text_elements = [e for e in elements if e['type'] == 'text']
        icons = [e for e in elements if e['type'] == 'icon']
        
        if len(text_elements) > 5:
            analysis['content_type'] = 'file_explorer'
            analysis['description'] = '文件资源管理器已打开'
        elif len(icons) > 3:
            analysis['content_type'] = 'extension_panel'
            analysis['description'] = '扩展面板或其他功能面板'
        else:
            analysis['content_type'] = 'minimal'
            analysis['description'] = '侧边栏最小化或空白'
    else:
        analysis['description'] = '侧边栏已关闭'
    
    return analysis

def analyze_editor_area(elements):
    """分析编辑器区域内容"""
    text_elements = [e for e in elements if e['type'] == 'text']
    links = [e for e in elements if e['type'] == 'link']
    icons = [e for e in elements if e['type'] == 'icon']
    
    analysis = {
        'has_content': len(elements) > 0,
        'text_areas': len(text_elements),
        'links': len(links),
        'icons': len(icons)
    }
    
    # 查找大的文本区域（主编辑器内容）
    large_text = [e for e in text_elements 
                  if (e['position']['x2'] - e['position']['x1']) * 
                     (e['position']['y2'] - e['position']['y1']) > 50000]
    
    if large_text:
        analysis['main_editor'] = {
            'active': True,
            'content_area': large_text[0]['position'],
            'estimated_content': '代码或文本文件'
        }
    else:
        analysis['main_editor'] = {
            'active': False,
            'estimated_content': '欢迎页面或空白编辑器'
        }
    
    # 分析链接（可能是欢迎页面的链接）
    if links:
        analysis['welcome_page'] = {
            'likely': True,
            'links_count': len(links),
            'description': '可能显示VSCode欢迎页面'
        }
    
    return analysis

def analyze_terminal_panel(elements):
    """分析终端面板内容"""
    icons = [e for e in elements if e['type'] == 'icon']
    text_elements = [e for e in elements if e['type'] == 'text']
    buttons = [e for e in elements if e['type'] == 'button']
    
    analysis = {
        'is_open': len(elements) > 0,
        'icons': len(icons),
        'text_lines': len(text_elements),
        'controls': len(buttons)
    }
    
    if len(elements) > 10:
        analysis['status'] = 'active'
        analysis['description'] = '终端面板已打开且活跃'
        
        if len(icons) > 20:
            analysis['content_type'] = 'rich_output'
            analysis['description'] += '，包含丰富的输出内容'
        elif len(text_elements) > 5:
            analysis['content_type'] = 'text_output'
            analysis['description'] += '，主要是文本输出'
    elif len(elements) > 0:
        analysis['status'] = 'minimal'
        analysis['description'] = '终端面板已打开但内容较少'
    else:
        analysis['status'] = 'closed'
        analysis['description'] = '终端面板已关闭'
    
    return analysis

def analyze_status_bar(elements):
    """分析状态栏内容"""
    text_elements = [e for e in elements if e['type'] == 'text']
    
    analysis = {
        'info_sections': len(text_elements),
        'content': []
    }
    
    # 按X坐标排序状态栏元素
    sorted_elements = sorted(text_elements, key=lambda x: x['position']['x1'])
    
    status_info = [
        "分支信息", "文件编码", "行列位置", "语言模式", 
        "缩进设置", "错误警告", "通知", "其他状态"
    ]
    
    for i, element in enumerate(sorted_elements):
        if i < len(status_info):
            analysis['content'].append({
                'type': status_info[i],
                'position': element['position'],
                'width': element['position']['x2'] - element['position']['x1']
            })
    
    return analysis

def print_analysis_results(analysis):
    """输出分析结果"""
    print("📋 窗口布局分析")
    print("-" * 40)
    
    # 标题栏
    title_bar = analysis['title_bar']
    print(f"标题栏: {title_bar['estimated_files']} | 窗口控制按钮: {title_bar['window_controls']}个")
    
    # 活动栏
    activity_bar = analysis['activity_bar']
    print(f"活动栏: {activity_bar['total_buttons']}个功能按钮")
    
    # 侧边栏
    sidebar = analysis['sidebar']
    print(f"侧边栏: {sidebar['description']}")
    
    # 编辑器区域
    editor = analysis['editor_area']
    if editor['main_editor']['active']:
        print(f"编辑器: 活跃 - {editor['main_editor']['estimated_content']}")
    else:
        print(f"编辑器: {editor['main_editor']['estimated_content']}")
    
    if 'welcome_page' in editor and editor['welcome_page']['likely']:
        print(f"  └─ {editor['welcome_page']['description']}")
    
    # 终端面板
    terminal = analysis['terminal_panel']
    print(f"终端: {terminal['description']}")
    
    # 状态栏
    status_bar = analysis['status_bar']
    print(f"状态栏: {status_bar['info_sections']}个信息区域")
    print()

def infer_work_state(analysis):
    """推断当前工作状态"""
    state = {
        'activity_level': 'unknown',
        'primary_task': 'unknown',
        'recommendations': []
    }
    
    editor = analysis['editor_area']
    terminal = analysis['terminal_panel']
    sidebar = analysis['sidebar']
    
    # 判断活跃程度
    if editor['main_editor']['active'] and terminal['status'] == 'active':
        state['activity_level'] = 'high'
        state['primary_task'] = 'active_development'
        state['description'] = '正在进行活跃的开发工作'
    elif editor['main_editor']['active']:
        state['activity_level'] = 'medium'
        state['primary_task'] = 'coding'
        state['description'] = '主要在编辑器中工作'
    elif terminal['status'] == 'active':
        state['activity_level'] = 'medium'
        state['primary_task'] = 'terminal_work'
        state['description'] = '主要在终端中工作'
    elif 'welcome_page' in editor and editor['welcome_page']['likely']:
        state['activity_level'] = 'low'
        state['primary_task'] = 'browsing'
        state['description'] = '浏览VSCode欢迎页面'
    else:
        state['activity_level'] = 'low'
        state['primary_task'] = 'idle'
        state['description'] = '相对空闲状态'
    
    # 生成建议
    if state['primary_task'] == 'active_development':
        state['recommendations'] = [
            "继续当前的开发工作",
            "注意终端输出信息",
            "适时保存文件"
        ]
    elif state['primary_task'] == 'browsing':
        state['recommendations'] = [
            "可以开始新的项目",
            "查看最近的文件",
            "探索VSCode功能"
        ]
    
    return state

def print_work_state(state):
    """输出工作状态分析"""
    print("🔮 工作状态推断")
    print("-" * 40)
    
    activity_icons = {
        'high': '🔥',
        'medium': '⚡',
        'low': '😴',
        'unknown': '❓'
    }
    
    icon = activity_icons.get(state['activity_level'], '❓')
    print(f"活跃程度: {icon} {state['activity_level'].upper()}")
    print(f"主要任务: {state['primary_task']}")
    print(f"状态描述: {state['description']}")
    
    if state['recommendations']:
        print("\n💡 建议:")
        for rec in state['recommendations']:
            print(f"  • {rec}")

if __name__ == "__main__":
    interpret_vscode_content('current_vscode_analysis.json')
