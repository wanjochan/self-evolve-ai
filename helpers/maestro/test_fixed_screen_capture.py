#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test script for the fixed screen capture implementation on macOS
"""

import os
import sys
import time
import logging
import numpy as np
from PIL import Image

# Import maestro modules
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from maestro.platform.macos import WindowManagerMacOS
from maestro.platform.fixed_screen_capture import FixedScreenCaptureMacOS

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("fixed_screen_capture_test")

def main():
    """Test fixed screen capture functionality"""
    logger.info("Testing fixed screen capture on macOS")
    
    # Create window manager and fixed screen capture instances
    window_manager = WindowManagerMacOS()
    screen_capture = FixedScreenCaptureMacOS(window_manager)
    
    # List all windows
    logger.info("Listing all windows:")
    windows = window_manager.find_all_windows()
    
    if not windows:
        logger.error("No windows found")
        return
    
    # Categorize windows
    vscode_windows = []
    other_windows = []
    
    vscode_patterns = [
        "visual studio code", 
        "code", 
        ".py —", 
        ".js —", 
        ".html —", 
        ".md —", 
        ".css —",
        "☁️ remote agent",
        "workspace"
    ]
    
    for window in windows:
        is_vscode = any(pattern.lower() in window.title.lower() for pattern in vscode_patterns)
        if is_vscode:
            vscode_windows.append(window)
        else:
            other_windows.append(window)
    
    # Display VSCode windows
    logger.info(f"Found {len(vscode_windows)} VSCode windows:")
    for i, window in enumerate(vscode_windows):
        logger.info(f"VSCode {i+1}. Title: {window.title} (ID: {window.id})")
    
    # Display other windows (limited to 10)
    logger.info("\n=== Other Windows ===")
    for i, window in enumerate(other_windows[:10]):
        logger.info(f"Other {i+1}. Title: {window.title} (ID: {window.id})")
    
    # Select a window for testing
    test_window = None
    
    # First try to find a VSCode window
    if vscode_windows:
        test_window = vscode_windows[0]
        logger.info(f"\nSelected VSCode window: {test_window.title} (ID: {test_window.id})")
    else:
        # If no VSCode window, find a suitable window for testing
        for window in windows:
            if window.title and len(window.title) > 0:
                test_window = window
                logger.info(f"\nNo VSCode window found. Selected window: {test_window.title} (ID: {test_window.id})")
                break
    
    if not test_window:
        logger.error("No suitable window found for testing")
        return
    
    # Set the window handle
    window_manager.set_window_handle(test_window.id)
    
    # Get window dimensions
    x, y, right, bottom = window_manager.get_window_rect()
    width = right - x
    height = bottom - y
    logger.info(f"Window dimensions: {width}x{height}")
    
    # Try to capture the window
    logger.info("Attempting to capture window...")
    try:
        # First try a smaller region
        region_width = min(400, width)
        region_height = min(300, height)
        region_x = x + (width - region_width) // 2
        region_y = y + (height - region_height) // 2
        
        logger.info(f"Capturing region: {region_x},{region_y} {region_width}x{region_height}")
        image = screen_capture.capture_region(region_x, region_y, region_width, region_height)
        
        if image:
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            filename = f"region_capture_{timestamp}.png"
            image.save(filename)
            logger.info(f"Region capture successful! Saved to {filename}")
        else:
            logger.error("Region capture failed, returned None")
        
        # Now try to capture the whole window
        logger.info("Capturing entire window...")
        image = screen_capture.capture()
        
        if image:
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            filename = f"window_capture_{timestamp}.png"
            image.save(filename)
            logger.info(f"Window capture successful! Saved to {filename}")
            
            # Get image dimensions
            img_width, img_height = image.size
            logger.info(f"Captured image dimensions: {img_width}x{img_height}")
        else:
            logger.error("Window capture failed, returned None")
        
        # Try to activate the window
        logger.info("Activating window...")
        if window_manager.activate_window():
            logger.info("Window activated successfully")
        else:
            logger.error("Failed to activate window")
            
    except Exception as e:
        logger.exception(f"Error during test: {e}")
    
    logger.info("Test completed")

if __name__ == "__main__":
    main()
