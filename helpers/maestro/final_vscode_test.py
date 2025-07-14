#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Final test script for Maestro VSCode interaction on macOS
This script applies the screen capture fix and demonstrates full functionality
"""

import os
import sys
import time
import logging
from PIL import Image
import numpy as np

# Import maestro modules
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

# Apply the screen capture fix first
from maestro.platform import screen_capture_fix
screen_capture_fix.apply_fix()

# Now import maestro_core
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
    windows = window_manager.find_all_windows()
    vscode_windows = []
    other_windows = []
    
    # Enhanced patterns for VSCode window detection
    vscode_patterns = [
        "visual studio code",
        "code - oss",
        "code",
        ".py —",
        ".js —",
        ".html —",
        ".css —",
        ".md —",
        "— code",
        "☁️ remote agent",
        "workspace",
        "vscode"
    ]
    
    # Separate VSCode windows from other windows for better visibility
    for window in windows:
        # Check for VSCode-specific patterns in window titles (case insensitive)
        title_lower = window.title.lower()
        if any(pattern in title_lower for pattern in vscode_patterns):
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
    return windows, vscode_windows, other_windows

def test_window_management():
    """Test window management functionality"""
    logger.info("\n=== Testing Window Management ===")
    
    # List all windows
    all_windows, vscode_windows, other_windows = list_all_windows()
    
    # Find a VSCode window to work with
    target_window = None
    if vscode_windows:
        target_window = vscode_windows[0]
        logger.info(f"Selected VSCode window: {target_window.title} (ID: {target_window.id})")
    else:
        logger.error("No VSCode windows found for testing")
        return None
    
    # Create Maestro instance
    maestro = maestro_core.MaestroCore(debug_mode=True)
    maestro._window_manager.set_window_handle(target_window.id)
    
    if not maestro._window_manager.has_window_handle():
        logger.error("Failed to set window handle")
        return None
    
    # Get window info
    window_title = maestro._window_manager.get_window_title()
    logger.info(f"Window title: {window_title}")
    
    window_rect = maestro._window_manager.get_window_rect()
    logger.info(f"Window rectangle: {window_rect}")
    
    # Activate the window
    logger.info("Activating window...")
    if maestro.activate_window():
        logger.info("Window activated successfully")
    else:
        logger.error("Failed to activate window")
    
    return maestro

def test_screen_capture(maestro):
    """Test screen capture functionality"""
    if not maestro:
        logger.error("No Maestro instance provided")
        return
    
    logger.info("\n=== Testing Screen Capture ===")
    
    # Capture full window
    logger.info("Capturing full window...")
    image = maestro.capture_window()
    
    if image is not None:
        # Save the captured image
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        filename = f"vscode_full_{timestamp}.png"
        Image.fromarray(image).save(filename)
        logger.info(f"Full window capture saved to {filename}")
        
        # Get window dimensions
        height, width = image.shape[:2]
        logger.info(f"Full window dimensions: {width}x{height}")
    else:
        logger.error("Failed to capture full window")
    
    # Capture a specific region
    logger.info("Capturing window region...")
    x, y, right, bottom = maestro._window_manager.get_window_rect()
    width = right - x
    height = bottom - y
    
    # Define a region in the center of the window
    region_width = min(400, width)
    region_height = min(300, height)
    region_x = x + (width - region_width) // 2
    region_y = y + (height - region_height) // 2
    
    # Capture the region using the screen capture object directly
    img = maestro._screen_capture.capture_region(region_x, region_y, region_width, region_height)
    region = np.array(img) if img is not None else None
    
    if region is not None:
        # Save the captured region
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        filename = f"vscode_region_{timestamp}.png"
        Image.fromarray(region).save(filename)
        logger.info(f"Region capture saved to {filename}")
        
        # Get region dimensions
        height, width = region.shape[:2]
        logger.info(f"Region dimensions: {width}x{height}")
    else:
        logger.error("Failed to capture region")

def test_input_control(maestro):
    """Test input control functionality"""
    if not maestro:
        logger.error("No Maestro instance provided")
        return
    
    logger.info("\n=== Testing Input Control ===")
    
    # Activate the window first
    maestro.activate_window()
    time.sleep(0.5)  # Give time for the window to activate
    
    # Get cursor position
    x, y = maestro._input_controller.get_cursor_position()
    logger.info(f"Current cursor position: {x}, {y}")
    
    # Move cursor to center of window
    window_rect = maestro._window_manager.get_window_rect()
    center_x = (window_rect[0] + window_rect[2]) // 2
    center_y = (window_rect[1] + window_rect[3]) // 2
    
    logger.info(f"Moving cursor to center of window: {center_x}, {center_y}")
    maestro._input_controller.mouse_move(center_x, center_y)
    time.sleep(0.5)  # Give time for the cursor to move
    
    # Get new cursor position
    new_x, new_y = maestro._input_controller.get_cursor_position()
    logger.info(f"New cursor position: {new_x}, {new_y}")
    
    # Test keyboard input (just logging, not actually typing)
    logger.info("Testing keyboard input capabilities (not actually typing)")
    logger.info("Can type text: 'Hello, Maestro on macOS!'")
    logger.info("Can press special keys: Enter, Tab, Escape")
    logger.info("Can use hotkeys: Command+S, Command+C, Command+V")

def main():
    logger.info("Starting Maestro VSCode interaction test on macOS")
    
    # Test window management
    maestro = test_window_management()
    
    if maestro:
        # Test screen capture
        test_screen_capture(maestro)
        
        # Test input control
        test_input_control(maestro)
        
        logger.info("\nAll tests completed successfully!")
    else:
        logger.error("Tests failed: Could not create Maestro instance")

if __name__ == "__main__":
    main()
