#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test script for Maestro VSCode interaction
"""

import os
import sys
import time
import logging
from PIL import Image
import numpy as np

# Import maestro_core directly
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import maestro_core

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("maestro_test")

def list_all_windows():
    """List all available windows on the system"""
    logger.info("Listing all available windows:")
    # Create a temporary instance just to access the window manager
    temp_maestro = maestro_core.MaestroCore(window_title="", debug_mode=True)
    
    # Get window manager and list all windows
    window_manager = temp_maestro._window_manager
    
    # For macOS, we need to use the platform-specific method
    if sys.platform == "darwin":
        windows = window_manager.find_all_windows()
        vscode_windows = []
        other_windows = []
        
        # Separate VSCode windows from other windows for better visibility
        for window in windows:
            # Check for VSCode-specific patterns in window titles
            if any(pattern in window.title for pattern in ["Visual Studio Code", "Code - OSS", ".py —", ".js —", ".html —", ".css —", ".md —", "— Code"]):
                vscode_windows.append(window)
            else:
                other_windows.append(window)
        
        # Log VSCode windows first with a clear header
        if vscode_windows:
            logger.info("=== VSCode Windows ===")
            for i, window in enumerate(vscode_windows):
                logger.info(f"VSCode {i+1}. Title: {window.title} (ID: {window.id})")
        else:
            logger.info("No VSCode windows found")
            
        # Log other windows
        logger.info("\n=== Other Windows ===")
        for i, window in enumerate(other_windows):
            logger.info(f"Other {i+1}. Title: {window.title} (ID: {window.id})")
            
        # Return all windows for further processing
        return windows
    else:
        # For Windows, we need a different approach
        logger.info("Windows platform detected, using win32gui to list windows")
        import win32gui
        def callback(hwnd, windows_list):
            if win32gui.IsWindowVisible(hwnd):
                title = win32gui.GetWindowText(hwnd)
                if title:
                    windows_list.append((hwnd, title))
            return True
        windows_list = []
        win32gui.EnumWindows(callback, windows_list)
        
        # Separate VSCode windows from other windows
        vscode_windows = []
        other_windows = []
        
        for hwnd, title in windows_list:
            if any(pattern in title for pattern in ["Visual Studio Code", "Code - OSS", ".py -", ".js -", ".html -", ".css -", ".md -", "- Code"]):
                vscode_windows.append((hwnd, title))
            else:
                other_windows.append((hwnd, title))
        
        # Log VSCode windows first
        if vscode_windows:
            logger.info("=== VSCode Windows ===")
            for i, (hwnd, title) in enumerate(vscode_windows):
                logger.info(f"VSCode {i+1}. Title: {title} (ID: {hwnd})")
        else:
            logger.info("No VSCode windows found")
            
        # Log other windows
        logger.info("\n=== Other Windows ===")
        for i, (hwnd, title) in enumerate(other_windows):
            logger.info(f"Other {i+1}. Title: {title} (ID: {hwnd})")
        
        return windows_list

def main():
    # List all windows first
    windows = list_all_windows()
    
    # Ask user to check the console output and identify the correct VSCode window title
    logger.info("\nPlease check the console output above to identify the correct VSCode window title.")
    
    # Try to find VSCode window with specific patterns that match the actual VSCode IDE
    vscode_title_patterns = [
        ".py —",  # macOS uses em dash between filename and app name
        ".js —",
        ".html —",
        ".md —",
        ".css —",
        "— Visual Studio Code",
        "Visual Studio Code",
        "Code - OSS",  # Open source version of VSCode
        "Code"
    ]
    
    # Create Maestro instance targeting VSCode
    logger.info("\nTrying to find VSCode window with common title patterns...")
    
    maestro = None
    for pattern in vscode_title_patterns:
        logger.info(f"Trying pattern: {pattern}")
        temp_maestro = maestro_core.MaestroCore(window_title=pattern, debug_mode=True)
        if temp_maestro._window_manager.has_window_handle():
            logger.info(f"Found window matching pattern: {pattern}")
            maestro = temp_maestro
            break
    
    # Check if window was found
    if maestro is None or not maestro._window_manager.has_window_handle():
        logger.error("VSCode window not found with any common patterns")
        return
    
    logger.info("VSCode window found!")
    
    # Get window info
    window_rect = maestro._window_manager.get_window_rect()
    logger.info(f"Window rectangle: {window_rect}")
    
    # Capture window content
    logger.info("Capturing window content...")
    image = maestro.capture_window()
    if image is not None:
        # Save the captured image
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        filename = f"vscode_capture_{timestamp}.png"
        Image.fromarray(image).save(filename)
        logger.info(f"Window capture saved to {filename}")
        
        # Get window dimensions
        height, width = image.shape[:2]
        logger.info(f"Window dimensions: {width}x{height}")
    else:
        logger.error("Failed to capture window content")
    
    # Activate the window
    logger.info("Activating window...")
    if maestro.activate_window():
        logger.info("Window activated successfully")
    else:
        logger.error("Failed to activate window")
        
    logger.info("Test completed")
    return maestro

if __name__ == "__main__":
    main()
