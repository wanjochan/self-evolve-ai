#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
UI控制工具 - CLI优先架构

既可以作为命令行工具使用，也可以作为Web API服务器运行。

CLI用法：
  python uictrl.py list-windows
  python uictrl.py find-ide
  python uictrl.py screenshot <window_id> [output.png]
  python uictrl.py send-keys <window_id> "ctrl+c"
  python uictrl.py type-text <window_id> "Hello World"
  python uictrl.py cursor-chat "你好"
  python uictrl.py serve [--port 9091]
"""

import os
import sys
import json
import base64
import subprocess
import time
import argparse
from typing import List, Dict, Optional, Any, Union
from enum import Enum

# Windows平台特定导入
import ctypes
from ctypes import wintypes
import win32gui
import win32con
import win32process
import win32api
from PIL import ImageGrab

# 简单数据结构定义（用于CLI和Web通用）
from dataclasses import dataclass
from typing import NamedTuple

@dataclass
class WindowBasic:
    """基本窗口信息"""
    id: str
    title: str

@dataclass 
class Position:
    """位置坐标"""
    x: int
    y: int

@dataclass
class Size:
    """尺寸"""
    width: int
    height: int

@dataclass
class WindowDetail:
    """详细窗口信息"""
    id: str
    title: str
    position: Position
    size: Size
    process_id: int

# 简单枚举定义（用于CLI）
class WindowControlAction(str, Enum):
    """窗口控制动作枚举"""
    MOVE = "move"
    RESIZE = "resize"
    CLOSE = "close"
    MINIMIZE = "minimize"
    MAXIMIZE = "maximize"
    RESTORE = "restore"

class MouseAction(str, Enum):
    """鼠标动作枚举"""
    MOVE = "move"
    CLICK = "click"
    DOUBLE_CLICK = "double_click"
    RIGHT_CLICK = "right_click"

class KeyboardAction(str, Enum):
    """键盘动作枚举"""
    TYPE_TEXT = "type_text"
    PRESS_KEYS = "press_keys"

@dataclass
class KeyboardPayload:
    """键盘操作负载"""
    text: Optional[str] = None
    keys: Optional[List[str]] = None

# 窗口操作辅助函数
def enum_windows() -> List[WindowBasic]:
    """枚举所有顶层窗口"""
    result = []
    
    def callback(hwnd, _):
        if win32gui.IsWindowVisible(hwnd) and win32gui.GetWindowText(hwnd):
            result.append(WindowBasic(
                id=str(hwnd),
                title=win32gui.GetWindowText(hwnd)
            ))
        return True
    
    win32gui.EnumWindows(callback, None)
    return result

def get_window_details(hwnd: int) -> WindowDetail:
    """获取窗口详细信息"""
    # 获取窗口位置和大小
    rect = win32gui.GetWindowRect(hwnd)
    left, top, right, bottom = rect
    
    # 获取进程ID
    _, process_id = win32process.GetWindowThreadProcessId(hwnd)
    
    return WindowDetail(
        id=str(hwnd),
        title=win32gui.GetWindowText(hwnd),
        position=Position(x=left, y=top),
        size=Size(width=right-left, height=bottom-top),
        process_id=process_id
    )

def capture_window_screenshot(hwnd: int) -> bytes:
    """捕获窗口截图"""
    # 获取窗口位置和大小
    rect = win32gui.GetWindowRect(hwnd)
    left, top, right, bottom = rect
    
    # 截取窗口图像
    screenshot = ImageGrab.grab(bbox=(left, top, right, bottom))
    
    # 转换为PNG字节
    import io
    img_byte_arr = io.BytesIO()
    screenshot.save(img_byte_arr, format='PNG')
    return img_byte_arr.getvalue()

def control_window(hwnd: int, action: WindowControlAction, payload: Optional[Dict[str, Any]] = None) -> str:
    """控制窗口"""
    if action == WindowControlAction.MOVE:
        if not payload or "x" not in payload or "y" not in payload:
            raise ValueError("Move action requires x and y coordinates")
        win32gui.SetWindowPos(hwnd, 0, payload["x"], payload["y"], 0, 0, 
                            win32con.SWP_NOSIZE | win32con.SWP_NOZORDER)
        return "Window moved successfully"
    
    elif action == WindowControlAction.RESIZE:
        if not payload or "width" not in payload or "height" not in payload:
            raise ValueError("Resize action requires width and height")
        rect = win32gui.GetWindowRect(hwnd)
        win32gui.SetWindowPos(hwnd, 0, rect[0], rect[1], 
                            payload["width"], payload["height"], 
                            win32con.SWP_NOMOVE | win32con.SWP_NOZORDER)
        return "Window resized successfully"
    
    elif action == WindowControlAction.CLOSE:
        win32gui.PostMessage(hwnd, win32con.WM_CLOSE, 0, 0)
        return "Window close command sent"
    
    elif action == WindowControlAction.MINIMIZE:
        win32gui.ShowWindow(hwnd, win32con.SW_MINIMIZE)
        return "Window minimized successfully"
    
    elif action == WindowControlAction.MAXIMIZE:
        win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
        return "Window maximized successfully"
    
    elif action == WindowControlAction.RESTORE:
        win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
        return "Window restored successfully"
    
    else:
        raise ValueError(f"Unsupported action: {action}")

def execute_command(command: str, args: List[str] = None, async_exec: bool = True) -> Dict[str, Any]:
    """执行命令"""
    if args is None:
        args = []
    
    full_command = [command] + args
    
    if async_exec:
        # 异步执行
        process = subprocess.Popen(full_command)
        return {
            "status": "started",
            "pid": process.pid,
            "command": " ".join(full_command)
        }
    else:
        # 同步执行
        result = subprocess.run(full_command, capture_output=True, text=True)
        return {
            "status": "completed",
            "returncode": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
            "command": " ".join(full_command)
        }

def send_mouse_input(hwnd: int, action: MouseAction, x: int, y: int) -> str:
    """发送鼠标输入"""
    # 获取窗口客户区域相对于屏幕的位置
    rect = win32gui.GetWindowRect(hwnd)
    client_rect = win32gui.GetClientRect(hwnd)
    
    # 计算客户区域的左上角在屏幕上的坐标
    left, top, _, _ = rect
    
    # 计算鼠标在屏幕上的绝对坐标
    screen_x = left + x
    screen_y = top + y
    
    # 设置鼠标位置
    win32api.SetCursorPos((screen_x, screen_y))
    
    if action == MouseAction.MOVE:
        return f"Mouse moved to ({x}, {y})"
    
    elif action == MouseAction.CLICK:
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        return f"Mouse clicked at ({x}, {y})"
    
    elif action == MouseAction.DOUBLE_CLICK:
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        return f"Mouse double-clicked at ({x}, {y})"
    
    elif action == MouseAction.RIGHT_CLICK:
        win32api.mouse_event(win32con.MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0)
        return f"Mouse right-clicked at ({x}, {y})"
    
    else:
        raise ValueError(f"Unsupported mouse action: {action}")

def send_keyboard_input(hwnd: int, action: KeyboardAction, payload: KeyboardPayload) -> str:
    """发送键盘输入到指定窗口，支持IDE快捷键"""
    import win32con
    import win32api
    import time
    
    # IDE特殊快捷键映射
    SPECIAL_KEYS = {
        'ctrl': win32con.VK_CONTROL,
        'shift': win32con.VK_SHIFT,
        'alt': win32con.VK_MENU,
        'enter': win32con.VK_RETURN,
        'tab': win32con.VK_TAB,
        'escape': win32con.VK_ESCAPE,
        'space': win32con.VK_SPACE,
        'backspace': win32con.VK_BACK,
        'delete': win32con.VK_DELETE,
        'home': win32con.VK_HOME,
        'end': win32con.VK_END,
        'pageup': win32con.VK_PRIOR,
        'pagedown': win32con.VK_NEXT,
        'up': win32con.VK_UP,
        'down': win32con.VK_DOWN,
        'left': win32con.VK_LEFT,
        'right': win32con.VK_RIGHT,
        'f1': win32con.VK_F1,
        'f2': win32con.VK_F2,
        'f3': win32con.VK_F3,
        'f4': win32con.VK_F4,
        'f5': win32con.VK_F5,
        'f6': win32con.VK_F6,
        'f7': win32con.VK_F7,
        'f8': win32con.VK_F8,
        'f9': win32con.VK_F9,
        'f10': win32con.VK_F10,
        'f11': win32con.VK_F11,
        'f12': win32con.VK_F12,
        '`': 192,  # 反引号，用于终端快捷键
        'l': ord('L'),
        'i': ord('I'),
        'p': ord('P'),
        's': ord('S'),
        'n': ord('N'),
        'h': ord('H'),
        'f': ord('F'),
        'c': ord('C'),
        'v': ord('V'),
        'x': ord('X'),
        'z': ord('Z'),
        'y': ord('Y')
    }
    
    def get_window_title(hwnd):
        """获取窗口标题用于检测IDE类型"""
        return win32gui.GetWindowText(hwnd).lower()
    
    def is_electron_ide(title):
        """检测是否为Electron IDE"""
        electron_ides = ['cursor', 'visual studio code', 'windsurf', 'theia', 'atom']
        return any(ide in title for ide in electron_ides)
    
    def send_char_to_ide(char: str, is_electron: bool = False):
        """发送单个字符，对IDE优化，支持中文字符"""
        try:
            if is_electron:
                # 对于Electron应用，使用Unicode输入
                # 先尝试VkKeyScan，如果失败则使用Unicode方式
                vk = win32api.VkKeyScan(char)
                if vk != -1:
                    # ASCII字符，使用键盘事件
                    key_code = vk & 0xff
                    win32api.keybd_event(key_code, 0, 0, 0)
                    time.sleep(0.01)
                    win32api.keybd_event(key_code, 0, win32con.KEYEVENTF_KEYUP, 0)
                else:
                    # Unicode字符（如中文），使用剪贴板
                    import win32clipboard
                    win32clipboard.OpenClipboard()
                    win32clipboard.EmptyClipboard()
                    win32clipboard.SetClipboardText(char)
                    win32clipboard.CloseClipboard()
                    
                    # 发送Ctrl+V
                    win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
                    win32api.keybd_event(ord('V'), 0, 0, 0)
                    time.sleep(0.01)
                    win32api.keybd_event(ord('V'), 0, win32con.KEYEVENTF_KEYUP, 0)
                    win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
            else:
                # 传统Win32输入，使用WM_CHAR消息支持Unicode
                win32api.PostMessage(hwnd, win32con.WM_CHAR, ord(char), 0)
                
        except Exception as e:
            # 如果字符输入失败，记录但不中断
            if is_electron and hasattr(send_char_to_ide, '_verbose'):
                print(f"Warning: Failed to send char '{char}': {e}")
    
    def send_special_key(key: str, is_electron: bool = False):
        """发送特殊键"""
        if key.lower() in SPECIAL_KEYS:
            vk = SPECIAL_KEYS[key.lower()]
            if is_electron:
                # 使用全局键盘事件
                win32api.keybd_event(vk, 0, 0, 0)
                return vk
            else:
                # 使用PostMessage
                scan = win32api.MapVirtualKey(vk, 0)
                win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, vk, 0x00000001 | (scan << 16))
                return vk
        else:
            # 普通字符
            if is_electron:
                win32api.keybd_event(ord(key.upper()), 0, 0, 0)
                return ord(key.upper())
            else:
                vk = win32api.VkKeyScan(key)
                if vk != -1:
                    scan = win32api.MapVirtualKey(vk & 0xff, 0)
                    win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, vk & 0xff, 0x00000001 | (scan << 16))
                    return vk & 0xff
        return None
    
    def release_special_key(vk: int, is_electron: bool = False):
        """释放特殊键"""
        if is_electron:
            win32api.keybd_event(vk, 0, win32con.KEYEVENTF_KEYUP, 0)
        else:
            scan = win32api.MapVirtualKey(vk, 0)
            win32api.PostMessage(hwnd, win32con.WM_KEYUP, vk, 0xC0000001 | (scan << 16))
    
    # 检测窗口类型
    window_title = get_window_title(hwnd)
    is_electron = is_electron_ide(window_title)
    
    # 如果是Electron IDE，先激活窗口
    if is_electron:
        win32gui.SetForegroundWindow(hwnd)
        time.sleep(0.1)  # 等待窗口激活
    
    if action == KeyboardAction.TYPE_TEXT:
        if not payload.text:
            raise ValueError("Type_text action requires text payload")
        
        # 检查是否包含非ASCII字符（如中文）
        has_unicode = any(ord(char) > 127 for char in payload.text)
        
        # 对于包含中文或长文本，使用剪贴板更可靠
        if has_unicode or len(payload.text) > 50:
            import win32clipboard
            try:
                win32clipboard.OpenClipboard()
                win32clipboard.EmptyClipboard()
                win32clipboard.SetClipboardText(payload.text)
                win32clipboard.CloseClipboard()
                
                time.sleep(0.1)  # 等待剪贴板更新
                
                # 发送Ctrl+V
                if is_electron:
                    win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
                    time.sleep(0.02)
                    win32api.keybd_event(ord('V'), 0, 0, 0)
                    time.sleep(0.02)
                    win32api.keybd_event(ord('V'), 0, win32con.KEYEVENTF_KEYUP, 0)
                    time.sleep(0.02)
                    win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
                else:
                    win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, win32con.VK_CONTROL, 0)
                    win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, ord('V'), 0)
                    win32api.PostMessage(hwnd, win32con.WM_KEYUP, ord('V'), 0)
                    win32api.PostMessage(hwnd, win32con.WM_KEYUP, win32con.VK_CONTROL, 0)
                
                return f"Text pasted to {'Electron IDE' if is_electron else 'window'} {hwnd} (Unicode: {has_unicode})"
                
            except Exception as e:
                return f"Clipboard paste failed: {e}"
        else:
            # 短ASCII文本逐字符输入
            for char in payload.text:
                send_char_to_ide(char, is_electron)
                time.sleep(0.01)  # 稍微增加延迟确保稳定性
            
            return f"Text typed to {'Electron IDE' if is_electron else 'window'} {hwnd}"
    
    elif action == KeyboardAction.PRESS_KEYS:
        if not payload.keys:
            raise ValueError("Press_keys action requires keys payload")
        
        pressed_keys = []
        
        # 按下所有修饰键
        for key in payload.keys:
            vk = send_special_key(key, is_electron)
            if vk:
                pressed_keys.append(vk)
            time.sleep(0.01)
        
        time.sleep(0.05)  # 等待组合键生效
        
        # 释放所有按键（逆序）
        for vk in reversed(pressed_keys):
            release_special_key(vk, is_electron)
            time.sleep(0.01)
            
        return f"Combo keys {'+'.join(payload.keys)} sent to {'Electron IDE' if is_electron else 'window'} {hwnd}"
    
    else:
        raise ValueError(f"Unsupported keyboard action: {action}")

def find_ide_windows() -> List[WindowBasic]:
    """查找所有IDE窗口"""
    all_windows = enum_windows()
    ide_keywords = ['cursor', 'visual studio code', 'windsurf', 'theia', 'atom', 'sublime', 'webstorm', 'pycharm', 'intellij']
    
    ide_windows = []
    for window in all_windows:
        title_lower = window.title.lower()
        if any(keyword in title_lower for keyword in ide_keywords):
            ide_windows.append(window)
    
    return ide_windows

def smart_ide_interaction(hwnd: int, action: str, params: Dict[str, Any] = None) -> str:
    """智能IDE交互，支持常见IDE操作"""
    window_title = win32gui.GetWindowText(hwnd).lower()
    
    # IDE类型检测
    ide_type = None
    if 'cursor' in window_title:
        ide_type = 'cursor'
    elif 'visual studio code' in window_title or 'vscode' in window_title:
        ide_type = 'vscode'
    elif 'windsurf' in window_title:
        ide_type = 'windsurf'
    
    # 常见IDE快捷键映射
    ide_shortcuts = {
        'cursor': {
            'open_chat': ['ctrl', 'shift', 'l'],
            'open_composer': ['ctrl', 'shift', 'i'],
            'command_palette': ['ctrl', 'shift', 'p'],
            'terminal': ['ctrl', '`'],
            'new_file': ['ctrl', 'n'],
            'save': ['ctrl', 's'],
            'find': ['ctrl', 'f'],
            'replace': ['ctrl', 'h'],
            'go_to_line': ['ctrl', 'g'],
            'format': ['shift', 'alt', 'f'],
            'comment': ['ctrl', '/']
        },
        'vscode': {
            'command_palette': ['ctrl', 'shift', 'p'],
            'terminal': ['ctrl', '`'],
            'new_file': ['ctrl', 'n'],
            'save': ['ctrl', 's'],
            'find': ['ctrl', 'f'],
            'replace': ['ctrl', 'h'],
            'go_to_line': ['ctrl', 'g'],
            'format': ['shift', 'alt', 'f'],
            'comment': ['ctrl', '/']
        },
        'windsurf': {
            'open_chat': ['ctrl', 'shift', 'l'],
            'command_palette': ['ctrl', 'shift', 'p'],
            'terminal': ['ctrl', '`'],
            'new_file': ['ctrl', 'n'],
            'save': ['ctrl', 's'],
            'find': ['ctrl', 'f'],
            'replace': ['ctrl', 'h']
        }
    }
    
    if not ide_type or ide_type not in ide_shortcuts:
        return f"Unknown IDE type for window {hwnd}"
    
    shortcuts = ide_shortcuts[ide_type]
    
    # 执行特定动作
    if action in shortcuts:
        keys = shortcuts[action]
        
        # 激活窗口
        win32gui.SetForegroundWindow(hwnd)
        time.sleep(0.2)
        
        # 发送快捷键
        result = send_keyboard_input(hwnd, KeyboardAction.PRESS_KEYS, 
                                   KeyboardPayload(keys=keys))
        
        return f"Executed {action} in {ide_type}: {result}"
    
    elif action == 'type_prompt':
        # 输入提示词到聊天界面
        prompt = params.get('prompt', '') if params else ''
        if not prompt:
            return "Prompt text required"
        
        # 先确保窗口获得焦点
        win32gui.SetForegroundWindow(hwnd)
        time.sleep(0.3)
        
        # 对于Cursor，尝试点击输入框而不是打开新的聊天
        if ide_type == 'cursor':
            # 获取窗口尺寸，估算输入框位置
            rect = win32gui.GetWindowRect(hwnd)
            width = rect[2] - rect[0]
            height = rect[3] - rect[1]
            
            # Cursor输入框通常在右下角
            input_x = rect[0] + int(width * 0.75)  # 右侧3/4位置
            input_y = rect[1] + int(height * 0.9)   # 底部90%位置
            
            # 点击输入框位置
            import win32api
            win32api.SetCursorPos((input_x, input_y))
            time.sleep(0.1)
            win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
            win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
            time.sleep(0.3)
        else:
            # 其他IDE使用快捷键打开聊天面板
            if 'open_chat' in shortcuts:
                send_keyboard_input(hwnd, KeyboardAction.PRESS_KEYS, 
                                  KeyboardPayload(keys=shortcuts['open_chat']))
                time.sleep(1)  # 等待面板打开
        
        # 输入提示词
        send_keyboard_input(hwnd, KeyboardAction.TYPE_TEXT, 
                          KeyboardPayload(text=prompt))
        time.sleep(0.2)
        
        # 发送（回车）
        send_keyboard_input(hwnd, KeyboardAction.PRESS_KEYS, 
                          KeyboardPayload(keys=['enter']))
        
        return f"Sent prompt to {ide_type} chat: {prompt[:50]}..."
    
    elif action == 'open_file':
        # 打开文件
        file_path = params.get('file_path', '') if params else ''
        if not file_path:
            return "File path required"
        
        # Ctrl+O 打开文件对话框
        send_keyboard_input(hwnd, KeyboardAction.PRESS_KEYS, 
                          KeyboardPayload(keys=['ctrl', 'o']))
        time.sleep(0.5)
        
        # 输入文件路径
        send_keyboard_input(hwnd, KeyboardAction.TYPE_TEXT, 
                          KeyboardPayload(text=file_path))
        time.sleep(0.2)
        
        # 回车确认
        send_keyboard_input(hwnd, KeyboardAction.PRESS_KEYS, 
                          KeyboardPayload(keys=['enter']))
        
        return f"Opened file in {ide_type}: {file_path}"
    
    elif action == 'run_command':
        # 运行命令面板命令
        command = params.get('command', '') if params else ''
        if not command:
            return "Command required"
        
        # 打开命令面板
        send_keyboard_input(hwnd, KeyboardAction.PRESS_KEYS, 
                          KeyboardPayload(keys=shortcuts['command_palette']))
        time.sleep(0.5)
        
        # 输入命令
        send_keyboard_input(hwnd, KeyboardAction.TYPE_TEXT, 
                          KeyboardPayload(text=command))
        time.sleep(0.2)
        
        # 回车执行
        send_keyboard_input(hwnd, KeyboardAction.PRESS_KEYS, 
                          KeyboardPayload(keys=['enter']))
        
        return f"Executed command in {ide_type}: {command}"
    
    else:
        available_actions = list(shortcuts.keys()) + ['type_prompt', 'open_file', 'run_command']
        return f"Unknown action '{action}'. Available actions: {', '.join(available_actions)}"

# 原有的Web API路由已移至 start_web_server() 函数中

# ============================================================================
# CLI核心功能
# ============================================================================

class UIController:
    """UI控制器CLI核心类"""
    
    def __init__(self):
        self.verbose = False
    
    def list_windows(self) -> List[Dict]:
        """列出所有窗口"""
        windows = enum_windows()
        return [{"id": w.id, "title": w.title} for w in windows]
    
    def find_ide_windows(self) -> List[Dict]:
        """查找IDE窗口"""
        ide_windows = find_ide_windows()
        return [{"id": w.id, "title": w.title} for w in ide_windows]
    
    def take_screenshot(self, window_id: str, output_path: Optional[str] = None) -> str:
        """截取窗口截图"""
        try:
            hwnd = int(window_id)
            screenshot_data = capture_window_screenshot(hwnd)
            
            if output_path:
                with open(output_path, 'wb') as f:
                    f.write(screenshot_data)
                return f"Screenshot saved to {output_path}"
            else:
                # 返回base64编码
                return base64.b64encode(screenshot_data).decode('utf-8')
        except Exception as e:
            return f"Error: {e}"
    
    def send_keys(self, window_id: str, keys: str) -> str:
        """发送快捷键"""
        try:
            hwnd = int(window_id)
            # 解析按键组合，如 "ctrl+c" -> ["ctrl", "c"]
            key_list = [k.strip() for k in keys.split('+')]
            
            result = send_keyboard_input(hwnd, KeyboardAction.PRESS_KEYS, 
                                       KeyboardPayload(keys=key_list))
            return result
        except Exception as e:
            return f"Error: {e}"
    
    def type_text(self, window_id: str, text: str) -> str:
        """输入文本"""
        try:
            hwnd = int(window_id)
            result = send_keyboard_input(hwnd, KeyboardAction.TYPE_TEXT,
                                       KeyboardPayload(text=text))
            return result
        except Exception as e:
            return f"Error: {e}"
    
    def cursor_chat(self, prompt: str) -> str:
        """向Cursor聊天界面发送提示词"""
        try:
            # 查找Cursor窗口
            ide_windows = find_ide_windows()
            cursor_window = None
            for window in ide_windows:
                title_lower = window.title.lower()
                if 'cursor' in title_lower and 'cursor' not in title_lower.replace('cursor', ''):
                    cursor_window = window
                    break
            
            if not cursor_window:
                return "Error: Cursor window not found. Available IDE windows: " + \
                       ", ".join([w.title for w in ide_windows])
            
            hwnd = int(cursor_window.id)
            
            # 如果是中文提示词，添加调试信息
            has_chinese = any(ord(char) > 127 for char in prompt)
            if self.verbose and has_chinese:
                print(f"Sending Chinese prompt to Cursor: {prompt[:20]}...")
            
            result = smart_ide_interaction(hwnd, 'type_prompt', {'prompt': prompt})
            return result
        except Exception as e:
            return f"Error: {e}"
    
    def analyze_cursor_dialog(self, window_id: str) -> Dict:
        """分析Cursor对话框位置"""
        try:
            hwnd = int(window_id)
            # 先截图
            screenshot_data = capture_window_screenshot(hwnd)
            
            # 获取窗口信息
            window_details = get_window_details(hwnd)
            
            # 简单的对话框位置推测（基于窗口大小）
            # 通常聊天输入框在窗口右侧下方
            width = window_details.size.width
            height = window_details.size.height
            
            # 推测输入框位置（窗口右侧1/3，下方1/4处）
            input_x = int(width * 0.7)
            input_y = int(height * 0.8)
            
            return {
                "window_info": {
                    "id": window_details.id,
                    "title": window_details.title,
                    "size": {"width": width, "height": height}
                },
                "estimated_input_position": {"x": input_x, "y": input_y},
                "screenshot_size": len(screenshot_data)
            }
        except Exception as e:
            return {"error": str(e)}

# CLI命令行界面
def main():
    """主CLI入口"""
    parser = argparse.ArgumentParser(description='UI控制工具')
    parser.add_argument('-v', '--verbose', action='store_true', help='详细输出')
    
    subparsers = parser.add_subparsers(dest='command', help='可用命令')
    
    # list-windows 命令
    list_parser = subparsers.add_parser('list-windows', help='列出所有窗口')
    
    # find-ide 命令
    ide_parser = subparsers.add_parser('find-ide', help='查找IDE窗口')
    
    # screenshot 命令
    screenshot_parser = subparsers.add_parser('screenshot', help='截取窗口截图')
    screenshot_parser.add_argument('window_id', help='窗口ID')
    screenshot_parser.add_argument('output', nargs='?', help='输出文件路径（可选）')
    
    # send-keys 命令
    keys_parser = subparsers.add_parser('send-keys', help='发送快捷键')
    keys_parser.add_argument('window_id', help='窗口ID')
    keys_parser.add_argument('keys', help='按键组合，如 "ctrl+c"')
    
    # type-text 命令
    type_parser = subparsers.add_parser('type-text', help='输入文本')
    type_parser.add_argument('window_id', help='窗口ID')
    type_parser.add_argument('text', help='要输入的文本')
    
    # cursor-chat 命令
    chat_parser = subparsers.add_parser('cursor-chat', help='向Cursor发送提示词')
    chat_parser.add_argument('prompt', help='提示词内容')
    
    # analyze-cursor 命令
    analyze_parser = subparsers.add_parser('analyze-cursor', help='分析Cursor对话框')
    analyze_parser.add_argument('window_id', help='Cursor窗口ID')
    
    # serve 命令
    serve_parser = subparsers.add_parser('serve', help='启动Web API服务器')
    serve_parser.add_argument('--port', type=int, default=9091, help='端口号')
    serve_parser.add_argument('--host', default='127.0.0.1', help='主机地址')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    controller = UIController()
    controller.verbose = args.verbose
    
    try:
        if args.command == 'list-windows':
            windows = controller.list_windows()
            print(json.dumps(windows, indent=2, ensure_ascii=False))
        
        elif args.command == 'find-ide':
            ide_windows = controller.find_ide_windows()
            print(json.dumps(ide_windows, indent=2, ensure_ascii=False))
        
        elif args.command == 'screenshot':
            result = controller.take_screenshot(args.window_id, args.output)
            if args.output:
                print(result)
            else:
                print(f"Screenshot base64 length: {len(result)}")
        
        elif args.command == 'send-keys':
            result = controller.send_keys(args.window_id, args.keys)
            print(result)
        
        elif args.command == 'type-text':
            result = controller.type_text(args.window_id, args.text)
            print(result)
        
        elif args.command == 'cursor-chat':
            result = controller.cursor_chat(args.prompt)
            print(result)
        
        elif args.command == 'analyze-cursor':
            result = controller.analyze_cursor_dialog(args.window_id)
            print(json.dumps(result, indent=2, ensure_ascii=False))
        
        elif args.command == 'serve':
            print(f"启动Web API服务器在 http://{args.host}:{args.port}")
            start_web_server(args.host, args.port)
        
    except Exception as e:
        print(f"Error: {e}")
        if controller.verbose:
            import traceback
            traceback.print_exc()

# ============================================================================
# Web API服务器（可选）
# ============================================================================

def start_web_server(host: str = "127.0.0.1", port: int = 9091):
    """启动Web API服务器"""
    try:
        import uvicorn
        from fastapi import FastAPI, HTTPException, Response, Query, Path, Body
        from fastapi.responses import JSONResponse
        from pydantic import BaseModel, Field
        
        # 创建FastAPI应用
        app = FastAPI(
            title="UI控制API",
            description="提供与桌面GUI交互的API端点",
            version="1.0.0",
        )
        
        # 创建控制器实例
        controller = UIController()
        
        # API路由 - 简单转发到CLI功能
        @app.get("/")
        def read_root():
            return {
                "name": "UI控制API",
                "version": "1.0.0",
                "description": "CLI优先架构的UI控制工具",
                "docs_url": "/docs"
            }
        
        @app.get("/windows")
        def get_windows():
            return controller.list_windows()
        
        @app.get("/ide/windows")
        def get_ide_windows():
            return controller.find_ide_windows()
        
        @app.get("/windows/{window_id}/screenshot")
        def get_window_screenshot(window_id: str):
            screenshot_data = controller.take_screenshot(window_id)
            if screenshot_data.startswith("Error:"):
                raise HTTPException(status_code=400, detail=screenshot_data)
            
            # 解码base64并返回图片
            img_data = base64.b64decode(screenshot_data)
            return Response(content=img_data, media_type="image/png")
        
        @app.post("/windows/{window_id}/keys")
        def send_keys(window_id: str, keys: str = Body(...)):
            result = controller.send_keys(window_id, keys)
            if result.startswith("Error:"):
                raise HTTPException(status_code=400, detail=result)
            return {"status": "success", "message": result}
        
        @app.post("/windows/{window_id}/text")
        def type_text(window_id: str, text: str = Body(...)):
            result = controller.type_text(window_id, text)
            if result.startswith("Error:"):
                raise HTTPException(status_code=400, detail=result)
            return {"status": "success", "message": result}
        
        @app.post("/cursor/chat")
        def cursor_chat(prompt: str = Body(...)):
            result = controller.cursor_chat(prompt)
            if result.startswith("Error:"):
                raise HTTPException(status_code=400, detail=result)
            return {"status": "success", "message": result}
        
        @app.get("/cursor/{window_id}/analyze")
        def analyze_cursor(window_id: str):
            result = controller.analyze_cursor_dialog(window_id)
            if "error" in result:
                raise HTTPException(status_code=400, detail=result["error"])
            return result
        
        # 启动服务器
        uvicorn.run(app, host=host, port=port)
        
    except ImportError:
        print("Web服务器需要安装 fastapi 和 uvicorn:")
        print("pip install fastapi uvicorn")
    except Exception as e:
        print(f"启动Web服务器失败: {e}")

# 主入口
if __name__ == "__main__":
    main()
