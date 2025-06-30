#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode Augment 对话处理工具
使用优化的UIA模块分析VSCode界面，检测"Would you like me to keep going"并自动回复"请继续"

Features:
    - 智能检测Augment对话内容
    - 自动识别"Would you like me to keep going"提示
    - 自动输入"请继续"并发送
    - 支持监控模式持续检测
    - 详细的分析和操作日志
"""

import sys
import os
import time
import argparse
from datetime import datetime
from typing import Dict, List, Optional, Any

# 添加当前目录到路径
sys.path.append(os.path.dirname(__file__))

try:
    from uia_module import UIAModule, UIAElement, quick_analyze_vscode
    from uictrl import UIController
except ImportError as e:
    print(f"导入模块失败: {e}")
    print("请确保 uia_module.py 和 uictrl.py 在同一目录")
    sys.exit(1)

class VSCodeAugmentHandler:
    """VSCode Augment 对话处理器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.uia_module = UIAModule(verbose=verbose)
        self.ui_controller = UIController()
        self.vscode_window = None
        
    def log(self, message: str, level: str = "INFO"):
        """日志输出"""
        if self.verbose:
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"[{timestamp}] [{level}] {message}")
    
    def find_vscode_window(self) -> bool:
        """查找并缓存VSCode窗口信息"""
        self.vscode_window = self.uia_module.find_vscode_window()
        if self.vscode_window:
            self.log(f"找到VSCode窗口: {self.vscode_window['title']}")
            return True
        else:
            self.log("未找到VSCode窗口", "ERROR")
            return False
    
    def analyze_augment_content(self) -> Dict[str, Any]:
        """分析Augment区域的内容"""
        if not self.vscode_window:
            if not self.find_vscode_window():
                return {'error': '未找到VSCode窗口'}
        
        hwnd = int(self.vscode_window['id'])
        root_element = self.uia_module.get_element_from_hwnd(hwnd)
        if not root_element:
            return {'error': '无法获取UI根元素'}
        
        self.log("开始分析Augment内容...")
        
        # 查找所有包含文本的元素
        all_elements = []
        
        def collect_text_elements(element: UIAElement, depth: int = 0):
            if depth > 8:  # 限制深度避免过深搜索
                return
            
            element_data = element.to_dict()
            
            # 收集有文本内容的元素
            name = element_data.get('name', '')
            value = element_data.get('value', '')
            text_content = element_data.get('text_content', '')
            
            if any([name, value, text_content]):
                all_elements.append({
                    'depth': depth,
                    'control_type': element_data.get('control_type'),
                    'name': name,
                    'value': value,
                    'text_content': text_content,
                    'is_enabled': element_data.get('is_enabled'),
                    'is_visible': element_data.get('is_visible'),
                    'bounding_rect': element_data.get('bounding_rect')
                })
            
            # 递归搜索子元素
            try:
                children = element.get_children()
                for child in children:
                    collect_text_elements(child, depth + 1)
            except:
                pass
        
        collect_text_elements(root_element)
        
        # 分析收集到的元素
        augment_elements = []
        keep_going_elements = []
        
        for elem in all_elements:
            # 查找Augment相关元素
            for text_field in ['name', 'value', 'text_content']:
                text = elem.get(text_field, '').lower()
                if 'augment' in text:
                    augment_elements.append(elem)
                    break
            
            # 查找"Would you like me to keep going"
            for text_field in ['name', 'value', 'text_content']:
                text = elem.get(text_field, '').lower()
                if 'would you like me to keep going' in text or 'keep going' in text:
                    keep_going_elements.append(elem)
                    self.log(f"发现'keep going'提示: {elem.get(text_field, '')}")
                    break
        
        return {
            'total_elements': len(all_elements),
            'augment_elements': augment_elements,
            'keep_going_elements': keep_going_elements,
            'all_text_elements': all_elements[:50] if self.verbose else []  # 限制输出数量
        }
    
    def find_input_area(self) -> Optional[Dict[str, Any]]:
        """查找输入区域"""
        if not self.vscode_window:
            return None
        
        hwnd = int(self.vscode_window['id'])
        root_element = self.uia_module.get_element_from_hwnd(hwnd)
        if not root_element:
            return None
        
        # 查找编辑框或输入区域
        input_criteria = [
            {'control_type': 'Edit'},
            {'name': 'Ask or instruct Augment Agent'},
            {'name': 'augment'},
        ]
        
        for criteria in input_criteria:
            elements = self.uia_module.find_elements_by_criteria(
                root_element, criteria, max_results=10
            )
            
            for elem in elements:
                if elem.get('is_enabled') and elem.get('is_visible'):
                    self.log(f"找到可能的输入区域: {elem.get('name', '')} ({elem.get('control_type')})")
                    return elem
        
        return None
    
    def send_continue_message(self) -> bool:
        """发送"请继续"消息"""
        if not self.vscode_window:
            return False
        
        hwnd = int(self.vscode_window['id'])
        
        try:
            # 方法1: 直接输入文本
            self.log("尝试输入'请继续'...")
            success = self.ui_controller.type_text(hwnd, "请继续")
            if success:
                time.sleep(0.5)  # 等待输入完成
                
                # 发送回车键
                self.log("发送回车键...")
                success = self.ui_controller.send_keys(hwnd, "enter")
                if success:
                    self.log("成功发送'请继续'消息", "SUCCESS")
                    return True
            
            # 方法2: 如果直接输入失败，尝试先点击输入区域
            input_area = self.find_input_area()
            if input_area and input_area.get('bounding_rect'):
                rect = input_area['bounding_rect']
                center_x = rect['left'] + rect['width'] // 2
                center_y = rect['top'] + rect['height'] // 2
                
                self.log(f"尝试点击输入区域: ({center_x}, {center_y})")
                click_success = self.ui_controller.click_window(hwnd, center_x, center_y)
                
                if click_success:
                    time.sleep(0.3)
                    success = self.ui_controller.type_text(hwnd, "请继续")
                    if success:
                        time.sleep(0.5)
                        success = self.ui_controller.send_keys(hwnd, "enter")
                        if success:
                            self.log("通过点击输入区域成功发送消息", "SUCCESS")
                            return True
            
            self.log("发送消息失败", "ERROR")
            return False
            
        except Exception as e:
            self.log(f"发送消息时出错: {e}", "ERROR")
            return False
    
    def check_and_handle_keep_going(self) -> Dict[str, Any]:
        """检查并处理"keep going"提示"""
        result = {
            'found_keep_going': False,
            'sent_continue': False,
            'analysis': None,
            'error': None
        }
        
        try:
            # 分析内容
            analysis = self.analyze_augment_content()
            result['analysis'] = analysis
            
            if 'error' in analysis:
                result['error'] = analysis['error']
                return result
            
            # 检查是否有"keep going"提示
            keep_going_elements = analysis.get('keep_going_elements', [])
            if keep_going_elements:
                result['found_keep_going'] = True
                self.log(f"检测到{len(keep_going_elements)}个'keep going'提示")
                
                # 发送"请继续"
                if self.send_continue_message():
                    result['sent_continue'] = True
                else:
                    result['error'] = '发送"请继续"失败'
            else:
                self.log("未检测到'keep going'提示")
        
        except Exception as e:
            result['error'] = str(e)
            self.log(f"处理过程中出错: {e}", "ERROR")
        
        return result
    
    def monitor_mode(self, interval: int = 5, max_iterations: int = 0):
        """监控模式，持续检查并处理"""
        self.log(f"启动监控模式，检查间隔: {interval}秒")
        
        iteration = 0
        while True:
            iteration += 1
            
            if max_iterations > 0 and iteration > max_iterations:
                self.log(f"达到最大迭代次数 {max_iterations}，退出监控")
                break
            
            self.log(f"=== 第 {iteration} 次检查 ===")
            
            result = self.check_and_handle_keep_going()
            
            if result.get('found_keep_going'):
                if result.get('sent_continue'):
                    self.log("成功处理'keep going'提示", "SUCCESS")
                else:
                    self.log("检测到'keep going'但处理失败", "WARNING")
            
            if result.get('error'):
                self.log(f"检查过程中出错: {result['error']}", "ERROR")
            
            # 等待下次检查
            self.log(f"等待 {interval} 秒后进行下次检查...")
            time.sleep(interval)

def main():
    parser = argparse.ArgumentParser(description='VSCode Augment 对话处理工具')
    parser.add_argument('--verbose', '-v', action='store_true', help='详细输出')
    parser.add_argument('--monitor', '-m', action='store_true', help='监控模式')
    parser.add_argument('--interval', '-i', type=int, default=5, help='监控间隔秒数 (默认: 5)')
    parser.add_argument('--max-iterations', type=int, default=0, help='最大监控次数 (0=无限制)')
    parser.add_argument('--analyze-only', action='store_true', help='仅分析不执行操作')
    parser.add_argument('--save-analysis', action='store_true', help='保存分析结果到文件')
    
    args = parser.parse_args()
    
    handler = VSCodeAugmentHandler(verbose=args.verbose)
    
    if not handler.find_vscode_window():
        print("错误: 未找到VSCode窗口")
        return 1
    
    if args.monitor:
        # 监控模式
        try:
            handler.monitor_mode(args.interval, args.max_iterations)
        except KeyboardInterrupt:
            print("\n监控已停止")
            return 0
    else:
        # 单次检查模式
        if args.analyze_only:
            # 仅分析模式
            analysis = handler.analyze_augment_content()
            print(f"分析结果:")
            print(f"  总元素数: {analysis.get('total_elements', 0)}")
            print(f"  Augment元素数: {len(analysis.get('augment_elements', []))}")
            print(f"  Keep Going元素数: {len(analysis.get('keep_going_elements', []))}")
            
            if args.save_analysis:
                import json
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"tools/vscode_augment_analysis_{timestamp}.json"
                with open(filename, 'w', encoding='utf-8') as f:
                    json.dump(analysis, f, ensure_ascii=False, indent=2)
                print(f"分析结果已保存到: {filename}")
        else:
            # 检查并处理
            result = handler.check_and_handle_keep_going()
            
            if result.get('found_keep_going'):
                if result.get('sent_continue'):
                    print("✅ 成功检测并处理'Would you like me to keep going'提示")
                else:
                    print("⚠️  检测到'Would you like me to keep going'但处理失败")
                    if result.get('error'):
                        print(f"错误: {result['error']}")
            else:
                print("ℹ️  未检测到'Would you like me to keep going'提示")
            
            if result.get('error'):
                print(f"❌ 处理过程中出错: {result['error']}")
                return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())