import win32gui
import win32ui
import win32con
import win32api
import numpy as np
from PIL import Image
from typing import Optional, Tuple
import time
import ctypes
import logging

# 配置日志
logger = logging.getLogger("ui_ctrl_v2.window")

# 加载user32.dll
user32 = ctypes.windll.user32

class WindowCapture:
    def __init__(self, verbose=True):
        self._hwnd = None
        self.verbose = verbose
        
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
            if self.verbose:
                logger.info(f"Found window handle: {self._hwnd}")
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
            if self.verbose:
                logger.info("Window is minimized, attempting to restore...")
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
            if self.verbose:
                print("Window is outside visible area, moving to center...")
            new_left = (screen_width - width) // 2
            new_top = (screen_height - height) // 2
            win32gui.MoveWindow(self._hwnd, new_left, new_top, width, height, True)
            time.sleep(0.5)
    
    def _try_capture_methods(self, dc_obj, mem_dc, width, height) -> Optional[np.ndarray]:
        """尝试不同的捕获方法"""
        methods = [
            {
                'name': 'Client DC with BitBlt',
                'func': lambda: self._capture_with_client_dc(mem_dc, width, height)
            },
            {
                'name': 'Window DC with BitBlt',
                'func': lambda: self._capture_with_window_dc(dc_obj, mem_dc, width, height)
            },
            {
                'name': 'Client DC with StretchBlt',
                'func': lambda: self._capture_with_stretch_blt(mem_dc, width, height)
            }
        ]
        
        for method in methods:
            try:
                if self.verbose:
                    print(f"\nTrying {method['name']}...")
                img = method['func']()
                if img is not None and not np.all(img == 0):
                    if self.verbose:
                        print(f"{method['name']} succeeded!")
                    return img
                else:
                    if self.verbose:
                        print(f"{method['name']} produced black image, trying next method...")
            except Exception as e:
                if self.verbose:
                    print(f"{method['name']} failed: {e}")
                
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
        
    def _capture_with_stretch_blt(self, mem_dc, width, height) -> Optional[np.ndarray]:
        """使用StretchBlt进行捕获"""
        client_dc = win32gui.GetDC(self._hwnd)
        try:
            client_dc_obj = win32ui.CreateDCFromHandle(client_dc)
            # 获取客户区大小
            client_rect = self.get_client_rect()
            client_width = client_rect[2] - client_rect[0]
            client_height = client_rect[3] - client_rect[1]
            
            mem_dc.StretchBlt(
                (0, 0), (width, height),
                client_dc_obj,
                (0, 0), (client_width, client_height),
                win32con.SRCCOPY
            )
            client_dc_obj.DeleteDC()
            return self._get_bitmap_data(mem_dc, width, height)
        finally:
            win32gui.ReleaseDC(self._hwnd, client_dc)
    
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
            
            if self.verbose:
                print(f"Window dimensions: {width}x{height}")
                print(f"Window position: ({left}, {top}) -> ({right}, {bottom})")
            
            # 创建设备上下文
            hwndDC = win32gui.GetWindowDC(self._hwnd)
            mfcDC = win32ui.CreateDCFromHandle(hwndDC)
            saveDC = mfcDC.CreateCompatibleDC()
            
            # 创建位图
            saveBitMap = win32ui.CreateBitmap()
            saveBitMap.CreateCompatibleBitmap(mfcDC, width, height)
            saveDC.SelectObject(saveBitMap)
            
            # 尝试使用PrintWindow
            if self.verbose:
                print("Trying PrintWindow...")
            result = user32.PrintWindow(self._hwnd, saveDC.GetSafeHdc(), 2)  # PW_RENDERFULLCONTENT = 2
            if self.verbose:
                print(f"PrintWindow result: {result}")
            
            if not result:
                if self.verbose:
                    print("PrintWindow failed, trying BitBlt...")
                result = saveDC.BitBlt((0, 0), (width, height), mfcDC, (0, 0), win32con.SRCCOPY)
                if self.verbose:
                    print(f"BitBlt result: {result}")
            
            # 获取位图数据
            bmpstr = saveBitMap.GetBitmapBits(True)
            
            # 转换为numpy数组
            img = np.frombuffer(bmpstr, dtype='uint8')
            img.shape = (height, width, 4)
            
            # 检查图像是否全黑
            is_black = np.all(img == 0)
            if self.verbose:
                print(f"Image is black: {is_black}")
                print(f"Image shape: {img.shape}")
                print(f"Image min/max values: {img.min()}, {img.max()}")
            
            # 如果图像全黑，尝试使用GetDC
            if is_black:
                if self.verbose:
                    print("Image is black, trying GetDC...")
                # 释放之前的资源
                win32gui.DeleteObject(saveBitMap.GetHandle())
                saveDC.DeleteDC()
                mfcDC.DeleteDC()
                win32gui.ReleaseDC(self._hwnd, hwndDC)
                
                # 使用GetDC重试
                hwndDC = win32gui.GetDC(self._hwnd)
                mfcDC = win32ui.CreateDCFromHandle(hwndDC)
                saveDC = mfcDC.CreateCompatibleDC()
                saveBitMap = win32ui.CreateBitmap()
                saveBitMap.CreateCompatibleBitmap(mfcDC, width, height)
                saveDC.SelectObject(saveBitMap)
                
                # 使用PrintWindow
                result = user32.PrintWindow(self._hwnd, saveDC.GetSafeHdc(), 2)
                if self.verbose:
                    print(f"GetDC PrintWindow result: {result}")
                
                if not result:
                    # 使用BitBlt复制窗口内容
                    result = saveDC.BitBlt((0, 0), (width, height), mfcDC, (0, 0), win32con.SRCCOPY)
                    if self.verbose:
                        print(f"GetDC BitBlt result: {result}")
                
                # 重新获取图像数据
                bmpstr = saveBitMap.GetBitmapBits(True)
                img = np.frombuffer(bmpstr, dtype='uint8')
                img.shape = (height, width, 4)
                
                # 再次检查图像是否全黑
                is_black = np.all(img == 0)
                if self.verbose:
                    print(f"Second attempt - Image is black: {is_black}")
                    print(f"Second attempt - Image min/max values: {img.min()}, {img.max()}")
            
            # 转换为PIL图像
            pil_img = Image.frombuffer('RGBA', (width, height), img, 'raw', 'BGRA', 0, 1)
            
            # 转换为RGB模式
            pil_img = pil_img.convert('RGB')
            
            if self.verbose:
                print(f"PIL Image size: {pil_img.size}")
                print(f"PIL Image mode: {pil_img.mode}")
            
            # 清理资源
            win32gui.DeleteObject(saveBitMap.GetHandle())
            saveDC.DeleteDC()
            mfcDC.DeleteDC()
            win32gui.ReleaseDC(self._hwnd, hwndDC)
            
            return pil_img
            
        except Exception as e:
            if self.verbose:
                print(f"Error capturing window: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def capture_to_file(self, filepath: str) -> bool:
        """Capture window screenshot and save to file"""
        img = self.capture()
        if img:
            img.save(filepath)
            if self.verbose:
                print(f"Image saved to {filepath}")
            # 验证保存的图像
            try:
                saved_img = Image.open(filepath)
                if self.verbose:
                    print(f"Saved image verified - Size: {saved_img.size}, Mode: {saved_img.mode}")
                is_black = np.all(np.array(saved_img) == 0)
                if self.verbose:
                    print(f"Saved image is black: {is_black}")
            except Exception as e:
                if self.verbose:
                    print(f"Error verifying saved image: {e}")
            return True
        return False 