"""
Window management functionality.
"""

import win32gui
import win32con
import win32ui
import win32api
from typing import Optional, Tuple, List, Dict
import numpy as np
from PIL import Image
import time

class WindowManager:
    """Manages window operations in the Windows session."""
    
    def __init__(self):
        """Initialize the window manager."""
        self._hwnd = None
        
    def find_window(self, window_title: str) -> bool:
        """Find window by title and store its handle."""
        def callback(hwnd, ctx):
            if win32gui.IsWindowVisible(hwnd):
                title = win32gui.GetWindowText(hwnd)
                if window_title.lower() in title.lower():
                    ctx.append(hwnd)
            return True
        
        found_windows = []
        win32gui.EnumWindows(callback, found_windows)
        
        if found_windows:
            self._hwnd = found_windows[0]
            return True
        return False
    
    def list_windows(self) -> List[Dict[str, any]]:
        """List all visible windows with their properties."""
        windows = []
        
        def callback(hwnd, ctx):
            if win32gui.IsWindowVisible(hwnd):
                title = win32gui.GetWindowText(hwnd)
                if title:  # Only include windows with titles
                    rect = win32gui.GetWindowRect(hwnd)
                    style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
                    windows.append({
                        'handle': hwnd,
                        'title': title,
                        'rect': rect,
                        'style': style,
                        'minimized': bool(style & win32con.WS_MINIMIZE),
                        'maximized': bool(style & win32con.WS_MAXIMIZE)
                    })
            return True
        
        win32gui.EnumWindows(callback, None)
        return windows
    
    def get_window_rect(self) -> Optional[Tuple[int, int, int, int]]:
        """Get window rectangle coordinates."""
        if not self._hwnd:
            return None
        return win32gui.GetWindowRect(self._hwnd)
    
    def move_window(self, x: int, y: int, width: int, height: int, repaint: bool = True) -> bool:
        """Move and resize window."""
        if not self._hwnd:
            return False
        try:
            win32gui.MoveWindow(self._hwnd, x, y, width, height, repaint)
            return True
        except Exception as e:
            print(f"Error moving window: {e}")
            return False
    
    def minimize_window(self) -> bool:
        """Minimize the window."""
        if not self._hwnd:
            return False
        try:
            win32gui.ShowWindow(self._hwnd, win32con.SW_MINIMIZE)
            return True
        except Exception as e:
            print(f"Error minimizing window: {e}")
            return False
    
    def maximize_window(self) -> bool:
        """Maximize the window."""
        if not self._hwnd:
            return False
        try:
            win32gui.ShowWindow(self._hwnd, win32con.SW_MAXIMIZE)
            return True
        except Exception as e:
            print(f"Error maximizing window: {e}")
            return False
    
    def restore_window(self) -> bool:
        """Restore the window from minimized/maximized state."""
        if not self._hwnd:
            return False
        try:
            win32gui.ShowWindow(self._hwnd, win32con.SW_RESTORE)
            return True
        except Exception as e:
            print(f"Error restoring window: {e}")
            return False
    
    def capture_window(self) -> Optional[Image.Image]:
        """Capture window content as PIL Image."""
        if not self._hwnd:
            return None
            
        try:
            # 处理最小化状态
            was_minimized = self.restore_window()
            
            # 获取窗口尺寸
            left, top, right, bottom = self.get_window_rect()
            width = right - left
            height = bottom - top
            
            # 创建设备上下文
            hwndDC = win32gui.GetWindowDC(self._hwnd)
            mfcDC = win32ui.CreateDCFromHandle(hwndDC)
            saveDC = mfcDC.CreateCompatibleDC()
            
            # 创建位图
            saveBitMap = win32ui.CreateBitmap()
            saveBitMap.CreateCompatibleBitmap(mfcDC, width, height)
            saveDC.SelectObject(saveBitMap)
            
            try:
                # 尝试使用PrintWindow
                result = win32gui.PrintWindow(self._hwnd, saveDC.GetSafeHdc(), 2)
                if not result:
                    # 如果PrintWindow失败，使用BitBlt
                    result = saveDC.BitBlt((0, 0), (width, height), mfcDC, (0, 0), win32con.SRCCOPY)
                
                # 获取位图数据
                bmpstr = saveBitMap.GetBitmapBits(True)
                
                # 转换为numpy数组
                img = np.frombuffer(bmpstr, dtype='uint8')
                img.shape = (height, width, 4)
                
                # 转换为PIL图像
                return Image.fromarray(img[..., :3])  # 移除alpha通道
                
            finally:
                # 清理资源
                win32gui.DeleteObject(saveBitMap.GetHandle())
                saveDC.DeleteDC()
                mfcDC.DeleteDC()
                win32gui.ReleaseDC(self._hwnd, hwndDC)
            
        except Exception as e:
            print(f"Error capturing window: {e}")
            return None 