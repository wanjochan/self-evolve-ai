#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode UI 组件结构 Dump 工具
专门用于导出VSCode的完整UI组件树结构

Requirements:
    pip install comtypes pywin32

Usage:
    python tools/vscode_ui_dump.py
    python tools/vscode_ui_dump.py --max-depth 5
    python tools/vscode_ui_dump.py --save-to-file
"""

import sys
import os
import argparse
import json
from datetime import datetime
import win32gui

# UI Automation imports
try:
    import comtypes
    import comtypes.client
    from comtypes.gen import UIAutomationClient
    import pythoncom
    UI_AUTOMATION_AVAILABLE = True
except ImportError as e:
    print(f"UI Automation不可用: {e}")
    UI_AUTOMATION_AVAILABLE = False

# 添加当前目录到路径，以便导入uictrl
sys.path.append(os.path.dirname(__file__))

from uictrl import UIController

class VSCodeUIDumper:
    def __init__(self, verbose=False):
        self.verbose = verbose
        self.automation = None
        self.element_count = 0
        
    def log(self, message):
        if self.verbose:
            print(f"[DUMP] {message}")
    
    def find_vscode_window(self):
        """查找VSCode窗口"""
        controller = UIController()
        ide_windows = controller.find_ide_windows()
        
        for window in ide_windows:
            if 'visual studio code' in window['title'].lower():
                return window
        return None
    
    def initialize_ui_automation(self):
        """初始化UI Automation"""
        if not UI_AUTOMATION_AVAILABLE:
            print("错误: UI Automation不可用，请安装comtypes")
            return False
        
        try:
            pythoncom.CoInitialize()
            
            self.automation = comtypes.client.CreateObject(
                UIAutomationClient.CUIAutomation,
                interface=UIAutomationClient.IUIAutomation
            )
            
            self.log("UI Automation初始化成功")
            return True
            
        except Exception as e:
            print(f"UI Automation初始化失败: {e}")
            return False
    
    def get_control_type_name(self, control_type_id):
        """获取控件类型名称"""
        control_types = {
            50000: "Button", 50001: "Calendar", 50002: "CheckBox", 50003: "ComboBox",
            50004: "Edit", 50005: "Hyperlink", 50006: "Image", 50007: "ListItem",
            50008: "List", 50009: "Menu", 50010: "MenuBar", 50011: "MenuItem",
            50012: "ProgressBar", 50013: "RadioButton", 50014: "ScrollBar", 50015: "Slider",
            50016: "Spinner", 50017: "StatusBar", 50018: "Tab", 50019: "TabItem",
            50020: "Text", 50021: "ToolBar", 50022: "ToolTip", 50023: "Tree",
            50024: "TreeItem", 50025: "Custom", 50026: "Group", 50027: "Thumb",
            50028: "DataGrid", 50029: "DataItem", 50030: "Document", 50031: "SplitButton",
            50032: "Window", 50033: "Pane", 50034: "Header", 50035: "HeaderItem",
            50036: "Table", 50037: "TitleBar", 50038: "Separator"
        }
        return control_types.get(control_type_id, f"Unknown({control_type_id})")
    
    def extract_element_info(self, element):
        """提取单个元素的完整信息"""
        info = {
            'element_id': self.element_count,
            'properties': {},
            'patterns': [],
            'bounding_rect': None,
            'children_count': 0
        }
        
        self.element_count += 1
        
        try:
            # 基本属性
            try:
                info['properties']['Name'] = element.CurrentName or ""
            except:
                info['properties']['Name'] = ""
            
            try:
                info['properties']['AutomationId'] = element.CurrentAutomationId or ""
            except:
                info['properties']['AutomationId'] = ""
            
            try:
                info['properties']['ClassName'] = element.CurrentClassName or ""
            except:
                info['properties']['ClassName'] = ""
            
            try:
                control_type_id = element.CurrentControlType
                info['properties']['ControlType'] = self.get_control_type_name(control_type_id)
                info['properties']['ControlTypeId'] = control_type_id
            except:
                info['properties']['ControlType'] = "Unknown"
                info['properties']['ControlTypeId'] = 0
            
            try:
                info['properties']['LocalizedControlType'] = element.CurrentLocalizedControlType or ""
            except:
                info['properties']['LocalizedControlType'] = ""
            
            try:
                info['properties']['IsEnabled'] = element.CurrentIsEnabled
            except:
                info['properties']['IsEnabled'] = False
            
            try:
                info['properties']['IsVisible'] = not element.CurrentIsOffscreen
            except:
                info['properties']['IsVisible'] = True
            
            try:
                info['properties']['HasKeyboardFocus'] = element.CurrentHasKeyboardFocus
            except:
                info['properties']['HasKeyboardFocus'] = False
            
            # 边界矩形
            try:
                rect = element.CurrentBoundingRectangle
                info['bounding_rect'] = {
                    'left': rect.left,
                    'top': rect.top,
                    'right': rect.right,
                    'bottom': rect.bottom,
                    'width': rect.right - rect.left,
                    'height': rect.bottom - rect.top
                }
            except:
                info['bounding_rect'] = None
            
            # 支持的模式
            try:
                supported_patterns = element.GetSupportedPatterns()
                for pattern_id in supported_patterns:
                    try:
                        pattern_name = self.get_pattern_name(pattern_id)
                        info['patterns'].append(pattern_name)
                        
                        # 尝试获取模式特定的信息
                        if pattern_id == UIAutomationClient.UIA_ValuePatternId:
                            try:
                                value_pattern = element.GetCurrentPattern(pattern_id)
                                info['properties']['Value'] = value_pattern.CurrentValue or ""
                            except:
                                pass
                        
                        elif pattern_id == UIAutomationClient.UIA_TextPatternId:
                            try:
                                text_pattern = element.GetCurrentPattern(pattern_id)
                                document_range = text_pattern.DocumentRange
                                info['properties']['TextContent'] = document_range.GetText(-1) or ""
                            except:
                                pass
                                
                    except:
                        continue
            except:
                pass
            
            # 子元素数量
            try:
                children = element.FindAll(UIAutomationClient.TreeScope_Children, 
                                         self.automation.CreateTrueCondition())
                info['children_count'] = children.Length if children else 0
            except:
                info['children_count'] = 0
            
        except Exception as e:
            self.log(f"提取元素信息时出错: {e}")
        
        return info
    
    def get_pattern_name(self, pattern_id):
        """获取模式名称"""
        patterns = {
            10000: "Invoke", 10001: "Selection", 10002: "Value", 10003: "RangeValue",
            10004: "Scroll", 10005: "ExpandCollapse", 10006: "Grid", 10007: "GridItem",
            10008: "MultipleView", 10009: "Window", 10010: "SelectionItem", 10011: "Dock",
            10012: "Table", 10013: "TableItem", 10014: "Text", 10015: "Toggle",
            10016: "Transform", 10017: "ScrollItem", 10018: "LegacyIAccessible",
            10019: "ItemContainer", 10020: "VirtualizedItem", 10021: "SynchronizedInput"
        }
        return patterns.get(pattern_id, f"Pattern({pattern_id})")
    
    def dump_element_tree(self, element, depth=0, max_depth=4):
        """递归dump元素树"""
        if depth > max_depth:
            return None
        
        # 提取当前元素信息
        element_info = self.extract_element_info(element)
        element_info['depth'] = depth
        element_info['children'] = []
        
        # 输出当前元素信息
        indent = "  " * depth
        name = element_info['properties']['Name']
        control_type = element_info['properties']['ControlType']
        automation_id = element_info['properties']['AutomationId']
        class_name = element_info['properties']['ClassName']
        
        display_name = name or automation_id or class_name or "Unknown"
        print(f"{indent}[{element_info['element_id']:3d}] {control_type}: {display_name}")
        
        if self.verbose:
            if element_info['properties']['AutomationId']:
                print(f"{indent}     AutomationId: {element_info['properties']['AutomationId']}")
            if element_info['properties']['ClassName']:
                print(f"{indent}     ClassName: {element_info['properties']['ClassName']}")
            if element_info['bounding_rect']:
                rect = element_info['bounding_rect']
                print(f"{indent}     Rect: ({rect['left']}, {rect['top']}, {rect['width']}x{rect['height']})")
            if element_info['patterns']:
                print(f"{indent}     Patterns: {', '.join(element_info['patterns'])}")
            if element_info['properties'].get('Value'):
                value = element_info['properties']['Value'][:50]
                print(f"{indent}     Value: {value}...")
            if element_info['properties'].get('TextContent'):
                text = element_info['properties']['TextContent'][:50]
                print(f"{indent}     Text: {text}...")
        
        # 递归处理子元素
        try:
            children = element.FindAll(UIAutomationClient.TreeScope_Children, 
                                     self.automation.CreateTrueCondition())
            
            if children and children.Length > 0:
                if self.verbose:
                    print(f"{indent}     Children: {children.Length}")
                
                # 限制子元素数量以避免过度递归
                max_children = min(children.Length, 50)
                for i in range(max_children):
                    try:
                        child = children.GetElement(i)
                        if child:
                            child_info = self.dump_element_tree(child, depth + 1, max_depth)
                            if child_info:
                                element_info['children'].append(child_info)
                    except Exception as e:
                        self.log(f"处理子元素 {i} 时出错: {e}")
                        continue
                
                if children.Length > max_children:
                    print(f"{indent}     ... 还有 {children.Length - max_children} 个子元素")
                    
        except Exception as e:
            self.log(f"获取子元素失败: {e}")
        
        return element_info
    
    def dump_vscode_ui(self, max_depth=4, save_to_file=False):
        """Dump VSCode的完整UI结构"""
        print("=== VSCode UI 组件结构 Dump ===")
        
        # 查找VSCode窗口
        vscode_window = self.find_vscode_window()
        if not vscode_window:
            print("错误: 未找到VSCode窗口")
            return None
        
        print(f"窗口: {vscode_window['title']} (ID: {vscode_window['id']})")
        
        # 初始化UI Automation
        if not self.initialize_ui_automation():
            return None
        
        # 获取根元素
        hwnd = int(vscode_window['id'])
        try:
            root_element = self.automation.ElementFromHandle(hwnd)
            if not root_element:
                print("错误: 无法获取UI根元素")
                return None
        except Exception as e:
            print(f"错误: 获取UI根元素失败: {e}")
            return None
        
        print(f"\n开始Dump UI结构 (最大深度: {max_depth})...")
        print("=" * 60)
        
        try:
            # 重置计数器
            self.element_count = 0
            
            # Dump完整的UI树
            ui_tree = self.dump_element_tree(root_element, max_depth=max_depth)
            
            print("=" * 60)
            print(f"Dump完成，共发现 {self.element_count} 个UI元素")
            
            # 保存到文件
            if save_to_file and ui_tree:
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"tools/vscode_ui_dump_{timestamp}.json"
                
                dump_data = {
                    'timestamp': datetime.now().isoformat(),
                    'window_info': vscode_window,
                    'total_elements': self.element_count,
                    'ui_tree': ui_tree
                }
                
                with open(filename, 'w', encoding='utf-8') as f:
                    json.dump(dump_data, f, ensure_ascii=False, indent=2)
                
                print(f"UI结构已保存到: {filename}")
            
            return ui_tree
            
        except Exception as e:
            print(f"Dump过程中出错: {e}")
            return None
        
        finally:
            try:
                self.automation = None
                pythoncom.CoUninitialize()
            except:
                pass

def main():
    parser = argparse.ArgumentParser(description="VSCode UI 组件结构 Dump 工具")
    parser.add_argument("--verbose", action="store_true", help="详细输出")
    parser.add_argument("--max-depth", type=int, default=4, help="最大扫描深度 (默认: 4)")
    parser.add_argument("--save-to-file", action="store_true", help="保存结果到JSON文件")
    
    args = parser.parse_args()
    
    dumper = VSCodeUIDumper(verbose=args.verbose)
    ui_tree = dumper.dump_vscode_ui(max_depth=args.max_depth, save_to_file=args.save_to_file)
    
    if ui_tree:
        print(f"\nDump成功完成！")
    else:
        print("Dump失败")

if __name__ == "__main__":
    main() 