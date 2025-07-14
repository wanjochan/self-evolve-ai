"""
Platform-specific window capture factory.

This module provides functions to get platform-specific implementations
of window managers and screen capture.
"""

import logging
import platform

from .base import WindowManagerBase, ScreenCaptureBase

logger = logging.getLogger("maestro.platform.window_capture")

# Cache for singleton instances
_window_manager = None
_screen_capture = None

def get_window_manager() -> WindowManagerBase:
    """Get platform-specific window manager implementation."""
    global _window_manager
    
    if _window_manager is None:
        system = platform.system()
        
        if system == "Windows":
            from .windows import WindowManagerWindows
            _window_manager = WindowManagerWindows()
            logger.debug("Using Windows window manager")
        elif system == "Darwin":  # macOS
            from .macos import WindowManagerMacOS
            _window_manager = WindowManagerMacOS()
            logger.debug("Using macOS window manager")
        else:
            # Fallback to base implementation which will raise NotImplementedError
            from .base import WindowManagerBase
            _window_manager = WindowManagerBase()
            logger.warning(f"No window manager implementation for {system}, using base class")
    
    return _window_manager

def get_screen_capture(window_manager=None) -> ScreenCaptureBase:
    """Get platform-specific screen capture implementation.
    
    Args:
        window_manager: Optional window manager to use with the screen capture.
                       If None, a new window manager will be created.
    """
    global _screen_capture
    
    if _screen_capture is None:
        system = platform.system()
        
        # If no window manager provided, get one
        if window_manager is None:
            window_manager = get_window_manager()
        
        if system == "Windows":
            from .windows import ScreenCaptureWindows
            _screen_capture = ScreenCaptureWindows(window_manager)
            logger.debug("Using Windows screen capture")
        elif system == "Darwin":  # macOS
            from .macos import ScreenCaptureMacOS
            _screen_capture = ScreenCaptureMacOS(window_manager)
            logger.debug("Using macOS screen capture")
        else:
            # Fallback to base implementation which will raise NotImplementedError
            from .base import ScreenCaptureBase
            _screen_capture = ScreenCaptureBase(window_manager)
            logger.warning(f"No screen capture implementation for {system}, using base class")
    
    return _screen_capture
