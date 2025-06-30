#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode Augment Continue Tool
自动检测VSCode左边的augment对话框，并输入"continue"继续执行

Usage:
    python tools/vscode_augment_continue.py
    python tools/vscode_augment_continue.py --screenshot-only
    python tools/vscode_augment_continue.py --verbose
"""

import sys
import os
import argparse
import time

# 添加当前目录到路径，以便导入uictrl
sys.path.append(os.path.dirname(__file__))

from uictrl import UIController

def find_vscode_window():
    """查找VSCode窗口"""
    controller = UIController()
    ide_windows = controller.find_ide_windows()
    
    for window in ide_windows:
        if 'visual studio code' in window['title'].lower():
            return window
    return None

def take_screenshot(window_id, filename, verbose=False):
    """获取VSCode截图"""
    controller = UIController()
    controller.verbose = verbose
    
    try:
        result = controller.take_screenshot(window_id, filename)
        if verbose:
            print(f"截图保存: {result}")
        return True
    except Exception as e:
        if verbose:
            print(f"截图失败: {e}")
        return False

def send_continue_command(window_id, verbose=False):
    """发送continue命令到VSCode"""
    controller = UIController()
    controller.verbose = verbose
    
    try:
        # 输入"continue"
        if verbose:
            print("输入 'continue'...")
        type_result = controller.type_text(window_id, "continue")
        if verbose:
            print(f"输入结果: {type_result}")
        
        # 短暂延迟
        time.sleep(0.3)
        
        # 按回车键
        if verbose:
            print("按回车键...")
        enter_result = controller.send_keys(window_id, "enter")
        if verbose:
            print(f"回车结果: {enter_result}")
        
        return True
    except Exception as e:
        if verbose:
            print(f"发送命令失败: {e}")
        return False

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='VSCode Augment Continue Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python tools/vscode_augment_continue.py                    # 执行continue操作
  python tools/vscode_augment_continue.py --screenshot-only # 仅截图查看状态
  python tools/vscode_augment_continue.py --verbose         # 详细输出
        """
    )
    
    parser.add_argument('--screenshot-only', action='store_true',
                       help='仅获取截图，不执行continue操作')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='详细输出')
    parser.add_argument('--output', '-o', default='tools/vscode_augment_status.png',
                       help='截图输出文件名 (默认: tools/vscode_augment_status.png)')
    
    args = parser.parse_args()
    
    # 查找VSCode窗口
    if args.verbose:
        print("=== VSCode Augment Continue Tool ===")
        print("查找VSCode窗口...")
    
    vscode_window = find_vscode_window()
    if not vscode_window:
        print("错误: 未找到VSCode窗口")
        print("请确保VSCode正在运行并且项目已打开")
        return 1
    
    if args.verbose:
        print(f"找到VSCode窗口: {vscode_window['title']} (ID: {vscode_window['id']})")
    
    # 获取当前状态截图
    if args.verbose:
        print(f"获取当前状态截图...")
    
    screenshot_success = take_screenshot(vscode_window['id'], args.output, args.verbose)
    if not screenshot_success:
        print("警告: 截图失败")
    
    # 如果只是截图模式，直接返回
    if args.screenshot_only:
        print(f"截图已保存: {args.output}")
        print("请手动检查VSCode左边的augment对话框状态")
        return 0
    
    # 执行continue操作
    if args.verbose:
        print("执行continue操作...")
    
    continue_success = send_continue_command(vscode_window['id'], args.verbose)
    
    if continue_success:
        if args.verbose:
            print("✅ continue命令发送成功")
        else:
            print("Continue command sent successfully")
        
        # 短暂延迟后再次截图确认结果
        time.sleep(1)
        final_screenshot = args.output.replace('.png', '_after.png')
        # 确保after截图也在tools目录
        if not final_screenshot.startswith('tools/'):
            final_screenshot = 'tools/' + os.path.basename(final_screenshot)
        take_screenshot(vscode_window['id'], final_screenshot, args.verbose)
        
        if args.verbose:
            print(f"操作后截图: {final_screenshot}")
        
        return 0
    else:
        print("❌ continue命令发送失败")
        return 1

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code) 