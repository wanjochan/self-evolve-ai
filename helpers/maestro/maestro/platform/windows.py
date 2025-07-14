"""
Windows-specific implementations of platform interfaces.

This module adapts the existing Windows-specific code to the platform
abstraction interfaces, using win32gui, win32api, and other Windows APIs.
"""

import win32gui
import win32ui
import win32con
import win32api
import numpy as np
import ctypes
import time
import logging
from typing import Optional, Tuple, List, Dict, Any
from PIL import Image

from .base import WindowManagerBase, ScreenCaptureBase, InputControllerBase, ClipboardManagerBase, WindowInfo

# Load user32.dll for direct API calls
user32 = ctypes.windll.user32

logger = logging.getLogger("maestro.platform.windows")


class WindowManagerWindows(WindowManagerBase):
    """Windows implementation of window management operations."""
    
    def __init__(self):
        self._hwnd = None
    
    def find_all_windows(self) -> List[WindowInfo]:
        """Find all visible windows."""
        def callback(hwnd, windows):
            if win32gui.IsWindowVisible(hwnd):
                title = win32gui.GetWindowText(hwnd)
                if title:  # Ignore windows without title
                    rect = win32gui.GetWindowRect(hwnd)
                    style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
                    is_minimized = bool(style & win32con.WS_MINIMIZE)
                    windows.append(WindowInfo(
                        id=hwnd,
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
            self._hwnd = found_windows[0]  # Use the first matching window
            logger.debug(f"Found window handle: {self._hwnd}")
            return True
        return False
    
    def set_window_handle(self, window_id: Any) -> None:
        """Set window handle directly."""
        self._hwnd = window_id
    
    def get_window_rect(self) -> Tuple[int, int, int, int]:
        """Get window rectangle coordinates."""
        if not self._hwnd:
            raise ValueError("No window handle set")
        return win32gui.GetWindowRect(self._hwnd)
    
    def get_client_rect(self) -> Tuple[int, int, int, int]:
        """Get client area rectangle coordinates."""
        if not self._hwnd:
            raise ValueError("No window handle set")
        return win32gui.GetClientRect(self._hwnd)
    
    def activate_window(self) -> bool:
        """Activate (bring to front) the current window."""
        if not self._hwnd:
            return False
            
        # Check if window is minimized
        style = win32gui.GetWindowLong(self._hwnd, win32con.GWL_STYLE)
        if style & win32con.WS_MINIMIZE:
            logger.debug("Window is minimized, attempting to restore...")
            # Send restore command
            win32gui.ShowWindow(self._hwnd, win32con.SW_RESTORE)
            # Wait for window to restore
            time.sleep(0.5)
            
        # Ensure window is in foreground
        win32gui.SetForegroundWindow(self._hwnd)
        time.sleep(0.1)
        return True
    
    def move_window(self, x: int, y: int, width: int, height: int) -> bool:
        """Move and resize window."""
        if not self._hwnd:
            return False
        try:
            win32gui.MoveWindow(self._hwnd, x, y, width, height, True)
            return True
        except Exception as e:
            logger.error(f"Error moving window: {e}")
            return False


class ScreenCaptureWindows(ScreenCaptureBase):
    """Windows implementation of screen capture operations."""
    
    def __init__(self, window_manager: Optional[WindowManagerWindows] = None):
        self.window_manager = window_manager or WindowManagerWindows()
    
    def _try_capture_methods(self, dc_obj, mem_dc, width, height) -> Optional[np.ndarray]:
        """Try different capture methods."""
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
        """Capture using PrintWindow."""
        hwnd = self.window_manager._hwnd
        result = user32.PrintWindow(hwnd, mem_dc.GetSafeHdc(), flags)
        if result:
            return self._get_bitmap_data(mem_dc, width, height)
        return None
    
    def _capture_with_client_dc(self, mem_dc, width, height) -> Optional[np.ndarray]:
        """Capture using client DC."""
        hwnd = self.window_manager._hwnd
        client_dc = win32gui.GetDC(hwnd)
        try:
            client_dc_obj = win32ui.CreateDCFromHandle(client_dc)
            mem_dc.BitBlt((0, 0), (width, height), client_dc_obj, (0, 0), win32con.SRCCOPY)
            client_dc_obj.DeleteDC()
            return self._get_bitmap_data(mem_dc, width, height)
        finally:
            win32gui.ReleaseDC(hwnd, client_dc)
    
    def _capture_with_window_dc(self, dc_obj, mem_dc, width, height) -> Optional[np.ndarray]:
        """Capture using window DC."""
        mem_dc.BitBlt((0, 0), (width, height), dc_obj, (0, 0), win32con.SRCCOPY)
        return self._get_bitmap_data(mem_dc, width, height)
    
    def _get_bitmap_data(self, mem_dc, width, height) -> Optional[np.ndarray]:
        """Get bitmap data from memory DC."""
        bitmap = mem_dc.GetCurrentBitmap()
        bmpstr = bitmap.GetBitmapBits(True)
        img = np.frombuffer(bmpstr, dtype='uint8')
        img.shape = (height, width, 4)
        return img
    
    def capture(self) -> Optional[Image.Image]:
        """Capture current window content as PIL Image."""
        hwnd = self.window_manager._hwnd
        if not hwnd:
            logger.error("No window handle set")
            return None
            
        try:
            # Ensure window is in visible area
            self.window_manager.activate_window()
            
            # Get window dimensions
            left, top, right, bottom = self.window_manager.get_window_rect()
            width = right - left
            height = bottom - top
            
            if width <= 0 or height <= 0:
                logger.error(f"Invalid window dimensions: {width}x{height}")
                return None
                
            logger.debug(f"Window dimensions: {width}x{height}")
            
            # Create device context
            hwndDC = win32gui.GetWindowDC(hwnd)
            mfcDC = win32ui.CreateDCFromHandle(hwndDC)
            saveDC = mfcDC.CreateCompatibleDC()
            
            # Create bitmap
            saveBitMap = win32ui.CreateBitmap()
            saveBitMap.CreateCompatibleBitmap(mfcDC, width, height)
            saveDC.SelectObject(saveBitMap)
            
            # Try different capture methods
            img_array = self._try_capture_methods(mfcDC, saveDC, width, height)
            
            # Clean up resources
            win32gui.DeleteObject(saveBitMap.GetHandle())
            saveDC.DeleteDC()
            mfcDC.DeleteDC()
            win32gui.ReleaseDC(hwnd, hwndDC)
            
            if img_array is None:
                logger.error("All capture methods failed")
                return None
                
            # Convert to PIL image
            img = Image.fromarray(img_array)
            return img
            
        except Exception as e:
            logger.error(f"Error capturing window: {e}")
            return None
    
    def capture_region(self, x: int, y: int, width: int, height: int) -> Optional[Image.Image]:
        """Capture specific region of the screen."""
        try:
            # Create device context for the entire screen
            hdc = win32gui.GetDC(0)
            mfcDC = win32ui.CreateDCFromHandle(hdc)
            saveDC = mfcDC.CreateCompatibleDC()
            
            # Create bitmap
            saveBitMap = win32ui.CreateBitmap()
            saveBitMap.CreateCompatibleBitmap(mfcDC, width, height)
            saveDC.SelectObject(saveBitMap)
            
            # Copy screen region to bitmap
            saveDC.BitBlt((0, 0), (width, height), mfcDC, (x, y), win32con.SRCCOPY)
            
            # Get bitmap data
            bitmap = saveDC.GetCurrentBitmap()
            bmpstr = bitmap.GetBitmapBits(True)
            img_array = np.frombuffer(bmpstr, dtype='uint8')
            img_array.shape = (height, width, 4)
            
            # Clean up resources
            win32gui.DeleteObject(saveBitMap.GetHandle())
            saveDC.DeleteDC()
            mfcDC.DeleteDC()
            win32gui.ReleaseDC(0, hdc)
            
            # Convert to PIL image
            img = Image.fromarray(img_array)
            return img
            
        except Exception as e:
            logger.error(f"Error capturing region: {e}")
            return None
    
    def capture_to_file(self, filepath: str) -> bool:
        """Capture window and save to file."""
        img = self.capture()
        if img:
            img.save(filepath)
            logger.info(f"Saved screenshot to {filepath}")
            return True
        return False


class InputControllerWindows(InputControllerBase):
    """Windows implementation of input control operations."""
    
    # Mouse button mapping
    MOUSE_BUTTONS = {
        "left": (win32con.MOUSEEVENTF_LEFTDOWN, win32con.MOUSEEVENTF_LEFTUP),
        "right": (win32con.MOUSEEVENTF_RIGHTDOWN, win32con.MOUSEEVENTF_RIGHTUP),
        "middle": (win32con.MOUSEEVENTF_MIDDLEDOWN, win32con.MOUSEEVENTF_MIDDLEUP)
    }
    
    # Virtual key codes for common keys
    KEY_CODES = {
        "backspace": 0x08,
        "tab": 0x09,
        "enter": 0x0D,
        "shift": 0x10,
        "ctrl": 0x11,
        "alt": 0x12,
        "pause": 0x13,
        "caps_lock": 0x14,
        "escape": 0x1B,
        "space": 0x20,
        "page_up": 0x21,
        "page_down": 0x22,
        "end": 0x23,
        "home": 0x24,
        "left": 0x25,
        "up": 0x26,
        "right": 0x27,
        "down": 0x28,
        "print_screen": 0x2C,
        "insert": 0x2D,
        "delete": 0x2E,
        # Numbers
        "0": 0x30, "1": 0x31, "2": 0x32, "3": 0x33, "4": 0x34,
        "5": 0x35, "6": 0x36, "7": 0x37, "8": 0x38, "9": 0x39,
        # Letters
        "a": 0x41, "b": 0x42, "c": 0x43, "d": 0x44, "e": 0x45,
        "f": 0x46, "g": 0x47, "h": 0x48, "i": 0x49, "j": 0x4A,
        "k": 0x4B, "l": 0x4C, "m": 0x4D, "n": 0x4E, "o": 0x4F,
        "p": 0x50, "q": 0x51, "r": 0x52, "s": 0x53, "t": 0x54,
        "u": 0x55, "v": 0x56, "w": 0x57, "x": 0x58, "y": 0x59, "z": 0x5A,
        # Function keys
        "f1": 0x70, "f2": 0x71, "f3": 0x72, "f4": 0x73, "f5": 0x74,
        "f6": 0x75, "f7": 0x76, "f8": 0x77, "f9": 0x78, "f10": 0x79,
        "f11": 0x7A, "f12": 0x7B
    }
    
    def __init__(self, window_manager: Optional[WindowManagerWindows] = None):
        self.window_manager = window_manager
    
    def get_cursor_position(self) -> Tuple[int, int]:
        """Get current cursor position."""
        return win32gui.GetCursorPos()
    
    def set_cursor_position(self, x: int, y: int) -> None:
        """Set cursor position."""
        win32api.SetCursorPos((x, y))
    
    def mouse_move(self, x: int, y: int) -> None:
        """Move mouse to specified coordinates."""
        self.set_cursor_position(x, y)
    
    def mouse_click(self, x: int, y: int, button: str = "left", double: bool = False) -> None:
        """Perform mouse click at specified coordinates."""
        # Ensure button is valid
        button = button.lower()
        if button not in self.MOUSE_BUTTONS:
            raise ValueError(f"Invalid mouse button: {button}")
        
        # Move cursor to position
        self.mouse_move(x, y)
        
        # Get button events
        down_event, up_event = self.MOUSE_BUTTONS[button]
        
        # Perform click
        win32api.mouse_event(down_event, 0, 0, 0, 0)
        win32api.mouse_event(up_event, 0, 0, 0, 0)
        
        # Double click if requested
        if double:
            time.sleep(0.1)
            win32api.mouse_event(down_event, 0, 0, 0, 0)
            win32api.mouse_event(up_event, 0, 0, 0, 0)
    
    def mouse_down(self, x: int, y: int, button: str = "left") -> None:
        """Press and hold mouse button at specified coordinates."""
        # Ensure button is valid
        button = button.lower()
        if button not in self.MOUSE_BUTTONS:
            raise ValueError(f"Invalid mouse button: {button}")
        
        # Move cursor to position
        self.mouse_move(x, y)
        
        # Get button down event
        down_event, _ = self.MOUSE_BUTTONS[button]
        
        # Perform mouse down
        win32api.mouse_event(down_event, 0, 0, 0, 0)
    
    def mouse_up(self, x: int, y: int, button: str = "left") -> None:
        """Release mouse button at specified coordinates."""
        # Ensure button is valid
        button = button.lower()
        if button not in self.MOUSE_BUTTONS:
            raise ValueError(f"Invalid mouse button: {button}")
        
        # Move cursor to position
        self.mouse_move(x, y)
        
        # Get button up event
        _, up_event = self.MOUSE_BUTTONS[button]
        
        # Perform mouse up
        win32api.mouse_event(up_event, 0, 0, 0, 0)
    
    def mouse_drag(self, x1: int, y1: int, x2: int, y2: int, button: str = "left", 
                  duration: float = 0.1) -> None:
        """Drag mouse from one position to another."""
        # Press mouse button at start position
        self.mouse_down(x1, y1, button)
        
        # Calculate intermediate points for smooth movement
        steps = max(int(duration * 10), 2)  # At least 2 steps
        sleep_time = duration / steps
        
        for i in range(1, steps + 1):
            # Calculate intermediate position
            x = x1 + (x2 - x1) * i // steps
            y = y1 + (y2 - y1) * i // steps
            
            # Move to intermediate position
            self.mouse_move(x, y)
            time.sleep(sleep_time)
        
        # Release mouse button at end position
        self.mouse_up(x2, y2, button)
    
    def _get_key_code(self, key: str) -> int:
        """Get virtual key code for a key."""
        key = key.lower()
        if key in self.KEY_CODES:
            return self.KEY_CODES[key]
        elif len(key) == 1:
            # Try to convert single character to virtual key code
            return ord(key.upper())
        else:
            raise ValueError(f"Unknown key: {key}")
    
    def key_press(self, key: str) -> None:
        """Press a key."""
        key_code = self._get_key_code(key)
        win32api.keybd_event(key_code, 0, 0, 0)
    
    def key_release(self, key: str) -> None:
        """Release a key."""
        key_code = self._get_key_code(key)
        win32api.keybd_event(key_code, 0, win32con.KEYEVENTF_KEYUP, 0)
    
    def key_tap(self, key: str) -> None:
        """Tap (press and release) a key."""
        self.key_press(key)
        time.sleep(0.05)
        self.key_release(key)
    
    def type_text(self, text: str, interval: float = 0.0) -> None:
        """Type text using keyboard."""
        for char in text:
            # Handle special characters
            if char == '\n':
                self.key_tap("enter")
            elif char == '\t':
                self.key_tap("tab")
            elif char == ' ':
                self.key_tap("space")
            else:
                # For regular characters, use VkKeyScan to get virtual key code
                vk = win32api.VkKeyScan(char)
                if vk == -1:
                    # Character not in keyboard layout
                    continue
                    
                # Extract virtual key code and shift state
                vk_code = vk & 0xFF
                shift_state = (vk >> 8) & 0xFF
                
                # Press shift if needed
                if shift_state & 1:
                    self.key_press("shift")
                    
                # Press and release the key
                win32api.keybd_event(vk_code, 0, 0, 0)
                win32api.keybd_event(vk_code, 0, win32con.KEYEVENTF_KEYUP, 0)
                
                # Release shift if needed
                if shift_state & 1:
                    self.key_release("shift")
            
            # Wait between keystrokes if interval is specified
            if interval > 0:
                time.sleep(interval)
    
    def hotkey(self, *keys: str) -> None:
        """Press multiple keys simultaneously."""
        # Press all keys in sequence
        for key in keys:
            self.key_press(key)
        
        # Small delay to ensure keys are registered
        time.sleep(0.05)
        
        # Release all keys in reverse order
        for key in reversed(keys):
            self.key_release(key)


class ClipboardManagerWindows(ClipboardManagerBase):
    """Windows implementation of clipboard operations."""
    
    def __init__(self):
        # Import pyperclip here to avoid dependency issues
        try:
            import pyperclip
            self.pyperclip = pyperclip
        except ImportError:
            logger.warning("pyperclip not installed, clipboard functionality will be limited")
            self.pyperclip = None
    
    def get_text(self) -> str:
        """Get text from clipboard."""
        if self.pyperclip:
            return self.pyperclip.paste()
        else:
            # Fallback implementation using ctypes
            try:
                win32clipboard.OpenClipboard()
                data = win32clipboard.GetClipboardData(win32clipboard.CF_UNICODETEXT)
                win32clipboard.CloseClipboard()
                return data
            except Exception as e:
                logger.error(f"Error getting clipboard text: {e}")
                return ""
    
    def set_text(self, text: str) -> None:
        """Set text to clipboard."""
        if self.pyperclip:
            self.pyperclip.copy(text)
        else:
            # Fallback implementation using ctypes
            try:
                win32clipboard.OpenClipboard()
                win32clipboard.EmptyClipboard()
                win32clipboard.SetClipboardText(text, win32clipboard.CF_UNICODETEXT)
                win32clipboard.CloseClipboard()
            except Exception as e:
                logger.error(f"Error setting clipboard text: {e}")
    
    def clear(self) -> None:
        """Clear clipboard."""
        try:
            import win32clipboard
            win32clipboard.OpenClipboard()
            win32clipboard.EmptyClipboard()
            win32clipboard.CloseClipboard()
        except Exception as e:
            logger.error(f"Error clearing clipboard: {e}")
