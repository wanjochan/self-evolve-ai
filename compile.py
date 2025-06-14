"""
使用pytcc编译bootstrap.c
"""

import sys
import os
try:
    import pytcc
except ImportError:
    print("请先安装pytcc: pip install pytcc")
    sys.exit(1)

def main():
    # 创建TCC实例
    tcc = pytcc.TCC()
    
    # 设置输出文件
    output_file = "bootstrap.exe"
    tcc.set_output_type("exe")
    tcc.set_output_file(output_file)
    
    # 添加源文件
    tcc.add_file("bootstrap.c")
    
    # 编译
    try:
        tcc.compile()
        print(f"编译成功: {output_file}")
    except Exception as e:
        print(f"编译失败: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 