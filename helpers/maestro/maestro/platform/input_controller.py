"""
Platform-specific input controller factory.

This module provides functions to get platform-specific implementations
of input controllers and window managers.
"""

import logging
import platform

from .base import InputControllerBase, WindowManagerBase

logger = logging.getLogger("maestro.platform.input_controller")

# Cache for singleton instances
_input_controller = None
_window_manager = None

def get_input_controller() -> InputControllerBase:
    """Get platform-specific input controller implementation."""
    global _input_controller
    
    if _input_controller is None:
        system = platform.system()
        
        if system == "Windows":
            from .windows import InputControllerWindows
            _input_controller = InputControllerWindows()
            logger.debug("Using Windows input controller")
        elif system == "Darwin":  # macOS
            from .macos import InputControllerMacOS
            _input_controller = InputControllerMacOS()
            logger.debug("Using macOS input controller")
        else:
            # Fallback to base implementation which will raise NotImplementedError
            from .base import InputControllerBase
            _input_controller = InputControllerBase()
            logger.warning(f"No input controller implementation for {system}, using base class")
    
    return _input_controller

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
