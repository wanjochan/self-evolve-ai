#!/usr/bin/env python3
"""
最简单的自动回复脚本
"""

import time
import os

def main():
    print("🤖 简单自动回复脚本启动")
    print("每30秒发送一次continue")
    
    count = 0
    while True:
        count += 1
        print(f"\n=== 第{count}次执行 ===")
        print(f"时间: {time.strftime('%H:%M:%S')}")
        
        # 执行命令
        cmd = 'python maestro/maestro_cli.py keyboard "self-evolve-ai - Visual Studio Code" type "continue" --no-activate'
        print(f"执行: {cmd}")
        
        result = os.system(cmd)
        if result == 0:
            print("✅ 发送成功")
        else:
            print(f"❌ 发送失败，返回码: {result}")
        
        print("💤 等待30秒...")
        time.sleep(30)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n🛑 停止脚本")
