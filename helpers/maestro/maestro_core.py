#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Maestro核心模块 - 优化版
减少文件操作，提升智能助理窗口管理和控制功能
"""

import os
import sys
import time
import json
import logging
from pathlib import Path
import datetime
import win32gui
import win32con
import win32api
from typing import List, Dict, Any, Tuple, Optional, Union
import numpy as np

try:
    from PIL import Image
    import pyperclip
    HAS_DEPENDENCIES = True
except ImportError:
    HAS_DEPENDENCIES = False

# 设置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger('maestro_core')

class MaestroCore:
    """Maestro核心类 - 优化版"""
    
    # 缓存有效期（秒）
    CACHE_EXPIRY = 30
    
    def __init__(self, window_title="Visual Studio Code", debug_mode=False):
        """初始化Maestro核心
        
        Args:
            window_title: 窗口标题
            debug_mode: 是否启用调试模式（保存截图和分析结果）
        """
        if not HAS_DEPENDENCIES:
            logger.warning("缺少依赖库，部分功能可能不可用")
        
        self.window_title = window_title
        self.debug_mode = debug_mode
        self.hwnd = None
        
        # 缓存
        self._ui_cache = {}
        self._ui_cache_time = 0
        self._window_rect = None
        
        # UI元素位置
        self.dialog_area = None
        self.input_area = None
        self.send_button = None
        
        # 查找窗口
        self.find_window()
        
        # 初始化UI元素
        if self.hwnd:
            self._initialize_ui_elements()
    
    def find_window(self):
        """查找窗口"""
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
            self.window_title = windows[0][1]
            logger.info(f"找到窗口: {self.window_title} (HWND: {self.hwnd})")
            return True
        else:
            logger.warning(f"未找到窗口: {self.window_title}")
            return False
    
    def _initialize_ui_elements(self):
        """初始化UI元素位置"""
        # 尝试检测对话区域
        self._detect_ui_elements()
        
        # 如果检测失败，使用默认值
        if not self.dialog_area:
            self._use_default_ui_elements()
    
    def _detect_ui_elements(self):
        """检测UI元素"""
        # 捕获窗口截图
        image = self.capture_window()
        if image is None:
            return False
        
        # 分析UI元素
        try:
            # 这里可以添加更复杂的UI元素检测逻辑
            # 简单起见，我们使用一些启发式方法
            
            # 获取窗口大小
            height, width = image.shape[:2]
            
            # 估计对话区域（窗口中部偏右）
            dialog_x1 = int(width * 0.5)
            dialog_y1 = int(height * 0.2)
            dialog_x2 = int(width * 0.9)
            dialog_y2 = int(height * 0.6)
            
            self.dialog_area = (dialog_x1, dialog_y1, dialog_x2, dialog_y2)
            logger.info(f"检测到对话区域: {self.dialog_area}")
            
            # 估计输入区域（对话区域下方）
            input_y1 = dialog_y2 + 10
            input_y2 = input_y1 + 30
            
            self.input_area = (dialog_x1, input_y1, dialog_x2, input_y2)
            logger.info(f"检测到输入区域: {self.input_area}")
            
            # 估计发送按钮（输入区域右侧）
            button_x1 = dialog_x2 + 5
            button_x2 = button_x1 + 30
            button_y1 = input_y1
            button_y2 = input_y2
            
            self.send_button = (button_x1, button_y1, button_x2, button_y2)
            logger.info(f"检测到发送按钮: {self.send_button}")
            
            return True
        except Exception as e:
            logger.error(f"检测UI元素失败: {e}")
            return False
    
    def _use_default_ui_elements(self):
        """使用默认UI元素位置"""
        # 获取窗口大小
        if self.hwnd:
            try:
                left, top, right, bottom = win32gui.GetWindowRect(self.hwnd)
                width = right - left
                height = bottom - top
                
                # 设置默认对话区域
                dialog_x1 = int(width * 0.5)
                dialog_y1 = int(height * 0.2)
                dialog_x2 = int(width * 0.9)
                dialog_y2 = int(height * 0.6)
                
                self.dialog_area = (dialog_x1, dialog_y1, dialog_x2, dialog_y2)
                logger.info(f"使用默认对话区域: {self.dialog_area}")
                
                # 设置默认输入区域
                input_y1 = dialog_y2 + 10
                input_y2 = input_y1 + 30
                
                self.input_area = (dialog_x1, input_y1, dialog_x2, input_y2)
                logger.info(f"使用默认输入区域: {self.input_area}")
                
                # 设置默认发送按钮
                button_x1 = dialog_x2 + 5
                button_x2 = button_x1 + 30
                button_y1 = input_y1
                button_y2 = input_y2
                
                self.send_button = (button_x1, button_y1, button_x2, button_y2)
                logger.info(f"使用默认发送按钮: {self.send_button}")
                
                return True
            except Exception as e:
                logger.error(f"获取窗口大小失败: {e}")
        
        # 如果无法获取窗口大小，使用固定值
        self.dialog_area = (500, 300, 800, 600)
        self.input_area = (500, 610, 800, 640)
        self.send_button = (805, 610, 835, 640)
        logger.info("使用固定UI元素位置")
        return False
    
    def capture_window(self):
        """捕获窗口截图"""
        if not self.hwnd:
            logger.warning("未找到窗口，无法捕获截图")
            return None
        
        try:
            # 获取窗口位置和大小
            left, top, right, bottom = win32gui.GetWindowRect(self.hwnd)
            self._window_rect = (left, top, right, bottom)
            width = right - left
            height = bottom - top
            
            # 如果没有PIL，无法捕获截图
            if not HAS_DEPENDENCIES:
                logger.warning("缺少PIL库，无法捕获截图")
                return None
            
            # 创建设备上下文
            hwndDC = win32gui.GetWindowDC(self.hwnd)
            mfcDC = win32gui.CreateDCFromHandle(hwndDC)
            saveDC = win32gui.CreateCompatibleDC(mfcDC)
            
            # 创建位图
            saveBitMap = win32gui.CreateCompatibleBitmap(mfcDC, width, height)
            win32gui.SelectObject(saveDC, saveBitMap)
            
            # 复制窗口内容到位图
            win32gui.BitBlt(saveDC, 0, 0, width, height, mfcDC, 0, 0, win32con.SRCCOPY)
            
            # 转换为PIL Image
            bmpinfo = saveBitMap.GetInfo()
            bmpstr = saveBitMap.GetBitmapBits(True)
            img = Image.frombuffer(
                'RGB',
                (bmpinfo['bmWidth'], bmpinfo['bmHeight']),
                bmpstr, 'raw', 'BGRX', 0, 1)
            
            # 转换为numpy数组
            image = np.array(img)
            
            # 清理资源
            win32gui.DeleteObject(saveBitMap)
            win32gui.DeleteDC(saveDC)
            win32gui.ReleaseDC(self.hwnd, hwndDC)
            
            # 如果是调试模式，保存截图
            if self.debug_mode:
                timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"{self.window_title.replace(' ', '_')}_{timestamp}.png"
                img.save(filename)
                logger.debug(f"保存截图: {filename}")
            
            return image
        except Exception as e:
            logger.error(f"捕获窗口截图失败: {e}")
            return None
    
    def activate_window(self):
        """激活窗口"""
        if not self.hwnd:
            logger.warning("未找到窗口，无法激活")
            return False
        
        try:
            win32gui.SetForegroundWindow(self.hwnd)
            time.sleep(0.2)  # 减少等待时间
            logger.debug("窗口已激活")
            return True
        except Exception as e:
            logger.error(f"激活窗口失败: {e}")
            return False
    
    def click(self, x, y, button="left", double_click=False):
        """点击指定位置
        
        Args:
            x: X坐标（窗口内相对坐标）
            y: Y坐标（窗口内相对坐标）
            button: 鼠标按钮，可选值为"left"、"right"、"middle"
            double_click: 是否双击
        """
        if not self.hwnd:
            logger.warning("未找到窗口，无法点击")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        try:
            # 获取窗口位置
            if not self._window_rect:
                left, top, right, bottom = win32gui.GetWindowRect(self.hwnd)
                self._window_rect = (left, top, right, bottom)
            else:
                left, top, right, bottom = self._window_rect
            
            # 计算屏幕坐标
            screen_x = left + x
            screen_y = top + y
            
            # 设置鼠标位置
            win32api.SetCursorPos((screen_x, screen_y))
            time.sleep(0.1)
            
            # 根据按钮类型设置事件标志
            if button == "right":
                down_event = win32con.MOUSEEVENTF_RIGHTDOWN
                up_event = win32con.MOUSEEVENTF_RIGHTUP
            elif button == "middle":
                down_event = win32con.MOUSEEVENTF_MIDDLEDOWN
                up_event = win32con.MOUSEEVENTF_MIDDLEUP
            else:  # 默认为左键
                down_event = win32con.MOUSEEVENTF_LEFTDOWN
                up_event = win32con.MOUSEEVENTF_LEFTUP
            
            # 点击
            win32api.mouse_event(down_event, 0, 0, 0, 0)
            time.sleep(0.05)
            win32api.mouse_event(up_event, 0, 0, 0, 0)
            
            # 如果是双击，再点击一次
            if double_click:
                time.sleep(0.05)
                win32api.mouse_event(down_event, 0, 0, 0, 0)
                time.sleep(0.05)
                win32api.mouse_event(up_event, 0, 0, 0, 0)
            
            logger.debug(f"点击位置: 窗口坐标({x}, {y}), 屏幕坐标({screen_x}, {screen_y})")
            return True
        except Exception as e:
            logger.error(f"点击位置失败: {e}")
            return False
    
    def click_input_area(self):
        """点击输入区域"""
        if not self.input_area:
            logger.warning("未找到输入区域，无法点击")
            return False
        
        x = (self.input_area[0] + self.input_area[2]) // 2
        y = (self.input_area[1] + self.input_area[3]) // 2
        
        return self.click(x, y)
    
    def click_send_button(self):
        """点击发送按钮"""
        if not self.send_button:
            logger.warning("未找到发送按钮，无法点击")
            return False
        
        x = (self.send_button[0] + self.send_button[2]) // 2
        y = (self.send_button[1] + self.send_button[3]) // 2
        
        return self.click(x, y)
    
    def type_text(self, text):
        """输入文本
        
        Args:
            text: 要输入的文本
        """
        if not self.hwnd:
            logger.warning("未找到窗口，无法输入文本")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        try:
            # 使用剪贴板粘贴文本（更快更可靠）
            if HAS_DEPENDENCIES:
                pyperclip.copy(text)
                
                # 按下Ctrl+V
                win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
                time.sleep(0.05)
                win32api.keybd_event(ord('V'), 0, 0, 0)
                time.sleep(0.05)
                win32api.keybd_event(ord('V'), 0, win32con.KEYEVENTF_KEYUP, 0)
                time.sleep(0.05)
                win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
                
                logger.debug(f"粘贴文本: {text[:50]}...")
                return True
            else:
                # 逐字符输入（较慢）
                for c in text:
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
                        logger.warning(f"输入字符 '{c}' 失败: {e}")
                
                logger.debug(f"输入文本: {text[:50]}...")
                return True
        except Exception as e:
            logger.error(f"输入文本失败: {e}")
            return False
    
    def press_key(self, key):
        """按下按键
        
        Args:
            key: 按键名称，如"enter"、"tab"、"escape"等
        """
        if not self.hwnd:
            logger.warning("未找到窗口，无法按键")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        try:
            # 映射按键名称到虚拟键码
            key_map = {
                "enter": win32con.VK_RETURN,
                "tab": win32con.VK_TAB,
                "escape": win32con.VK_ESCAPE,
                "space": win32con.VK_SPACE,
                "backspace": win32con.VK_BACK,
                "delete": win32con.VK_DELETE,
                "up": win32con.VK_UP,
                "down": win32con.VK_DOWN,
                "left": win32con.VK_LEFT,
                "right": win32con.VK_RIGHT,
                "home": win32con.VK_HOME,
                "end": win32con.VK_END,
                "pageup": win32con.VK_PRIOR,
                "pagedown": win32con.VK_NEXT,
                "f1": win32con.VK_F1,
                "f2": win32con.VK_F2,
                "f3": win32con.VK_F3,
                "f4": win32con.VK_F4,
                "f5": win32con.VK_F5,
                "f6": win32con.VK_F6,
                "f7": win32con.VK_F7,
                "f8": win32con.VK_F8,
                "f9": win32con.VK_F9,
                "f10": win32con.VK_F10,
                "f11": win32con.VK_F11,
                "f12": win32con.VK_F12,
            }
            
            # 获取虚拟键码
            vk_code = key_map.get(key.lower())
            if vk_code is None:
                if len(key) == 1:
                    vk_code = win32api.VkKeyScan(key)
                    if vk_code == -1:
                        logger.warning(f"无法映射按键: {key}")
                        return False
                    vk_code = vk_code & 0xFF
                else:
                    logger.warning(f"未知按键: {key}")
                    return False
            
            # 按下并释放按键
            win32api.keybd_event(vk_code, 0, 0, 0)
            time.sleep(0.05)
            win32api.keybd_event(vk_code, 0, win32con.KEYEVENTF_KEYUP, 0)
            
            logger.debug(f"按下按键: {key}")
            return True
        except Exception as e:
            logger.error(f"按下按键失败: {e}")
            return False
    
    def press_hotkey(self, hotkey):
        """按下组合键
        
        Args:
            hotkey: 组合键，如"ctrl+c"、"alt+tab"等
        """
        if not self.hwnd:
            logger.warning("未找到窗口，无法按组合键")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        try:
            # 解析组合键
            keys = hotkey.lower().split("+")
            
            # 映射修饰键
            modifier_map = {
                "ctrl": win32con.VK_CONTROL,
                "control": win32con.VK_CONTROL,
                "alt": win32con.VK_MENU,
                "shift": win32con.VK_SHIFT,
                "win": win32con.VK_LWIN,
                "windows": win32con.VK_LWIN
            }
            
            # 按下修饰键
            modifiers = []
            for key in keys[:-1]:
                vk_code = modifier_map.get(key)
                if vk_code:
                    win32api.keybd_event(vk_code, 0, 0, 0)
                    modifiers.append(vk_code)
                else:
                    logger.warning(f"未知修饰键: {key}")
            
            # 按下主键
            main_key = keys[-1]
            self.press_key(main_key)
            
            # 释放修饰键（按相反顺序）
            for vk_code in reversed(modifiers):
                win32api.keybd_event(vk_code, 0, win32con.KEYEVENTF_KEYUP, 0)
            
            logger.debug(f"按下组合键: {hotkey}")
            return True
        except Exception as e:
            logger.error(f"按下组合键失败: {e}")
            return False
    
    def send_message(self, message):
        """发送消息到对话区域
        
        Args:
            message: 要发送的消息
        """
        if not self.hwnd:
            logger.warning("未找到窗口，无法发送消息")
            return False
        
        # 点击输入区域
        if not self.click_input_area():
            return False
        
        # 清空输入区域（Ctrl+A然后删除）
        self.press_hotkey("ctrl+a")
        time.sleep(0.1)
        self.press_key("delete")
        time.sleep(0.1)
        
        # 输入消息
        if not self.type_text(message):
            return False
        
        # 发送消息（按回车键或点击发送按钮）
        if self.send_button:
            success = self.click_send_button()
        else:
            success = self.press_key("enter")
        
        if success:
            logger.info(f"发送消息: {message[:50]}...")
            return True
        else:
            logger.warning("发送消息失败")
            return False
    
    def get_dialog_content(self):
        """获取对话内容"""
        if not self.hwnd or not self.dialog_area:
            logger.warning("未找到窗口或对话区域，无法获取对话内容")
            return None
        
        # 捕获窗口截图
        image = self.capture_window()
        if image is None:
            return None
        
        # 裁剪对话区域
        x1, y1, x2, y2 = self.dialog_area
        dialog_image = image[y1:y2, x1:x2]
        
        # 如果是调试模式，保存对话区域截图
        if self.debug_mode:
            try:
                timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"dialog_{timestamp}.png"
                Image.fromarray(dialog_image).save(filename)
                logger.debug(f"保存对话区域截图: {filename}")
            except Exception as e:
                logger.warning(f"保存对话区域截图失败: {e}")
        
        # 这里可以添加OCR识别文本的逻辑
        # 由于OCR依赖较重，这里省略
        
        return "对话内容（需要OCR支持）"
    
    def wait_for_response(self, timeout=30, check_interval=0.5):
        """等待响应
        
        Args:
            timeout: 超时时间（秒）
            check_interval: 检查间隔（秒）
        """
        if not self.hwnd:
            logger.warning("未找到窗口，无法等待响应")
            return None
        
        logger.info(f"等待响应，最多 {timeout} 秒...")
        
        # 记录初始对话内容
        initial_content = self.get_dialog_content()
        
        start_time = time.time()
        while time.time() - start_time < timeout:
            # 等待一段时间
            time.sleep(check_interval)
            
            # 读取当前对话内容
            current_content = self.get_dialog_content()
            
            # 如果对话内容发生变化，说明有响应
            if current_content != initial_content:
                # 再等待一段时间，确保响应完成
                time.sleep(1)
                final_content = self.get_dialog_content()
                logger.info("检测到响应")
                return final_content
        
        logger.warning(f"等待响应超时（{timeout}秒）")
        return None
    
    def execute_task(self, task_description, wait_for_response=True, timeout=60):
        """执行任务
        
        Args:
            task_description: 任务描述
            wait_for_response: 是否等待响应
            timeout: 等待响应的超时时间（秒）
        """
        # 构建任务消息
        message = f"请执行以下任务：{task_description}"
        
        # 发送消息
        logger.info(f"执行任务: {task_description[:50]}...")
        if not self.send_message(message):
            return False
        
        # 等待响应
        if wait_for_response:
            response = self.wait_for_response(timeout=timeout)
            return response is not None
        
        return True

# 单例模式
_instance = None

def get_instance(window_title="Visual Studio Code", debug_mode=False):
    """获取Maestro核心实例（单例模式）"""
    global _instance
    if _instance is None:
        _instance = MaestroCore(window_title=window_title, debug_mode=debug_mode)
    return _instance

# 便捷函数
def send_message(message, window_title="Visual Studio Code"):
    """发送消息到对话区域"""
    maestro = get_instance(window_title=window_title)
    return maestro.send_message(message)

def execute_task(task_description, window_title="Visual Studio Code", wait_for_response=True, timeout=60):
    """执行任务"""
    maestro = get_instance(window_title=window_title)
    return maestro.execute_task(task_description, wait_for_response=wait_for_response, timeout=timeout)

def main():
    """命令行入口"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Maestro核心模块")
    parser.add_argument("--window", "-w", default="Visual Studio Code", help="窗口标题")
    parser.add_argument("--message", "-m", help="要发送的消息")
    parser.add_argument("--task", "-t", help="要执行的任务")
    parser.add_argument("--debug", "-d", action="store_true", help="启用调试模式")
    parser.add_argument("--timeout", "-o", type=int, default=60, help="等待响应的超时时间（秒）")
    
    args = parser.parse_args()
    
    # 创建Maestro核心实例
    maestro = MaestroCore(window_title=args.window, debug_mode=args.debug)
    
    if not maestro.hwnd:
        print(f"未找到窗口: {args.window}")
        return
    
    # 发送消息
    if args.message:
        maestro.send_message(args.message)
        maestro.wait_for_response(timeout=args.timeout)
    
    # 执行任务
    if args.task:
        maestro.execute_task(args.task, timeout=args.timeout)

if __name__ == "__main__":
    main() 