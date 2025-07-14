"""
Platform abstraction layer for Maestro.

This module provides platform-independent interfaces and platform-specific 
implementations for window management, screen capture, and input control.
"""

import platform
import logging

logger = logging.getLogger("maestro.platform")

def is_windows():
    """Check if the current platform is Windows."""
    return platform.system() == "Windows"

def is_macos():
    """Check if the current platform is macOS."""
    return platform.system() == "Darwin"

def is_linux():
    """Check if the current platform is Linux."""
    return platform.system() == "Linux"

def get_platform():
    """Get the current platform name."""
    system = platform.system()
    if system == "Windows":
        return "windows"
    elif system == "Darwin":
        return "macos"
    elif system == "Linux":
        return "linux"
    else:
        return "unknown"

# Import platform-specific implementations
try:
    if is_windows():
        logger.info("Loading Windows platform implementation")
        from .windows import (
            WindowManagerWindows as WindowManager,
            ScreenCaptureWindows as ScreenCapture,
            InputControllerWindows as InputController,
            ClipboardManagerWindows as ClipboardManager
        )
    elif is_macos():
        logger.info("Loading macOS platform implementation")
        from .macos import (
            WindowManagerMacOS as WindowManager,
            ScreenCaptureMacOS as ScreenCapture,
            InputControllerMacOS as InputController,
            ClipboardManagerMacOS as ClipboardManager
        )
    else:
        logger.warning(f"Unsupported platform: {platform.system()}")
        # Import base classes as fallback
        from .base import (
            WindowManagerBase as WindowManager,
            ScreenCaptureBase as ScreenCapture,
            InputControllerBase as InputController,
            ClipboardManagerBase as ClipboardManager
        )
except ImportError as e:
    logger.error(f"Error importing platform implementation: {e}")
    # Import base classes as fallback
    from .base import (
        WindowManagerBase as WindowManager,
        ScreenCaptureBase as ScreenCapture,
        InputControllerBase as InputController,
        ClipboardManagerBase as ClipboardManager
    )
