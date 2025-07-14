#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Patch script to update the macOS screen capture implementation
"""

import os
import shutil
import logging

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("patch_script")

def apply_patch():
    """Apply the screen capture patch to the macOS platform file"""
    # Get the current directory
    current_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Define file paths
    macos_file = os.path.join(current_dir, "macos.py")
    fixed_file = os.path.join(current_dir, "fixed_screen_capture.py")
    backup_file = os.path.join(current_dir, "macos.py.bak")
    
    # Check if files exist
    if not os.path.exists(macos_file):
        logger.error(f"macOS platform file not found: {macos_file}")
        return False
    
    if not os.path.exists(fixed_file):
        logger.error(f"Fixed screen capture file not found: {fixed_file}")
        return False
    
    try:
        # Create backup of original file
        logger.info(f"Creating backup of {macos_file}")
        shutil.copy2(macos_file, backup_file)
        
        # Read the fixed screen capture implementation
        with open(fixed_file, 'r') as f:
            fixed_content = f.read()
        
        # Extract the FixedScreenCaptureMacOS class
        import re
        class_match = re.search(r'class FixedScreenCaptureMacOS\(ScreenCaptureBase\):.*?def capture_to_file\(.*?\):.*?return False', 
                               fixed_content, re.DOTALL)
        
        if not class_match:
            logger.error("Could not extract FixedScreenCaptureMacOS class from fixed file")
            return False
        
        fixed_class = class_match.group(0)
        
        # Read the original macOS file
        with open(macos_file, 'r') as f:
            macos_content = f.read()
        
        # Replace the ScreenCaptureMacOS class with the fixed implementation
        pattern = r'class ScreenCaptureMacOS\(ScreenCaptureBase\):.*?def capture_to_file\(.*?\):.*?return False'
        updated_content = re.sub(pattern, 
                               fixed_class.replace('FixedScreenCaptureMacOS', 'ScreenCaptureMacOS'), 
                               macos_content, 
                               flags=re.DOTALL)
        
        # Write the updated content back to the file
        with open(macos_file, 'w') as f:
            f.write(updated_content)
        
        logger.info(f"Successfully updated {macos_file} with fixed screen capture implementation")
        return True
        
    except Exception as e:
        logger.exception(f"Error applying patch: {e}")
        
        # Restore backup if it exists
        if os.path.exists(backup_file):
            logger.info(f"Restoring backup from {backup_file}")
            shutil.copy2(backup_file, macos_file)
        
        return False

if __name__ == "__main__":
    if apply_patch():
        logger.info("Patch applied successfully")
    else:
        logger.error("Failed to apply patch")
