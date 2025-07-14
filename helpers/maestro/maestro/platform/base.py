"""
Base abstract classes for platform-specific implementations.

This module defines the interfaces that platform-specific implementations
must adhere to. These base classes define the common API that will be used
by the rest of the Maestro codebase.
"""

from abc import ABC, abstractmethod
from typing import Optional, Tuple, List, Dict, Any
from PIL import Image


class WindowInfo:
    """Window information container"""
    def __init__(self, id: Any, title: str, rect: Tuple[int, int, int, int], 
                 is_visible: bool, is_minimized: bool):
        self.id = id  # Platform-specific window identifier
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
        return f"Window(id={self.id}, title='{self.title}', " \
               f"size={self.width}x{self.height}, visible={self.is_visible})"


class WindowManagerBase(ABC):
    """Base class for window management operations."""
    
    @abstractmethod
    def find_all_windows(self) -> List[WindowInfo]:
        """Find all visible windows.
        
        Returns:
            List of WindowInfo objects representing all visible windows.
        """
        pass
    
    @abstractmethod
    def find_window(self, window_title: str) -> bool:
        """Find window by title and store its handle.
        
        Args:
            window_title: Title of the window to find (can be partial match).
            
        Returns:
            True if window was found, False otherwise.
        """
        pass
    
    @abstractmethod
    def set_window_handle(self, window_id: Any) -> None:
        """Set window handle/identifier directly.
        
        Args:
            window_id: Platform-specific window identifier.
        """
        pass
    
    @abstractmethod
    def get_window_rect(self) -> Tuple[int, int, int, int]:
        """Get window rectangle coordinates.
        
        Returns:
            Tuple of (left, top, right, bottom) coordinates.
        """
        pass
    
    @abstractmethod
    def get_client_rect(self) -> Tuple[int, int, int, int]:
        """Get client area rectangle coordinates.
        
        Returns:
            Tuple of (left, top, right, bottom) coordinates.
        """
        pass
    
    @abstractmethod
    def activate_window(self) -> bool:
        """Activate (bring to front) the current window.
        
        Returns:
            True if successful, False otherwise.
        """
        pass
    
    @abstractmethod
    def move_window(self, x: int, y: int, width: int, height: int) -> bool:
        """Move and resize window.
        
        Args:
            x: Left position.
            y: Top position.
            width: Window width.
            height: Window height.
            
        Returns:
            True if successful, False otherwise.
        """
        pass


class ScreenCaptureBase(ABC):
    """Base class for screen capture operations."""
    
    @abstractmethod
    def capture(self) -> Optional[Image.Image]:
        """Capture current window content as PIL Image.
        
        Returns:
            PIL Image of the window content or None if failed.
        """
        pass
    
    @abstractmethod
    def capture_region(self, x: int, y: int, width: int, height: int) -> Optional[Image.Image]:
        """Capture specific region of the screen.
        
        Args:
            x: Left position.
            y: Top position.
            width: Region width.
            height: Region height.
            
        Returns:
            PIL Image of the region or None if failed.
        """
        pass
    
    @abstractmethod
    def capture_to_file(self, filepath: str) -> bool:
        """Capture window and save to file.
        
        Args:
            filepath: Path to save the image.
            
        Returns:
            True if successful, False otherwise.
        """
        pass


class InputControllerBase(ABC):
    """Base class for input control operations."""
    
    @abstractmethod
    def get_cursor_position(self) -> Tuple[int, int]:
        """Get current cursor position.
        
        Returns:
            Tuple of (x, y) coordinates.
        """
        pass
    
    @abstractmethod
    def set_cursor_position(self, x: int, y: int) -> None:
        """Set cursor position.
        
        Args:
            x: X coordinate.
            y: Y coordinate.
        """
        pass
    
    @abstractmethod
    def mouse_move(self, x: int, y: int) -> None:
        """Move mouse to specified coordinates.
        
        Args:
            x: X coordinate.
            y: Y coordinate.
        """
        pass
    
    @abstractmethod
    def mouse_click(self, x: int, y: int, button: str = "left", double: bool = False) -> None:
        """Perform mouse click at specified coordinates.
        
        Args:
            x: X coordinate.
            y: Y coordinate.
            button: Mouse button ("left", "right", "middle").
            double: Whether to perform a double-click.
        """
        pass
    
    @abstractmethod
    def mouse_down(self, x: int, y: int, button: str = "left") -> None:
        """Press and hold mouse button at specified coordinates.
        
        Args:
            x: X coordinate.
            y: Y coordinate.
            button: Mouse button ("left", "right", "middle").
        """
        pass
    
    @abstractmethod
    def mouse_up(self, x: int, y: int, button: str = "left") -> None:
        """Release mouse button at specified coordinates.
        
        Args:
            x: X coordinate.
            y: Y coordinate.
            button: Mouse button ("left", "right", "middle").
        """
        pass
    
    @abstractmethod
    def mouse_drag(self, x1: int, y1: int, x2: int, y2: int, button: str = "left", 
                  duration: float = 0.1) -> None:
        """Drag mouse from one position to another.
        
        Args:
            x1: Starting X coordinate.
            y1: Starting Y coordinate.
            x2: Ending X coordinate.
            y2: Ending Y coordinate.
            button: Mouse button ("left", "right", "middle").
            duration: Duration of drag in seconds.
        """
        pass
    
    @abstractmethod
    def key_press(self, key: str) -> None:
        """Press a key.
        
        Args:
            key: Key to press (e.g., "a", "enter", "ctrl").
        """
        pass
    
    @abstractmethod
    def key_release(self, key: str) -> None:
        """Release a key.
        
        Args:
            key: Key to release (e.g., "a", "enter", "ctrl").
        """
        pass
    
    @abstractmethod
    def key_tap(self, key: str) -> None:
        """Tap (press and release) a key.
        
        Args:
            key: Key to tap (e.g., "a", "enter", "ctrl").
        """
        pass
    
    @abstractmethod
    def type_text(self, text: str, interval: float = 0.0) -> None:
        """Type text using keyboard.
        
        Args:
            text: Text to type.
            interval: Interval between keystrokes in seconds.
        """
        pass
    
    @abstractmethod
    def hotkey(self, *keys: str) -> None:
        """Press multiple keys simultaneously.
        
        Args:
            *keys: Keys to press (e.g., "ctrl", "c").
        """
        pass


class ClipboardManagerBase(ABC):
    """Base class for clipboard operations."""
    
    @abstractmethod
    def get_text(self) -> str:
        """Get text from clipboard.
        
        Returns:
            Text from clipboard.
        """
        pass
    
    @abstractmethod
    def set_text(self, text: str) -> None:
        """Set text to clipboard.
        
        Args:
            text: Text to set.
        """
        pass
    
    @abstractmethod
    def clear(self) -> None:
        """Clear clipboard."""
        pass
