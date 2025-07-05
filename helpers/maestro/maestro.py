#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Maestro - 智能助理窗口管理工具
简洁的命令行界面，用于更方便地使用Maestro
"""

import os
import sys
import time
import argparse
from pathlib import Path

# 添加当前目录到路径
sys.path.append(str(Path(__file__).parent))

# 导入Maestro核心模块
from maestro_core import MaestroCore, send_message, execute_task

def cmd_send(args):
    """发送消息命令"""
    result = send_message(args.message, window_title=args.window)
    if result:
        print(f"消息已发送: {args.message[:50]}...")
    else:
        print("发送消息失败")

def cmd_task(args):
    """执行任务命令"""
    result = execute_task(args.task, window_title=args.window, timeout=args.timeout)
    if result:
        print(f"任务已执行: {args.task[:50]}...")
    else:
        print("执行任务失败")

def cmd_interact(args):
    """交互模式命令"""
    maestro = MaestroCore(window_title=args.window, debug_mode=args.debug)
    
    if not maestro.hwnd:
        print(f"未找到窗口: {args.window}")
        return
    
    print(f"进入交互模式，窗口: {maestro.window_title}")
    print("输入消息发送到窗口，输入'exit'或'quit'退出")
    
    while True:
        try:
            message = input("> ")
            if message.lower() in ["exit", "quit"]:
                break
            
            if message.strip():
                result = maestro.send_message(message)
                if result:
                    print("消息已发送")
                    if args.wait:
                        print("等待响应...")
                        maestro.wait_for_response(timeout=args.timeout)
                else:
                    print("发送消息失败")
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"错误: {e}")
    
    print("退出交互模式")

def cmd_click(args):
    """点击命令"""
    maestro = MaestroCore(window_title=args.window, debug_mode=args.debug)
    
    if not maestro.hwnd:
        print(f"未找到窗口: {args.window}")
        return
    
    result = maestro.click(args.x, args.y, button=args.button, double_click=args.double)
    if result:
        print(f"点击位置: ({args.x}, {args.y})")
    else:
        print("点击失败")

def cmd_key(args):
    """按键命令"""
    maestro = MaestroCore(window_title=args.window, debug_mode=args.debug)
    
    if not maestro.hwnd:
        print(f"未找到窗口: {args.window}")
        return
    
    if args.hotkey:
        result = maestro.press_hotkey(args.hotkey)
        if result:
            print(f"按下组合键: {args.hotkey}")
        else:
            print("按下组合键失败")
    else:
        result = maestro.press_key(args.key)
        if result:
            print(f"按下按键: {args.key}")
        else:
            print("按下按键失败")

def cmd_batch(args):
    """批处理命令"""
    maestro = MaestroCore(window_title=args.window, debug_mode=args.debug)
    
    if not maestro.hwnd:
        print(f"未找到窗口: {args.window}")
        return
    
    if not os.path.exists(args.file):
        print(f"文件不存在: {args.file}")
        return
    
    try:
        with open(args.file, "r", encoding="utf-8") as f:
            lines = f.readlines()
        
        for i, line in enumerate(lines):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            
            print(f"执行第 {i+1} 行: {line[:50]}...")
            maestro.send_message(line)
            
            if args.wait:
                print("等待响应...")
                maestro.wait_for_response(timeout=args.timeout)
            
            if i < len(lines) - 1:
                time.sleep(args.interval)
        
        print("批处理完成")
    except Exception as e:
        print(f"批处理错误: {e}")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description="Maestro - 智能助理窗口管理工具")
    parser.add_argument("--window", "-w", default="Visual Studio Code", help="窗口标题")
    parser.add_argument("--debug", "-d", action="store_true", help="启用调试模式")
    
    subparsers = parser.add_subparsers(dest="command", help="子命令")
    
    # 发送消息命令
    send_parser = subparsers.add_parser("send", help="发送消息")
    send_parser.add_argument("message", help="要发送的消息")
    send_parser.set_defaults(func=cmd_send)
    
    # 执行任务命令
    task_parser = subparsers.add_parser("task", help="执行任务")
    task_parser.add_argument("task", help="要执行的任务")
    task_parser.add_argument("--timeout", "-t", type=int, default=60, help="等待响应的超时时间（秒）")
    task_parser.set_defaults(func=cmd_task)
    
    # 交互模式命令
    interact_parser = subparsers.add_parser("interact", help="交互模式")
    interact_parser.add_argument("--wait", "-t", action="store_true", help="等待响应")
    interact_parser.add_argument("--timeout", "-o", type=int, default=60, help="等待响应的超时时间（秒）")
    interact_parser.set_defaults(func=cmd_interact)
    
    # 点击命令
    click_parser = subparsers.add_parser("click", help="点击位置")
    click_parser.add_argument("x", type=int, help="X坐标")
    click_parser.add_argument("y", type=int, help="Y坐标")
    click_parser.add_argument("--button", "-b", default="left", choices=["left", "right", "middle"], help="鼠标按钮")
    click_parser.add_argument("--double", "-d", action="store_true", help="双击")
    click_parser.set_defaults(func=cmd_click)
    
    # 按键命令
    key_parser = subparsers.add_parser("key", help="按键")
    key_group = key_parser.add_mutually_exclusive_group(required=True)
    key_group.add_argument("--key", "-k", help="按键名称")
    key_group.add_argument("--hotkey", "-h", help="组合键")
    key_parser.set_defaults(func=cmd_key)
    
    # 批处理命令
    batch_parser = subparsers.add_parser("batch", help="批处理")
    batch_parser.add_argument("file", help="批处理文件")
    batch_parser.add_argument("--wait", "-t", action="store_true", help="等待响应")
    batch_parser.add_argument("--timeout", "-o", type=int, default=60, help="等待响应的超时时间（秒）")
    batch_parser.add_argument("--interval", "-i", type=float, default=1.0, help="消息间隔时间（秒）")
    batch_parser.set_defaults(func=cmd_batch)
    
    args = parser.parse_args()
    
    if args.command is None:
        parser.print_help()
    else:
        args.func(args)

if __name__ == "__main__":
    main() 