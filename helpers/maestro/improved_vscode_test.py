#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Improved test script for Maestro VSCode interaction on macOS
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
from maestro.platform import window_capture

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("maestro_test")

def safely_reshape_image_data(img_data, expected_height, expected_width):
    """Safely reshape image data to handle dimension mismatches."""
    expected_size = expected_height * expected_width * 4  # RGBA format (4 bytes per pixel)
    actual_size = len(img_data)
    
    if actual_size != expected_size:
        logger.warning(f"Image buffer size mismatch: expected {expected_size}, got {actual_size}")
        
        # Calculate the actual dimensions based on the buffer size
        if actual_size % 4 == 0:  # Ensure divisible by 4 (RGBA)
            pixel_count = actual_size // 4
            
            # Try to maintain the aspect ratio
            if expected_width > 0 and expected_height > 0:
                aspect_ratio = expected_width / expected_height
                
                # Calculate new dimensions based on aspect ratio
                new_height = int(np.sqrt(pixel_count / aspect_ratio))
                if new_height <= 0:
                    new_height = 1
                
                new_width = pixel_count // new_height
                if new_width <= 0:
                    new_width = 1
                
                # Adjust to ensure exact match
                while new_width * new_height * 4 != actual_size:
                    if new_width * new_height * 4 < actual_size:
                        new_width += 1
                    else:
                        new_height -= 1
                    
                    # Safety check
                    if new_width <= 0 or new_height <= 0:
                        # Fall back to simple square calculation
                        new_dim = int(np.sqrt(pixel_count))
                        new_height = new_dim
                        new_width = pixel_count // new_dim
                        break
                
                logger.info(f"Adjusted dimensions to: {new_width}x{new_height}")
            else:
                # If dimensions are invalid, use a square shape
                new_dim = int(np.sqrt(pixel_count))
                new_height = new_dim
                new_width = pixel_count // new_dim
            
            # Reshape with adjusted dimensions
            try:
                return img_data.reshape(new_height, new_width, 4)
            except ValueError as e:
                logger.error(f"Reshape failed with adjusted dimensions: {e}")
                return None
        else:
            logger.error("Buffer size is not divisible by 4, cannot reshape to RGBA format")
            return None
    else:
        # Original dimensions are correct
        try:
            return img_data.reshape(expected_height, expected_width, 4)
        except ValueError as e:
            logger.error(f"Reshape failed with original dimensions: {e}")
            return None

def safe_capture_window(maestro):
    """Safely capture window content with error handling for reshape issues."""
    try:
        # Get the window dimensions
        x, y, right, bottom = maestro._window_manager.get_window_rect()
        width = right - x
        height = bottom - y
        
        # Create a direct reference to the screen capture object
        screen_capture = maestro._screen_capture
        
        # Try to capture the window directly using the platform's screen capture
        image = screen_capture.capture()
        
        if image is not None:
            # Convert PIL Image to numpy array
            return np.array(image)
        
        # If direct capture failed, try to capture a region instead
        logger.info("Direct window capture failed, trying region capture...")
        
        # Try a smaller region first
        region_width = min(800, width)
        region_height = min(600, height)
        region_x = x + (width - region_width) // 2
        region_y = y + (height - region_height) // 2
        
        image = screen_capture.capture_region(region_x, region_y, region_width, region_height)
        
        if image is not None:
            return np.array(image)
        
        logger.error("All capture methods failed")
        return None
        
    except Exception as e:
        logger.exception(f"Error in safe_capture_window: {e}")
        return None

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

def main():
    # List all windows first
    all_windows, vscode_windows, other_windows = list_all_windows()
    
    # Ask user to check the console output and identify the correct VSCode window title
    logger.info("\nPlease check the console output above to identify the correct VSCode window.")
    
    maestro = None
    
    # If VSCode windows were found, try to use the first one
    if vscode_windows:
        target_window = vscode_windows[0]
        logger.info(f"\nTrying to interact with VSCode window: {target_window.title}")
        
        # Create Maestro instance with this window
        maestro = maestro_core.MaestroCore(debug_mode=True)
        maestro._window_manager.set_window_handle(target_window.id)
        
        if not maestro._window_manager.has_window_handle():
            logger.error("Failed to set window handle")
            maestro = None
    
    # If no VSCode window was found or setting the handle failed, try with patterns
    if maestro is None:
        # Try to find VSCode window with specific patterns that match the actual VSCode IDE
        vscode_title_patterns = [
            "☁️ Remote Agent:",
            ".py —",  # macOS uses em dash between filename and app name
            ".js —",
            ".html —",
            ".md —",
            ".css —",
            "— Visual Studio Code",
            "Visual Studio Code",
            "Code - OSS",  # Open source version of VSCode
            "Code",
            "workspace"
        ]
        
        # Create Maestro instance targeting VSCode
        logger.info("\nTrying to find VSCode window with common title patterns...")
        
        for pattern in vscode_title_patterns:
            logger.info(f"Trying pattern: {pattern}")
            temp_maestro = maestro_core.MaestroCore(window_title=pattern, debug_mode=True)
            if temp_maestro._window_manager.has_window_handle():
                logger.info(f"Found window matching pattern: {pattern}")
                maestro = temp_maestro
                break
    
    # Check if window was found
    if maestro is None or not maestro._window_manager.has_window_handle():
        logger.error("VSCode window not found with any method")
        return
    
    # Get the actual window title
    window_title = maestro._window_manager.get_window_title()
    logger.info(f"VSCode window found: {window_title}")
    
    # Get window info
    window_rect = maestro._window_manager.get_window_rect()
    logger.info(f"Window rectangle: {window_rect}")
    
    # Capture window content using our safe method
    logger.info("Capturing window content...")
    image = safe_capture_window(maestro)
    
    if image is not None:
        # Save the captured image
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        filename = f"vscode_capture_{timestamp}.png"
        Image.fromarray(image).save(filename)
        logger.info(f"Window capture saved to {filename}")
        
        # Get window dimensions
        height, width = image.shape[:2]
        logger.info(f"Captured image dimensions: {width}x{height}")
    else:
        logger.error("Failed to capture window content")
    
    # Activate the window
    logger.info("Activating window...")
    if maestro.activate_window():
        logger.info("Window activated successfully")
    else:
        logger.error("Failed to activate window")
    
    # Wait a moment to see the activation
    time.sleep(1)
        
    logger.info("Test completed")
    return maestro

if __name__ == "__main__":
    main()
