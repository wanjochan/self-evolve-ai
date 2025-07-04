import time
import logging
import ctypes
from ctypes import wintypes
import win32api
import win32con
import win32gui
from typing import Tuple, List, Optional, Union

logger = logging.getLogger("ui_ctrl_v2.input")

# Windows API constants
INPUT_MOUSE = 0
INPUT_KEYBOARD = 1
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
MOUSEEVENTF_RIGHTDOWN = 0x0008
MOUSEEVENTF_RIGHTUP = 0x0010
MOUSEEVENTF_MIDDLEDOWN = 0x0020
MOUSEEVENTF_MIDDLEUP = 0x0040
MOUSEEVENTF_WHEEL = 0x0800
MOUSEEVENTF_ABSOLUTE = 0x8000

# 键盘事件常量
KEYEVENTF_EXTENDEDKEY = 0x0001
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_UNICODE = 0x0004
KEYEVENTF_SCANCODE = 0x0008

# 虚拟键码映射
VK_CODES = {
    'backspace': 0x08,
    'tab': 0x09,
    'enter': 0x0D,
    'shift': 0x10,
    'ctrl': 0x11,
    'alt': 0x12,
    'pause': 0x13,
    'caps_lock': 0x14,
    'esc': 0x1B,
    'space': 0x20,
    'page_up': 0x21,
    'page_down': 0x22,
    'end': 0x23,
    'home': 0x24,
    'left': 0x25,
    'up': 0x26,
    'right': 0x27,
    'down': 0x28,
    'print_screen': 0x2C,
    'insert': 0x2D,
    'delete': 0x2E,
    'win': 0x5B,
    'f1': 0x70,
    'f2': 0x71,
    'f3': 0x72,
    'f4': 0x73,
    'f5': 0x74,
    'f6': 0x75,
    'f7': 0x76,
    'f8': 0x77,
    'f9': 0x78,
    'f10': 0x79,
    'f11': 0x7A,
    'f12': 0x7B,
}

# Windows API structures
class POINT(ctypes.Structure):
    _fields_ = [("x", ctypes.c_long), ("y", ctypes.c_long)]

class MOUSEINPUT(ctypes.Structure):
    _fields_ = [
        ("dx", wintypes.LONG),
        ("dy", wintypes.LONG),
        ("mouseData", wintypes.DWORD),
        ("dwFlags", wintypes.DWORD),
        ("time", wintypes.DWORD),
        ("dwExtraInfo", ctypes.POINTER(wintypes.ULONG))
    ]

class KEYBDINPUT(ctypes.Structure):
    _fields_ = [
        ("wVk", wintypes.WORD),
        ("wScan", wintypes.WORD),
        ("dwFlags", wintypes.DWORD),
        ("time", wintypes.DWORD),
        ("dwExtraInfo", ctypes.POINTER(wintypes.ULONG))
    ]

class HARDWAREINPUT(ctypes.Structure):
    _fields_ = [
        ("uMsg", wintypes.DWORD),
        ("wParamL", wintypes.WORD),
        ("wParamH", wintypes.WORD)
    ]

class INPUT_UNION(ctypes.Union):
    _fields_ = [
        ("mi", MOUSEINPUT),
        ("ki", KEYBDINPUT),
        ("hi", HARDWAREINPUT)
    ]

class INPUT(ctypes.Structure):
    _fields_ = [
        ("type", wintypes.DWORD),
        ("union", INPUT_UNION)
    ]

class InputController:
    """Input control for mouse and keyboard"""
    
    def __init__(self):
        """Initialize input controller"""
        self.user32 = ctypes.windll.user32
        
    def get_cursor_position(self) -> Tuple[int, int]:
        """Get current cursor position"""
        point = POINT()
        self.user32.GetCursorPos(ctypes.byref(point))
        return (point.x, point.y)
        
    def set_cursor_position(self, x: int, y: int):
        """Set cursor position"""
        # Convert to normalized coordinates (0-65535)
        screen_width = win32api.GetSystemMetrics(win32con.SM_CXSCREEN)
        screen_height = win32api.GetSystemMetrics(win32con.SM_CYSCREEN)
        
        nx = int(65535 * (x / screen_width))
        ny = int(65535 * (y / screen_height))
        
        # Prepare input structure
        extra = ctypes.c_ulong(0)
        ii = INPUT_UNION()
        ii.mi = MOUSEINPUT(nx, ny, 0, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, 0, ctypes.pointer(extra))
        command = INPUT(INPUT_MOUSE, ii)
        
        # Send input
        self.user32.SendInput(1, ctypes.byref(command), ctypes.sizeof(command))
        logger.debug(f"Set cursor position to ({x}, {y})")
        
    def mouse_click(self, button: str = "left", double: bool = False):
        """Perform mouse click"""
        # Map button to flags
        if button == "left":
            down_flag = MOUSEEVENTF_LEFTDOWN
            up_flag = MOUSEEVENTF_LEFTUP
        elif button == "right":
            down_flag = MOUSEEVENTF_RIGHTDOWN
            up_flag = MOUSEEVENTF_RIGHTUP
        elif button == "middle":
            down_flag = MOUSEEVENTF_MIDDLEDOWN
            up_flag = MOUSEEVENTF_MIDDLEUP
        else:
            logger.error(f"Unknown button: {button}")
            return
            
        # Prepare input structures
        extra = ctypes.c_ulong(0)
        ii_down = INPUT_UNION()
        ii_down.mi = MOUSEINPUT(0, 0, 0, down_flag, 0, ctypes.pointer(extra))
        command_down = INPUT(INPUT_MOUSE, ii_down)
        
        ii_up = INPUT_UNION()
        ii_up.mi = MOUSEINPUT(0, 0, 0, up_flag, 0, ctypes.pointer(extra))
        command_up = INPUT(INPUT_MOUSE, ii_up)
        
        # Send input
        self.user32.SendInput(1, ctypes.byref(command_down), ctypes.sizeof(command_down))
        time.sleep(0.05)
        self.user32.SendInput(1, ctypes.byref(command_up), ctypes.sizeof(command_up))
        
        if double:
            time.sleep(0.1)
            self.user32.SendInput(1, ctypes.byref(command_down), ctypes.sizeof(command_down))
            time.sleep(0.05)
            self.user32.SendInput(1, ctypes.byref(command_up), ctypes.sizeof(command_up))
            
        logger.debug(f"Mouse {button} {'double ' if double else ''}click")
        
    def click_at_position(self, x: int, y: int, button: str = "left", double: bool = False):
        """Move cursor to position and click"""
        self.set_cursor_position(x, y)
        time.sleep(0.1)  # Small delay to ensure position is set
        self.mouse_click(button, double)
        
    def click_element(self, element, button: str = "left", double: bool = False):
        """Click on a UI element"""
        # Get center of element
        x1, y1, x2, y2 = element.bbox
        center_x = (x1 + x2) // 2
        center_y = (y1 + y2) // 2
        
        self.click_at_position(center_x, center_y, button, double)
        logger.info(f"Clicked {button} on element at ({center_x}, {center_y})")
        
    def activate_window(self, hwnd: int) -> bool:
        """Bring window to foreground"""
        try:
            if win32gui.IsIconic(hwnd):  # If minimized
                win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
                
            # Try to bring to foreground
            result = win32gui.SetForegroundWindow(hwnd)
            time.sleep(0.1)  # Give time for window to activate
            return result != 0
        except Exception as e:
            logger.error(f"Failed to activate window: {e}")
            return False
            
    def _send_key_event(self, key_code: int, key_up: bool = False):
        """Send a key event"""
        flags = 0
        if key_up:
            flags |= KEYEVENTF_KEYUP
            
        extra = ctypes.c_ulong(0)
        ii = INPUT_UNION()
        ii.ki = KEYBDINPUT(key_code, 0, flags, 0, ctypes.pointer(extra))
        command = INPUT(INPUT_KEYBOARD, ii)
        
        self.user32.SendInput(1, ctypes.byref(command), ctypes.sizeof(command))
            
    def press_key(self, key: str):
        """Press and release a key"""
        # Convert key name to virtual key code
        key = key.lower()
        if key in VK_CODES:
            key_code = VK_CODES[key]
        elif len(key) == 1:  # Single character
            key_code = ord(key.upper())
        else:
            logger.error(f"Unknown key: {key}")
            return
            
        # Press key down
        self._send_key_event(key_code, False)
        time.sleep(0.05)
        # Release key
        self._send_key_event(key_code, True)
        logger.debug(f"Pressed key: {key}")
            
    def press_hotkey(self, keys: List[str]):
        """Press a combination of keys"""
        # Convert keys to virtual key codes
        key_codes = []
        for key in keys:
            key = key.lower()
            if key in VK_CODES:
                key_codes.append(VK_CODES[key])
            elif len(key) == 1:  # Single character
                key_codes.append(ord(key.upper()))
            else:
                logger.error(f"Unknown key in hotkey: {key}")
                return
                
        # Press all keys down
        for key_code in key_codes:
            self._send_key_event(key_code, False)
            time.sleep(0.05)
            
        # Release all keys in reverse order
        for key_code in reversed(key_codes):
            self._send_key_event(key_code, True)
            time.sleep(0.05)
            
        logger.debug(f"Pressed hotkey: {'+'.join(keys)}")
            
    def type_text(self, text: str):
        """Type a sequence of characters"""
        for char in text:
            # For special characters, we might need to use different approach
            # But for basic ASCII, this works
            key_code = ord(char.upper())
            
            # Press key down
            self._send_key_event(key_code, False)
            time.sleep(0.05)
            # Release key
            self._send_key_event(key_code, True)
            time.sleep(0.05)
            
        logger.debug(f"Typed text: {text}") 