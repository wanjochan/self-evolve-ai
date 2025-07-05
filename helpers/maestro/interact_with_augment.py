#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import time
import json
import argparse
from pathlib import Path
import win32gui
import win32con
import win32api

# 添加当前目录到路径
sys.path.append(str(Path(__file__).parent))

# 尝试导入ui_ctrl_v2模块
try:
    from ui_ctrl_v2.input_controller import InputController
    from ui_ctrl_v2.window_capture import WindowCapture
    UI_CTRL_V2_AVAILABLE = True
except ImportError:
    print("警告: ui_ctrl_v2模块不可用，使用替代方法")
    UI_CTRL_V2_AVAILABLE = False

class AugmentInteractor:
    """与VSCode中的augment对话区域交互的类"""
    
    def __init__(self, window_title="Visual Studio Code"):
        """初始化交互器
        
        Args:
            window_title: VSCode窗口标题
        """
        self.window_title = window_title
        self.hwnd = None
        
        # 查找窗口
        self.find_window()
        
        # 初始化输入控制器
        if UI_CTRL_V2_AVAILABLE:
            self.input_controller = InputController()
            self.window_capture = WindowCapture()
        
        # 加载augment对话区域
        self.dialog_area = None
        self.input_area = None
        self.send_button = None
        self.load_dialog_areas()
    
    def find_window(self):
        """查找VSCode窗口"""
        def callback(hwnd, windows):
            if win32gui.IsWindowVisible(hwnd):
                title = win32gui.GetWindowText(hwnd)
                if self.window_title.lower() in title.lower():
                    windows.append((hwnd, title))
            return True
        
        windows = []
        win32gui.EnumWindows(callback, windows)
        
        if windows:
            self.hwnd = windows[0][0]
            print(f"找到窗口: {windows[0][1]} (HWND: {self.hwnd})")
            return True
        else:
            print(f"未找到窗口: {self.window_title}")
            return False
    
    def load_dialog_areas(self):
        """加载augment对话区域"""
        # 尝试从分析结果加载
        analysis_file = Path(__file__).parent / "augment_analysis.json"
        if analysis_file.exists():
            try:
                with open(analysis_file, 'r') as f:
                    data = json.load(f)
                
                if "dialog_areas" in data and data["dialog_areas"]:
                    self.dialog_area = data["dialog_areas"][0]["area"]
                    print(f"加载对话区域: {self.dialog_area}")
                    
                    # 根据对话区域估计输入区域和发送按钮
                    self.estimate_input_area()
                    return True
            except Exception as e:
                print(f"加载分析结果失败: {e}")
        
        # 如果无法加载，使用默认值
        print("使用默认对话区域坐标")
        self.dialog_area = [496, 283, 839, 598]
        self.estimate_input_area()
        return False
    
    def estimate_input_area(self):
        """根据对话区域估计输入区域和发送按钮"""
        if not self.dialog_area:
            return
        
        # 输入区域通常在对话区域下方
        x1, y1, x2, y2 = self.dialog_area
        input_y1 = y2 + 10
        input_y2 = input_y1 + 30
        
        self.input_area = [x1, input_y1, x2, input_y2]
        print(f"估计输入区域: {self.input_area}")
        
        # 发送按钮通常在输入区域右侧
        button_x1 = x2 + 5
        button_x2 = button_x1 + 30
        button_y1 = input_y1
        button_y2 = input_y2
        
        self.send_button = [button_x1, button_y1, button_x2, button_y2]
        print(f"估计发送按钮: {self.send_button}")
    
    def activate_window(self):
        """激活VSCode窗口"""
        if not self.hwnd:
            print("未找到窗口，无法激活")
            return False
        
        try:
            win32gui.SetForegroundWindow(self.hwnd)
            time.sleep(0.5)  # 等待窗口激活
            print("窗口已激活")
            return True
        except Exception as e:
            print(f"激活窗口失败: {e}")
            return False
    
    def click_input_area(self):
        """点击输入区域"""
        if not self.input_area:
            print("未找到输入区域，无法点击")
            return False
        
        x = (self.input_area[0] + self.input_area[2]) // 2
        y = (self.input_area[1] + self.input_area[3]) // 2
        
        if UI_CTRL_V2_AVAILABLE:
            self.input_controller.click(x, y)
        else:
            # 使用替代方法
            self.activate_window()
            
            # 获取窗口位置
            try:
                left, top, right, bottom = win32gui.GetWindowRect(self.hwnd)
                # 计算窗口内的相对坐标
                screen_x = left + x
                screen_y = top + y
                
                # 设置鼠标位置并点击
                win32api.SetCursorPos((screen_x, screen_y))
                time.sleep(0.1)
                win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
                time.sleep(0.1)
                win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
                
                print(f"点击输入区域: 窗口坐标({x}, {y}), 屏幕坐标({screen_x}, {screen_y})")
                return True
            except Exception as e:
                print(f"点击输入区域失败: {e}")
                return False
        
        time.sleep(0.2)
        return True
    
    def send_message(self, message):
        """发送消息到augment对话区域
        
        Args:
            message: 要发送的消息
        """
        if not self.hwnd:
            print("未找到窗口，无法发送消息")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        # 点击输入区域
        if not self.click_input_area():
            return False
        
        # 输入消息
        if UI_CTRL_V2_AVAILABLE:
            self.input_controller.type_text(message)
        else:
            # 使用替代方法 - 使用剪贴板
            try:
                import pyperclip
                pyperclip.copy(message)
                
                # 按下Ctrl+V
                win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
                time.sleep(0.1)
                win32api.keybd_event(ord('V'), 0, 0, 0)
                time.sleep(0.1)
                win32api.keybd_event(ord('V'), 0, win32con.KEYEVENTF_KEYUP, 0)
                time.sleep(0.1)
                win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
                
                print(f"粘贴消息: {message}")
            except ImportError:
                print("pyperclip模块不可用，使用逐字符输入")
                # 逐字符输入
                for c in message:
                    try:
                        # 使用VkKeyScan获取虚拟键码
                        vk = win32api.VkKeyScan(c)
                        if vk == -1:
                            continue  # 跳过无法映射的字符
                        
                        # 提取虚拟键码和修饰键信息
                        vk_code = vk & 0xFF
                        shift_state = (vk >> 8) & 0xFF
                        
                        # 如果需要按下Shift键
                        if shift_state & 1:
                            win32api.keybd_event(win32con.VK_SHIFT, 0, 0, 0)
                        
                        # 按下并释放字符键
                        win32api.keybd_event(vk_code, 0, 0, 0)
                        time.sleep(0.01)
                        win32api.keybd_event(vk_code, 0, win32con.KEYEVENTF_KEYUP, 0)
                        
                        # 如果按下了Shift键，释放它
                        if shift_state & 1:
                            win32api.keybd_event(win32con.VK_SHIFT, 0, win32con.KEYEVENTF_KEYUP, 0)
                    except Exception as e:
                        print(f"输入字符 '{c}' 失败: {e}")
        
        time.sleep(0.2)
        print(f"输入消息: {message}")
        
        # 按回车键发送
        if UI_CTRL_V2_AVAILABLE:
            self.input_controller.key_press('enter')
        else:
            # 使用替代方法
            win32api.keybd_event(win32con.VK_RETURN, 0, 0, 0)
            time.sleep(0.1)
            win32api.keybd_event(win32con.VK_RETURN, 0, win32con.KEYEVENTF_KEYUP, 0)
        
        print("消息已发送")
        return True

def main():
    parser = argparse.ArgumentParser(description="与VSCode中的augment对话区域交互")
    parser.add_argument("--window", "-w", default="Visual Studio Code", help="VSCode窗口标题")
    parser.add_argument("--message", "-m", required=True, help="要发送的消息")
    
    args = parser.parse_args()
    
    interactor = AugmentInteractor(window_title=args.window)
    interactor.send_message(args.message)

if __name__ == "__main__":
    main() 