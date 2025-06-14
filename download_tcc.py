"""
下载和解压TinyCC
"""

import os
import sys
import urllib.request
import zipfile
import shutil

def main():
    # TinyCC下载地址
    url = "https://download.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip"
    zip_file = "tcc-win64.zip"
    
    print(f"下载TinyCC: {url}")
    try:
        urllib.request.urlretrieve(url, zip_file)
    except Exception as e:
        print(f"下载失败: {e}")
        sys.exit(1)
    
    print("解压TinyCC")
    try:
        with zipfile.ZipFile(zip_file, 'r') as zip_ref:
            zip_ref.extractall("tcc")
    except Exception as e:
        print(f"解压失败: {e}")
        sys.exit(1)
    
    print("TinyCC安装完成")
    print("使用方法: .\\tcc\\tcc.exe -o bootstrap.exe bootstrap.c")

if __name__ == "__main__":
    main() 