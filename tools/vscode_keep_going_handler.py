#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode "Keep Going" 自动处理工具
专门检测VSCode中Augment对话的"Would you like me to keep going?"提示，并自动回复"请继续"

使用方法:
    python vscode_keep_going_handler.py                    # 单次检查
    python vscode_keep_going_handler.py --monitor          # 监控模式
    python vscode_keep_going_handler.py --monitor -i 3     # 3秒间隔监控
"""

import sys
import os
import time
import argparse
from datetime import datetime

# 添加当前目录到路径
sys.path.append(os.path.dirname(__file__))

try:
    from uia_module import UIAModule
    from uictrl import UIController
except ImportError as e:
    print(f"❌ 导入模块失败: {e}")
    print("请确保 uia_module.py 和 uictrl.py 在同一目录")
    sys.exit(1)

class KeepGoingHandler:
    """Keep Going 处理器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.uia_module = UIAModule(verbose=False)  # UIA模块不输出详细日志
        self.ui_controller = UIController()
        
    def log(self, message: str, emoji: str = "ℹ️"):
        """日志输出"""
        if self.verbose:
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"[{timestamp}] {emoji} {message}")
        else:
            print(f"{emoji} {message}")
    
    def find_keep_going_text(self) -> bool:
        """查找'Would you like me to keep going'文本"""
        # 查找VSCode窗口
        vscode_window = self.uia_module.find_vscode_window()
        if not vscode_window:
            self.log("未找到VSCode窗口", "❌")
            return False
        
        if self.verbose:
            self.log(f"找到VSCode窗口: {vscode_window['title']}")
        
        # 获取UI根元素
        hwnd = int(vscode_window['id'])
        root_element = self.uia_module.get_element_from_hwnd(hwnd)
        if not root_element:
            self.log("无法获取UI根元素", "❌")
            return False
        
        # 快速搜索包含关键文本的元素
        keep_going_phrases = [
            "would you like me to keep going",
            "keep going",
            "continue",
            "继续"
        ]
        
        def search_text_recursive(element, depth=0):
            if depth > 6:  # 限制搜索深度
                return False
            
            try:
                # 获取元素文本
                element_data = element.to_dict(include_patterns=False, include_rect=False)
                
                # 检查各种文本字段
                texts_to_check = [
                    element_data.get('name', ''),
                    element_data.get('value', ''),
                    element_data.get('text_content', '')
                ]
                
                for text in texts_to_check:
                    if text:
                        text_lower = text.lower()
                        for phrase in keep_going_phrases:
                            if phrase in text_lower:
                                if self.verbose:
                                    self.log(f"找到匹配文本: '{text[:100]}...'")
                                return True
                
                # 搜索子元素
                children = element.get_children()
                for child in children:
                    if search_text_recursive(child, depth + 1):
                        return True
                        
            except Exception as e:
                if self.verbose:
                    self.log(f"搜索元素时出错: {e}")
                pass
            
            return False
        
        return search_text_recursive(root_element)
    
    def send_continue_response(self) -> bool:
        """发送'请继续'回复"""
        # 查找VSCode窗口
        vscode_window = self.uia_module.find_vscode_window()
        if not vscode_window:
            return False
        
        hwnd = int(vscode_window['id'])
        
        try:
            # 确保窗口处于前台
            self.ui_controller.bring_window_to_front(hwnd)
            time.sleep(0.2)
            
            # 输入"请继续"
            success = self.ui_controller.type_text(hwnd, "请继续")
            if not success:
                self.log("输入文本失败", "❌")
                return False
            
            time.sleep(0.3)  # 等待输入完成
            
            # 发送回车键
            success = self.ui_controller.send_keys(hwnd, "enter")
            if not success:
                self.log("发送回车键失败", "❌")
                return False
            
            return True
            
        except Exception as e:
            self.log(f"发送回复时出错: {e}", "❌")
            return False
    
    def check_and_handle(self) -> dict:
        """检查并处理keep going提示"""
        result = {
            'found': False,
            'handled': False,
            'error': None
        }
        
        try:
            # 检查是否有keep going提示
            if self.find_keep_going_text():
                result['found'] = True
                self.log("检测到'Would you like me to keep going'提示", "🔍")
                
                # 发送回复
                if self.send_continue_response():
                    result['handled'] = True
                    self.log("已自动回复'请继续'", "✅")
                else:
                    result['error'] = "发送回复失败"
                    self.log("发送回复失败", "❌")
            else:
                if self.verbose:
                    self.log("未检测到'keep going'提示")
                    
        except Exception as e:
            result['error'] = str(e)
            self.log(f"处理过程出错: {e}", "❌")
        
        return result
    
    def monitor(self, interval: int = 5, max_checks: int = 0):
        """监控模式"""
        self.log(f"开始监控模式 (间隔: {interval}秒)", "🚀")
        
        check_count = 0
        handled_count = 0
        
        try:
            while True:
                check_count += 1
                
                if max_checks > 0 and check_count > max_checks:
                    self.log(f"达到最大检查次数 {max_checks}", "🏁")
                    break
                
                if self.verbose:
                    self.log(f"第 {check_count} 次检查...")
                
                result = self.check_and_handle()
                
                if result['found']:
                    if result['handled']:
                        handled_count += 1
                        self.log(f"成功处理 (总计: {handled_count})", "🎉")
                    else:
                        self.log("检测到但处理失败", "⚠️")
                
                if result['error']:
                    self.log(f"检查出错: {result['error']}", "❌")
                
                # 等待下次检查
                time.sleep(interval)
                
        except KeyboardInterrupt:
            self.log(f"监控已停止 (共检查 {check_count} 次，处理 {handled_count} 次)", "👋")

def main():
    parser = argparse.ArgumentParser(
        description='VSCode "Keep Going" 自动处理工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  python vscode_keep_going_handler.py                    # 单次检查
  python vscode_keep_going_handler.py --monitor          # 监控模式
  python vscode_keep_going_handler.py --monitor -i 3     # 3秒间隔监控
  python vscode_keep_going_handler.py -v                 # 详细输出
        """
    )
    
    parser.add_argument('--monitor', '-m', action='store_true', 
                       help='监控模式，持续检查')
    parser.add_argument('--interval', '-i', type=int, default=5,
                       help='监控间隔秒数 (默认: 5)')
    parser.add_argument('--max-checks', type=int, default=0,
                       help='最大检查次数 (0=无限制)')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='详细输出')
    
    args = parser.parse_args()
    
    handler = KeepGoingHandler(verbose=args.verbose)
    
    if args.monitor:
        # 监控模式
        handler.monitor(args.interval, args.max_checks)
    else:
        # 单次检查
        result = handler.check_and_handle()
        
        if result['found']:
            if result['handled']:
                print("✅ 成功检测并处理'Would you like me to keep going'提示")
                return 0
            else:
                print("⚠️  检测到提示但处理失败")
                if result['error']:
                    print(f"❌ 错误: {result['error']}")
                return 1
        else:
            print("ℹ️  当前未检测到'Would you like me to keep going'提示")
            return 0

if __name__ == "__main__":
    sys.exit(main()) 