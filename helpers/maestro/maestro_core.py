#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Maestro核心模块 - 跨平台版
支持Windows和macOS，使用平台抽象层提供统一接口
"""

import os
import sys
import time
import json
import logging
import platform
from pathlib import Path
import datetime
from typing import List, Dict, Any, Tuple, Optional, Union
import numpy as np

# Import platform-specific implementations
from maestro.platform import input_controller, window_capture

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
    """Maestro核心类 - 跨平台版"""
    
    # 缓存有效期（秒）
    CACHE_EXPIRY = 30
    
    def __init__(self, window_title="Visual Studio Code", debug_mode=False):
        """初始Maestro核心
        
        Args:
            window_title: 窗口标题
            debug_mode: 是否启用调试模式（保存截图和分析结果）
        """
        if not HAS_DEPENDENCIES:
            logger.warning("缺少依赖库，部分功能可能不可用")
        
        self.window_title = window_title
        self.debug_mode = debug_mode
        
        # 初始化平台抽象层
        self._window_manager = window_capture.get_window_manager()
        self._screen_capture = window_capture.get_screen_capture(self._window_manager)
        self._input_controller = input_controller.get_input_controller()
        
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
        
        # 初始UI元素
        if self._window_manager.has_window_handle():
            self._initialize_ui_elements()
    
    def find_window(self):
        """查找窗口"""
        # 如果没有提供窗口标题，则跳过查找
        if not self.window_title:
            logger.debug("未提供窗口标题，跳过查找窗口")
            return False
            
        # 使用平台抽象层查找窗口
        result = self._window_manager.find_window(self.window_title)
        
        if result:
            # 更新窗口标题为实际找到的标题
            self.window_title = self._window_manager.get_window_title()
            logger.info(f"找到窗口: {self.window_title}")
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
        try:
            if self._window_manager.has_window_handle():
                # 获取窗口矩形
                left, top, right, bottom = self._window_manager.get_window_rect()
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
        if not self._window_manager.has_window_handle():
            logger.warning("未找到窗口，无法捕获截图")
            return None
        
        try:
            # 获取窗口位置和大小
            left, top, right, bottom = self._window_manager.get_window_rect()
            self._window_rect = (left, top, right, bottom)
            
            # 如果没有PIL，无法处理截图
            if not HAS_DEPENDENCIES:
                logger.warning("缺少PIL库，无法处理截图")
                return None
            
            # 使用平台抽象层捕获窗口
            img = self._screen_capture.capture()
            
            if img is None:
                logger.error("捕获窗口失败")
                return None
                
            # 转换为numpy数组
            image = np.array(img)
            
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
        if not self._window_manager.has_window_handle():
            logger.warning("未找到窗口，无法激活")
            return False
        
        try:
            result = self._window_manager.activate_window()
            time.sleep(0.2)  # 减少等待时间
            logger.debug("窗口已激活")
            return result
        except Exception as e:
            logger.error(f"激活窗口失败: {e}")
            return False
    
    def click_position(self, x, y, button="left"):
        """点击指定位置
        
        Args:
            x: 相对于窗口左上角的x坐标
            y: 相对于窗口左上角的y坐标
            button: 鼠标按钮，"left"或"right"
        """
        if not self._window_manager.has_window_handle():
            logger.warning("未找到窗口，无法点击")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        try:
            # 获取窗口位置
            left, top, right, bottom = self._window_manager.get_window_rect()
            
            # 计算全屏坐标
            screen_x = left + x
            screen_y = top + y
            
            # 使用平台抽象层移动鼠标并点击
            self._input_controller.move_to(screen_x, screen_y)
            time.sleep(0.1)
            
            # 点击
            if button.lower() == "left":
                self._input_controller.click(button="left")
            elif button.lower() == "right":
                self._input_controller.click(button="right")
            
            logger.debug(f"点击位置: ({x}, {y}) 按钮: {button}")
            return True
        except Exception as e:
            logger.error(f"点击位置失败: {e}")
            return False
    
    def click_input_area(self):
        """点击输入区域"""
        if not self._window_manager.has_window_handle() or not self.input_area:
            logger.warning("未找到窗口或输入区域，无法点击")
            return False
        
        # 计算输入区域中心点
        x1, y1, x2, y2 = self.input_area
        x = (x1 + x2) // 2
        y = (y1 + y2) // 2
        
        return self.click_position(x, y)
    
    def click_send_button(self):
        """点击发送按钮"""
        if not self._window_manager.has_window_handle() or not self.send_button:
            logger.warning("未找到窗口或发送按钮，无法点击")
            return False
        
        # 计算发送按钮中心点
        x1, y1, x2, y2 = self.send_button
        x = (x1 + x2) // 2
        y = (y1 + y2) // 2
        
        return self.click_position(x, y)
    
    def type_text(self, text):
        """输入文本
        
        Args:
            text: 要输入的文本
        """
        if not self._window_manager.has_window_handle():
            logger.warning("未找到窗口，无法输入文本")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        try:
            # 尝试使用剪贴板
            try:
                # 获取剪贴板管理器
                clipboard_manager = input_controller.get_clipboard_manager()
                original_clipboard = clipboard_manager.get_text()
                clipboard_manager.set_text(text)
                time.sleep(0.1)
                self.press_hotkey("ctrl+v")
                time.sleep(0.1)
                clipboard_manager.set_text(original_clipboard)  # 恢复剪贴板
                return True
            except Exception as e:
                logger.warning(f"使用剪贴板输入文本失败: {e}")
            
            # 如果剪贴板方法失败，尝试直接输入
            self._input_controller.type_text(text)
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
        if not self._window_manager.has_window_handle():
            logger.warning("未找到窗口，无法按键")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        try:
            # 使用平台抽象层按键
            self._input_controller.key_tap(key)
            
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
        if not self._window_manager.has_window_handle():
            logger.warning("未找到窗口，无法按组合键")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        try:
            # 使用平台抽象层按组合键
            self._input_controller.hotkey(hotkey)
            
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
        if not self._window_manager.has_window_handle():
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
        if not self._window_manager.has_window_handle() or not self.dialog_area:
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
        if not self._window_manager.has_window_handle():
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
    
    parser = argparse.ArgumentParser(description="Maestro核心模块 - 跨平台版")
    parser.add_argument("--window", "-w", default="Visual Studio Code", help="窗口标题")
    parser.add_argument("--message", "-m", help="要发送的消息")
    parser.add_argument("--task", "-t", help="要执行的任务")
    parser.add_argument("--debug", "-d", action="store_true", help="启用调试模式")
    parser.add_argument("--timeout", "-o", type=int, default=60, help="等待响应的超时时间（秒）")
    
    args = parser.parse_args()
    
    # 创建Maestro核心实例
    maestro = MaestroCore(window_title=args.window, debug_mode=args.debug)
    
    if not maestro._window_manager.has_window_handle():
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