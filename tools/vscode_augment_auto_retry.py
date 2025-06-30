#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
VSCode Augment Auto Retry Tool
检测VSCode augment对话框中的"We encountered an issue sending your message. Please try again"
并自动点击"try again"链接

Usage:
    python tools/vscode_augment_auto_retry.py
    python tools/vscode_augment_auto_retry.py --verbose
    python tools/vscode_augment_auto_retry.py --monitor
"""

import sys
import os
import argparse
import time
from datetime import datetime
import win32gui
import win32con
import win32api

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

def take_screenshot_and_check(window_id, verbose=False):
    """获取截图并检查是否包含错误信息"""
    controller = UIController()
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"tools/vscode_retry_check_{timestamp}.png"
    
    try:
        result = controller.take_screenshot(window_id, filename)
        if verbose:
            print(f"截图保存: {result}")
        return filename
    except Exception as e:
        if verbose:
            print(f"截图失败: {e}")
        return None

def detect_error_message_by_analysis(screenshot_path, verbose=False):
    """通过图像分析检测错误信息（简单的像素分析）"""
    try:
        from PIL import Image
        
        image = Image.open(screenshot_path)
        width, height = image.size
        
        # 转换为RGB模式
        if image.mode != 'RGB':
            image = image.convert('RGB')
        
        # 分析底部区域（错误信息通常在对话框底部）
        bottom_region_height = min(200, height // 4)
        bottom_region = image.crop((0, height - bottom_region_height, width, height))
        
        # 简单的颜色分析：错误信息通常有特定的颜色模式
        pixels = list(bottom_region.getdata())
        
        # 检查是否有较多的红色或警告色调
        red_pixels = 0
        blue_pixels = 0  # "try again"链接通常是蓝色
        
        for r, g, b in pixels:
            # 检测红色调（错误信息）
            if r > 150 and g < 100 and b < 100:
                red_pixels += 1
            # 检测蓝色调（链接）
            if b > 150 and r < 100 and g < 150:
                blue_pixels += 1
        
        total_pixels = len(pixels)
        red_ratio = red_pixels / total_pixels if total_pixels > 0 else 0
        blue_ratio = blue_pixels / total_pixels if total_pixels > 0 else 0
        
        # 如果有一定比例的红色和蓝色像素，可能存在错误信息和重试链接
        has_error_colors = red_ratio > 0.001 and blue_ratio > 0.001
        
        if verbose:
            print(f"颜色分析: 红色比例={red_ratio:.4f}, 蓝色比例={blue_ratio:.4f}")
            print(f"可能包含错误信息: {has_error_colors}")
        
        return has_error_colors
        
    except Exception as e:
        if verbose:
            print(f"图像分析失败: {e}")
        return False

def click_try_again_area(window_id, verbose=False):
    """点击try again区域"""
    controller = UIController()
    
    try:
        # 获取窗口信息
        hwnd = int(window_id)
        rect = win32gui.GetWindowRect(hwnd)
        window_width = rect[2] - rect[0]
        window_height = rect[3] - rect[1]
        
        # 基于截图分析，try again链接通常在底部中央区域
        # 从图片看，大约在底部200像素范围内，水平居中偏左
        click_x = window_width // 2 - 100  # 稍微偏左
        click_y = window_height - 100      # 底部100像素处
        
        if verbose:
            print(f"尝试点击位置: ({click_x}, {click_y})")
        
        # 首先确保VSCode窗口获得焦点
        try:
            win32gui.SetForegroundWindow(hwnd)
            time.sleep(0.3)
        except:
            pass
        
        # 点击try again区域
        # 使用相对坐标点击
        result = controller.send_keys(window_id, "tab")  # 先按Tab键导航
        time.sleep(0.2)
        result = controller.send_keys(window_id, "enter")  # 然后按回车
        
        if verbose:
            print(f"键盘导航结果: {result}")
        
        # 备用方法：直接在预估位置点击
        try:
            # 计算绝对坐标
            abs_x = rect[0] + click_x
            abs_y = rect[1] + click_y
            
            # 移动鼠标到位置并点击
            win32api.SetCursorPos((abs_x, abs_y))
            time.sleep(0.1)
            win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
            time.sleep(0.05)
            win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
            
            if verbose:
                print(f"鼠标点击位置: ({abs_x}, {abs_y})")
        
        except Exception as e:
            if verbose:
                print(f"鼠标点击失败: {e}")
        
        return True
        
    except Exception as e:
        if verbose:
            print(f"点击try again失败: {e}")
        return False

def check_and_retry_once(verbose=False):
    """检查一次并在需要时重试"""
    if verbose:
        print("=== VSCode Augment Auto Retry Check ===")
    
    # 查找VSCode窗口
    vscode_window = find_vscode_window()
    if not vscode_window:
        print("错误: 未找到VSCode窗口")
        return False
    
    if verbose:
        print(f"找到VSCode窗口: {vscode_window['title']} (ID: {vscode_window['id']})")
    
    # 获取截图
    screenshot_path = take_screenshot_and_check(vscode_window['id'], verbose)
    if not screenshot_path:
        print("错误: 无法获取截图")
        return False
    
    # 检测错误信息
    has_error = detect_error_message_by_analysis(screenshot_path, verbose)
    
    if has_error:
        if verbose:
            print("🔍 检测到可能的错误信息，尝试点击try again...")
        else:
            print("检测到错误信息，正在重试...")
        
        # 点击try again
        success = click_try_again_area(vscode_window['id'], verbose)
        
        if success:
            time.sleep(1)  # 等待一秒
            
            # 再次截图确认
            after_screenshot = take_screenshot_and_check(vscode_window['id'], verbose)
            if verbose and after_screenshot:
                print(f"重试后截图: {after_screenshot}")
            
            print("✅ 已尝试点击try again")
            return True
        else:
            print("❌ 点击try again失败")
            return False
    else:
        if verbose:
            print("✅ 未检测到错误信息")
        else:
            print("无需重试")
        return True

def monitor_mode(check_interval=10, verbose=False):
    """监控模式：持续检查并自动重试"""
    print(f"🔄 开始监控模式，每{check_interval}秒检查一次...")
    print("按 Ctrl+C 停止监控")
    
    try:
        while True:
            try:
                check_and_retry_once(verbose)
                if verbose:
                    print(f"等待{check_interval}秒后下次检查...")
                time.sleep(check_interval)
            except KeyboardInterrupt:
                print("\n🛑 监控已停止")
                break
            except Exception as e:
                print(f"检查过程中出错: {e}")
                time.sleep(check_interval)
                
    except KeyboardInterrupt:
        print("\n🛑 监控已停止")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='VSCode Augment Auto Retry Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python tools/vscode_augment_auto_retry.py                    # 检查一次并重试
  python tools/vscode_augment_auto_retry.py --verbose          # 详细输出
  python tools/vscode_augment_auto_retry.py --monitor          # 监控模式
  python tools/vscode_augment_auto_retry.py --monitor --interval 5  # 5秒间隔监控
        """
    )
    
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='详细输出')
    parser.add_argument('--monitor', '-m', action='store_true',
                       help='监控模式：持续检查并自动重试')
    parser.add_argument('--interval', '-i', type=int, default=10,
                       help='监控模式的检查间隔（秒），默认10秒')
    
    args = parser.parse_args()
    
    if args.monitor:
        monitor_mode(args.interval, args.verbose)
    else:
        success = check_and_retry_once(args.verbose)
        return 0 if success else 1

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code) 