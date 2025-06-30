#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
UI Automation 模块
优化的UI Automation封装，提供高效的组件树分析和交互功能

Requirements:
    pip install comtypes pywin32

Features:
    - 快速组件树扫描
    - 智能内容提取
    - 交互元素识别
    - 缓存机制优化
    - 多种查找策略
"""

import sys
import os
import time
import json
from datetime import datetime
from typing import Dict, List, Optional, Any, Callable
import win32gui
import win32con

# UI Automation imports
try:
    import comtypes
    import comtypes.client
    from comtypes.gen import UIAutomationClient
    import pythoncom
    UI_AUTOMATION_AVAILABLE = True
except ImportError as e:
    UI_AUTOMATION_AVAILABLE = False

# 添加当前目录到路径
sys.path.append(os.path.dirname(__file__))
from uictrl import UIController

class UIAElement:
    """UI Automation 元素封装类"""
    
    def __init__(self, element, element_id: int = 0):
        self.element = element
        self.element_id = element_id
        self._cached_properties = {}
        self._cached_patterns = None
        self._cached_children = None
        
    def get_property(self, property_name: str, use_cache: bool = True):
        """获取元素属性，支持缓存"""
        if use_cache and property_name in self._cached_properties:
            return self._cached_properties[property_name]
        
        try:
            if property_name == 'Name':
                value = self.element.CurrentName or ""
            elif property_name == 'AutomationId':
                value = self.element.CurrentAutomationId or ""
            elif property_name == 'ClassName':
                value = self.element.CurrentClassName or ""
            elif property_name == 'ControlType':
                control_type_id = self.element.CurrentControlType
                value = UIAModule.get_control_type_name(control_type_id)
            elif property_name == 'ControlTypeId':
                value = self.element.CurrentControlType
            elif property_name == 'LocalizedControlType':
                value = self.element.CurrentLocalizedControlType or ""
            elif property_name == 'IsEnabled':
                value = self.element.CurrentIsEnabled
            elif property_name == 'IsVisible':
                value = not self.element.CurrentIsOffscreen
            elif property_name == 'HasKeyboardFocus':
                value = self.element.CurrentHasKeyboardFocus
            elif property_name == 'BoundingRectangle':
                rect = self.element.CurrentBoundingRectangle
                value = {
                    'left': rect.left, 'top': rect.top,
                    'right': rect.right, 'bottom': rect.bottom,
                    'width': rect.right - rect.left,
                    'height': rect.bottom - rect.top
                }
            elif property_name == 'Value':
                try:
                    value_pattern = self.element.GetCurrentPattern(UIAutomationClient.UIA_ValuePatternId)
                    value = value_pattern.CurrentValue if value_pattern else ""
                except:
                    value = ""
            elif property_name == 'TextContent':
                try:
                    text_pattern = self.element.GetCurrentPattern(UIAutomationClient.UIA_TextPatternId)
                    if text_pattern:
                        document_range = text_pattern.DocumentRange
                        value = document_range.GetText(-1) if document_range else ""
                    else:
                        value = ""
                except:
                    value = ""
            else:
                value = None
                
            if use_cache:
                self._cached_properties[property_name] = value
            return value
            
        except Exception:
            return None
    
    def get_patterns(self, use_cache: bool = True) -> List[str]:
        """获取支持的模式列表"""
        if use_cache and self._cached_patterns is not None:
            return self._cached_patterns
        
        patterns = []
        try:
            supported_patterns = self.element.GetSupportedPatterns()
            for pattern_id in supported_patterns:
                pattern_name = UIAModule.get_pattern_name(pattern_id)
                patterns.append(pattern_name)
        except:
            pass
        
        if use_cache:
            self._cached_patterns = patterns
        return patterns
    
    def get_children_count(self) -> int:
        """获取子元素数量"""
        try:
            children = self.element.FindAll(
                UIAutomationClient.TreeScope_Children,
                UIAModule._automation.CreateTrueCondition()
            )
            return children.Length if children else 0
        except:
            return 0
    
    def get_children(self, use_cache: bool = True) -> List['UIAElement']:
        """获取子元素列表"""
        if use_cache and self._cached_children is not None:
            return self._cached_children
        
        children = []
        try:
            child_elements = self.element.FindAll(
                UIAutomationClient.TreeScope_Children,
                UIAModule._automation.CreateTrueCondition()
            )
            
            if child_elements:
                for i in range(child_elements.Length):
                    child = child_elements.GetElement(i)
                    if child:
                        children.append(UIAElement(child, element_id=i))
        except:
            pass
        
        if use_cache:
            self._cached_children = children
        return children
    
    def to_dict(self, include_patterns: bool = True, include_rect: bool = True) -> Dict[str, Any]:
        """转换为字典格式"""
        data = {
            'element_id': self.element_id,
            'name': self.get_property('Name'),
            'automation_id': self.get_property('AutomationId'),
            'class_name': self.get_property('ClassName'),
            'control_type': self.get_property('ControlType'),
            'control_type_id': self.get_property('ControlTypeId'),
            'is_enabled': self.get_property('IsEnabled'),
            'is_visible': self.get_property('IsVisible'),
            'has_focus': self.get_property('HasKeyboardFocus'),
            'children_count': self.get_children_count()
        }
        
        # 获取值内容
        value = self.get_property('Value')
        if value:
            data['value'] = value
            
        text_content = self.get_property('TextContent')
        if text_content:
            data['text_content'] = text_content
        
        if include_patterns:
            data['patterns'] = self.get_patterns()
        
        if include_rect:
            data['bounding_rect'] = self.get_property('BoundingRectangle')
        
        return data

class UIAModule:
    """UI Automation 主模块类"""
    
    _automation = None
    _element_counter = 0
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.initialized = False
        
    def log(self, message: str):
        """日志输出"""
        if self.verbose:
            print(f"[UIA] {message}")
    
    @classmethod
    def initialize(cls) -> bool:
        """初始化UI Automation"""
        if cls._automation is not None:
            return True
            
        if not UI_AUTOMATION_AVAILABLE:
            return False
        
        try:
            pythoncom.CoInitialize()
            cls._automation = comtypes.client.CreateObject(
                UIAutomationClient.CUIAutomation,
                interface=UIAutomationClient.IUIAutomation
            )
            return True
        except Exception:
            return False
    
    @classmethod
    def cleanup(cls):
        """清理资源"""
        try:
            cls._automation = None
            pythoncom.CoUninitialize()
        except:
            pass
    
    @staticmethod
    def get_control_type_name(control_type_id: int) -> str:
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
    
    @staticmethod
    def get_pattern_name(pattern_id: int) -> str:
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
    
    def get_element_from_hwnd(self, hwnd: int) -> Optional[UIAElement]:
        """从窗口句柄获取UI元素"""
        if not self.initialize():
            return None
        
        try:
            if not win32gui.IsWindow(hwnd):
                return None
            
            element = self._automation.ElementFromHandle(hwnd)
            if element:
                return UIAElement(element, element_id=0)
        except Exception as e:
            self.log(f"获取元素失败: {e}")
        
        return None
    
    def find_vscode_window(self) -> Optional[Dict[str, Any]]:
        """查找VSCode窗口"""
        controller = UIController()
        ide_windows = controller.find_ide_windows()
        
        for window in ide_windows:
            if 'visual studio code' in window['title'].lower():
                return window
        return None
    
    def scan_tree_optimized(self, 
                           root_element: UIAElement, 
                           max_depth: int = 4,
                           max_children_per_level: int = 50,
                           filter_func: Optional[Callable[[UIAElement], bool]] = None,
                           progress_callback: Optional[Callable[[int, str], None]] = None) -> Dict[str, Any]:
        """优化的树扫描，支持过滤和进度回调"""
        
        start_time = time.time()
        self._element_counter = 0
        
        def scan_recursive(element: UIAElement, depth: int = 0) -> Dict[str, Any]:
            if depth > max_depth:
                return None
            
            self._element_counter += 1
            
            # 进度回调
            if progress_callback and self._element_counter % 10 == 0:
                progress_callback(self._element_counter, f"扫描第{depth}层...")
            
            # 获取元素基本信息
            element_data = element.to_dict()
            element_data['depth'] = depth
            element_data['children'] = []
            
            # 应用过滤器
            if filter_func and not filter_func(element):
                return None
            
            # 获取子元素
            try:
                children = element.get_children()
                if children:
                    # 限制子元素数量
                    limited_children = children[:max_children_per_level]
                    
                    for child in limited_children:
                        child_data = scan_recursive(child, depth + 1)
                        if child_data:
                            element_data['children'].append(child_data)
                    
                    # 记录被截断的子元素数量
                    if len(children) > max_children_per_level:
                        element_data['truncated_children'] = len(children) - max_children_per_level
                        
            except Exception as e:
                self.log(f"扫描子元素失败: {e}")
            
            return element_data
        
        result = scan_recursive(root_element)
        
        scan_time = time.time() - start_time
        
        return {
            'tree_data': result,
            'scan_stats': {
                'total_elements': self._element_counter,
                'scan_time': scan_time,
                'max_depth': max_depth,
                'timestamp': datetime.now().isoformat()
            }
        }
    
    def find_elements_by_criteria(self, 
                                 root_element: UIAElement,
                                 criteria: Dict[str, Any],
                                 max_results: int = 100) -> List[Dict[str, Any]]:
        """根据条件查找元素"""
        results = []
        
        def check_element(element: UIAElement) -> bool:
            element_data = element.to_dict()
            
            for key, expected_value in criteria.items():
                if key in element_data:
                    actual_value = element_data[key]
                    
                    # 字符串匹配（支持部分匹配）
                    if isinstance(expected_value, str) and isinstance(actual_value, str):
                        if expected_value.lower() not in actual_value.lower():
                            return False
                    # 精确匹配
                    elif actual_value != expected_value:
                        return False
                else:
                    return False
            
            return True
        
        def search_recursive(element: UIAElement, depth: int = 0):
            if len(results) >= max_results or depth > 10:
                return
            
            if check_element(element):
                element_data = element.to_dict()
                element_data['depth'] = depth
                results.append(element_data)
            
            # 搜索子元素
            try:
                children = element.get_children()
                for child in children:
                    search_recursive(child, depth + 1)
            except:
                pass
        
        search_recursive(root_element)
        return results
    
    def find_interactive_elements(self, root_element: UIAElement) -> List[Dict[str, Any]]:
        """查找可交互的元素"""
        interactive_types = [
            "Button", "CheckBox", "ComboBox", "Edit", "Hyperlink",
            "ListItem", "MenuItem", "RadioButton", "TabItem"
        ]
        
        interactive_patterns = [
            "Invoke", "Toggle", "Value", "Selection", "SelectionItem"
        ]
        
        def is_interactive(element: UIAElement) -> bool:
            element_data = element.to_dict()
            
            # 检查控件类型
            if element_data.get('control_type') in interactive_types:
                return True
            
            # 检查模式
            patterns = element_data.get('patterns', [])
            if any(pattern in interactive_patterns for pattern in patterns):
                return True
            
            # 检查是否可用且可见
            if element_data.get('is_enabled') and element_data.get('is_visible'):
                # 检查是否有有意义的名称
                name = element_data.get('name', '')
                if name and len(name.strip()) > 0:
                    return True
            
            return False
        
        return self.find_elements_by_criteria(root_element, {}, max_results=200)
    
    def analyze_vscode_ui(self, 
                         max_depth: int = 4,
                         include_interactive_only: bool = False,
                         save_to_file: bool = False) -> Dict[str, Any]:
        """分析VSCode UI结构"""
        
        # 查找VSCode窗口
        vscode_window = self.find_vscode_window()
        if not vscode_window:
            return {'error': '未找到VSCode窗口'}
        
        self.log(f"找到VSCode窗口: {vscode_window['title']}")
        
        # 获取根元素
        hwnd = int(vscode_window['id'])
        root_element = self.get_element_from_hwnd(hwnd)
        if not root_element:
            return {'error': '无法获取UI根元素'}
        
        # 定义进度回调
        def progress_callback(count: int, message: str):
            if self.verbose:
                print(f"\r[UIA] 已扫描 {count} 个元素 - {message}", end='', flush=True)
        
        # 扫描UI树
        self.log("开始扫描UI树...")
        
        filter_func = None
        if include_interactive_only:
            def filter_func(element: UIAElement) -> bool:
                # 简单的交互元素过滤
                control_type = element.get_property('ControlType')
                return control_type in ["Button", "Edit", "Hyperlink", "TabItem", "MenuItem"]
        
        scan_result = self.scan_tree_optimized(
            root_element,
            max_depth=max_depth,
            filter_func=filter_func,
            progress_callback=progress_callback
        )
        
        if self.verbose:
            print()  # 换行
        
        # 查找特定元素
        self.log("查找Augment相关元素...")
        augment_elements = self.find_elements_by_criteria(
            root_element,
            {'name': 'augment'},
            max_results=20
        )
        
        # 查找交互元素
        self.log("查找交互元素...")
        interactive_elements = self.find_interactive_elements(root_element)
        
        # 组织结果
        result = {
            'window_info': vscode_window,
            'ui_tree': scan_result['tree_data'],
            'scan_stats': scan_result['scan_stats'],
            'augment_elements': augment_elements,
            'interactive_elements': interactive_elements[:50],  # 限制数量
            'analysis_summary': {
                'total_elements': scan_result['scan_stats']['total_elements'],
                'augment_elements_count': len(augment_elements),
                'interactive_elements_count': len(interactive_elements),
                'scan_time': scan_result['scan_stats']['scan_time']
            }
        }
        
        # 保存到文件
        if save_to_file:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"tools/vscode_uia_analysis_{timestamp}.json"
            
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(result, f, ensure_ascii=False, indent=2)
            
            self.log(f"分析结果已保存到: {filename}")
            result['saved_file'] = filename
        
        return result

# 便捷函数
def quick_analyze_vscode(verbose: bool = True, 
                        max_depth: int = 3,
                        save_to_file: bool = False) -> Dict[str, Any]:
    """快速分析VSCode UI的便捷函数"""
    module = UIAModule(verbose=verbose)
    try:
        return module.analyze_vscode_ui(
            max_depth=max_depth,
            save_to_file=save_to_file
        )
    finally:
        UIAModule.cleanup()

def find_augment_elements(verbose: bool = False) -> List[Dict[str, Any]]:
    """查找Augment相关元素的便捷函数"""
    module = UIAModule(verbose=verbose)
    try:
        vscode_window = module.find_vscode_window()
        if not vscode_window:
            return []
        
        root_element = module.get_element_from_hwnd(int(vscode_window['id']))
        if not root_element:
            return []
        
        return module.find_elements_by_criteria(
            root_element,
            {'name': 'augment'},
            max_results=50
        )
    finally:
        UIAModule.cleanup()

if __name__ == "__main__":
    # 测试代码
    result = quick_analyze_vscode(verbose=True, max_depth=3, save_to_file=True)
    if 'error' not in result:
        print(f"\n=== 分析完成 ===")
        print(f"总元素数: {result['analysis_summary']['total_elements']}")
        print(f"Augment元素数: {result['analysis_summary']['augment_elements_count']}")
        print(f"交互元素数: {result['analysis_summary']['interactive_elements_count']}")
        print(f"扫描时间: {result['analysis_summary']['scan_time']:.2f}秒")
    else:
        print(f"分析失败: {result['error']}")