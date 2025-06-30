#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Augment 聊天对话框智能回复工具
专门处理VSCode中Augment对话的"Would you like me to keep going?"提示
确保在正确的对话框位置输入"请继续"
"""

import sys
import os
import time
import win32gui
import win32con
import win32api
from datetime import datetime
import win32clipboard

# 添加当前目录到路径
sys.path.append(os.path.dirname(__file__))

try:
    from uia_module import UIAModule
    from uictrl import UIController
except ImportError as e:
    print(f"❌ 导入模块失败: {e}")
    sys.exit(1)

class AugmentChatResponder:
    """Augment聊天对话框智能回复器"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.uia_module = UIAModule(verbose=verbose)
        self.ui_controller = UIController()
        
    def log(self, message: str, icon: str = "ℹ️"):
        """日志输出"""
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"[{timestamp}] {icon} {message}")
    
    def find_chat_input_box(self, root_element):
        """查找Augment聊天输入框"""
        chat_input_candidates = []
        
        # 搜索策略1: 查找包含"Ask or instruct"的元素附近的输入框
        try:
            results = self.uia_module.search_text_in_tree(
                root_element, 
                ["Ask or instruct", "instruct", "Ask"], 
                max_depth=15
            )
            
            for result in results:
                if self.verbose:
                    self.log(f"找到指导文本: '{result.get('text_content', '')[:50]}...'")
                
                # 获取这个元素的父元素，然后查找附近的编辑框
                element = result.get('uia_element')
                if element:
                    # 尝试在同一父元素下查找编辑框
                    try:
                        parent = element.GetCurrentParent()
                        children = parent.FindAll(
                            self.uia_module.ui_automation.CreateTreeScope_Children(),
                            self.uia_module.ui_automation.CreateTrueCondition()
                        )
                        
                        for i in range(children.Length):
                            child = children.GetElement(i)
                            control_type = child.GetCurrentControlType()
                            
                            # 查找编辑框类型的控件
                            if control_type in [50004, 50030]:  # Edit, Document
                                rect = child.GetCurrentBoundingRectangle()
                                if rect.width > 200 and rect.height > 30:  # 合理的输入框大小
                                    chat_input_candidates.append({
                                        'element': child,
                                        'rect': {
                                            'left': rect.left,
                                            'top': rect.top,
                                            'width': rect.width,
                                            'height': rect.height
                                        },
                                        'source': 'near_instruction'
                                    })
                                    if self.verbose:
                                        self.log(f"找到候选输入框: {rect.width}x{rect.height} at ({rect.left}, {rect.top})")
                    except:
                        pass
        except Exception as e:
            if self.verbose:
                self.log(f"搜索指导文本时出错: {e}")
        
        # 搜索策略2: 查找底部的编辑框（通常聊天输入框在底部）
        try:
            all_elements = self.uia_module.search_text_in_tree(
                root_element, 
                [""],  # 空字符串会匹配所有文本元素
                max_depth=15
            )
            
            edit_boxes = []
            for result in all_elements:
                control_type = result.get('control_type', '')
                if control_type in ['Edit', 'Document']:
                    rect = result.get('bounding_rect')
                    if rect and rect['width'] > 200 and rect['height'] > 20:
                        edit_boxes.append(result)
            
            # 按Y坐标排序，找到最底部的编辑框
            edit_boxes.sort(key=lambda x: x.get('bounding_rect', {}).get('top', 0), reverse=True)
            
            for edit_box in edit_boxes[:3]:  # 只检查最底部的3个
                rect = edit_box.get('bounding_rect')
                chat_input_candidates.append({
                    'element': edit_box.get('uia_element'),
                    'rect': rect,
                    'source': 'bottom_edit_box'
                })
                if self.verbose:
                    self.log(f"找到底部编辑框: {rect['width']}x{rect['height']} at ({rect['left']}, {rect['top']})")
                    
        except Exception as e:
            if self.verbose:
                self.log(f"搜索底部编辑框时出错: {e}")
        
        return chat_input_candidates
    
    def click_and_focus_input(self, hwnd, rect):
        """点击并聚焦到输入框"""
        try:
            # 激活窗口
            win32gui.SetForegroundWindow(hwnd)
            time.sleep(0.2)
            
            # 计算点击位置（输入框的中心）
            click_x = rect['left'] + rect['width'] // 2
            click_y = rect['top'] + rect['height'] // 2
            
            if self.verbose:
                self.log(f"在位置 ({click_x}, {click_y}) 点击输入框")
            
            # 移动鼠标并点击
            win32api.SetCursorPos((click_x, click_y))
            time.sleep(0.1)
            
            # 执行点击
            win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
            win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
            time.sleep(0.3)
            
            return True
            
        except Exception as e:
            if self.verbose:
                self.log(f"点击输入框失败: {e}")
            return False
    
    def clear_and_type_response(self, hwnd):
        """清除输入框并输入回复"""
        try:
            # 全选现有内容
            win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
            win32api.keybd_event(ord('A'), 0, 0, 0)
            win32api.keybd_event(ord('A'), 0, win32con.KEYEVENTF_KEYUP, 0)
            win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
            time.sleep(0.1)
            
            # 删除现有内容
            win32api.keybd_event(win32con.VK_DELETE, 0, 0, 0)
            win32api.keybd_event(win32con.VK_DELETE, 0, win32con.KEYEVENTF_KEYUP, 0)
            time.sleep(0.1)
            
            # 使用剪贴板输入中文
            win32clipboard.OpenClipboard()
            win32clipboard.EmptyClipboard()
            win32clipboard.SetClipboardText("请继续")
            win32clipboard.CloseClipboard()
            
            # Ctrl+V 粘贴
            win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
            win32api.keybd_event(ord('V'), 0, 0, 0)
            win32api.keybd_event(ord('V'), 0, win32con.KEYEVENTF_KEYUP, 0)
            win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
            
            time.sleep(0.3)
            
            # 发送回车键
            self.log("发送回车键...")
            win32api.keybd_event(win32con.VK_RETURN, 0, 0, 0)
            win32api.keybd_event(win32con.VK_RETURN, 0, win32con.KEYEVENTF_KEYUP, 0)
            
            return True
            
        except Exception as e:
            self.log(f"输入回复失败: {e}", "❌")
            return False
    
    def respond_to_keep_going_prompt(self):
        """检测并响应'keep going'提示"""
        try:
            # 查找VSCode窗口
            vscode_window = self.uia_module.find_vscode_window()
            if not vscode_window:
                self.log("未找到VSCode窗口", "❌")
                return False
            
            hwnd = int(vscode_window['id'])
            root_element = self.uia_module.get_element_from_hwnd(hwnd)
            if not root_element:
                self.log("无法获取UI根元素", "❌")
                return False
            
            # 1. 首先检查是否有"keep going"提示
            self.log("检查是否存在'keep going'提示...")
            found_prompt = False
            prompt_text = ""
            
            for keyword in ["going", "keep", "would"]:
                try:
                    results = self.uia_module.search_text_in_tree(
                        root_element,
                        [keyword],
                        max_depth=12,
                        case_sensitive=False
                    )
                    
                    for result in results:
                        text_content = result.get('text_content', '').lower()
                        if 'would' in text_content and 'keep' in text_content and 'going' in text_content:
                            found_prompt = True
                            prompt_text = result.get('text_content', '')
                            self.log(f"检测到提示: '{prompt_text[:60]}...'", "🎯")
                            break
                    
                    if found_prompt:
                        break
                        
                except Exception:
                    pass
            
            if not found_prompt:
                self.log("未检测到'keep going'提示")
                return False
            
            # 2. 查找聊天输入框
            self.log("查找Augment聊天输入框...")
            input_candidates = self.find_chat_input_box(root_element)
            
            if not input_candidates:
                self.log("未找到聊天输入框，使用通用方法", "⚠️")
                # 备用方法：激活窗口后直接输入
                win32gui.SetForegroundWindow(hwnd)
                time.sleep(0.5)
                return self.clear_and_type_response(hwnd)
            
            # 3. 尝试使用找到的输入框
            for i, candidate in enumerate(input_candidates):
                self.log(f"尝试输入框 {i+1}/{len(input_candidates)} (来源: {candidate['source']})")
                
                # 点击并聚焦到输入框
                if self.click_and_focus_input(hwnd, candidate['rect']):
                    # 输入回复
                    if self.clear_and_type_response(hwnd):
                        self.log("成功发送回复!", "✅")
                        return True
                    else:
                        self.log(f"输入框 {i+1} 输入失败，尝试下一个...")
                        continue
                else:
                    self.log(f"输入框 {i+1} 点击失败，尝试下一个...")
                    continue
            
            self.log("所有输入框尝试失败", "❌")
            return False
            
        except Exception as e:
            self.log(f"处理过程出错: {e}", "❌")
            return False

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Augment聊天对话框智能回复工具')
    parser.add_argument('--verbose', '-v', action='store_true', help='详细输出')
    parser.add_argument('--monitor', action='store_true', help='持续监控模式')
    parser.add_argument('--interval', type=int, default=5, help='监控间隔(秒)')
    parser.add_argument('--max-checks', type=int, default=10, help='最大检查次数')
    
    args = parser.parse_args()
    
    responder = AugmentChatResponder(verbose=args.verbose)
    
    if args.monitor:
        # 监控模式
        print(f"🔄 开始监控模式 (间隔: {args.interval}秒, 最大检查: {args.max_checks}次)")
        
        for i in range(args.max_checks):
            print(f"\n=== 检查 {i+1}/{args.max_checks} ===")
            
            success = responder.respond_to_keep_going_prompt()
            if success:
                print("✅ 检测到提示并成功处理!")
                break
            else:
                print("ℹ️ 未检测到需要处理的提示")
            
            if i < args.max_checks - 1:  # 不是最后一次检查
                print(f"⏳ 等待 {args.interval} 秒后继续...")
                time.sleep(args.interval)
        
        print(f"\n🏁 监控结束")
    else:
        # 单次检查
        print("🔍 检查'keep going'提示并自动响应...")
        success = responder.respond_to_keep_going_prompt()
        if success:
            print("✅ 成功检测并处理提示!")
        else:
            print("ℹ️ 当前未检测到需要处理的提示")

if __name__ == "__main__":
    main() 