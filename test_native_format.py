#!/usr/bin/env python3
"""
Test script to verify .native file format
"""

import struct
import sys

def test_native_file(filename):
    """Test if a file follows the .native format"""
    try:
        with open(filename, 'rb') as f:
            data = f.read()
            
        if len(data) < 64:
            print(f"ERROR {filename}: File too small (< 64 bytes)")
            return False
            
        # Check magic number
        magic = struct.unpack('<I', data[0:4])[0]
        if magic == 0x5654414E:  # "NATV"
            print(f"OK {filename}: Valid .native format detected")
            
            # Parse header
            version = struct.unpack('<I', data[4:8])[0]
            architecture = struct.unpack('<I', data[8:12])[0]
            module_type = struct.unpack('<I', data[12:16])[0]
            
            code_offset = struct.unpack('<Q', data[16:24])[0]
            code_size = struct.unpack('<Q', data[24:32])[0]
            data_offset = struct.unpack('<Q', data[32:40])[0]
            data_size = struct.unpack('<Q', data[40:48])[0]
            
            export_table_offset = struct.unpack('<Q', data[48:56])[0]
            export_count = struct.unpack('<I', data[56:60])[0]
            entry_point_offset = struct.unpack('<I', data[60:64])[0]
            
            print(f"  Version: {version}")
            print(f"  Architecture: {architecture} ({'x86_64' if architecture == 1 else 'unknown'})")
            print(f"  Module type: {module_type} ({'VM' if module_type == 1 else 'libc' if module_type == 2 else 'User'})")
            print(f"  Code size: {code_size} bytes")
            print(f"  Data size: {data_size} bytes")
            print(f"  Exports: {export_count}")
            print(f"  Entry point: 0x{entry_point_offset:04X}")
            
            return True
        else:
            print(f"ERROR {filename}: Invalid magic number: 0x{magic:08X} (expected 0x5654414E)")
            
            # Check if it's the old format
            if len(data) >= 16:
                old_magic = struct.unpack('<I', data[0:4])[0]
                if old_magic == 0x43545341:  # "ASTC"
                    print(f"  Note: This appears to be an ASTC file, not a .native file")
                else:
                    print(f"  Raw header: {data[:16].hex()}")
            
            return False
            
    except Exception as e:
        print(f"ERROR {filename}: Error reading file: {e}")
        return False

def main():
    files_to_test = [
        "tests/vm_x64_64.native",
        "tests/libc_x64_64.native",
        "tests/complete_cycle_test.native"
    ]
    
    print("Testing .native file format...")
    print("=" * 50)
    
    success_count = 0
    for filename in files_to_test:
        if test_native_file(filename):
            success_count += 1
        print()
    
    print("=" * 50)
    print(f"Results: {success_count}/{len(files_to_test)} files passed format validation")
    
    if success_count == len(files_to_test):
        print("SUCCESS: All .native files have correct format!")
        return 0
    else:
        print("WARNING: Some files failed format validation")
        return 1

if __name__ == "__main__":
    exit(main())
