#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode UI Automation 修复版
修复了COM指针访问问题的UI Automation工具

Requirements:
    pip install comtypes pywin32

Usage:
    python tools/vscode_ui_automation_fixed.py
    python tools/vscode_ui_automation_fixed.py --verbose
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

class VSCodeUIAutomationAnalyzer:
    def __init__(self, verbose=False):
        self.verbose = verbose
        self.automation = None
        
    def log(self, message):
        if self.verbose:
            print(f"[UIA] {message}")
    
    def find_vscode_window(self):
        """查找VSCode窗口"""
        controller = UIController()
        ide_windows = controller.find_ide_windows()
        
        for window in ide_windows:
            if 'visual studio code' in window['title'].lower():
                return window
        return None
    
    def initialize_ui_automation(self):
        """正确初始化UI Automation，避免COM指针问题"""
        if not UI_AUTOMATION_AVAILABLE:
            self.log("UI Automation不可用，需要安装comtypes")
            return False
        
        try:
            # 初始化COM
            pythoncom.CoInitialize()
            
            self.log("尝试创建UI Automation对象...")
            
            # 方法1：直接创建对象
            try:
                self.automation = comtypes.client.CreateObject(
                    UIAutomationClient.CUIAutomation,
                    interface=UIAutomationClient.IUIAutomation
                )
                self.log("✓ 成功创建UI Automation对象")
                return True
            except Exception as e:
                self.log(f"✗ 方法1失败: {e}")
            
            # 方法2：使用CLSID
            try:
                self.automation = comtypes.client.CreateObject(
                    "{ff48dba4-60ef-4201-aa87-54103eef594e}",
                    interface=UIAutomationClient.IUIAutomation
                )
                self.log("✓ 成功创建UI Automation对象（CLSID）")
                return True
            except Exception as e:
                self.log(f"✗ 方法2失败: {e}")
            
            return False
            
        except Exception as e:
            self.log(f"初始化UI Automation失败: {e}")
            return False
    
    def get_element_safely(self, hwnd):
        """安全地获取UI元素，避免COM指针问题"""
        try:
            if not self.automation:
                self.log("UI Automation对象未初始化")
                return None
            
            # 确保hwnd是有效的
            if not win32gui.IsWindow(hwnd):
                self.log("无效的窗口句柄")
                return None
            
            element = self.automation.ElementFromHandle(hwnd)
            if not element:
                self.log("无法从窗口句柄获取UI元素")
                return None
            
            # 测试元素是否可访问
            try:
                _ = element.CurrentName
                self.log("✓ UI元素可访问")
                return element
            except Exception as e:
                self.log(f"✗ UI元素不可访问: {e}")
                return None
                
        except Exception as e:
            self.log(f"获取UI元素失败: {e}")
            return None
    
    def extract_text_safely(self, element):
        """安全地提取文本内容"""
        text_contents = []
        
        try:
            # 获取基本属性
            try:
                name = element.CurrentName
                if name and name.strip():
                    text_contents.append(f"Name: {name.strip()}")
            except:
                pass
            
            try:
                automation_id = element.CurrentAutomationId
                if automation_id and automation_id.strip():
                    text_contents.append(f"AutomationId: {automation_id.strip()}")
            except:
                pass
            
            try:
                class_name = element.CurrentClassName
                if class_name and class_name.strip():
                    text_contents.append(f"ClassName: {class_name.strip()}")
            except:
                pass
            
            # 尝试获取Value pattern
            try:
                value_pattern = element.GetCurrentPattern(UIAutomationClient.UIA_ValuePatternId)
                if value_pattern:
                    value = value_pattern.CurrentValue
                    if value and value.strip():
                        text_contents.append(f"Value: {value.strip()}")
            except:
                pass
            
            # 尝试获取Text pattern
            try:
                text_pattern = element.GetCurrentPattern(UIAutomationClient.UIA_TextPatternId)
                if text_pattern:
                    document_range = text_pattern.DocumentRange
                    if document_range:
                        text = document_range.GetText(-1)
                        if text and text.strip():
                            text_contents.append(f"Text: {text.strip()}")
            except:
                pass
            
            # 尝试获取LegacyIAccessible pattern
            try:
                legacy_pattern = element.GetCurrentPattern(UIAutomationClient.UIA_LegacyIAccessiblePatternId)
                if legacy_pattern:
                    name = legacy_pattern.CurrentName
                    value = legacy_pattern.CurrentValue
                    if name and name.strip():
                        text_contents.append(f"Legacy Name: {name.strip()}")
                    if value and value.strip():
                        text_contents.append(f"Legacy Value: {value.strip()}")
            except:
                pass
            
        except Exception as e:
            self.log(f"提取文本时出错: {e}")
        
        return text_contents
    
    def scan_elements_recursively(self, element, depth=0, max_depth=3):
        """递归扫描元素"""
        if depth > max_depth:
            return []
        
        results = []
        
        try:
            # 获取当前元素的文本
            text_content = self.extract_text_safely(element)
            if text_content:
                results.extend(text_content)
                
                if self.verbose:
                    indent = "  " * depth
                    for content in text_content:
                        self.log(f"{indent}{content[:60]}...")
            
            # 查找子元素
            try:
                # 使用更安全的查找方法
                condition = self.automation.CreateTrueCondition()
                children = element.FindAll(UIAutomationClient.TreeScope_Children, condition)
                
                if children and children.Length > 0:
                    # 限制扫描的子元素数量
                    max_children = min(children.Length, 20)
                    self.log(f"扫描 {max_children}/{children.Length} 个子元素...")
                    
                    for i in range(max_children):
                        try:
                            child = children.GetElement(i)
                            if child:
                                child_results = self.scan_elements_recursively(child, depth + 1, max_depth)
                                results.extend(child_results)
                        except Exception as e:
                            self.log(f"处理子元素 {i} 时出错: {e}")
                            continue
                            
            except Exception as e:
                self.log(f"查找子元素失败: {e}")
        
        except Exception as e:
            self.log(f"扫描元素失败: {e}")
        
        return results
    
    def find_specific_text(self, element, search_patterns):
        """查找包含特定文本的元素"""
        found_items = []
        
        try:
            for pattern in search_patterns:
                self.log(f"搜索模式: {pattern}")
                
                # 创建条件查找包含特定文本的元素
                try:
                    condition = self.automation.CreatePropertyCondition(
                        UIAutomationClient.UIA_NamePropertyId,
                        pattern
                    )
                    
                    elements = element.FindAll(UIAutomationClient.TreeScope_Descendants, condition)
                    
                    if elements and elements.Length > 0:
                        self.log(f"找到 {elements.Length} 个匹配 '{pattern}' 的元素")
                        
                        for i in range(elements.Length):
                            try:
                                match_element = elements.GetElement(i)
                                text_content = self.extract_text_safely(match_element)
                                if text_content:
                                    found_items.append({
                                        'pattern': pattern,
                                        'content': text_content
                                    })
                            except Exception as e:
                                self.log(f"处理匹配元素时出错: {e}")
                                
                except Exception as e:
                    self.log(f"搜索模式 '{pattern}' 失败: {e}")
                    
        except Exception as e:
            self.log(f"查找特定文本失败: {e}")
        
        return found_items
    
    def analyze_vscode(self):
        """分析VSCode内容"""
        self.log("开始分析VSCode...")
        
        # 查找VSCode窗口
        vscode_window = self.find_vscode_window()
        if not vscode_window:
            print("错误: 未找到VSCode窗口")
            return None
        
        self.log(f"找到VSCode窗口: {vscode_window['title']} (ID: {vscode_window['id']})")
        
        # 初始化UI Automation
        if not self.initialize_ui_automation():
            print("错误: UI Automation初始化失败")
            return None
        
        # 获取根元素
        hwnd = int(vscode_window['id'])
        root_element = self.get_element_safely(hwnd)
        if not root_element:
            print("错误: 无法获取UI根元素")
            return None
        
        results = {
            'window_info': vscode_window,
            'extracted_text': [],
            'specific_matches': [],
            'timestamp': datetime.now().isoformat()
        }
        
        try:
            # 扫描所有元素
            self.log("开始扫描UI元素...")
            extracted_text = self.scan_elements_recursively(root_element, max_depth=2)
            results['extracted_text'] = extracted_text
            
            # 查找特定文本
            search_patterns = [
                "continue",
                "try again",
                "error",
                "failed",
                "Would you like me to keep going",
                "Please try again",
                "We encountered an issue"
            ]
            
            self.log("查找特定文本模式...")
            specific_matches = self.find_specific_text(root_element, search_patterns)
            results['specific_matches'] = specific_matches
            
            # 输出结果
            print(f"\n=== VSCode UI Automation 分析结果 ===")
            print(f"窗口: {vscode_window['title']}")
            print(f"提取文本数量: {len(extracted_text)}")
            print(f"特定匹配数量: {len(specific_matches)}")
            
            if extracted_text:
                print(f"\n=== 提取的文本内容 ===")
                for i, text in enumerate(extracted_text[:20]):  # 显示前20个
                    print(f"{i+1:2d}. {text}")
                
                if len(extracted_text) > 20:
                    print(f"... 还有 {len(extracted_text) - 20} 项")
            
            if specific_matches:
                print(f"\n=== 特定匹配项 ===")
                for match in specific_matches:
                    print(f"模式: {match['pattern']}")
                    for content in match['content']:
                        print(f"  - {content}")
                    print()
            
            return results
            
        except Exception as e:
            self.log(f"分析过程中出错: {e}")
            return results
        
        finally:
            # 清理COM对象
            try:
                self.automation = None
                pythoncom.CoUninitialize()
            except:
                pass

def main():
    parser = argparse.ArgumentParser(description="VSCode UI Automation 修复版分析工具")
    parser.add_argument("--verbose", action="store_true", help="详细输出")
    
    args = parser.parse_args()
    
    analyzer = VSCodeUIAutomationAnalyzer(verbose=args.verbose)
    results = analyzer.analyze_vscode()
    
    if results:
        print(f"\n分析完成，提取了 {len(results['extracted_text'])} 个文本项")
    else:
        print("分析失败")

if __name__ == "__main__":
    main() 