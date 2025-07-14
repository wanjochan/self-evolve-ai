#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test script for debugging screen capture on macOS
"""

import os
import sys
import time
import logging
import numpy as np
from PIL import Image

# Import maestro_core directly
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from maestro.platform import window_capture

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("screen_capture_test")

def main():
    """Test screen capture functionality"""
    logger.info("Testing screen capture on macOS")
    
    # Create window manager and screen capture instances
    window_manager = window_capture.get_window_manager()
    screen_capture = window_capture.get_screen_capture(window_manager)
    
    # List all windows
    logger.info("Listing all windows:")
    windows = window_manager.find_all_windows()
    
    if not windows:
        logger.error("No windows found")
        return
    
    # Find a suitable window for testing
    test_window = None
    for window in windows:
        if window.title and len(window.title) > 0:
            test_window = window
            break
    
    if not test_window:
        logger.error("No suitable window found for testing")
        return
    
    logger.info(f"Selected test window: {test_window.title} (ID: {test_window.id})")
    
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
        # Capture a smaller region instead of the whole window
        # This might help avoid memory issues
        region_width = min(800, width)
        region_height = min(600, height)
        region_x = x + (width - region_width) // 2
        region_y = y + (height - region_height) // 2
        
        logger.info(f"Capturing region: {region_x},{region_y} {region_width}x{region_height}")
        image = screen_capture.capture_region(region_x, region_y, region_width, region_height)
        
        if image:
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            filename = f"test_capture_{timestamp}.png"
            image.save(filename)
            logger.info(f"Capture successful! Saved to {filename}")
        else:
            logger.error("Capture failed, returned None")
    except Exception as e:
        logger.exception(f"Error during capture: {e}")
    
    logger.info("Test completed")

if __name__ == "__main__":
    main()
