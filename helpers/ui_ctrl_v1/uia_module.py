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
import concurrent.futures
from queue import Queue
from threading import Lock

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
        self._cached_text_content = None
        
        # 创建缓存请求
        self._cache_request = None
        try:
            self._cache_request = UIAModule._automation.CreateCacheRequest()
            self._cache_request.TreeScope = UIAutomationClient.TreeScope_Element | UIAutomationClient.TreeScope_Children
            # 添加常用属性到缓存
            self._cache_request.AddProperty(UIAutomationClient.UIA_NamePropertyId)
            self._cache_request.AddProperty(UIAutomationClient.UIA_AutomationIdPropertyId)
            self._cache_request.AddProperty(UIAutomationClient.UIA_ClassNamePropertyId)
            self._cache_request.AddProperty(UIAutomationClient.UIA_ControlTypePropertyId)
            self._cache_request.AddProperty(UIAutomationClient.UIA_IsEnabledPropertyId)
            self._cache_request.AddProperty(UIAutomationClient.UIA_BoundingRectanglePropertyId)
            # 添加常用模式到缓存
            self._cache_request.AddPattern(UIAutomationClient.UIA_ValuePatternId)
            self._cache_request.AddPattern(UIAutomationClient.UIA_TextPatternId)
            self._cache_request.AddPattern(UIAutomationClient.UIA_LegacyIAccessiblePatternId)
        except:
            pass
        
    def get_property(self, property_name: str, use_cache: bool = True):
        """获取元素属性，支持缓存"""
        if use_cache and property_name in self._cached_properties:
            return self._cached_properties[property_name]
        
        try:
            # 使用缓存请求获取属性
            if self._cache_request:
                cached_element = self.element.BuildUpdatedCache(self._cache_request)
                if cached_element:
                    if property_name == 'Name':
                        value = cached_element.CachedName or ""
                    elif property_name == 'AutomationId':
                        value = cached_element.CachedAutomationId or ""
                    elif property_name == 'ClassName':
                        value = cached_element.CachedClassName or ""
                    elif property_name == 'ControlType':
                        control_type_id = cached_element.CachedControlType
                        value = UIAModule.get_control_type_name(control_type_id)
                    elif property_name == 'ControlTypeId':
                        value = cached_element.CachedControlType
                    elif property_name == 'IsEnabled':
                        value = cached_element.CachedIsEnabled
                    elif property_name == 'BoundingRectangle':
                        rect = cached_element.CachedBoundingRectangle
                        value = {
                            'left': rect.left, 'top': rect.top,
                            'right': rect.right, 'bottom': rect.bottom,
                            'width': rect.right - rect.left,
                            'height': rect.bottom - rect.top
                        }
                    else:
                        # 回退到非缓存方式
                        value = self._get_property_no_cache(property_name)
                else:
                    value = self._get_property_no_cache(property_name)
            else:
                value = self._get_property_no_cache(property_name)
                
            if use_cache:
                self._cached_properties[property_name] = value
            return value
            
        except Exception:
            return None
            
    def _get_property_no_cache(self, property_name: str):
        """不使用缓存获取属性的回退方法"""
        try:
            if property_name == 'Name':
                return self.element.CurrentName or ""
            elif property_name == 'AutomationId':
                return self.element.CurrentAutomationId or ""
            elif property_name == 'ClassName':
                return self.element.CurrentClassName or ""
            elif property_name == 'ControlType':
                control_type_id = self.element.CurrentControlType
                return UIAModule.get_control_type_name(control_type_id)
            elif property_name == 'ControlTypeId':
                return self.element.CurrentControlType
            elif property_name == 'LocalizedControlType':
                return self.element.CurrentLocalizedControlType or ""
            elif property_name == 'IsEnabled':
                return self.element.CurrentIsEnabled
            elif property_name == 'IsVisible':
                return not self.element.CurrentIsOffscreen
            elif property_name == 'HasKeyboardFocus':
                return self.element.CurrentHasKeyboardFocus
            elif property_name == 'BoundingRectangle':
                rect = self.element.CurrentBoundingRectangle
                return {
                    'left': rect.left, 'top': rect.top,
                    'right': rect.right, 'bottom': rect.bottom,
                    'width': rect.right - rect.left,
                    'height': rect.bottom - rect.top
                }
            elif property_name == 'Value':
                try:
                    value_pattern = self.element.GetCurrentPattern(UIAutomationClient.UIA_ValuePatternId)
                    return value_pattern.CurrentValue if value_pattern else ""
                except:
                    return ""
            elif property_name == 'TextContent':
                return self.get_text_content()
            else:
                return None
        except Exception:
            return None
    
    def get_text_content(self, use_cache: bool = True) -> str:
        """使用TextPattern获取元素的文本内容"""
        if use_cache and self._cached_text_content is not None:
            return self._cached_text_content
        
        text_content = ""
        
        try:
            # 使用缓存请求获取文本
            if self._cache_request:
                cached_element = self.element.BuildUpdatedCache(self._cache_request)
                if cached_element:
                    # 尝试使用TextPattern获取文本
                    text_pattern = cached_element.GetCachedPattern(UIAutomationClient.UIA_TextPatternId)
                    if text_pattern:
                        document_range = text_pattern.DocumentRange
                        if document_range:
                            text_content = document_range.GetText(-1)  # -1表示获取所有文本
        except Exception:
            pass
        
        # 如果缓存获取失败，使用回退方法
        if not text_content:
            try:
                # 尝试使用TextPattern获取文本
                text_pattern = self.element.GetCurrentPattern(UIAutomationClient.UIA_TextPatternId)
                if text_pattern:
                    document_range = text_pattern.DocumentRange
                    if document_range:
                        text_content = document_range.GetText(-1)  # -1表示获取所有文本
            except Exception:
                pass
        
        # 如果TextPattern失败，尝试使用LegacyIAccessible
        if not text_content:
            try:
                legacy_pattern = self.element.GetCurrentPattern(UIAutomationClient.UIA_LegacyIAccessiblePatternId)
                if legacy_pattern:
                    text_content = legacy_pattern.CurrentValue or ""
            except Exception:
                pass
        
        # 最后尝试直接获取Name属性
        if not text_content:
            try:
                text_content = self.element.CurrentName or ""
            except Exception:
                pass
        
        if use_cache:
            self._cached_text_content = text_content
        
        return text_content
    
    def search_text_in_element(self, search_phrases: List[str], case_sensitive: bool = False) -> List[Dict[str, Any]]:
        """在元素中搜索指定的文本短语"""
        results = []
        
        # 获取当前元素的文本内容
        text_content = self.get_text_content()
        if text_content:
            text_to_search = text_content if case_sensitive else text_content.lower()
            
            for phrase in search_phrases:
                search_phrase = phrase if case_sensitive else phrase.lower()
                if search_phrase in text_to_search:
                    results.append({
                        'element_id': self.element_id,
                        'found_phrase': phrase,
                        'text_content': text_content,
                        'control_type': self.get_property('ControlType'),
                        'name': self.get_property('Name'),
                        'bounding_rect': self.get_property('BoundingRectangle')
                    })
        
        return results
    
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
    _counter_lock = Lock()
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.initialized = False
        self._thread_pool = concurrent.futures.ThreadPoolExecutor(max_workers=4)
        
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
    
    def find_vscode_window(self) -> dict:
        """查找VSCode窗口"""
        try:
            print("开始查找VSCode窗口...")
            import uiautomation as auto
            desktop = auto.GetRootControl()
            print("获取到桌面元素")
            
            # 查找VSCode窗口
            vscode = desktop.WindowControl(Name="Visual Studio Code")
            if vscode.Exists(1):
                print(f"找到VSCode窗口: {vscode.Name}")
                return {
                    'title': vscode.Name,
                    'element': vscode
                }
            else:
                print("未找到VSCode窗口")
                return None
                
        except Exception as e:
            print(f"查找VSCode窗口时出错: {str(e)}")
            import traceback
            print(traceback.format_exc())
            return None
    
    def scan_tree_optimized(self, 
                           root_element: UIAElement, 
                           max_depth: int = 4,
                           max_children_per_level: int = 50,
                           filter_func: Optional[Callable[[UIAElement], bool]] = None,
                           progress_callback: Optional[Callable[[int, str], None]] = None) -> Dict[str, Any]:
        """优化的树扫描方法
        
        Args:
            root_element: 根元素
            max_depth: 最大扫描深度
            max_children_per_level: 每层最大子元素数量
            filter_func: 元素过滤函数
            progress_callback: 进度回调函数
            
        Returns:
            包含树结构的字典
        """
        
        # 创建缓存请求
        cache_request = UIAModule._automation.CreateCacheRequest()
        cache_request.TreeScope = UIAutomationClient.TreeScope_Element | UIAutomationClient.TreeScope_Children
        # 添加常用属性到缓存
        cache_request.AddProperty(UIAutomationClient.UIA_NamePropertyId)
        cache_request.AddProperty(UIAutomationClient.UIA_AutomationIdPropertyId)
        cache_request.AddProperty(UIAutomationClient.UIA_ClassNamePropertyId)
        cache_request.AddProperty(UIAutomationClient.UIA_ControlTypePropertyId)
        cache_request.AddProperty(UIAutomationClient.UIA_IsEnabledPropertyId)
        cache_request.AddProperty(UIAutomationClient.UIA_BoundingRectanglePropertyId)
        
        scanned_count = 0
        
        def scan_recursive(element: UIAElement, depth: int = 0) -> Dict[str, Any]:
            nonlocal scanned_count
            
            if depth > max_depth:
                return None
                
            # 使用缓存请求获取元素信息
            try:
                cached_element = element.element.BuildUpdatedCache(cache_request)
                if not cached_element:
                    return None
            except:
                return None
                
            # 获取基本属性
            result = {
                'id': element.element_id,
                'name': cached_element.CachedName or "",
                'automation_id': cached_element.CachedAutomationId or "",
                'class_name': cached_element.CachedClassName or "",
                'control_type': UIAModule.get_control_type_name(cached_element.CachedControlType),
                'is_enabled': cached_element.CachedIsEnabled,
                'depth': depth,
                'children': []
            }
            
            # 应用过滤器
            if filter_func and not filter_func(element):
                return None
                
            scanned_count += 1
            if progress_callback:
                progress_callback(scanned_count, f"Scanning depth {depth}...")
            
            # 获取子元素
            try:
                children = cached_element.GetCachedChildren()
                if children and children.Length > 0:
                    # 限制每层的子元素数量
                    child_count = min(children.Length, max_children_per_level)
                    for i in range(child_count):
                        child_element = children.GetElement(i)
                        if child_element:
                            child_id = UIAModule._element_counter
                            UIAModule._element_counter += 1
                            child = UIAElement(child_element, child_id)
                            child_result = scan_recursive(child, depth + 1)
                            if child_result:
                                result['children'].append(child_result)
            except:
                pass
                
            return result
            
        # 开始扫描
        try:
            tree = scan_recursive(root_element)
            if progress_callback:
                progress_callback(scanned_count, "Scan completed")
            return tree or {}
        except Exception as e:
            self.log(f"Error scanning tree: {str(e)}")
            return {}
    
    def find_elements_by_criteria(self, 
                                 root_element: UIAElement,
                                 criteria: Dict[str, Any],
                                 max_results: int = 100) -> List[Dict[str, Any]]:
        """根据条件查找元素
        
        Args:
            root_element: 根元素
            criteria: 搜索条件字典
            max_results: 最大结果数量
            
        Returns:
            匹配的元素列表
        """
        
        # 创建缓存请求
        cache_request = UIAModule._automation.CreateCacheRequest()
        cache_request.TreeScope = UIAutomationClient.TreeScope_Element
        # 添加需要的属性到缓存
        for property_name in criteria.keys():
            if property_name == 'Name':
                cache_request.AddProperty(UIAutomationClient.UIA_NamePropertyId)
            elif property_name == 'AutomationId':
                cache_request.AddProperty(UIAutomationClient.UIA_AutomationIdPropertyId)
            elif property_name == 'ClassName':
                cache_request.AddProperty(UIAutomationClient.UIA_ClassNamePropertyId)
            elif property_name == 'ControlType':
                cache_request.AddProperty(UIAutomationClient.UIA_ControlTypePropertyId)
            elif property_name == 'IsEnabled':
                cache_request.AddProperty(UIAutomationClient.UIA_IsEnabledPropertyId)
        
        results = []
        
        def check_element(element: UIAElement) -> bool:
            """检查元素是否匹配条件"""
            try:
                # 使用缓存请求获取属性
                cached_element = element.element.BuildUpdatedCache(cache_request)
                if not cached_element:
                    return False
                    
                for key, value in criteria.items():
                    if key == 'Name':
                        if value != (cached_element.CachedName or ""):
                            return False
                    elif key == 'AutomationId':
                        if value != (cached_element.CachedAutomationId or ""):
                            return False
                    elif key == 'ClassName':
                        if value != (cached_element.CachedClassName or ""):
                            return False
                    elif key == 'ControlType':
                        if value != UIAModule.get_control_type_name(cached_element.CachedControlType):
                            return False
                    elif key == 'IsEnabled':
                        if value != cached_element.CachedIsEnabled:
                            return False
                return True
            except:
                return False
        
        def search_recursive(element: UIAElement):
            """递归搜索匹配的元素"""
            if len(results) >= max_results:
                return
                
            # 检查当前元素
            if check_element(element):
                results.append(element.to_dict())
                
            # 获取子元素
            try:
                # 使用TreeScope_Children创建条件
                children = element.element.FindAll(
                    UIAutomationClient.TreeScope_Children,
                    UIAModule._automation.CreateTrueCondition()
                )
                if children:
                    for i in range(children.Length):
                        if len(results) >= max_results:
                            break
                        child_element = children.GetElement(i)
                        if child_element:
                            child = UIAElement(child_element, UIAModule._element_counter)
                            UIAModule._element_counter += 1
                            search_recursive(child)
            except:
                pass
        
        try:
            search_recursive(root_element)
        except Exception as e:
            self.log(f"Error searching elements: {str(e)}")
            
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
            'ui_tree': scan_result,
            'scan_stats': {
                'total_elements': scanned_count,
                'scan_time': time.time() - start_time,
                'max_depth': max_depth,
                'timestamp': datetime.now().isoformat()
            },
            'augment_elements': augment_elements,
            'interactive_elements': interactive_elements[:50],  # 限制数量
            'analysis_summary': {
                'total_elements': scanned_count,
                'augment_elements_count': len(augment_elements),
                'interactive_elements_count': len(interactive_elements),
                'scan_time': time.time() - start_time
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

    def search_text_in_tree(self, 
                           root_element: UIAElement,
                           search_phrases: List[str],
                           max_depth: int = 6,
                           case_sensitive: bool = False) -> List[Dict[str, Any]]:
        """在UI树中搜索文本
        
        Args:
            root_element: 根元素
            search_phrases: 要搜索的文本短语列表
            max_depth: 最大搜索深度
            case_sensitive: 是否区分大小写
            
        Returns:
            包含匹配结果的列表
        """
        
        # 创建缓存请求
        cache_request = UIAModule._automation.CreateCacheRequest()
        cache_request.TreeScope = UIAutomationClient.TreeScope_Element
        # 添加需要的属性和模式到缓存
        cache_request.AddProperty(UIAutomationClient.UIA_NamePropertyId)
        cache_request.AddProperty(UIAutomationClient.UIA_ControlTypePropertyId)
        cache_request.AddPattern(UIAutomationClient.UIA_TextPatternId)
        cache_request.AddPattern(UIAutomationClient.UIA_ValuePatternId)
        cache_request.AddPattern(UIAutomationClient.UIA_LegacyIAccessiblePatternId)
        
        results = []
        
        def search_recursive(element: UIAElement, depth: int = 0):
            """递归搜索文本"""
            if depth > max_depth:
                return
                
            try:
                # 使用缓存请求获取元素信息
                cached_element = element.element.BuildUpdatedCache(cache_request)
                if not cached_element:
                    return
                    
                # 获取文本内容
                text_content = ""
                
                # 尝试使用TextPattern
                try:
                    text_pattern = cached_element.GetCachedPattern(UIAutomationClient.UIA_TextPatternId)
                    if text_pattern:
                        document_range = text_pattern.DocumentRange
                        if document_range:
                            text_content = document_range.GetText(-1)
                except:
                    pass
                
                # 如果TextPattern失败，尝试其他模式
                if not text_content:
                    try:
                        value_pattern = cached_element.GetCachedPattern(UIAutomationClient.UIA_ValuePatternId)
                        if value_pattern:
                            text_content = value_pattern.CurrentValue
                    except:
                        pass
                        
                if not text_content:
                    try:
                        legacy_pattern = cached_element.GetCachedPattern(UIAutomationClient.UIA_LegacyIAccessiblePatternId)
                        if legacy_pattern:
                            text_content = legacy_pattern.CurrentValue
                    except:
                        pass
                
                # 最后尝试Name属性
                if not text_content:
                    text_content = cached_element.CachedName or ""
                
                # 搜索文本
                if text_content:
                    text_to_search = text_content if case_sensitive else text_content.lower()
                    
                    for phrase in search_phrases:
                        search_phrase = phrase if case_sensitive else phrase.lower()
                        if search_phrase in text_to_search:
                            results.append({
                                'element_id': element.element_id,
                                'found_phrase': phrase,
                                'text_content': text_content,
                                'control_type': UIAModule.get_control_type_name(cached_element.CachedControlType),
                                'name': cached_element.CachedName or "",
                                'depth': depth
                            })
                
                # 搜索子元素
                children = element.element.FindAll(
                    UIAutomationClient.TreeScope_Children,
                    UIAModule._automation.CreateTrueCondition()
                )
                if children:
                    for i in range(children.Length):
                        child_element = children.GetElement(i)
                        if child_element:
                            child = UIAElement(child_element, UIAModule._element_counter)
                            UIAModule._element_counter += 1
                            search_recursive(child, depth + 1)
                            
            except Exception as e:
                self.log(f"Error searching text in element: {str(e)}")
        
        try:
            search_recursive(root_element)
        except Exception as e:
            self.log(f"Error searching text in tree: {str(e)}")
            
        return results

    def dump_tree(self, max_depth=None, parallel=False, min_children_for_parallel=5):
        """获取UI树结构
        
        Args:
            max_depth: 最大深度,None表示不限制
            parallel: 是否并行处理
            min_children_for_parallel: 并行处理的最小子元素数量
        """
        try:
            print("\n=== 开始获取UI树 ===")
            print(f"配置: 最大深度={max_depth}, 并行={parallel}, 并行阈值={min_children_for_parallel}")
            
            import uiautomation as auto
            desktop = auto.GetRootControl()
            
            def process_element(element, depth=0):
                if max_depth is not None and depth > max_depth:
                    return
                    
                try:
                    name = element.Name
                    class_name = element.ClassName
                    control_type = element.ControlType
                    
                    indent = "  " * depth
                    print(f"{indent}名称: {name}")
                    print(f"{indent}类型: {control_type}")
                    print(f"{indent}类名: {class_name}")
                    
                    children = element.GetChildren()
                    if children:
                        print(f"{indent}子元素数量: {len(children)}")
                        
                        if parallel and len(children) >= min_children_for_parallel:
                            # 并行处理子元素
                            from concurrent.futures import ThreadPoolExecutor
                            with ThreadPoolExecutor() as executor:
                                executor.map(
                                    lambda child: process_element(child, depth + 1),
                                    children
                                )
                        else:
                            # 串行处理子元素
                            for child in children:
                                process_element(child, depth + 1)
                except Exception as e:
                    print(f"处理元素时出错: {str(e)}")
                    
            process_element(desktop)
            print("\n=== UI树获取完成 ===")
            
        except Exception as e:
            print(f"获取UI树时出错: {str(e)}")
            import traceback
            print(traceback.format_exc())

    def dump_vscode_tree(self, verbose: bool = False) -> Dict[str, Any]:
        """快速获取VSCode的UI树结构
        
        专门为VSCode优化的树结构导出:
        1. 只获取编辑器相关的UI元素
        2. 使用并行处理加速
        3. 智能过滤无关元素
        
        Returns:
            包含VSCode UI树的字典
        """
        
        module = UIAModule(verbose=verbose)
        if not module.initialize():
            return {'error': 'Failed to initialize UI Automation'}
        
        try:
            # 查找VSCode窗口
            vscode_info = module.find_vscode_window()
            if not vscode_info:
                return {'error': 'VSCode window not found'}
            
            # 获取根元素
            root_element = module.get_element_from_hwnd(vscode_info['hwnd'])
            if not root_element:
                return {'error': 'Failed to get root element'}
            
            # 创建过滤器函数
            def vscode_filter(element: UIAElement) -> bool:
                """过滤VSCode UI元素"""
                try:
                    name = element.get_property('Name', use_cache=True)
                    class_name = element.get_property('ClassName', use_cache=True)
                    control_type = element.get_property('ControlType', use_cache=True)
                    
                    # 排除一些明显无关的元素
                    if class_name and any(x in class_name.lower() for x in ['tooltip', 'popup', 'flyout']):
                        return False
                        
                    # 保留编辑器相关的元素
                    if name and any(x in name.lower() for x in ['editor', 'terminal', 'output', 'workbench']):
                        return True
                        
                    # 保留可能包含有用信息的元素
                    return True
                    
                except:
                    return True
                    
            # 使用优化的dump_tree方法
            result = module.dump_tree(
                root_element,
                max_depth=8,  # 增加深度以确保捕获所有重要元素
                parallel=True,
                min_children_for_parallel=3  # 降低并行阈值以提高性能
            )
            
            if not result.get('tree'):
                return {'error': 'Failed to dump tree'}
            
            # 添加VSCode特定的信息
            result['vscode_info'] = {
                'window_title': vscode_info.get('title', ''),
                'window_class': vscode_info.get('class_name', ''),
                'window_handle': vscode_info.get('hwnd', 0)
            }
            
            return result
            
        except Exception as e:
            return {
                'error': f'Error dumping VSCode tree: {str(e)}'
            }
        finally:
            module.cleanup()

    def click_try_again(self) -> bool:
        """点击VSCode augment对话框中的try again链接"""
        try:
            print("\n=== 开始查找try again链接 ===")
            
            import uiautomation as auto
            auto.SetGlobalSearchTimeout(1.0)  # 设置较短的超时时间
            
            # 查找VSCode窗口
            print("\n查找VSCode窗口...")
            desktop = auto.GetRootControl()
            vscode = None
            
            # 遍历顶级窗口
            windows = desktop.GetChildren()
            for window in windows:
                try:
                    name = window.Name
                    if name and "Visual Studio Code" in name:
                        print(f"找到VSCode窗口: {name}")
                        vscode = window
                        break
                except:
                    continue
            
            if not vscode:
                print("未找到VSCode窗口")
                return False
            
            # 尝试使用模拟鼠标点击
            print("\n尝试使用模拟鼠标点击...")
            
            # 确保VSCode窗口处于活动状态
            try:
                vscode.SetFocus()
                print("已激活VSCode窗口")
                
                # 等待窗口完全激活
                import time
                time.sleep(0.5)
                
                # 获取窗口位置和大小
                try:
                    rect = vscode.BoundingRectangle
                    print(f"窗口位置: {rect}")
                    
                    # 计算点击位置（在窗口中心偏右下方区域）
                    width = rect.width() if callable(rect.width) else rect.width
                    height = rect.height() if callable(rect.height) else rect.height
                    
                    x = rect.left + int(width * 0.7)  # 窗口右侧70%位置
                    y = rect.top + int(height * 0.7)   # 窗口下方70%位置
                    
                    print(f"尝试点击位置: ({x}, {y})")
                    
                    # 移动鼠标并点击
                    auto.SetCursorPos(x, y)
                    time.sleep(0.1)
                    auto.Click(x, y)
                    
                    # 等待一下看是否有反应
                    time.sleep(0.5)
                    
                    # 尝试在周围区域点击
                    offsets = [(0, 0), (-50, 0), (50, 0), (0, -50), (0, 50)]
                    for dx, dy in offsets:
                        new_x = x + dx
                        new_y = y + dy
                        print(f"尝试点击位置: ({new_x}, {new_y})")
                        auto.SetCursorPos(new_x, new_y)
                        time.sleep(0.1)
                        auto.Click(new_x, new_y)
                        time.sleep(0.5)
                        
                        # 检查是否有新的对话框出现
                        try:
                            focused = auto.GetFocusedControl()
                            if focused:
                                name = focused.Name
                                if name and "try again" in name.lower():
                                    print(f"找到try again元素: {name}")
                                    auto.SendKeys('{ENTER}')
                                    print("点击成功")
                                    return True
                        except:
                            pass
                    
                    print("点击完成")
                    return True
                    
                except Exception as e:
                    print(f"获取窗口位置失败: {str(e)}")
                    return False
                
            except Exception as e:
                print(f"设置焦点失败: {str(e)}")
                return False
                
        except Exception as e:
            print(f"\n出错: {str(e)}")
            import traceback
            print(traceback.format_exc())
            return False

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

def search_keep_going_text(verbose: bool = False) -> List[Dict[str, Any]]:
    """搜索'Would you like me to keep going'文本的便捷函数"""
    module = UIAModule(verbose=verbose)
    try:
        vscode_window = module.find_vscode_window()
        if not vscode_window:
            return []
        
        root_element = module.get_element_from_hwnd(int(vscode_window['id']))
        if not root_element:
            return []
        
        search_phrases = [
            "Would you like me to keep going",
            "keep going",
            "continue",
            "继续"
        ]
        
        return module.search_text_in_tree(
            root_element,
            search_phrases,
            max_depth=8,  # 增加搜索深度
            case_sensitive=False
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