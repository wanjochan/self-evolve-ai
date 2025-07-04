import win32gui
import win32ui
import win32con
import numpy as np
from PIL import Image
from typing import Optional, Tuple

class WindowCapture:
    def __init__(self):
        self._hwnd = None
        
    def find_window(self, window_title: str) -> bool:
        """Find window by title and store its handle"""
        self._hwnd = win32gui.FindWindow(None, window_title)
        return bool(self._hwnd)
    
    def set_window_handle(self, hwnd: int):
        """Set window handle directly"""
        self._hwnd = hwnd
    
    def get_window_rect(self) -> Tuple[int, int, int, int]:
        """Get window rectangle coordinates"""
        if not self._hwnd:
            raise ValueError("No window handle set")
        return win32gui.GetWindowRect(self._hwnd)
    
    def capture(self) -> Optional[Image.Image]:
        """Capture window content as PIL Image"""
        if not self._hwnd:
            return None
            
        # Get window dimensions
        left, top, right, bottom = self.get_window_rect()
        width = right - left
        height = bottom - top
        
        # Create device context
        hwndDC = win32gui.GetWindowDC(self._hwnd)
        mfcDC = win32ui.CreateDCFromHandle(hwndDC)
        saveDC = mfcDC.CreateCompatibleDC()
        
        # Create bitmap
        saveBitMap = win32ui.CreateBitmap()
        saveBitMap.CreateCompatibleBitmap(mfcDC, width, height)
        saveDC.SelectObject(saveBitMap)
        
        # Copy window content
        saveDC.BitBlt((0, 0), (width, height), mfcDC, (0, 0), win32con.SRCCOPY)
        
        # Convert to numpy array
        bmpinfo = saveBitMap.GetInfo()
        bmpstr = saveBitMap.GetBitmapBits(True)
        img = np.frombuffer(bmpstr, dtype='uint8')
        img.shape = (height, width, 4)
        
        # Cleanup
        win32gui.DeleteObject(saveBitMap.GetHandle())
        saveDC.DeleteDC()
        mfcDC.DeleteDC()
        win32gui.ReleaseDC(self._hwnd, hwndDC)
        
        # Convert to PIL Image
        return Image.fromarray(img[..., :3])  # Remove alpha channel
    
    def capture_to_file(self, filepath: str) -> bool:
        """Capture window content and save to file"""
        img = self.capture()
        if img:
            img.save(filepath)
            return True
        return False 