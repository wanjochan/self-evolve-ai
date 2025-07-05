#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import time
import json
import argparse
from pathlib import Path
import datetime

# 添加当前目录到路径
sys.path.append(str(Path(__file__).parent))

# 导入交互器
from interact_with_augment import AugmentInteractor

class TaskAutomator:
    """任务自动化执行器"""
    
    def __init__(self, tasks_file="tasks.json", output_md="../../docs/cursor_running.md", window_title="Visual Studio Code"):
        """初始化任务自动化执行器
        
        Args:
            tasks_file: 任务文件路径
            output_md: 输出的Markdown文件路径
            window_title: VSCode窗口标题
        """
        self.tasks_file = tasks_file
        self.output_md = output_md
        self.window_title = window_title
        
        # 创建交互器
        self.interactor = AugmentInteractor(window_title=window_title)
        
        # 加载任务
        self.tasks = self._load_tasks()
    
    def _load_tasks(self):
        """加载任务"""
        if os.path.exists(self.tasks_file):
            try:
                with open(self.tasks_file, 'r', encoding='utf-8') as f:
                    return json.load(f)
            except Exception as e:
                print(f"加载任务失败: {e}")
                return []
        else:
            print(f"任务文件不存在: {self.tasks_file}")
            return []
    
    def _save_tasks(self):
        """保存任务"""
        try:
            with open(self.tasks_file, 'w', encoding='utf-8') as f:
                json.dump(self.tasks, f, ensure_ascii=False, indent=2)
            print(f"已保存任务到 {self.tasks_file}")
        except Exception as e:
            print(f"保存任务失败: {e}")
    
    def add_task(self, description, status="待处理", priority=0):
        """添加任务
        
        Args:
            description: 任务描述
            status: 任务状态
            priority: 优先级
        """
        task = {
            "id": f"task_{len(self.tasks) + 1}",
            "description": description,
            "status": status,
            "priority": priority,
            "created_at": datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "updated_at": datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        }
        
        self.tasks.append(task)
        self._save_tasks()
        self.update_markdown()
        
        print(f"已添加任务: {task['id']} - {description}")
        return task
    
    def update_task(self, task_id, status=None, description=None, priority=None):
        """更新任务
        
        Args:
            task_id: 任务ID
            status: 新状态
            description: 新描述
            priority: 新优先级
        """
        for task in self.tasks:
            if task["id"] == task_id:
                if status:
                    task["status"] = status
                
                if description:
                    task["description"] = description
                
                if priority is not None:
                    task["priority"] = priority
                
                task["updated_at"] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                
                self._save_tasks()
                self.update_markdown()
                
                print(f"已更新任务: {task_id}")
                return True
        
        print(f"任务不存在: {task_id}")
        return False
    
    def get_next_task(self):
        """获取下一个要执行的任务"""
        # 筛选待处理的任务
        todo_tasks = [t for t in self.tasks if t["status"] == "待处理"]
        
        if not todo_tasks:
            print("没有待处理的任务")
            return None
        
        # 按优先级排序
        todo_tasks.sort(key=lambda t: t["priority"], reverse=True)
        
        # 返回优先级最高的任务
        return todo_tasks[0]
    
    def execute_task(self, task_id=None):
        """执行任务
        
        Args:
            task_id: 要执行的任务ID，如果为None则自动选择下一个任务
        """
        # 获取要执行的任务
        task = None
        if task_id:
            task = next((t for t in self.tasks if t["id"] == task_id), None)
            if not task:
                print(f"任务不存在: {task_id}")
                return False
        else:
            task = self.get_next_task()
        
        if not task:
            print("没有可执行的任务")
            return False
        
        # 更新任务状态为进行中
        self.update_task(task["id"], status="进行中")
        
        # 构建任务消息
        message = f"请执行以下任务：{task['description']}"
        
        # 发送消息
        print(f"执行任务: {task['id']} - {task['description']}")
        self.interactor.send_message(message)
        
        # 等待一段时间，让用户手动检查结果
        print("任务已发送，请检查结果...")
        
        # 更新任务状态为已完成
        self.update_task(task["id"], status="已完成")
        
        return True
    
    def run_all_tasks(self):
        """执行所有待处理的任务"""
        while True:
            task = self.get_next_task()
            if not task:
                break
            
            self.execute_task(task["id"])
            
            # 等待一段时间再执行下一个任务
            time.sleep(2)
        
        print("所有任务已执行完毕")
    
    def update_markdown(self):
        """更新Markdown文件"""
        try:
            # 准备Markdown内容
            content = "# 任务状态\n\n"
            content += f"更新时间: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n"
            
            # 按状态分组
            status_groups = {
                "进行中": [],
                "待处理": [],
                "已完成": [],
                "失败": []
            }
            
            for task in self.tasks:
                status = task.get("status", "待处理")
                if status in status_groups:
                    status_groups[status].append(task)
            
            # 进行中的任务
            content += "## 进行中的任务\n\n"
            if status_groups["进行中"]:
                content += "| ID | 描述 | 优先级 | 更新时间 |\n"
                content += "| --- | --- | --- | --- |\n"
                for task in status_groups["进行中"]:
                    content += f"| {task['id']} | {task['description']} | {task['priority']} | {task['updated_at']} |\n"
            else:
                content += "暂无进行中的任务\n"
            
            content += "\n"
            
            # 待处理的任务
            content += "## 待处理的任务\n\n"
            if status_groups["待处理"]:
                content += "| ID | 描述 | 优先级 | 依赖 |\n"
                content += "| --- | --- | --- | --- |\n"
                for task in sorted(status_groups["待处理"], key=lambda t: t["priority"], reverse=True):
                    deps = task.get("dependencies", [])
                    deps_str = ", ".join(deps) if deps else "无"
                    content += f"| {task['id']} | {task['description']} | {task['priority']} | {deps_str} |\n"
            else:
                content += "暂无待处理的任务\n"
            
            content += "\n"
            
            # 已完成的任务
            content += "## 已完成的任务\n\n"
            if status_groups["已完成"]:
                content += "| ID | 描述 | 完成时间 |\n"
                content += "| --- | --- | --- |\n"
                for task in sorted(status_groups["已完成"], key=lambda t: t["updated_at"], reverse=True):
                    content += f"| {task['id']} | {task['description']} | {task['updated_at']} |\n"
            else:
                content += "暂无已完成的任务\n"
            
            content += "\n"
            
            # 失败的任务
            content += "## 失败的任务\n\n"
            if status_groups["失败"]:
                content += "| ID | 描述 | 失败原因 | 失败时间 |\n"
                content += "| --- | --- | --- | --- |\n"
                for task in status_groups["失败"]:
                    reason = task.get("result", "未知原因")
                    content += f"| {task['id']} | {task['description']} | {reason} | {task['updated_at']} |\n"
            else:
                content += "暂无失败的任务\n"
            
            # 创建目录（如果不存在）
            os.makedirs(os.path.dirname(self.output_md), exist_ok=True)
            
            # 写入Markdown文件
            with open(self.output_md, 'w', encoding='utf-8') as f:
                f.write(content)
            
            print(f"已更新Markdown文件: {self.output_md}")
            return True
        
        except Exception as e:
            print(f"更新Markdown失败: {e}")
            return False

def main():
    parser = argparse.ArgumentParser(description="任务自动化执行器")
    parser.add_argument("--tasks", "-t", default="tasks.json", help="任务文件路径")
    parser.add_argument("--output", "-o", default="../../docs/cursor_running.md", help="输出的Markdown文件路径")
    parser.add_argument("--window", "-w", default="Visual Studio Code", help="VSCode窗口标题")
    parser.add_argument("--add", "-a", help="添加新任务")
    parser.add_argument("--priority", "-p", type=int, default=0, help="任务优先级")
    parser.add_argument("--execute", "-e", help="执行指定任务")
    parser.add_argument("--run-all", "-r", action="store_true", help="执行所有待处理的任务")
    parser.add_argument("--update", "-u", help="更新任务状态 (格式: task_id:status)")
    
    args = parser.parse_args()
    
    # 创建任务自动化执行器
    automator = TaskAutomator(
        tasks_file=args.tasks,
        output_md=args.output,
        window_title=args.window
    )
    
    # 添加任务
    if args.add:
        automator.add_task(args.add, priority=args.priority)
    
    # 更新任务状态
    if args.update:
        try:
            task_id, status = args.update.split(":", 1)
            automator.update_task(task_id, status=status)
        except ValueError:
            print(f"无效的更新格式: {args.update}，应为 task_id:status")
    
    # 执行任务
    if args.execute:
        automator.execute_task(args.execute)
    
    # 执行所有任务
    if args.run_all:
        automator.run_all_tasks()
    
    # 如果没有指定操作，则更新Markdown
    if not (args.add or args.execute or args.run_all or args.update):
        automator.update_markdown()

if __name__ == "__main__":
    main() 