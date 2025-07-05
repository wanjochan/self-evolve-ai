#!/usr/bin/env python3
"""
简单的VSCode自动回复脚本
每隔几秒自动发送'continue'到VSCode窗口
"""

import time
import subprocess
import sys

def send_continue():
    """发送continue到VSCode窗口"""
    try:
        # 发送continue文本
        result1 = subprocess.run([
            "python", "maestro/maestro_cli.py",
            "keyboard", "self-evolve-ai - Visual Studio Code",
            "type", "continue", "--no-activate"
        ], capture_output=True, text=True, timeout=10, encoding='utf-8', errors='ignore')
        
        if result1.returncode == 0:
            print(f"[{time.strftime('%H:%M:%S')}] ✅ 发送'continue'成功")
            
            # 等待一下再发送回车
            time.sleep(0.5)
            
            # 发送回车键
            result2 = subprocess.run([
                "python", "maestro/maestro_cli.py",
                "keyboard", "self-evolve-ai - Visual Studio Code",
                "key", "Return"
            ], capture_output=True, text=True, timeout=10, encoding='utf-8', errors='ignore')
            
            if result2.returncode == 0:
                print(f"[{time.strftime('%H:%M:%S')}] ✅ 发送回车成功")
                return True
            else:
                print(f"[{time.strftime('%H:%M:%S')}] ❌ 发送回车失败: {result2.stderr}")
        else:
            print(f"[{time.strftime('%H:%M:%S')}] ❌ 发送'continue'失败: {result1.stderr}")
            
    except Exception as e:
        print(f"[{time.strftime('%H:%M:%S')}] ❌ 执行失败: {e}")
    
    return False

def check_window():
    """检查VSCode窗口是否存在"""
    try:
        result = subprocess.run([
            "python", "maestro/maestro_cli.py", "list"
        ], capture_output=True, text=True, timeout=10, encoding='utf-8', errors='ignore')

        if result.returncode == 0 and "self-evolve-ai - Visual Studio Code" in result.stdout:
            return True
        else:
            # 打印调试信息
            print(f"[DEBUG] 窗口检查结果: returncode={result.returncode}")
            if result.stdout:
                print(f"[DEBUG] stdout包含的窗口: {[line.strip() for line in result.stdout.split('\\n') if 'Visual Studio Code' in line]}")
    except Exception as e:
        print(f"[DEBUG] 窗口检查异常: {e}")
    return False

def main():
    print("🤖 VSCode自动回复脚本启动")
    print("功能: 每30秒自动发送'continue'到VSCode")
    print("按 Ctrl+C 停止")
    print("=" * 40)
    
    interval = 30  # 30秒间隔
    
    try:
        while True:
            # 检查窗口是否存在
            if check_window():
                print(f"[{time.strftime('%H:%M:%S')}] 🔍 检测到VSCode窗口，发送continue...")
                send_continue()
            else:
                print(f"[{time.strftime('%H:%M:%S')}] ⚠️ 未找到VSCode窗口")
            
            # 等待下次执行
            print(f"[{time.strftime('%H:%M:%S')}] 💤 等待{interval}秒...")
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print(f"\n[{time.strftime('%H:%M:%S')}] 🛑 收到停止信号，退出")
    except Exception as e:
        print(f"\n[{time.strftime('%H:%M:%S')}] ❌ 程序出错: {e}")

if __name__ == "__main__":
    main()
