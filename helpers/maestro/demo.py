#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import time
import argparse
from pathlib import Path

# 添加父目录到路径
sys.path.append(str(Path(__file__).parent))

# 导入助理管理器和任务管理器
from assistant_manager import AssistantManager
from task_manager import TaskManager

def demo_assistant_manager(window_title="Visual Studio Code"):
    """演示助理管理器的基本功能"""
    print("=== 演示助理管理器 ===")
    
    # 创建助理管理器
    assistant = AssistantManager(window_title=window_title)
    
    if not assistant.hwnd:
        print(f"未找到窗口: {window_title}")
        return
    
    # 发送消息并等待响应
    message = "你好，请简单介绍一下Maestro工具的功能。"
    print(f"\n发送消息: {message}")
    
    assistant.send_message(message)
    response = assistant.wait_for_response(timeout=30)
    
    if response:
        print(f"\n助理响应: {response[:200]}...")
    else:
        print("\n助理未响应")
    
    # 再发送一条消息
    message = "请列出Maestro工具的主要模块。"
    print(f"\n发送消息: {message}")
    
    assistant.send_message(message)
    response = assistant.wait_for_response(timeout=30)
    
    if response:
        print(f"\n助理响应: {response[:200]}...")
    else:
        print("\n助理未响应")
    
    # 导出对话
    output_file = "demo_conversation.json"
    assistant.export_conversation(output_file)
    print(f"\n对话已导出到: {output_file}")

def demo_task_manager(window_title="Visual Studio Code"):
    """演示任务管理器的基本功能"""
    print("\n=== 演示任务管理器 ===")
    
    # 创建任务管理器
    tasks_file = "demo_tasks.json"
    output_md = "demo_tasks.md"
    manager = TaskManager(
        tasks_file=tasks_file,
        output_md=output_md,
        assistant_window=window_title
    )
    
    # 添加任务
    print("\n添加任务...")
    manager.add_task("分析VSCode窗口中的UI元素", priority=3)
    manager.add_task("识别对话区域和输入框", priority=2)
    manager.add_task("实现自动回复功能", priority=1)
    
    # 更新Markdown
    manager.update_markdown()
    print(f"任务已添加并更新到: {output_md}")
    
    # 执行第一个任务
    print("\n执行第一个任务...")
    task = manager.get_next_task()
    if task:
        manager.execute_task(task.id)
    
    # 更新任务状态
    print("\n手动更新任务状态...")
    tasks = list(manager.tasks.values())
    if len(tasks) >= 2:
        manager.update_task(tasks[1].id, status="进行中")
    
    # 再次更新Markdown
    manager.update_markdown()
    print(f"任务状态已更新到: {output_md}")

def demo_combined_workflow(window_title="Visual Studio Code"):
    """演示完整工作流程"""
    print("\n=== 演示完整工作流程 ===")
    
    # 创建任务管理器
    tasks_file = "workflow_tasks.json"
    output_md = "docs/cursor_running.md"
    manager = TaskManager(
        tasks_file=tasks_file,
        output_md=output_md,
        assistant_window=window_title
    )
    
    # 添加一系列任务
    print("\n添加任务序列...")
    manager.add_task("分析当前项目结构", priority=5)
    task2 = manager.add_task("识别主要功能模块", priority=4)
    task3 = manager.add_task("提出改进建议", priority=3, dependencies=[task2.id])
    task4 = manager.add_task("实现自动化测试", priority=2, dependencies=[task3.id])
    manager.add_task("生成项目文档", priority=1, dependencies=[task4.id])
    
    # 更新Markdown
    manager.update_markdown()
    print(f"工作流任务已添加并更新到: {output_md}")
    
    # 执行第一个任务
    print("\n执行第一个任务...")
    task = manager.get_next_task()
    if task:
        manager.execute_task(task.id)
    
    # 查看下一个任务
    next_task = manager.get_next_task()
    if next_task:
        print(f"\n下一个任务: {next_task}")
    else:
        print("\n没有更多可执行的任务")

def main():
    parser = argparse.ArgumentParser(description="Maestro工具演示")
    parser.add_argument("--window", "-w", default="Visual Studio Code", help="窗口标题")
    parser.add_argument("--mode", "-m", choices=["assistant", "task", "workflow", "all"], 
                        default="all", help="演示模式")
    
    args = parser.parse_args()
    
    if args.mode == "assistant" or args.mode == "all":
        demo_assistant_manager(args.window)
    
    if args.mode == "task" or args.mode == "all":
        demo_task_manager(args.window)
    
    if args.mode == "workflow" or args.mode == "all":
        demo_combined_workflow(args.window)

if __name__ == "__main__":
    main() 