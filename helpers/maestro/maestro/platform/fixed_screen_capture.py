#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Fixed screen capture implementation for macOS
"""

import time
import logging
import Quartz
import AppKit
import Foundation
import numpy as np
from typing import Optional, Tuple, List, Dict, Any
from PIL import Image

from .base import ScreenCaptureBase
from .macos import WindowManagerMacOS

logger = logging.getLogger("maestro.platform.macos")

class FixedScreenCaptureMacOS(ScreenCaptureBase):
    """Fixed macOS implementation of screen capture operations."""
    
    def __init__(self, window_manager: Optional[WindowManagerMacOS] = None):
        self.window_manager = window_manager or WindowManagerMacOS()
    
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
        """Capture current window content as PIL Image."""
        if not self.window_manager._window_id:
            logger.error("No window handle set")
            return None
        
        try:
            # Get window bounds
            x, y, right, bottom = self.window_manager.get_window_rect()
            width = right - x
            height = bottom - y
            
            # Create CGRect for the window
            rect = Quartz.CGRectMake(x, y, width, height)
            
            # Capture window image
            image = Quartz.CGWindowListCreateImage(
                rect,
                Quartz.kCGWindowListOptionIncludingWindow,
                self.window_manager._window_id,
                Quartz.kCGWindowImageBoundsIgnoreFraming
            )
            
            if not image:
                logger.error("Failed to capture window image")
                return None
            
            # Convert to PIL Image
            width = Quartz.CGImageGetWidth(image)
            height = Quartz.CGImageGetHeight(image)
            
            # Create a bitmap context for the image
            context = Quartz.CGBitmapContextCreate(
                None,
                width,
                height,
                8,  # bits per component
                width * 4,  # bytes per row (explicit calculation)
                Quartz.CGImageGetColorSpace(image),
                Quartz.kCGImageAlphaPremultipliedLast
            )
            
            # Draw the image to the context
            rect = Quartz.CGRectMake(0, 0, width, height)
            Quartz.CGContextDrawImage(context, rect, image)
            
            # Get image data
            data = Quartz.CGBitmapContextCreateImage(context)
            provider = Quartz.CGImageGetDataProvider(data)
            buffer = Quartz.CGDataProviderCopyData(provider)
            
            # Convert to numpy array
            img_data = np.frombuffer(buffer, dtype=np.uint8)
            
            # Safely reshape the image data
            reshaped_data = self._safely_reshape_image_data(img_data, height, width)
            if reshaped_data is None:
                return None
            
            # Convert to PIL Image
            img = Image.fromarray(reshaped_data)
            return img
            
        except Exception as e:
            logger.error(f"Error capturing window: {e}")
            return None
    
    def capture_region(self, x: int, y: int, width: int, height: int) -> Optional[Image.Image]:
        """Capture specific region of the screen."""
        try:
            # Create CGRect for the region
            rect = Quartz.CGRectMake(x, y, width, height)
            
            # Capture screen region
            image = Quartz.CGWindowListCreateImage(
                rect,
                Quartz.kCGWindowListOptionOnScreenOnly,
                Quartz.kCGNullWindowID,
                Quartz.kCGWindowImageDefault
            )
            
            if not image:
                logger.error("Failed to capture screen region")
                return None
            
            # Convert to PIL Image
            width = Quartz.CGImageGetWidth(image)
            height = Quartz.CGImageGetHeight(image)
            
            # Create a bitmap context for the image
            context = Quartz.CGBitmapContextCreate(
                None,
                width,
                height,
                8,  # bits per component
                width * 4,  # bytes per row (explicit calculation)
                Quartz.CGImageGetColorSpace(image),
                Quartz.kCGImageAlphaPremultipliedLast
            )
            
            # Draw the image to the context
            rect = Quartz.CGRectMake(0, 0, width, height)
            Quartz.CGContextDrawImage(context, rect, image)
            
            # Get image data
            data = Quartz.CGBitmapContextCreateImage(context)
            provider = Quartz.CGImageGetDataProvider(data)
            buffer = Quartz.CGDataProviderCopyData(provider)
            
            # Convert to numpy array
            img_data = np.frombuffer(buffer, dtype=np.uint8)
            
            # Safely reshape the image data
            reshaped_data = self._safely_reshape_image_data(img_data, height, width)
            if reshaped_data is None:
                return None
            
            # Convert to PIL Image
            img = Image.fromarray(reshaped_data)
            return img
            
        except Exception as e:
            logger.error(f"Error capturing region: {e}")
            return None
    
    def capture_to_file(self, filepath: str) -> bool:
        """Capture window and save to file."""
        image = self.capture()
        if image:
            try:
                image.save(filepath)
                return True
            except Exception as e:
                logger.error(f"Error saving image to file: {e}")
        return False
