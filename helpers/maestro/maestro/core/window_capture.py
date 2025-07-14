import numpy as np
from PIL import Image
from typing import Optional, Tuple, List, Dict
import time
import logging

# Import platform-specific implementations
from ..platform import window_capture as platform_capture
from ..platform.base import WindowInfo

# 设置日志
logging.basicConfig(level=logging.INFO, 
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("maestro.capture")

# WindowInfo is now imported from platform.base

class WindowCapture:
    """Window capture using platform abstraction layer"""
    
    def __init__(self, debug: bool = False):
        """Initialize window capture using platform-specific implementation"""
        self._window_manager = platform_capture.get_window_manager()
        self._screen_capture = platform_capture.get_screen_capture(self._window_manager)
        self.debug = debug
        if debug:
            logger.setLevel(logging.DEBUG)
        
    def find_all_windows(self) -> List[WindowInfo]:
        """Find all visible windows"""
        return self._window_manager.find_all_windows()
        
    def find_window(self, window_title: str) -> bool:
        """Find window by title and store its handle"""
        result = self._window_manager.find_window(window_title)
        if result:
            logger.debug(f"Found window: {window_title}")
        else:
            logger.debug(f"Window not found: {window_title}")
        return result
    
    def set_window_handle(self, window_id):
        """Set window handle directly"""
        self._window_manager.set_window_handle(window_id)
    
    def get_window_rect(self) -> Tuple[int, int, int, int]:
        """Get window rectangle coordinates"""
        return self._window_manager.get_window_rect()
        
    def get_client_rect(self) -> Tuple[int, int, int, int]:
        """Get client area rectangle coordinates"""
        return self._window_manager.get_client_rect()
    
    def activate_window(self) -> bool:
        """Activate (bring to front) the current window"""
        return self._window_manager.activate_window()
    
    def move_window(self, x: int, y: int, width: int, height: int) -> bool:
        """Move and resize window"""
        return self._window_manager.move_window(x, y, width, height)
    
    def capture(self) -> Optional[Image.Image]:
        """Capture window content as PIL Image"""
        # Ensure window is activated before capture
        self.activate_window()
        
        # Use platform-specific screen capture implementation
        return self._screen_capture.capture()
    
    def capture_region(self, x: int, y: int, width: int, height: int) -> Optional[Image.Image]:
        """Capture specific region of the screen"""
        return self._screen_capture.capture_region(x, y, width, height)
    
    def capture_to_file(self, filepath: str) -> bool:
        """Capture window and save to file"""
        return self._screen_capture.capture_to_file(filepath)