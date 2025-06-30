#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode UI Automation 改进版分析工具
使用更简单直接的UI Automation方法来获取VSCode窗口内容

Requirements:
    pip install pywin32 comtypes

Usage:
    python tools/vscode_ui_automation_improved.py
    python tools/vscode_ui_automation_improved.py --verbose
    python tools/vscode_ui_automation_improved.py --deep-scan
"""

import sys
import os
import argparse
import time
from datetime import datetime
import win32gui
import win32con
import win32api

# 添加当前目录到路径，以便导入uictrl
sys.path.append(os.path.dirname(__file__))

from uictrl import UIController

# UI Automation imports
try:
    import comtypes
    import comtypes.client
    from comtypes import COMError
    UI_AUTOMATION_AVAILABLE = True
except ImportError:
    UI_AUTOMATION_AVAILABLE = False

class VSCodeUIAnalyzer:
    def __init__(self, verbose=False):
        self.verbose = verbose
        self.automation = None
        
    def log(self, message):
        if self.verbose:
            print(f"[UIA] {message}")
    
    def initialize_ui_automation(self):
        """初始化UI Automation"""
        if not UI_AUTOMATION_AVAILABLE:
            print("错误: UI Automation不可用，请安装comtypes: pip install comtypes")
            return False
        
        try:
            # 初始化COM
            comtypes.CoInitialize()
            
            # 创建UI Automation对象
            self.automation = comtypes.client.CreateObject(
                "{ff48dba4-60ef-4201-aa87-54103eef594e}",  # CUIAutomation CLSID
                interface=comtypes.gen.UIAutomationClient.IUIAutomation
            )
            
            self.log("UI Automation初始化成功")
            return True
            
        except Exception as e:
            self.log(f"UI Automation初始化失败: {e}")
            try:
                # 尝试生成类型库
                comtypes.client.GetModule("UIAutomationCore.dll")
                self.automation = comtypes.client.CreateObject(
                    "CUIAutomation",
                    interface=comtypes.gen.UIAutomationClient.IUIAutomation
                )
                self.log("UI Automation初始化成功（通过类型库）")
                return True
            except Exception as e2:
                self.log(f"类型库方法也失败: {e2}")
                return False
    
    def find_vscode_window(self):
        """查找VSCode窗口"""
        controller = UIController()
        ide_windows = controller.find_ide_windows()
        
        for window in ide_windows:
            if 'visual studio code' in window['title'].lower():
                return window
        return None
    
    def get_element_from_hwnd(self, hwnd):
        """从窗口句柄获取UI Automation元素"""
        try:
            element = self.automation.ElementFromHandle(hwnd)
            return element
        except Exception as e:
            self.log(f"获取元素失败: {e}")
            return None
    
    def get_element_text_simple(self, element):
        """简单获取元素文本"""
        try:
            # 尝试获取Name属性
            name = element.CurrentName
            if name and name.strip():
                return name.strip()
            
            # 尝试获取Value属性
            try:
                value = element.CurrentValue
                if value and value.strip():
                    return value.strip()
            except:
                pass
            
            # 尝试获取LegacyIAccessible pattern
            try:
                legacy_pattern = element.GetCurrentPattern(10018)  # LegacyIAccessiblePatternId
                if legacy_pattern:
                    name = legacy_pattern.CurrentName
                    value = legacy_pattern.CurrentValue
                    if name and name.strip():
                        return name.strip()
                    if value and value.strip():
                        return value.strip()
            except:
                pass
            
            return None
            
        except Exception as e:
            return None
    
    def scan_element_tree(self, element, depth=0, max_depth=3):
        """递归扫描元素树"""
        if depth > max_depth:
            return []
        
        results = []
        
        try:
            # 获取当前元素的文本
            text = self.get_element_text_simple(element)
            if text and len(text.strip()) > 2:
                control_type = ""
                try:
                    control_type_id = element.CurrentControlType
                    control_type = self.get_control_type_name(control_type_id)
                except:
                    pass
                
                results.append({
                    'text': text,
                    'depth': depth,
                    'control_type': control_type
                })
                
                if self.verbose and depth <= 2:
                    indent = "  " * depth
                    self.log(f"{indent}[{control_type}] {text[:60]}...")
            
            # 获取子元素
            try:
                children = element.FindAll(4, self.automation.CreateTrueCondition())  # TreeScope_Descendants
                if children and children.Length > 0:
                    # 限制扫描的子元素数量以避免过度递归
                    max_children = min(children.Length, 50)
                    for i in range(max_children):
                        child = children.GetElement(i)
                        child_results = self.scan_element_tree(child, depth + 1, max_depth)
                        results.extend(child_results)
            except Exception as e:
                if self.verbose:
                    self.log(f"扫描子元素失败: {e}")
            
        except Exception as e:
            if self.verbose:
                self.log(f"扫描元素失败: {e}")
        
        return results
    
    def get_control_type_name(self, control_type_id):
        """获取控件类型名称"""
        control_types = {
            50000: "Button",
            50001: "Calendar", 
            50002: "CheckBox",
            50003: "ComboBox",
            50004: "Edit",
            50005: "Hyperlink",
            50006: "Image",
            50007: "ListItem",
            50008: "List",
            50009: "Menu",
            50010: "MenuBar",
            50011: "MenuItem",
            50012: "ProgressBar",
            50013: "RadioButton",
            50014: "ScrollBar",
            50015: "Slider",
            50016: "Spinner",
            50017: "StatusBar",
            50018: "Tab",
            50019: "TabItem",
            50020: "Text",
            50021: "ToolBar",
            50022: "ToolTip",
            50023: "Tree",
            50024: "TreeItem",
            50025: "Custom",
            50026: "Group",
            50027: "Thumb",
            50028: "DataGrid",
            50029: "DataItem",
            50030: "Document",
            50031: "SplitButton",
            50032: "Window",
            50033: "Pane",
            50034: "Header",
            50035: "HeaderItem",
            50036: "Table",
            50037: "TitleBar",
            50038: "Separator"
        }
        return control_types.get(control_type_id, f"Unknown({control_type_id})")
    
    def find_specific_elements(self, root_element, search_text_patterns):
        """查找包含特定文本模式的元素"""
        found_elements = []
        
        try:
            # 创建条件查找包含特定文本的元素
            for pattern in search_text_patterns:
                try:
                    # 使用Name属性查找
                    name_condition = self.automation.CreatePropertyCondition(
                        30005,  # UIA_NamePropertyId
                        pattern
                    )
                    
                    elements = root_element.FindAll(4, name_condition)  # TreeScope_Descendants
                    
                    for i in range(elements.Length):
                        element = elements.GetElement(i)
                        text = self.get_element_text_simple(element)
                        if text:
                            found_elements.append({
                                'pattern': pattern,
                                'text': text,
                                'element': element
                            })
                            
                except Exception as e:
                    self.log(f"查找模式 '{pattern}' 失败: {e}")
                    
        except Exception as e:
            self.log(f"查找特定元素失败: {e}")
        
        return found_elements
    
    def analyze_vscode_content(self, deep_scan=False):
        """分析VSCode内容"""
        self.log("开始分析VSCode内容...")
        
        # 初始化UI Automation
        if not self.initialize_ui_automation():
            return None
        
        # 查找VSCode窗口
        vscode_window = self.find_vscode_window()
        if not vscode_window:
            print("错误: 未找到VSCode窗口")
            return None
        
        self.log(f"找到VSCode窗口: {vscode_window['title']} (ID: {vscode_window['id']})")
        
        # 获取根元素
        hwnd = int(vscode_window['id'])
        root_element = self.get_element_from_hwnd(hwnd)
        if not root_element:
            print("错误: 无法获取UI Automation根元素")
            return None
        
        results = {
            'window_info': vscode_window,
            'elements': [],
            'specific_findings': {},
            'summary': {}
        }
        
        try:
            # 基本扫描
            self.log("开始基本元素扫描...")
            max_depth = 4 if deep_scan else 2
            elements = self.scan_element_tree(root_element, max_depth=max_depth)
            results['elements'] = elements
            
            # 查找特定内容
            self.log("查找特定内容...")
            search_patterns = [
                "continue",
                "try again", 
                "error",
                "failed",
                "augment",
                "chat",
                "Would you like me to keep going",
                "Please try again"
            ]
            
            specific_elements = self.find_specific_elements(root_element, search_patterns)
            results['specific_findings'] = specific_elements
            
            # 生成摘要
            text_elements = [e for e in elements if e['text'] and len(e['text'].strip()) > 5]
            button_elements = [e for e in elements if e['control_type'] == 'Button']
            edit_elements = [e for e in elements if e['control_type'] == 'Edit']
            
            results['summary'] = {
                'total_elements': len(elements),
                'text_elements': len(text_elements),
                'button_elements': len(button_elements),
                'edit_elements': len(edit_elements),
                'specific_matches': len(specific_elements)
            }
            
            # 输出结果
            print(f"\n=== VSCode UI Automation 分析结果 ===")
            print(f"窗口: {vscode_window['title']}")
            print(f"总元素数: {results['summary']['total_elements']}")
            print(f"文本元素: {results['summary']['text_elements']}")
            print(f"按钮元素: {results['summary']['button_elements']}")
            print(f"编辑框元素: {results['summary']['edit_elements']}")
            print(f"特定匹配: {results['summary']['specific_matches']}")
            
            if results['specific_findings']:
                print(f"\n=== 找到的特定内容 ===")
                for finding in results['specific_findings']:
                    print(f"模式: {finding['pattern']}")
                    print(f"文本: {finding['text']}")
                    print()
            
            if self.verbose and text_elements:
                print(f"\n=== 所有文本元素 ===")
                for i, element in enumerate(text_elements[:20]):  # 限制显示数量
                    indent = "  " * element['depth']
                    print(f"{i+1:2d}. {indent}[{element['control_type']}] {element['text'][:80]}...")
                
                if len(text_elements) > 20:
                    print(f"... 还有 {len(text_elements) - 20} 个元素")
            
            return results
            
        except Exception as e:
            self.log(f"分析过程中出错: {e}")
            return results
        
        finally:
            try:
                comtypes.CoUninitialize()
            except:
                pass
    
    def take_screenshot(self):
        """截取VSCode窗口截图"""
        vscode_window = self.find_vscode_window()
        if not vscode_window:
            return None
        
        controller = UIController()
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"tools/vscode_ui_screenshot_{timestamp}.png"
        
        try:
            result = controller.take_screenshot(vscode_window['id'], filename)
            if result:
                print(f"截图已保存: {filename}")
                return filename
        except Exception as e:
            self.log(f"截图失败: {e}")
        
        return None

def main():
    parser = argparse.ArgumentParser(description="VSCode UI Automation 改进版分析工具")
    parser.add_argument("--verbose", action="store_true", help="详细输出")
    parser.add_argument("--deep-scan", action="store_true", help="深度扫描（更多层级）")
    parser.add_argument("--screenshot", action="store_true", help="同时截图")
    
    args = parser.parse_args()
    
    analyzer = VSCodeUIAnalyzer(verbose=args.verbose)
    
    if args.screenshot:
        analyzer.take_screenshot()
    
    results = analyzer.analyze_vscode_content(deep_scan=args.deep_scan)
    
    if results:
        print(f"\n分析完成，找到 {results['summary']['total_elements']} 个UI元素")
        if results['specific_findings']:
            print(f"发现 {len(results['specific_findings'])} 个特定匹配项")
    else:
        print("分析失败")

if __name__ == "__main__":
    main() 