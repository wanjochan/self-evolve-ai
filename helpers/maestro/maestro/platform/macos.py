"""
macOS-specific implementations of platform interfaces.

This module implements the platform abstraction interfaces for macOS
using pyobjc, Quartz, and AppKit APIs.
"""

import time
import logging
import Quartz
import AppKit
import Foundation
import objc
import numpy as np
from typing import Optional, Tuple, List, Dict, Any
from PIL import Image, ImageOps
import io

from .base import WindowManagerBase, ScreenCaptureBase, InputControllerBase, ClipboardManagerBase, WindowInfo

logger = logging.getLogger("maestro.platform.macos")

# Constants for mouse buttons
LEFT_BUTTON = 0
RIGHT_BUTTON = 1
MIDDLE_BUTTON = 2


class WindowManagerMacOS(WindowManagerBase):
    """macOS implementation of window management operations."""
    
    def __init__(self):
        self._window_id = None
        self._window_name = None
        self._window_info = None
        self._workspace = AppKit.NSWorkspace.sharedWorkspace()
    
    def find_all_windows(self) -> List[WindowInfo]:
        """Find all visible windows."""
        windows = []
        
        # Get all windows
        window_info_list = Quartz.CGWindowListCopyWindowInfo(
            Quartz.kCGWindowListOptionOnScreenOnly | Quartz.kCGWindowListExcludeDesktopElements,
            Quartz.kCGNullWindowID
        )
        
        for window_info in window_info_list:
            window_id = window_info.get(Quartz.kCGWindowNumber)
            window_name = window_info.get(Quartz.kCGWindowName, "")
            owner_name = window_info.get(Quartz.kCGWindowOwnerName, "")
            
            # Skip windows without names
            if not window_name and not owner_name:
                continue
                
            # Get window bounds
            bounds = window_info.get(Quartz.kCGWindowBounds, {})
            x = bounds.get('X', 0)
            y = bounds.get('Y', 0)
            width = bounds.get('Width', 0)
            height = bounds.get('Height', 0)
            
            # Check if window is on screen and has dimensions
            if width > 0 and height > 0:
                title = window_name or owner_name
                windows.append(WindowInfo(
                    id=window_id,
                    title=title,
                    rect=(int(x), int(y), int(x + width), int(y + height)),
                    is_visible=True,
                    is_minimized=False  # Hard to determine without additional API calls
                ))
        
        return windows
    
    def find_window(self, window_title: str) -> bool:
        """Find window by title and store its handle."""
        windows = self.find_all_windows()
        
        for window in windows:
            if window_title.lower() in window.title.lower():
                self._window_id = window.id
                self._window_name = window.title
                self._window_info = window
                logger.debug(f"Found window: {window.title} (ID: {window.id})")
                return True
        
        logger.debug(f"Window not found: {window_title}")
        return False
    
    def set_window_handle(self, window_id: Any) -> None:
        """Set window handle directly."""
        self._window_id = window_id
        
        # Update window info
        windows = self.find_all_windows()
        for window in windows:
            if window.id == window_id:
                self._window_name = window.title
                self._window_info = window
                break
    
    def get_window_rect(self) -> Tuple[int, int, int, int]:
        """Get window rectangle coordinates."""
        if not self._window_id or not self._window_info:
            raise ValueError("No window handle set")
        
        return self._window_info.rect
    
    def get_client_rect(self) -> Tuple[int, int, int, int]:
        """Get client area rectangle coordinates.
        
        In macOS, the client rect is typically the same as the window rect,
        minus the title bar height (approximately 22 pixels).
        """
        if not self._window_id or not self._window_info:
            raise ValueError("No window handle set")
        
        x, y, right, bottom = self._window_info.rect
        # Adjust for title bar (approximately 22 pixels)
        title_bar_height = 22
        return (0, 0, right - x, bottom - y - title_bar_height)
    
    def activate_window(self) -> bool:
        """Activate (bring to front) the current window."""
        if not self._window_id:
            return False
        
        try:
            # Find the application that owns this window
            window_info_list = Quartz.CGWindowListCopyWindowInfo(
                Quartz.kCGWindowListOptionIncludingWindow,
                self._window_id
            )
            
            if not window_info_list:
                logger.error("Window info not found")
                return False
            
            window_info = window_info_list[0]
            pid = window_info.get(Quartz.kCGWindowOwnerPID)
            
            if not pid:
                logger.error("Window owner PID not found")
                return False
            
            # Get the application with this PID
            running_apps = self._workspace.runningApplications()
            target_app = None
            
            for app in running_apps:
                if app.processIdentifier() == pid:
                    target_app = app
                    break
            
            if not target_app:
                logger.error(f"Application with PID {pid} not found")
                return False
            
            # Activate the application
            result = target_app.activateWithOptions_(AppKit.NSApplicationActivateIgnoringOtherApps)
            time.sleep(0.1)  # Give time for activation to complete
            
            return result
            
        except Exception as e:
            logger.error(f"Error activating window: {e}")
            return False
    
    def move_window(self, x: int, y: int, width: int, height: int) -> bool:
        """Move and resize window.
        
        Note: In macOS, moving windows programmatically is more restricted
        than in Windows. This implementation attempts to use AppleScript
        to move the window.
        """
        if not self._window_id or not self._window_name:
            return False
        
        try:
            # Create AppleScript to move and resize the window
            script = f'''
            tell application "System Events"
                set frontApp to first application process whose frontmost is true
                tell window 1 of frontApp
                    set position to {{{x}, {y}}}
                    set size to {{{width}, {height}}}
                end tell
            end tell
            '''
            
            # Execute AppleScript
            ns_script = Foundation.NSAppleScript.alloc().initWithSource_(script)
            result, error = ns_script.executeAndReturnError_(None)
            
            if error:
                logger.error(f"AppleScript error: {error}")
                return False
                
            time.sleep(0.1)  # Give time for the window to move
            return True
            
        except Exception as e:
            logger.error(f"Error moving window: {e}")
            return False


class ScreenCaptureMacOS(ScreenCaptureBase):
    """macOS implementation of screen capture operations."""
    
    def __init__(self, window_manager: Optional[WindowManagerMacOS] = None):
        self.window_manager = window_manager or WindowManagerMacOS()
    
    def capture(self) -> Optional[Image.Image]:
        """Capture current window content as PIL Image."""
        if not self.window_manager._window_id:
            logger.error("No window handle set")
            return None
        
        try:
            # Get window bounds
            x, y, right, bottom = self.window_manager.get_window_rect()
            width = right - x
            height = bottom - y
            
            # Create CGRect for the window
            rect = Quartz.CGRectMake(x, y, width, height)
            
            # Capture window image
            image = Quartz.CGWindowListCreateImage(
                rect,
                Quartz.kCGWindowListOptionIncludingWindow,
                self.window_manager._window_id,
                Quartz.kCGWindowImageBoundsIgnoreFraming
            )
            
            if not image:
                logger.error("Failed to capture window image")
                return None
            
            # Convert to PIL Image
            width = Quartz.CGImageGetWidth(image)
            height = Quartz.CGImageGetHeight(image)
            
            # Create a bitmap context for the image
            context = Quartz.CGBitmapContextCreate(
                None,
                width,
                height,
                8,  # bits per component
                0,  # auto-calculate bytes per row
                Quartz.CGImageGetColorSpace(image),
                Quartz.kCGImageAlphaPremultipliedLast
            )
            
            # Draw the image to the context
            rect = Quartz.CGRectMake(0, 0, width, height)
            Quartz.CGContextDrawImage(context, rect, image)
            
            # Get image data
            data = Quartz.CGBitmapContextCreateImage(context)
            provider = Quartz.CGImageGetDataProvider(data)
            buffer = Quartz.CGDataProviderCopyData(provider)
            
            # Convert to numpy array
            img_data = np.frombuffer(buffer, dtype=np.uint8)
            img_data = img_data.reshape(height, width, 4)  # RGBA format
            
            # Convert to PIL Image
            img = Image.fromarray(img_data)
            return img
            
        except Exception as e:
            logger.error(f"Error capturing window: {e}")
            return None
    
    def capture_region(self, x: int, y: int, width: int, height: int) -> Optional[Image.Image]:
        """Capture specific region of the screen."""
        try:
            # Create CGRect for the region
            rect = Quartz.CGRectMake(x, y, width, height)
            
            # Capture screen region
            image = Quartz.CGWindowListCreateImage(
                rect,
                Quartz.kCGWindowListOptionOnScreenOnly,
                Quartz.kCGNullWindowID,
                Quartz.kCGWindowImageDefault
            )
            
            if not image:
                logger.error("Failed to capture screen region")
                return None
            
            # Convert to PIL Image
            width = Quartz.CGImageGetWidth(image)
            height = Quartz.CGImageGetHeight(image)
            
            # Create a bitmap context for the image
            context = Quartz.CGBitmapContextCreate(
                None,
                width,
                height,
                8,  # bits per component
                0,  # auto-calculate bytes per row
                Quartz.CGImageGetColorSpace(image),
                Quartz.kCGImageAlphaPremultipliedLast
            )
            
            # Draw the image to the context
            rect = Quartz.CGRectMake(0, 0, width, height)
            Quartz.CGContextDrawImage(context, rect, image)
            
            # Get image data
            data = Quartz.CGBitmapContextCreateImage(context)
            provider = Quartz.CGImageGetDataProvider(data)
            buffer = Quartz.CGDataProviderCopyData(provider)
            
            # Convert to numpy array
            img_data = np.frombuffer(buffer, dtype=np.uint8)
            img_data = img_data.reshape(height, width, 4)  # RGBA format
            
            # Convert to PIL Image
            img = Image.fromarray(img_data)
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


class InputControllerMacOS(InputControllerBase):
    """macOS implementation of input control operations."""
    
    # Map of key names to macOS virtual key codes
    KEY_CODES = {
        "backspace": 0x33,
        "tab": 0x30,
        "enter": 0x24,
        "return": 0x24,
        "shift": 0x38,
        "ctrl": 0x3B,
        "alt": 0x3A,
        "option": 0x3A,
        "command": 0x37,
        "capslock": 0x39,
        "escape": 0x35,
        "space": 0x31,
        "page_up": 0x74,
        "page_down": 0x79,
        "end": 0x77,
        "home": 0x73,
        "left": 0x7B,
        "up": 0x7E,
        "right": 0x7C,
        "down": 0x7D,
        "delete": 0x75,
        # Numbers
        "0": 0x1D, "1": 0x12, "2": 0x13, "3": 0x14, "4": 0x15,
        "5": 0x17, "6": 0x16, "7": 0x1A, "8": 0x1C, "9": 0x19,
        # Letters
        "a": 0x00, "b": 0x0B, "c": 0x08, "d": 0x02, "e": 0x0E,
        "f": 0x03, "g": 0x05, "h": 0x04, "i": 0x22, "j": 0x26,
        "k": 0x28, "l": 0x25, "m": 0x2E, "n": 0x2D, "o": 0x1F,
        "p": 0x23, "q": 0x0C, "r": 0x0F, "s": 0x01, "t": 0x11,
        "u": 0x20, "v": 0x09, "w": 0x0D, "x": 0x07, "y": 0x10, "z": 0x06,
        # Function keys
        "f1": 0x7A, "f2": 0x78, "f3": 0x63, "f4": 0x76,
        "f5": 0x60, "f6": 0x61, "f7": 0x62, "f8": 0x64,
        "f9": 0x65, "f10": 0x6D, "f11": 0x67, "f12": 0x6F,
    }
    
    def __init__(self, window_manager: Optional[WindowManagerMacOS] = None):
        self.window_manager = window_manager
    
    def get_cursor_position(self) -> Tuple[int, int]:
        """Get current cursor position."""
        pos = AppKit.NSEvent.mouseLocation()
        # Convert to screen coordinates (origin at top-left)
        screen = AppKit.NSScreen.mainScreen()
        screen_height = screen.frame().size.height
        return (int(pos.x), int(screen_height - pos.y))
    
    def set_cursor_position(self, x: int, y: int) -> None:
        """Set cursor position."""
        # Create mouse move event
        event = Quartz.CGEventCreateMouseEvent(
            None,
            Quartz.kCGEventMouseMoved,
            (x, y),
            0  # left button
        )
        # Post the event
        Quartz.CGEventPost(Quartz.kCGHIDEventTap, event)
    
    def mouse_move(self, x: int, y: int) -> None:
        """Move mouse to specified coordinates."""
        self.set_cursor_position(x, y)
    
    def _get_mouse_button_type(self, button: str) -> int:
        """Get mouse button type from string."""
        button = button.lower()
        if button == "left":
            return LEFT_BUTTON
        elif button == "right":
            return RIGHT_BUTTON
        elif button == "middle":
            return MIDDLE_BUTTON
        else:
            raise ValueError(f"Invalid mouse button: {button}")
    
    def mouse_click(self, x: int, y: int, button: str = "left", double: bool = False) -> None:
        """Perform mouse click at specified coordinates."""
        button_type = self._get_mouse_button_type(button)
        
        # Move cursor to position
        self.mouse_move(x, y)
        
        # Create mouse down event
        down_event = Quartz.CGEventCreateMouseEvent(
            None,
            Quartz.kCGEventLeftMouseDown if button_type == LEFT_BUTTON else 
            Quartz.kCGEventRightMouseDown if button_type == RIGHT_BUTTON else
            Quartz.kCGEventOtherMouseDown,
            (x, y),
            button_type
        )
        
        # Create mouse up event
        up_event = Quartz.CGEventCreateMouseEvent(
            None,
            Quartz.kCGEventLeftMouseUp if button_type == LEFT_BUTTON else 
            Quartz.kCGEventRightMouseUp if button_type == RIGHT_BUTTON else
            Quartz.kCGEventOtherMouseUp,
            (x, y),
            button_type
        )
        
        # Post the events
        Quartz.CGEventPost(Quartz.kCGHIDEventTap, down_event)
        time.sleep(0.01)
        Quartz.CGEventPost(Quartz.kCGHIDEventTap, up_event)
        
        # Double click if requested
        if double:
            time.sleep(0.1)
            Quartz.CGEventPost(Quartz.kCGHIDEventTap, down_event)
            time.sleep(0.01)
            Quartz.CGEventPost(Quartz.kCGHIDEventTap, up_event)
    
    def mouse_down(self, x: int, y: int, button: str = "left") -> None:
        """Press and hold mouse button at specified coordinates."""
        button_type = self._get_mouse_button_type(button)
        
        # Move cursor to position
        self.mouse_move(x, y)
        
        # Create mouse down event
        down_event = Quartz.CGEventCreateMouseEvent(
            None,
            Quartz.kCGEventLeftMouseDown if button_type == LEFT_BUTTON else 
            Quartz.kCGEventRightMouseDown if button_type == RIGHT_BUTTON else
            Quartz.kCGEventOtherMouseDown,
            (x, y),
            button_type
        )
        
        # Post the event
        Quartz.CGEventPost(Quartz.kCGHIDEventTap, down_event)
    
    def mouse_up(self, x: int, y: int, button: str = "left") -> None:
        """Release mouse button at specified coordinates."""
        button_type = self._get_mouse_button_type(button)
        
        # Move cursor to position
        self.mouse_move(x, y)
        
        # Create mouse up event
        up_event = Quartz.CGEventCreateMouseEvent(
            None,
            Quartz.kCGEventLeftMouseUp if button_type == LEFT_BUTTON else 
            Quartz.kCGEventRightMouseUp if button_type == RIGHT_BUTTON else
            Quartz.kCGEventOtherMouseUp,
            (x, y),
            button_type
        )
        
        # Post the event
        Quartz.CGEventPost(Quartz.kCGHIDEventTap, up_event)
    
    def mouse_drag(self, x1: int, y1: int, x2: int, y2: int, button: str = "left", 
                  duration: float = 0.1) -> None:
        """Drag mouse from one position to another."""
        button_type = self._get_mouse_button_type(button)
        
        # Press mouse button at start position
        self.mouse_down(x1, y1, button)
        
        # Calculate intermediate points for smooth movement
        steps = max(int(duration * 10), 2)  # At least 2 steps
        sleep_time = duration / steps
        
        for i in range(1, steps + 1):
            # Calculate intermediate position
            x = x1 + (x2 - x1) * i // steps
            y = y1 + (y2 - y1) * i // steps
            
            # Create mouse drag event
            drag_event = Quartz.CGEventCreateMouseEvent(
                None,
                Quartz.kCGEventLeftMouseDragged if button_type == LEFT_BUTTON else 
                Quartz.kCGEventRightMouseDragged if button_type == RIGHT_BUTTON else
                Quartz.kCGEventOtherMouseDragged,
                (x, y),
                button_type
            )
            
            # Post the event
            Quartz.CGEventPost(Quartz.kCGHIDEventTap, drag_event)
            time.sleep(sleep_time)
        
        # Release mouse button at end position
        self.mouse_up(x2, y2, button)
    
    def _get_key_code(self, key: str) -> int:
        """Get virtual key code for a key."""
        key = key.lower()
        if key in self.KEY_CODES:
            return self.KEY_CODES[key]
        elif len(key) == 1 and key.isascii():
            # Try to find the key in our mapping
            if key in self.KEY_CODES:
                return self.KEY_CODES[key]
            else:
                raise ValueError(f"Unknown key: {key}")
        else:
            raise ValueError(f"Unknown key: {key}")
    
    def key_press(self, key: str) -> None:
        """Press a key."""
        key_code = self._get_key_code(key)
        
        # Create key down event
        event = Quartz.CGEventCreateKeyboardEvent(None, key_code, True)
        Quartz.CGEventPost(Quartz.kCGHIDEventTap, event)
    
    def key_release(self, key: str) -> None:
        """Release a key."""
        key_code = self._get_key_code(key)
        
        # Create key up event
        event = Quartz.CGEventCreateKeyboardEvent(None, key_code, False)
        Quartz.CGEventPost(Quartz.kCGHIDEventTap, event)
    
    def key_tap(self, key: str) -> None:
        """Tap (press and release) a key."""
        self.key_press(key)
        time.sleep(0.05)
        self.key_release(key)
    
    def type_text(self, text: str, interval: float = 0.0) -> None:
        """Type text using keyboard."""
        # For macOS, we can use a more direct approach for typing text
        for char in text:
            # Handle special characters
            if char == '\n':
                self.key_tap("return")
            elif char == '\t':
                self.key_tap("tab")
            elif char == ' ':
                self.key_tap("space")
            else:
                # Create a string with the character
                string = objc.pyobjc_unicode(char)
                
                # Create a keyboard event source
                source = Quartz.CGEventSourceCreate(Quartz.kCGEventSourceStateHIDSystemState)
                
                # Create a keyboard event for key down
                event = Quartz.CGEventCreateKeyboardEvent(source, 0, True)
                
                # Set the Unicode string
                Quartz.CGEventKeyboardSetUnicodeString(event, len(string), string)
                
                # Post the event
                Quartz.CGEventPost(Quartz.kCGHIDEventTap, event)
                
                # Create a keyboard event for key up
                event = Quartz.CGEventCreateKeyboardEvent(source, 0, False)
                
                # Set the Unicode string
                Quartz.CGEventKeyboardSetUnicodeString(event, len(string), string)
                
                # Post the event
                Quartz.CGEventPost(Quartz.kCGHIDEventTap, event)
            
            # Wait between keystrokes if interval is specified
            if interval > 0:
                time.sleep(interval)
    
    def hotkey(self, *keys: str) -> None:
        """Press multiple keys simultaneously."""
        # Get key codes
        key_codes = [self._get_key_code(key) for key in keys]
        
        # Press all keys in sequence
        for key_code in key_codes:
            event = Quartz.CGEventCreateKeyboardEvent(None, key_code, True)
            Quartz.CGEventPost(Quartz.kCGHIDEventTap, event)
            time.sleep(0.01)
        
        # Small delay to ensure keys are registered
        time.sleep(0.05)
        
        # Release all keys in reverse order
        for key_code in reversed(key_codes):
            event = Quartz.CGEventCreateKeyboardEvent(None, key_code, False)
            Quartz.CGEventPost(Quartz.kCGHIDEventTap, event)
            time.sleep(0.01)


class ClipboardManagerMacOS(ClipboardManagerBase):
    """macOS implementation of clipboard operations."""
    
    def __init__(self):
        self.pasteboard = AppKit.NSPasteboard.generalPasteboard()
    
    def get_text(self) -> str:
        """Get text from clipboard."""
        text = self.pasteboard.stringForType_(AppKit.NSPasteboardTypeString)
        return text or ""
    
    def set_text(self, text: str) -> None:
        """Set text to clipboard."""
        self.pasteboard.clearContents()
        self.pasteboard.setString_forType_(text, AppKit.NSPasteboardTypeString)
    
    def clear(self) -> None:
        """Clear clipboard."""
        self.pasteboard.clearContents()
