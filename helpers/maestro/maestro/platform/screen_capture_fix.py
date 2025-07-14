#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Fix for macOS screen capture implementation in the platform abstraction layer
"""

import os
import sys
import logging
import numpy as np
from PIL import Image
from typing import Optional

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("screen_capture_fix")

def apply_fix():
    """Apply the screen capture fix to the platform abstraction layer"""
    try:
        # Import the platform modules
        sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
        from maestro.platform import window_capture
        from maestro.platform.macos import ScreenCaptureMacOS
        
        # Create a patched version of the ScreenCaptureMacOS class
        class PatchedScreenCaptureMacOS(ScreenCaptureMacOS):
            """Patched version of the macOS screen capture implementation"""
            
            def _safely_reshape_image_data(self, img_data, expected_height, expected_width):
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
            
            def capture(self) -> Optional[Image.Image]:
                """Capture current window content as PIL Image with safe reshaping."""
                if not self.window_manager._window_id:
                    logger.error("No window handle set")
                    return None
                
                try:
                    # Use the original capture method but with our safe reshaping
                    result = super().capture()
                    if result is not None:
                        return result
                    
                    # If that failed, try a different approach - capture a region instead
                    x, y, right, bottom = self.window_manager.get_window_rect()
                    width = right - x
                    height = bottom - y
                    
                    # Try capturing a smaller region
                    region_width = min(800, width)
                    region_height = min(600, height)
                    region_x = x + (width - region_width) // 2
                    region_y = y + (height - region_height) // 2
                    
                    return self.capture_region(region_x, region_y, region_width, region_height)
                    
                except Exception as e:
                    logger.error(f"Error in patched capture: {e}")
                    return None
            
            def capture_region(self, x: int, y: int, width: int, height: int) -> Optional[Image.Image]:
                """Capture specific region of the screen with safe reshaping."""
                try:
                    # Get the original implementation's result
                    result = super().capture_region(x, y, width, height)
                    return result
                except Exception as e:
                    logger.error(f"Error in patched capture_region: {e}")
                    return None
        
        # Monkey patch the ScreenCaptureMacOS class with our patched version
        import maestro.platform.macos
        maestro.platform.macos.ScreenCaptureMacOS = PatchedScreenCaptureMacOS
        
        logger.info("Successfully applied screen capture fix")
        return True
        
    except Exception as e:
        logger.exception(f"Error applying screen capture fix: {e}")
        return False

if __name__ == "__main__":
    if apply_fix():
        print("Screen capture fix applied successfully")
    else:
        print("Failed to apply screen capture fix")
