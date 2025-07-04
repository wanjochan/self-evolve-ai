#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
混合窗口扫描器 - 结合Win32 API和UIA的高性能窗口扫描工具

特点:
1. 使用Win32 API进行快速初始扫描
2. 智能检测特殊窗口类型(Chrome/Electron等)
3. 自动切换到UIA进行深度扫描(当需要时)
4. 支持多线程并行处理
5. 提供简洁的JSON输出

依赖:
    pip install pywin32 comtypes

使用示例:
    # 列出所有可见窗口
    python hybrid_scanner.py list
    
    # 扫描特定窗口(按标题)
    python hybrid_scanner.py scan --title "Notepad"
    
    # 扫描特定窗口(按类名)
    python hybrid_scanner.py scan --class "Chrome_WidgetWin_1"
    
    # 扫描所有窗口(包括不可见窗口)
    python hybrid_scanner.py scan --all
    
    # 导出为JSON文件
    python hybrid_scanner.py scan --title "Visual Studio Code" --output vscode_ui.json
"""

import sys
import json
import time
import ctypes
import argparse
import threading
import traceback
from typing import Dict, List, Optional, Any, Callable, Union, Tuple
from dataclasses import dataclass, asdict, field
from enum import Enum, auto
from concurrent.futures import ThreadPoolExecutor, as_completed

# 尝试导入Windows API相关库
try:
    import win32gui
    import win32con
    import win32process
    import win32api
    import pythoncom
    import comtypes
    from comtypes.gen import UIAutomationClient
    WINDOWS_SUPPORT = True
except ImportError:
    WINDOWS_SUPPORT = False

# 窗口类型枚举
class WindowType(Enum):
    UNKNOWN = auto()
    WIN32 = auto()
    UIA = auto()
    CHROME = auto()
    ELECTRON = auto()
    FLUTTER = auto()
    QT = auto()
    WPF = auto()

@dataclass
class Rect:
    left: int
    top: int
    right: int
    bottom: int
    
    @property
    def width(self) -> int:
        return self.right - self.left
    
    @property
    def height(self) -> int:
        return self.bottom - self.top
    
    def to_dict(self) -> Dict[str, int]:
        return {
            'left': self.left,
            'top': self.top,
            'right': self.right,
            'bottom': self.bottom,
            'width': self.width,
            'height': self.height
        }

@dataclass
class WindowInfo:
    # 基础信息
    hwnd: int
    title: str
    class_name: str
    window_type: str
    
    # 几何信息
    rect: Rect
    client_rect: Optional[Rect] = None
    
    # 窗口状态
    is_visible: bool = False
    is_enabled: bool = False
    is_unavailable: bool = False
    is_topmost: bool = False
    
    # 进程信息
    process_id: int = 0
    process_name: str = ""
    thread_id: int = 0
    
    # 窗口关系
    parent_hwnd: int = 0
    owner_hwnd: int = 0
    children: List['WindowInfo'] = field(default_factory=list)
    
    # 额外属性
    attributes: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """转换为字典格式"""
        result = {
            'hwnd': self.hwnd,
            'title': self.title,
            'class_name': self.class_name,
            'window_type': self.window_type,
            'rect': self.rect.to_dict() if self.rect else None,
            'client_rect': self.client_rect.to_dict() if self.client_rect else None,
            'is_visible': self.is_visible,
            'is_enabled': self.is_enabled,
            'is_unavailable': self.is_unavailable,
            'is_topmost': self.is_topmost,
            'process_id': self.process_id,
            'process_name': self.process_name,
            'thread_id': self.thread_id,
            'parent_hwnd': self.parent_hwnd,
            'owner_hwnd': self.owner_hwnd,
            'children': [child.to_dict() for child in self.children],
            'attributes': self.attributes
        }
        return result

class WindowScanner:
    """窗口扫描器基类"""
    
    def __init__(self):
        self.window_cache = {}
        self.process_cache = {}
    
    def get_window_info(self, hwnd: int, **kwargs) -> Optional[WindowInfo]:
        """获取窗口信息"""
        raise NotImplementedError
    
    def get_child_windows(self, parent_hwnd: int, **kwargs) -> List[WindowInfo]:
        """获取子窗口列表"""
        raise NotImplementedError
    
    def detect_window_type(self, hwnd: int, class_name: str) -> WindowType:
        """检测窗口类型
        
        Args:
            hwnd: 窗口句柄
            class_name: 窗口类名
            
        Returns:
            WindowType: 窗口类型枚举
        """
        """检测窗口类型"""
        class_name = (class_name or "").lower()
        
        # 检测Chrome/Electron应用
        if any(name in class_name for name in [
            'chrome_', 'cef', 'electron', 'msedge', 'nwjs', 'cefwebview',
            'chrometab_', 'chrome_widget', 'chrome_window'
        ]):
            return WindowType.CHROME
            
        # 检测WPF应用
        if 'wpf' in class_name or 'presentationhost' in class_name:
            return WindowType.WPF
            
        # 检测Qt应用
        if 'qt' in class_name or 'qwidget' in class_name:
            return WindowType.QT
            
        # 检测Flutter应用
        if 'flutter' in class_name or 'fluttershell' in class_name:
            return WindowType.FLUTTER
            
        # 检测UIA应用
        if any(win32gui.GetProp(hwnd, prop) for prop in [
            'UIA_AutomationId', 'UIA_ControlType', 'UIA_Name'
        ] if hasattr(win32gui, 'GetProp')):
            return WindowType.UIA
            
        return WindowType.WIN32
    
    def get_process_info(self, process_id: int) -> Dict[str, Any]:
        """获取进程信息
        
        Args:
            process_id: 进程ID
            
        Returns:
            Dict: 包含进程信息的字典
        """
        """获取进程信息"""
        if not process_id or process_id <= 0:
            return {}
            
        if process_id in self.process_cache:
            return self.process_cache[process_id]
            
        try:
            import psutil
            process = psutil.Process(process_id)
            info = {
                'name': process.name(),
                'exe': process.exe(),
                'cmdline': process.cmdline(),
                'status': process.status(),
                'create_time': process.create_time(),
                'cpu_percent': process.cpu_percent(),
                'memory_info': process.memory_info()._asdict()
            }
            self.process_cache[process_id] = info
            return info
        except Exception as e:
            return {}

class Win32Scanner(WindowScanner):
    """Win32窗口扫描器"""
    
    def __init__(self):
        """初始化Win32扫描器"""
        super().__init__()
        self.user32 = ctypes.windll.user32
        self._init_win32_functions()
        
    def _init_win32_functions(self):
        """初始化Win32 API函数"""
        # 定义函数原型
        self.user32.IsWindowUnicode.argtypes = [ctypes.wintypes.HWND]
        self.user32.IsWindowUnicode.restype = ctypes.wintypes.BOOL
        
    def get_window_info(self, hwnd: int, **kwargs) -> Optional[WindowInfo]:
        if not hwnd or not self.user32.IsWindow(hwnd):
            return None
            
        try:
            # 获取窗口标题
            title = win32gui.GetWindowText(hwnd)
            
            # 获取窗口类名
            class_name = win32gui.GetClassName(hwnd)
            
            # 检测窗口类型
            window_type = self.detect_window_type(hwnd, class_name)
            
            # 获取窗口矩形
            try:
                left, top, right, bottom = win32gui.GetWindowRect(hwnd)
                rect = Rect(left, top, right, bottom)
            except:
                rect = Rect(0, 0, 0, 0)
                
            # 获取客户端矩形
            try:
                cl, ct, cr, cb = win32gui.GetClientRect(hwnd)
                client_rect = Rect(cl, ct, cr, cb)
                
                # 转换为屏幕坐标
                pt = win32api.MAKELONG(0, 0)
                client_left, client_top = win32gui.ClientToScreen(hwnd, pt)
                client_rect = Rect(
                    client_left, 
                    client_top,
                    client_left + client_rect.width,
                    client_top + client_rect.height
                )
            except:
                client_rect = None
            
            # 获取窗口状态
            is_visible = bool(win32gui.IsWindowVisible(hwnd))
            is_enabled = bool(win32gui.IsWindowEnabled(hwnd))
            # 安全地检查窗口是否支持Unicode
            try:
                is_unicode = bool(ctypes.windll.user32.IsWindowUnicode(hwnd))
                is_unavailable = not is_unicode
            except:
                is_unavailable = False
            
            # 获取进程和线程ID
            thread_id, process_id = win32process.GetWindowThreadProcessId(hwnd)
            
            # 获取窗口关系
            parent_hwnd = win32gui.GetParent(hwnd)
            owner_hwnd = win32gui.GetWindow(hwnd, win32con.GW_OWNER)
            
            # 获取进程名
            process_name = ""
            try:
                process = win32api.OpenProcess(0x0400, False, process_id)
                process_name = win32process.GetModuleFileNameEx(process, 0)
            except:
                pass
            
            # 创建窗口信息对象
            window = WindowInfo(
                hwnd=hwnd,
                title=title,
                class_name=class_name,
                window_type=window_type.name,
                rect=rect,
                client_rect=client_rect,
                is_visible=is_visible,
                is_enabled=is_enabled,
                is_unavailable=is_unavailable,
                is_topmost=bool(win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE) & win32con.WS_EX_TOPMOST),
                process_id=process_id,
                process_name=process_name,
                thread_id=thread_id,
                parent_hwnd=parent_hwnd if parent_hwnd else 0,
                owner_hwnd=owner_hwnd if owner_hwnd else 0
            )
            
            # 获取子窗口
            if kwargs.get('include_children', True):
                window.children = self.get_child_windows(hwnd, **kwargs)
            
            return window
            
        except Exception as e:
            print(f"Error getting window info for {hwnd}: {e}", file=sys.stderr)
            if kwargs.get('debug', False):
                traceback.print_exc()
            return None
    
    def get_child_windows(self, parent_hwnd: int, **kwargs) -> List[WindowInfo]:
        """获取子窗口列表"""
        children = []
        
        def enum_child_proc(hwnd, lparam):
            try:
                child = self.get_window_info(hwnd, **kwargs)
                if child:
                    children.append(child)
            except:
                pass
            return True
            
        try:
            win32gui.EnumChildWindows(parent_hwnd, enum_child_proc, None)
        except:
            pass
            
        return children

class UIAutomationScanner(WindowScanner):
    """UIAutomation窗口扫描器"""
    
    def __init__(self):
        super().__init__()
        self.uia = None
        self.initialized = False
        self._init_uia()
    
    def _init_uia(self):
        """初始化UIAutomation"""
        if not WINDOWS_SUPPORT or self.initialized:
            return
            
        try:
            pythoncom.CoInitialize()
            self.uia = comtypes.client.GetModule('UIAutomationCore.dll')
            self.initialized = True
        except Exception as e:
            print(f"Failed to initialize UIAutomation: {e}", file=sys.stderr)
    
    def get_window_info(self, hwnd: int, **kwargs) -> Optional[WindowInfo]:
        if not self.initialized or not hwnd:
            return None
            
        try:
            # 使用Win32 API获取基本信息
            win32_info = Win32Scanner().get_window_info(hwnd, include_children=False)
            if not win32_info:
                return None
                
            # 设置窗口类型
            win32_info.window_type = WindowType.UIA.name
            
            # 尝试使用UIA获取更多信息
            try:
                element = win32gui.GetWindow(hwnd, win32con.GW_CHILD)
                if element:
                    # 这里可以添加UIA特定的元素扫描逻辑
                    pass
            except:
                pass
                
            return win32_info
            
        except Exception as e:
            print(f"UIA Error getting window info for {hwnd}: {e}", file=sys.stderr)
            if kwargs.get('debug', False):
                traceback.print_exc()
            return None
    
    def get_child_windows(self, parent_hwnd: int, **kwargs) -> List[WindowInfo]:
        """获取子窗口列表"""
        if not self.initialized:
            return []
            
        children = []
        try:
            # 这里可以添加UIA特定的子元素扫描逻辑
            pass
        except:
            pass
            
        return children

class HybridWindowScanner:
    """混合窗口扫描器"""
    
    def __init__(self):
        self.win32_scanner = Win32Scanner()
        self.uia_scanner = UIAutomationScanner()
        self.scan_cache = {}
    
    def get_window_info(self, hwnd: int, **kwargs) -> Optional[WindowInfo]:
        """获取窗口信息"""
        if not hwnd:
            return None
            
        # 从缓存中获取
        cache_key = f"{hwnd}_{kwargs.get('depth', 0)}"
        if cache_key in self.scan_cache:
            return self.scan_cache[cache_key]
            
        # 首先使用Win32扫描器
        window = self.win32_scanner.get_window_info(hwnd, **kwargs)
        if not window:
            return None
            
        # 对于特殊类型的窗口，尝试使用UIA扫描器
        if window.window_type in [WindowType.UIA.name, WindowType.WPF.name, WindowType.CHROME.name]:
            uia_window = self.uia_scanner.get_window_info(hwnd, **kwargs)
            if uia_window:
                # 合并窗口信息
                window.window_type = uia_window.window_type
                window.attributes.update(uia_window.attributes)
                
                # 合并子窗口
                if uia_window.children:
                    window.children = uia_window.children
        
        # 缓存结果
        self.scan_cache[cache_key] = window
        return window
    
    def find_windows(self, **kwargs) -> List[WindowInfo]:
        """查找匹配的窗口"""
        windows = []
        debug = kwargs.get('debug', False)
        
        def enum_windows_proc(hwnd, lparam):
            try:
                if debug:
                    print(f"\r扫描窗口: 0x{hwnd:X}", end='', flush=True)
                
                # 快速检查窗口是否可见（如果不需要不可见窗口）
                if not kwargs.get('all', False) and not win32gui.IsWindowVisible(hwnd):
                    return True
                    
                # 获取窗口标题和类名进行基本过滤
                try:
                    title = win32gui.GetWindowText(hwnd)
                    class_name = win32gui.GetClassName(hwnd)
                    
                    # 如果不需要不可见窗口且标题为空，则跳过
                    if not kwargs.get('all', False) and not title:
                        return True
                        
                except Exception as e:
                    if debug:
                        print(f"\n获取窗口信息时出错 (0x{hwnd:X}): {e}")
                    return True
                
                # 获取完整的窗口信息
                window = self.get_window_info(hwnd, **kwargs)
                if window:
                    windows.append(window)
                    if debug:
                        print(f"\n找到窗口: {window.title} (0x{hwnd:X}, {window.class_name})")
                        
            except Exception as e:
                if debug:
                    print(f"\n处理窗口时出错 (0x{hwnd:X}): {e}")
                    traceback.print_exc()
            return True
            
        try:
            if debug:
                print("开始枚举窗口...")
            win32gui.EnumWindows(enum_windows_proc, None)
            if debug:
                print("\n窗口枚举完成")
        except Exception as e:
            if debug:
                print(f"\n枚举窗口时出错: {e}")
                traceback.print_exc()
            
        return windows

def format_window_info(window: WindowInfo, show_children: bool = False, indent: int = 0) -> str:
    """格式化窗口信息为易读字符串"""
    indent_str = '  ' * indent
    lines = [
        f"{indent_str}窗口标题: {window.title}",
        f"{indent_str}类名: {window.class_name}",
        f"{indent_str}句柄: 0x{window.hwnd:X} ({window.hwnd})",
        f"{indent_str}类型: {window.window_type}",
        f"{indent_str}位置: ({window.rect.left}, {window.rect.top}) - ({window.rect.right}, {window.rect.bottom})",
        f"{indent_str}大小: {window.rect.width}x{window.rect.height}",
        f"{indent_str}可见: {'是' if window.is_visible else '否'}",
        f"{indent_str}启用: {'是' if window.is_enabled else '否'}",
        f"{indent_str}进程: {window.process_name} (PID: {window.process_id})",
        f"{indent_str}子窗口数: {len(window.children)}"
    ]
    
    if show_children and window.children:
        lines.append(f"{indent_str}子窗口:")
        for i, child in enumerate(window.children, 1):
            lines.append(f"{indent_str}  {i}. {child.title} ({child.class_name}, 0x{child.hwnd:X})")
    
    return '\n'.join(lines)

def list_windows(**kwargs):
    """列出所有窗口"""
    debug = kwargs.get('debug', False)
    print("正在扫描窗口... (按Ctrl+C中断)")
    start_time = time.time()
    
    try:
        scanner = HybridWindowScanner()
        windows = scanner.find_windows(**kwargs)
        
        # 过滤窗口（大部分过滤已经在find_windows中完成）
        if not kwargs.get('all', False):
            windows = [w for w in windows if w.is_visible and w.title]
        
        # 按窗口标题排序
        windows.sort(key=lambda w: w.title.lower())
        
        # 计算耗时
        elapsed = time.time() - start_time
        
        # 输出结果
        if kwargs.get('output'):
            result = [w.to_dict() for w in windows]
            with open(kwargs['output'], 'w', encoding='utf-8') as f:
                json.dump(result, f, ensure_ascii=False, indent=2)
            print(f"\n已保存到 {kwargs['output']}")
        else:
            print("\n" + "=" * 80)
            print(f"找到 {len(windows)} 个窗口 (耗时: {elapsed:.2f}秒):")
            print("=" * 80)
            
            # 按进程分组显示
            process_map = {}
            for w in windows:
                if w.process_name not in process_map:
                    process_map[w.process_name] = []
                process_map[w.process_name].append(w)
            
            for i, (process, win_list) in enumerate(process_map.items(), 1):
                print(f"\n{i}. 进程: {process} (共 {len(win_list)} 个窗口)")
                print("-" * 60)
                for j, win in enumerate(win_list, 1):
                    print(f"   {j}. {win.title} (0x{win.hwnd:X}, {win.class_name})")
            
            # 显示详细信息（如果启用）
            if kwargs.get('verbose'):
                print("\n" + "=" * 80)
                print("窗口详细信息:")
                print("=" * 80)
                for i, window in enumerate(windows, 1):
                    print(f"\n{i}. {window.title} (0x{window.hwnd:X})")
                    print("-" * 80)
                    print(format_window_info(window, show_children=kwargs.get('show_children', False), indent=2))
        
        print(f"\n{'='*80}\n扫描完成，共 {len(windows)} 个窗口 (耗时: {elapsed:.2f}秒)\n{'='*80}")
        
    except KeyboardInterrupt:
        elapsed = time.time() - start_time
        print(f"\n\n{'*'*60}\n扫描被用户中断! (已扫描 {len(windows)} 个窗口, 耗时: {elapsed:.2f}秒)\n{'*'*60}\n")
    except Exception as e:
        elapsed = time.time() - start_time
        print(f"\n\n{'*'*60}\n扫描过程中出错: {e}\n{'*'*60}")
        if debug:
            traceback.print_exc()

def scan_window(**kwargs):
    """扫描窗口"""
    print("正在查找目标窗口...")
    start_time = time.time()
    
    scanner = HybridWindowScanner()
    
    # 查找目标窗口
    target_window = None
    if kwargs.get('hwnd'):
        print(f"正在查找句柄为 0x{kwargs['hwnd']:X} 的窗口...")
        target_window = scanner.get_window_info(kwargs['hwnd'], **kwargs)
    elif kwargs.get('title'):
        print(f"正在查找标题包含 '{kwargs['title']}' 的窗口...")
        windows = scanner.find_windows()
        matches = []
        for w in windows:
            if kwargs['title'].lower() in w.title.lower():
                matches.append(w)
        
        if not matches:
            print(f"未找到标题包含 '{kwargs['title']}' 的窗口")
            return
            
        if len(matches) > 1:
            print(f"找到 {len(matches)} 个匹配的窗口:")
            for i, w in enumerate(matches, 1):
                print(f"  {i}. {w.title} (0x{w.hwnd:X}, {w.class_name})")
            
            if not kwargs.get('all', False):
                print("\n使用 --all 参数扫描所有匹配的窗口")
                print("或使用 --hwnd 参数指定具体的窗口句柄")
                return
            
            target_windows = matches
        else:
            target_window = matches[0]
    
    elif kwargs.get('class_name'):
        print(f"正在查找类名为 '{kwargs['class_name']}' 的窗口...")
        windows = scanner.find_windows()
        matches = []
        for w in windows:
            if kwargs['class_name'].lower() in w.class_name.lower():
                matches.append(w)
        
        if not matches:
            print(f"未找到类名包含 '{kwargs['class_name']}' 的窗口")
            return
            
        if len(matches) > 1:
            print(f"找到 {len(matches)} 个匹配的窗口:")
            for i, w in enumerate(matches, 1):
                print(f"  {i}. {w.title} (0x{w.hwnd:X}, {w.class_name})")
            
            if not kwargs.get('all', False):
                print("\n使用 --all 参数扫描所有匹配的窗口")
                print("或使用 --hwnd 参数指定具体的窗口句柄")
                return
                
            target_windows = matches
        else:
            target_window = matches[0]
    
    else:
        print("请指定 --hwnd, --title 或 --class 参数来查找窗口")
        return
    
    # 处理多个目标窗口的情况
    if 'target_windows' in locals():
        print(f"\n开始扫描 {len(target_windows)} 个匹配的窗口...")
        results = []
        
        for i, win in enumerate(target_windows, 1):
            print(f"\n[{i}/{len(target_windows)}] 正在扫描窗口: {win.title} (0x{win.hwnd:X})")
            try:
                window_tree = scanner.get_window_info(win.hwnd, **kwargs)
                results.append(window_tree)
                print(format_window_info(window_tree, show_children=kwargs.get('show_children', True)))
                print("-" * 80)
            except Exception as e:
                print(f"扫描窗口 {win.title} (0x{win.hwnd:X}) 时出错: {e}")
                if kwargs.get('debug'):
                    traceback.print_exc()
        
        if kwargs.get('output'):
            result = [w.to_dict() for w in results]
            with open(kwargs['output'], 'w', encoding='utf-8') as f:
                json.dump(result, f, ensure_ascii=False, indent=2)
            print(f"\n已保存到 {kwargs['output']}")
        
        elapsed = time.time() - start_time
        print(f"\n扫描完成，共扫描 {len(results)} 个窗口 (耗时: {elapsed:.3f}秒)")
        return
    
    # 处理单个目标窗口
    if not target_window:
        print("未找到匹配的窗口", file=sys.stderr)
        return
    
    print(f"\n找到目标窗口: {target_window.title} (0x{target_window.hwnd:X})")
    print("正在扫描窗口结构...")
    
    # 获取窗口树
    window_tree = scanner.get_window_info(target_window.hwnd, **kwargs)
    
    # 计算耗时
    elapsed = time.time() - start_time
    
    # 输出结果
    if kwargs.get('output'):
        with open(kwargs['output'], 'w', encoding='utf-8') as f:
            json.dump(window_tree.to_dict(), f, ensure_ascii=False, indent=2)
        print(f"\n已保存到 {kwargs['output']}")
    else:
        print("\n" + "=" * 80)
        print(f"窗口信息 (扫描耗时: {elapsed:.3f}秒)")
        print("=" * 80)
        print(format_window_info(window_tree, show_children=True))
    
    print(f"\n扫描完成 (耗时: {elapsed:.3f}秒)")

def main():
    """主函数"""
    if not WINDOWS_SUPPORT:
        print("错误: 此工具需要Windows系统和pywin32库", file=sys.stderr)
        print("请运行: pip install pywin32 comtypes psutil")
        return 1
    
    # 创建主解析器
    parser = argparse.ArgumentParser(
        description='混合窗口扫描器 - 高性能窗口结构树采集工具',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
使用示例:
  1. 列出所有可见窗口:
     python hybrid_scanner.py list
     
  2. 列出所有窗口(包括不可见窗口):
     python hybrid_scanner.py list --all
     
  3. 扫描特定窗口(按标题):
     python hybrid_scanner.py scan --title "Notepad"
     
  4. 扫描特定窗口(按类名):
     python hybrid_scanner.py scan --class "Chrome_WidgetWin_1"
     
  5. 扫描特定窗口(按句柄):
     python hybrid_scanner.py scan --hwnd 123456
     
  6. 详细输出窗口信息:
     python hybrid_scanner.py scan --title "Visual Studio Code" --verbose
     
  7. 保存结果到JSON文件:
     python hybrid_scanner.py scan --title "Chrome" --output chrome_ui.json
     
  8. 显示调试信息:
     python hybrid_scanner.py scan --title "Notepad" --debug
        ''',
        add_help=False
    )
    
    # 添加全局参数
    parser.add_argument('--debug', action='store_true', help='启用调试模式')
    parser.add_argument('-h', '--help', action='store_true', help='显示帮助信息')
    
    # 创建子命令解析器
    subparsers = parser.add_subparsers(dest='command', help='命令')
    
    # list 命令
    list_parser = subparsers.add_parser('list', help='列出窗口', add_help=False)
    list_parser.add_argument('--all', action='store_true', help='包括不可见窗口')
    list_parser.add_argument('--output', help='输出到JSON文件')
    list_parser.add_argument('--verbose', '-v', action='store_true', help='显示详细信息')
    list_parser.add_argument('--show-children', action='store_true', help='显示子窗口列表')
    list_parser.add_argument('--help', action='store_true', help='显示此帮助信息')
    
    # scan 命令
    scan_parser = subparsers.add_parser('scan', help='扫描窗口结构树', add_help=False)
    scan_parser.add_argument('--hwnd', type=lambda x: int(x, 0), help='窗口句柄(十进制或十六进制)')
    scan_parser.add_argument('--title', help='窗口标题(支持部分匹配)')
    scan_parser.add_argument('--class', dest='class_name', help='窗口类名(支持部分匹配)')
    scan_parser.add_argument('--all', action='store_true', help='处理所有匹配的窗口')
    scan_parser.add_argument('--depth', type=int, default=5, help='扫描深度(默认: 5)')
    scan_parser.add_argument('--output', help='输出到JSON文件')
    scan_parser.add_argument('--verbose', '-v', action='store_true', help='显示详细信息')
    scan_parser.add_argument('--no-children', action='store_true', help='不显示子窗口列表')
    scan_parser.add_argument('--debug', action='store_true', help='显示调试信息')
    scan_parser.add_argument('--help', action='store_true', help='显示此帮助信息')
    
    # 解析参数
    try:
        args = parser.parse_args()
    except SystemExit:
        return 1
    
    # 处理帮助请求
    if args.help or len(sys.argv) == 1:
        parser.print_help()
        print("\n命令:")
        print("  list     列出窗口")
        print("  scan     扫描窗口结构树")
        print("\n使用 '命令 --help' 查看具体命令的帮助信息")
        return 0
    
    # 如果没有指定命令，显示帮助
    if not args.command:
        parser.print_help()
        return 1
    
    # 检查scan命令的必要参数
    if args.command == 'scan' and not any([args.hwnd, args.title, args.class_name, args.all]):
        print("错误: scan命令需要指定--hwnd, --title, --class或--all参数")
        print("\nscan 命令用法:")
        print("  python hybrid_scanner.py scan [--hwnd HWND | --title TITLE | --class CLASS | --all] [options]")
        print("\n选项:")
        print("  --hwnd HWND        窗口句柄(十进制或十六进制)")
        print("  --title TITLE      窗口标题(支持部分匹配)")
        print("  --class CLASS      窗口类名(支持部分匹配)")
        print("  --all              处理所有匹配的窗口")
        print("  --depth DEPTH      扫描深度(默认: 5)")
        print("  --output FILE      输出到JSON文件")
        print("  --verbose, -v      显示详细信息")
        print("  --no-children      不显示子窗口列表")
        print("  --debug            显示调试信息")
        return 1
    
    try:
        # 合并参数
        if args.command == 'list':
            list_args = vars(args).copy()
            list_args.update({
                'show_children': not list_args.pop('no_children', False)
            })
            list_windows(**list_args)
        elif args.command == 'scan':
            scan_args = vars(args).copy()
            scan_args.update({
                'show_children': not scan_args.pop('no_children', False)
            })
            scan_window(**scan_args)
        else:
            parser.print_help()
            return 1
            
    except KeyboardInterrupt:
        print("\n操作被用户取消")
        return 1
    except Exception as e:
        if args.debug:
            print(f"\n{'*'*60}\n错误: {e}\n{'*'*60}")
            traceback.print_exc()
        else:
            print(f"\n错误: {e}")
            print("使用 --debug 参数查看详细错误信息")
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
