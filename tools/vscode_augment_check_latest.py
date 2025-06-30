#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode Augment Check Latest Tool
使用Windows UI Automation和组件分析获取VSCode左边augment对话框的最新内容

Requirements:
    pip install comtypes

Usage:
    python tools/vscode_augment_check_latest.py
    python tools/vscode_augment_check_latest.py --verbose
    python tools/vscode_augment_check_latest.py --save-screenshot
"""

import sys
import os
import argparse
import time
from datetime import datetime
import win32gui
import win32con
import win32api

# UI Automation imports
try:
    import comtypes.client
    from comtypes.gen import UIAutomationClient
    UI_AUTOMATION_AVAILABLE = True
except ImportError:
    UI_AUTOMATION_AVAILABLE = False

# 添加当前目录到路径，以便导入uictrl
sys.path.append(os.path.dirname(__file__))

from uictrl import UIController

def find_vscode_window():
    """查找VSCode窗口"""
    controller = UIController()
    ide_windows = controller.find_ide_windows()
    
    for window in ide_windows:
        if 'visual studio code' in window['title'].lower():
            return window
    return None

def take_augment_screenshot(window_id, verbose=False):
    """获取augment对话框区域截图"""
    controller = UIController()
    controller.verbose = verbose
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"tools/vscode_augment_latest_{timestamp}.png"
    
    try:
        result = controller.take_screenshot(window_id, filename)
        if verbose:
            print(f"截图保存: {result}")
        return filename
    except Exception as e:
        if verbose:
            print(f"截图失败: {e}")
        return None

def analyze_augment_content(screenshot_path, verbose=False):
    """分析augment对话框内容"""
    if not screenshot_path or not os.path.exists(screenshot_path):
        return None
    
    # 这里可以添加OCR或图像分析逻辑
    # 目前返回基本信息
    file_size = os.path.getsize(screenshot_path)
    
    analysis = {
        'screenshot_path': screenshot_path,
        'file_size': file_size,
        'timestamp': datetime.now().isoformat(),
        'status': 'screenshot_captured'
    }
    
    if verbose:
        print(f"分析结果:")
        print(f"  截图文件: {screenshot_path}")
        print(f"  文件大小: {file_size} bytes")
        print(f"  时间戳: {analysis['timestamp']}")
    
    return analysis

def extract_text_content_from_components(window_id, verbose=False):
    """使用Windows组件分析和UI Automation技术提取文本内容"""
    try:
        hwnd = int(window_id)
        
        if verbose:
            print("开始组件分析...")
        
        # 首先尝试UI Automation
        ui_automation_result = extract_with_ui_automation(hwnd, verbose)
        if ui_automation_result:
            return ui_automation_result
        
        # 回退到基本的Windows API方法
        return extract_with_basic_api(hwnd, verbose)
            
    except Exception as e:
        if verbose:
            print(f"组件分析失败: {e}")
        return None

def extract_with_ui_automation(hwnd, verbose=False):
    """使用UI Automation提取Electron应用内容"""
    if not UI_AUTOMATION_AVAILABLE:
        if verbose:
            print("UI Automation不可用，需要安装comtypes")
        return None
    
    try:
        if verbose:
            print("尝试UI Automation分析...")
        
        # 使用更简单的方法创建UI Automation
        import comtypes
        
        # 生成类型库
        try:
            comtypes.client.GetModule("UIAutomationCore.dll")
        except:
            if verbose:
                print("无法加载UI Automation类型库")
            return None
        
        # 创建UI Automation对象
        try:
            from comtypes.gen.UIAutomationClient import CUIAutomation, IUIAutomation
            automation = comtypes.client.CreateObject(CUIAutomation, interface=IUIAutomation)
        except Exception as e:
            if verbose:
                print(f"创建UI Automation对象失败: {e}")
            return None
        
        # 从窗口句柄获取元素
        try:
            element = automation.ElementFromHandle(hwnd)
            if not element:
                if verbose:
                    print("无法从窗口句柄获取UI元素")
                return None
        except Exception as e:
            if verbose:
                print(f"获取UI元素失败: {e}")
            return None
        
        # 收集所有文本内容
        text_contents = []
        
        # 查找所有文本元素
        text_condition = automation.CreatePropertyCondition(
            UIAutomationClient.UIA_ControlTypePropertyId,
            UIAutomationClient.UIA_TextControlTypeId
        )
        
        text_elements = element.FindAll(UIAutomationClient.TreeScope_Descendants, text_condition)
        
        if verbose:
            print(f"找到 {text_elements.Length} 个文本元素")
        
        for i in range(text_elements.Length):
            text_element = text_elements.GetElement(i)
            try:
                name = text_element.CurrentName
                value = ""
                
                # 尝试获取Value pattern
                try:
                    value_pattern = text_element.GetCurrentPattern(UIAutomationClient.UIA_ValuePatternId)
                    if value_pattern:
                        value = value_pattern.CurrentValue
                except:
                    pass
                
                # 尝试获取Text pattern
                try:
                    text_pattern = text_element.GetCurrentPattern(UIAutomationClient.UIA_TextPatternId)
                    if text_pattern:
                        text_range = text_pattern.DocumentRange
                        value = text_range.GetText(-1)
                except:
                    pass
                
                if name or value:
                    content = f"{name}: {value}" if (name and value) else (name or value)
                    if content.strip():
                        text_contents.append(content.strip())
                        if verbose:
                            print(f"  文本元素: {content[:60]}...")
                            
            except Exception as e:
                if verbose:
                    print(f"  处理文本元素时出错: {e}")
                continue
        
        # 查找编辑框和其他输入元素
        edit_condition = automation.CreatePropertyCondition(
            UIAutomationClient.UIA_ControlTypePropertyId,
            UIAutomationClient.UIA_EditControlTypeId
        )
        
        edit_elements = element.FindAll(UIAutomationClient.TreeScope_Descendants, edit_condition)
        
        if verbose:
            print(f"找到 {edit_elements.Length} 个编辑框元素")
        
        for i in range(edit_elements.Length):
            edit_element = edit_elements.GetElement(i)
            try:
                name = edit_element.CurrentName
                value = ""
                
                # 获取编辑框的值
                try:
                    value_pattern = edit_element.GetCurrentPattern(UIAutomationClient.UIA_ValuePatternId)
                    if value_pattern:
                        value = value_pattern.CurrentValue
                except:
                    pass
                
                if name or value:
                    content = f"[编辑框] {name}: {value}" if (name and value) else f"[编辑框] {name or value}"
                    if content.strip():
                        text_contents.append(content.strip())
                        if verbose:
                            print(f"  编辑框: {content[:60]}...")
                            
            except Exception as e:
                if verbose:
                    print(f"  处理编辑框时出错: {e}")
                continue
        
        # 查找按钮和其他交互元素
        button_condition = automation.CreatePropertyCondition(
            UIAutomationClient.UIA_ControlTypePropertyId,
            UIAutomationClient.UIA_ButtonControlTypeId
        )
        
        button_elements = element.FindAll(UIAutomationClient.TreeScope_Descendants, button_condition)
        
        if verbose:
            print(f"找到 {button_elements.Length} 个按钮元素")
        
        for i in range(button_elements.Length):
            button_element = button_elements.GetElement(i)
            try:
                name = button_element.CurrentName
                if name and name.strip():
                    text_contents.append(f"[按钮] {name.strip()}")
                    if verbose:
                        print(f"  按钮: {name[:40]}...")
                        
            except Exception as e:
                if verbose:
                    print(f"  处理按钮时出错: {e}")
                continue
        
        if text_contents:
            result = "\n".join(text_contents)
            if verbose:
                print(f"UI Automation成功提取了 {len(text_contents)} 个文本元素")
            return result
        else:
            if verbose:
                print("UI Automation未找到任何文本内容")
            return None
            
    except Exception as e:
        if verbose:
            print(f"UI Automation分析失败: {e}")
        return None
    finally:
        try:
            pythoncom.CoUninitialize()
        except:
            pass

def extract_with_basic_api(hwnd, verbose=False):
    """使用基本Windows API的回退方法"""
    try:
        if verbose:
            print("使用基本Windows API分析...")
        
        # 获取窗口信息
        window_text = win32gui.GetWindowText(hwnd)
        class_name = win32gui.GetClassName(hwnd)
        
        if verbose:
            print(f"主窗口: {window_text} ({class_name})")
        
        # 枚举所有子窗口组件
        components = []
        text_content = []
        
        def enum_child_callback(child_hwnd, _):
            try:
                if win32gui.IsWindowVisible(child_hwnd):
                    child_text = win32gui.GetWindowText(child_hwnd)
                    child_class = win32gui.GetClassName(child_hwnd)
                    child_rect = win32gui.GetWindowRect(child_hwnd)
                    
                    component_info = {
                        'hwnd': child_hwnd,
                        'text': child_text,
                        'class': child_class,
                        'rect': child_rect,
                        'size': (child_rect[2] - child_rect[0], child_rect[3] - child_rect[1])
                    }
                    
                    components.append(component_info)
                    
                    # 收集有文本内容的组件
                    if child_text and len(child_text.strip()) > 0:
                        text_content.append(f"[{child_class}] {child_text}")
                        
                        if verbose:
                            print(f"  发现文本组件: {child_class} -> {child_text[:50]}...")
            except:
                pass
            return True
        
        # 枚举子窗口
        win32gui.EnumChildWindows(hwnd, enum_child_callback, None)
        
        if verbose:
            print(f"找到 {len(components)} 个组件，{len(text_content)} 个包含文本")
        
        # 组合所有文本内容
        if text_content:
            combined_text = "\n".join(text_content)
            return combined_text
        else:
            return analyze_electron_app(hwnd, verbose)
            
    except Exception as e:
        if verbose:
            print(f"基本API分析失败: {e}")
        return None

def get_additional_text_content(hwnd, verbose=False):
    """尝试使用SendMessage获取额外的文本内容"""
    additional_texts = []
    
    try:
        # 对于Electron应用，尝试一些常见的消息
        messages_to_try = [
            win32con.WM_GETTEXT,
            win32con.WM_GETTEXTLENGTH,
        ]
        
        for msg in messages_to_try:
            try:
                # 这里可以尝试发送消息获取文本
                # 但Electron应用通常不响应这些标准消息
                pass
            except:
                continue
                
    except Exception as e:
        if verbose:
            print(f"获取额外文本内容失败: {e}")
    
    return additional_texts

def analyze_electron_app(hwnd, verbose=False):
    """专门分析Electron应用的方法"""
    try:
        if verbose:
            print("尝试Electron应用专用分析...")
        
        # 获取窗口标题作为基础信息
        window_title = win32gui.GetWindowText(hwnd)
        
        # 分析窗口结构
        rect = win32gui.GetWindowRect(hwnd)
        width = rect[2] - rect[0]
        height = rect[3] - rect[1]
        
        analysis_result = f"""
=== VSCode窗口组件分析 ===
窗口标题: {window_title}
窗口尺寸: {width}x{height}
窗口类型: Electron应用 (Chrome_WidgetWin_1)

由于VSCode是Electron应用，其内容主要在Chrome渲染进程中，
无法通过标准Windows API直接获取文本内容。

建议的替代方案:
1. 查看截图文件了解对话框内容
2. 使用键盘导航 (Tab键) 来定位到输入框
3. 使用快捷键与augment对话框交互

当前状态: 需要手动检查augment对话框内容
        """.strip()
        
        if verbose:
            print("Electron应用分析完成")
        
        return analysis_result
        
    except Exception as e:
        if verbose:
            print(f"Electron应用分析失败: {e}")
        return "无法分析Electron应用内容"

def analyze_image_basic(screenshot_path, verbose=False):
    """基本图像分析，提供截图的基本信息"""
    try:
        from PIL import Image
        
        image = Image.open(screenshot_path)
        width, height = image.size
        
        # 转换为RGB模式进行分析
        if image.mode != 'RGB':
            image = image.convert('RGB')
        
        # 简单的颜色分析
        pixels = list(image.getdata())
        total_pixels = len(pixels)
        
        # 统计主要颜色
        dark_pixels = sum(1 for r, g, b in pixels if r < 50 and g < 50 and b < 50)
        light_pixels = sum(1 for r, g, b in pixels if r > 200 and g > 200 and b > 200)
        
        dark_ratio = dark_pixels / total_pixels
        light_ratio = light_pixels / total_pixels
        
        # 分析左侧区域（augment对话框通常在左侧）
        left_region_width = min(300, width // 3)
        left_region = image.crop((0, 0, left_region_width, height))
        
        analysis_text = f"""
图像基本分析:
- 尺寸: {width}x{height}
- 总像素: {total_pixels:,}
- 深色像素比例: {dark_ratio:.2%}
- 浅色像素比例: {light_ratio:.2%}
- 左侧区域尺寸: {left_region_width}x{height}

提示: 要查看实际文本内容，请安装pytesseract:
pip install pytesseract

或者手动查看截图文件: {screenshot_path}
        """.strip()
        
        if verbose:
            print("基本图像分析完成")
        
        return analysis_text
        
    except Exception as e:
        if verbose:
            print(f"基本图像分析失败: {e}")
        return f"无法分析图像，请手动查看截图: {screenshot_path}"

def check_for_continue_prompt(text_content, verbose=False):
    """检查是否包含需要continue的提示"""
    if not text_content:
        return False
    
    continue_keywords = [
        "would you like me to keep going",
        "continue",
        "keep going",
        "shall i continue",
        "do you want me to continue"
    ]
    
    text_lower = text_content.lower()
    found_keywords = []
    
    for keyword in continue_keywords:
        if keyword in text_lower:
            found_keywords.append(keyword)
    
    if found_keywords and verbose:
        print(f"发现continue提示关键词: {found_keywords}")
    
    return len(found_keywords) > 0

def get_latest_augment_context(verbose=False):
    """获取最新的augment对话框内容"""
    if verbose:
        print("=== VSCode Augment Latest Context Check ===")
    
    # 查找VSCode窗口
    vscode_window = find_vscode_window()
    if not vscode_window:
        print("错误: 未找到VSCode窗口")
        return None
    
    if verbose:
        print(f"找到VSCode窗口: {vscode_window['title']} (ID: {vscode_window['id']})")
    
    # 获取截图
    screenshot_path = take_augment_screenshot(vscode_window['id'], verbose)
    if not screenshot_path:
        print("错误: 无法获取截图")
        return None
    
    # 分析内容
    analysis = analyze_augment_content(screenshot_path, verbose)
    
    # 使用组件分析提取文本
    text_content = extract_text_content_from_components(vscode_window['id'], verbose)
    if text_content:
        analysis['text_content'] = text_content
        analysis['has_continue_prompt'] = check_for_continue_prompt(text_content, verbose)
        
        if verbose:
            print("\n组件分析提取的内容:")
            print("-" * 50)
            print(text_content)
            print("-" * 50)
    else:
        analysis['text_content'] = None
        analysis['has_continue_prompt'] = None
        
        if verbose:
            print("组件分析未能提取到文本内容")
    
    return analysis

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='VSCode Augment Check Latest Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python tools/vscode_augment_check_latest.py                # 检查最新内容
  python tools/vscode_augment_check_latest.py --verbose      # 详细输出
  python tools/vscode_augment_check_latest.py --save-screenshot # 保存截图
        """
    )
    
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='详细输出')
    parser.add_argument('--save-screenshot', action='store_true',
                       help='保存截图文件')
    parser.add_argument('--text-only', action='store_true',
                       help='仅输出文本内容')
    
    args = parser.parse_args()
    
    # 获取最新内容
    result = get_latest_augment_context(args.verbose)
    
    if not result:
        return 1
    
    # 输出结果
    if args.text_only:
        if result.get('text_content'):
            print(result['text_content'])
        else:
            print("无法提取文本内容")
            print(f"请查看截图: {result['screenshot_path']}")
    else:
        print(f"📸 截图文件: {result['screenshot_path']}")
        print(f"🕐 时间戳: {result['timestamp']}")
        
        if result.get('text_content'):
            print(f"🔍 包含continue提示: {'是' if result.get('has_continue_prompt') else '否'}")
            
            print(f"\n📄 内容分析:")
            print("-" * 60)
            if not args.verbose:
                # 简化输出模式下显示文本摘要
                text = result['text_content']
                if len(text) > 300:
                    print(f"{text[:300]}...")
                    print(f"\n[内容较长，使用 --verbose 查看完整内容]")
                else:
                    print(text)
            else:
                print(result['text_content'])
            print("-" * 60)
        else:
            print("📄 文本内容: 无法提取")
            print(f"💡 建议: 请直接查看截图文件了解对话框内容")
    
    # 如果不需要保存截图，删除临时文件
    if not args.save_screenshot and not args.verbose:
        try:
            os.remove(result['screenshot_path'])
            if args.verbose:
                print(f"临时截图已删除: {result['screenshot_path']}")
        except:
            pass
    
    return 0

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code) 