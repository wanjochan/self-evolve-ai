import win32gui
import win32ui
import win32con
import win32api
import numpy as np
from PIL import Image
from typing import Optional, Tuple, List, Dict
import time
import ctypes
import logging

# 设置日志
logging.basicConfig(level=logging.INFO, 
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("maestro.capture")

# 加载user32.dll
user32 = ctypes.windll.user32

class WindowInfo:
    """Window information container"""
    def __init__(self, hwnd: int, title: str, rect: Tuple[int, int, int, int], 
                 is_visible: bool, is_minimized: bool):
        self.hwnd = hwnd
        self.title = title
        self.rect = rect  # left, top, right, bottom
        self.is_visible = is_visible
        self.is_minimized = is_minimized
        
    @property
    def width(self) -> int:
        return self.rect[2] - self.rect[0]
        
    @property
    def height(self) -> int:
        return self.rect[3] - self.rect[1]
        
    def __str__(self) -> str:
        return f"Window(hwnd={self.hwnd}, title='{self.title}', " \
               f"size={self.width}x{self.height}, visible={self.is_visible})"

class WindowCapture:
    def __init__(self, debug: bool = False):
        self._hwnd = None
        self.debug = debug
        if debug:
            logger.setLevel(logging.DEBUG)
        
    def find_all_windows(self) -> List[WindowInfo]:
        """Find all visible windows"""
        def callback(hwnd, windows):
            if win32gui.IsWindowVisible(hwnd):
                title = win32gui.GetWindowText(hwnd)
                if title:  # 忽略无标题窗口
                    rect = win32gui.GetWindowRect(hwnd)
                    style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
                    is_minimized = bool(style & win32con.WS_MINIMIZE)
                    windows.append(WindowInfo(
                        hwnd=hwnd,
                        title=title,
                        rect=rect,
                        is_visible=True,
                        is_minimized=is_minimized
                    ))
            return True
        
        windows = []
        win32gui.EnumWindows(callback, windows)
        return windows
        
    def find_window(self, window_title: str) -> bool:
        """Find window by title and store its handle"""
        def callback(hwnd, ctx):
            if win32gui.IsWindowVisible(hwnd):
                title = win32gui.GetWindowText(hwnd)
                if window_title.lower() in title.lower():
                    ctx.append(hwnd)
            return True
        
        found_windows = []
        win32gui.EnumWindows(callback, found_windows)
        
        if found_windows:
            self._hwnd = found_windows[0]  # 使用第一个匹配的窗口
            logger.debug(f"Found window handle: {self._hwnd}")
            return True
        return False
    
    def set_window_handle(self, hwnd: int):
        """Set window handle directly"""
        self._hwnd = hwnd
    
    def get_window_rect(self) -> Tuple[int, int, int, int]:
        """Get window rectangle coordinates"""
        if not self._hwnd:
            raise ValueError("No window handle set")
        return win32gui.GetWindowRect(self._hwnd)
        
    def get_client_rect(self) -> Tuple[int, int, int, int]:
        """Get client area rectangle coordinates"""
        if not self._hwnd:
            raise ValueError("No window handle set")
        return win32gui.GetClientRect(self._hwnd)
    
    def _restore_window(self) -> bool:
        """尝试恢复最小化的窗口"""
        if not self._hwnd:
            return False
            
        # 检查窗口是否最小化
        style = win32gui.GetWindowLong(self._hwnd, win32con.GWL_STYLE)
        if style & win32con.WS_MINIMIZE:
            logger.debug("Window is minimized, attempting to restore...")
            # 发送恢复命令
            win32gui.ShowWindow(self._hwnd, win32con.SW_RESTORE)
            # 等待窗口恢复
            time.sleep(0.5)
            # 确保窗口在前台
            win32gui.SetForegroundWindow(self._hwnd)
            time.sleep(0.5)
            return True
        return False
    
    def _move_window_to_visible_area(self):
        """将窗口移动到可见区域"""
        if not self._hwnd:
            return
            
        # 获取主显示器分辨率
        screen_width = win32api.GetSystemMetrics(win32con.SM_CXSCREEN)
        screen_height = win32api.GetSystemMetrics(win32con.SM_CYSCREEN)
        
        # 获取当前窗口位置
        left, top, right, bottom = self.get_window_rect()
        width = right - left
        height = bottom - top
        
        # 如果窗口在屏幕外或位置异常，移动到屏幕中心
        if left < 0 or top < 0 or left >= screen_width or top >= screen_height:
            logger.debug("Window is outside visible area, moving to center...")
            new_left = (screen_width - width) // 2
            new_top = (screen_height - height) // 2
            win32gui.MoveWindow(self._hwnd, new_left, new_top, width, height, True)
            time.sleep(0.5)
    
    def _try_capture_methods(self, dc_obj, mem_dc, width, height) -> Optional[np.ndarray]:
        """尝试不同的捕获方法"""
        methods = [
            {
                'name': 'PrintWindow with PW_RENDERFULLCONTENT',
                'func': lambda: self._capture_with_print_window(mem_dc, width, height, 2)
            },
            {
                'name': 'PrintWindow standard',
                'func': lambda: self._capture_with_print_window(mem_dc, width, height, 0)
            },
            {
                'name': 'Client DC with BitBlt',
                'func': lambda: self._capture_with_client_dc(mem_dc, width, height)
            },
            {
                'name': 'Window DC with BitBlt',
                'func': lambda: self._capture_with_window_dc(dc_obj, mem_dc, width, height)
            }
        ]
        
        for method in methods:
            try:
                logger.debug(f"Trying {method['name']}...")
                img = method['func']()
                if img is not None and not np.all(img == 0):
                    logger.debug(f"{method['name']} succeeded!")
                    return img
                else:
                    logger.debug(f"{method['name']} produced black image, trying next method...")
            except Exception as e:
                logger.debug(f"{method['name']} failed: {e}")
                
        return None
        
    def _capture_with_print_window(self, mem_dc, width, height, flags) -> Optional[np.ndarray]:
        """使用PrintWindow捕获窗口"""
        result = user32.PrintWindow(self._hwnd, mem_dc.GetSafeHdc(), flags)
        if result:
            return self._get_bitmap_data(mem_dc, width, height)
        return None
        
    def _capture_with_client_dc(self, mem_dc, width, height) -> Optional[np.ndarray]:
        """使用客户区DC进行捕获"""
        client_dc = win32gui.GetDC(self._hwnd)
        try:
            client_dc_obj = win32ui.CreateDCFromHandle(client_dc)
            mem_dc.BitBlt((0, 0), (width, height), client_dc_obj, (0, 0), win32con.SRCCOPY)
            client_dc_obj.DeleteDC()
            return self._get_bitmap_data(mem_dc, width, height)
        finally:
            win32gui.ReleaseDC(self._hwnd, client_dc)
            
    def _capture_with_window_dc(self, dc_obj, mem_dc, width, height) -> Optional[np.ndarray]:
        """使用窗口DC进行捕获"""
        mem_dc.BitBlt((0, 0), (width, height), dc_obj, (0, 0), win32con.SRCCOPY)
        return self._get_bitmap_data(mem_dc, width, height)
    
    def _get_bitmap_data(self, mem_dc, width, height) -> Optional[np.ndarray]:
        """从内存DC获取位图数据"""
        bitmap = mem_dc.GetCurrentBitmap()
        bmpstr = bitmap.GetBitmapBits(True)
        img = np.frombuffer(bmpstr, dtype='uint8')
        img.shape = (height, width, 4)
        return img
    
    def capture(self) -> Optional[Image.Image]:
        """Capture window content as PIL Image"""
        if not self._hwnd:
            logger.error("No window handle set")
            return None
            
        try:
            # 处理最小化状态
            was_minimized = self._restore_window()
            
            # 确保窗口在可见区域
            self._move_window_to_visible_area()
            
            # 获取窗口尺寸
            left, top, right, bottom = self.get_window_rect()
            width = right - left
            height = bottom - top
            
            if width <= 0 or height <= 0:
                logger.error(f"Invalid window dimensions: {width}x{height}")
                return None
                
            logger.debug(f"Window dimensions: {width}x{height}")
            logger.debug(f"Window position: ({left}, {top}) -> ({right}, {bottom})")
            
            # 创建设备上下文
            hwndDC = win32gui.GetWindowDC(self._hwnd)
            mfcDC = win32ui.CreateDCFromHandle(hwndDC)
            saveDC = mfcDC.CreateCompatibleDC()
            
            # 创建位图
            saveBitMap = win32ui.CreateBitmap()
            saveBitMap.CreateCompatibleBitmap(mfcDC, width, height)
            saveDC.SelectObject(saveBitMap)
            
            # 尝试不同的捕获方法
            img_array = self._try_capture_methods(mfcDC, saveDC, width, height)
            
            # 清理资源
            win32gui.DeleteObject(saveBitMap.GetHandle())
            saveDC.DeleteDC()
            mfcDC.DeleteDC()
            win32gui.ReleaseDC(self._hwnd, hwndDC)
            
            if img_array is None:
                logger.error("All capture methods failed")
                return None
                
            # 转换为PIL图像
            img = Image.fromarray(img_array)
            return img
            
        except Exception as e:
            logger.error(f"Error capturing window: {e}")
            return None
    
    def capture_to_file(self, filepath: str) -> bool:
        """Capture window and save to file"""
        img = self.capture()
        if img:
            img.save(filepath)
            logger.info(f"Saved screenshot to {filepath}")
            return True
        return False 