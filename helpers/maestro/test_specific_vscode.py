#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Test script for interacting with a specific VSCode window
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

def main():
    # Target the specific VSCode window we found
    window_title = "☁️ Remote Agent:"
    
    logger.info(f"Creating Maestro instance for window: {window_title}")
    maestro = maestro_core.MaestroCore(window_title=window_title, debug_mode=True)
    
    # Check if window was found
    if not maestro._window_manager.has_window_handle():
        logger.error(f"Window not found: {window_title}")
        return
    
    logger.info(f"Window found: {maestro.window_title}")
    
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
    
    # Wait a moment
    time.sleep(1)
    
    logger.info("Test completed")
    return maestro

if __name__ == "__main__":
    main()
