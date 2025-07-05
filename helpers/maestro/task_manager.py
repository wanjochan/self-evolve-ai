#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import time
import json
import logging
from pathlib import Path
import argparse
import re
from datetime import datetime

# 添加父目录到路径
sys.path.append(str(Path(__file__).parent))

# 导入助理管理器
from assistant_manager import AssistantManager

# 设置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger('task_manager')

class Task:
    """任务类"""
    
    STATUS_TODO = "待处理"
    STATUS_IN_PROGRESS = "进行中"
    STATUS_COMPLETED = "已完成"
    STATUS_FAILED = "失败"
    
    def __init__(self, id, description, status=None, priority=0, dependencies=None):
        """初始化任务
        
        Args:
            id: 任务ID
            description: 任务描述
            status: 任务状态
            priority: 优先级（数字越大优先级越高）
            dependencies: 依赖任务ID列表
        """
        self.id = id
        self.description = description
        self.status = status or self.STATUS_TODO
        self.priority = priority
        self.dependencies = dependencies or []
        self.created_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.updated_at = self.created_at
        self.result = None
    
    def update_status(self, status):
        """更新任务状态"""
        self.status = status
        self.updated_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    def to_dict(self):
        """转换为字典"""
        return {
            "id": self.id,
            "description": self.description,
            "status": self.status,
            "priority": self.priority,
            "dependencies": self.dependencies,
            "created_at": self.created_at,
            "updated_at": self.updated_at,
            "result": self.result
        }
    
    @classmethod
    def from_dict(cls, data):
        """从字典创建任务"""
        task = cls(
            id=data["id"],
            description=data["description"],
            status=data["status"],
            priority=data.get("priority", 0),
            dependencies=data.get("dependencies", [])
        )
        task.created_at = data.get("created_at", task.created_at)
        task.updated_at = data.get("updated_at", task.updated_at)
        task.result = data.get("result")
        return task
    
    def __str__(self):
        return f"Task {self.id}: {self.description} [{self.status}]"

class TaskManager:
    """任务管理器"""
    
    def __init__(self, tasks_file=None, output_md=None, assistant_window="Visual Studio Code"):
        """初始化任务管理器
        
        Args:
            tasks_file: 任务文件路径
            output_md: 输出的Markdown文件路径
            assistant_window: 助理窗口标题
        """
        self.tasks_file = tasks_file or "tasks.json"
        self.output_md = output_md or "docs/cursor_running.md"
        self.tasks = {}
        self.assistant = AssistantManager(window_title=assistant_window)
        
        # 加载任务
        self._load_tasks()
    
    def _load_tasks(self):
        """加载任务"""
        if os.path.exists(self.tasks_file):
            try:
                with open(self.tasks_file, 'r', encoding='utf-8') as f:
                    tasks_data = json.load(f)
                
                for task_data in tasks_data:
                    task = Task.from_dict(task_data)
                    self.tasks[task.id] = task
                
                logger.info(f"已加载 {len(self.tasks)} 个任务")
            except Exception as e:
                logger.error(f"加载任务失败: {e}")
        else:
            logger.info(f"任务文件不存在: {self.tasks_file}")
    
    def _save_tasks(self):
        """保存任务"""
        try:
            tasks_data = [task.to_dict() for task in self.tasks.values()]
            with open(self.tasks_file, 'w', encoding='utf-8') as f:
                json.dump(tasks_data, f, ensure_ascii=False, indent=2)
            
            logger.info(f"已保存 {len(self.tasks)} 个任务到 {self.tasks_file}")
        except Exception as e:
            logger.error(f"保存任务失败: {e}")
    
    def add_task(self, description, priority=0, dependencies=None):
        """添加任务
        
        Args:
            description: 任务描述
            priority: 优先级
            dependencies: 依赖任务ID列表
            
        Returns:
            Task: 新添加的任务
        """
        # 生成任务ID
        task_id = f"task_{len(self.tasks) + 1}"
        
        # 创建任务
        task = Task(
            id=task_id,
            description=description,
            priority=priority,
            dependencies=dependencies
        )
        
        # 添加到任务列表
        self.tasks[task_id] = task
        
        # 保存任务
        self._save_tasks()
        
        # 更新Markdown
        self.update_markdown()
        
        logger.info(f"已添加任务: {task}")
        return task
    
    def update_task(self, task_id, status=None, description=None, priority=None, result=None):
        """更新任务
        
        Args:
            task_id: 任务ID
            status: 新状态
            description: 新描述
            priority: 新优先级
            result: 任务结果
            
        Returns:
            bool: 是否成功更新
        """
        if task_id not in self.tasks:
            logger.warning(f"任务不存在: {task_id}")
            return False
        
        task = self.tasks[task_id]
        
        # 更新任务属性
        if status:
            task.update_status(status)
        
        if description:
            task.description = description
        
        if priority is not None:
            task.priority = priority
        
        if result is not None:
            task.result = result
        
        # 保存任务
        self._save_tasks()
        
        # 更新Markdown
        self.update_markdown()
        
        logger.info(f"已更新任务: {task}")
        return True
    
    def get_next_task(self):
        """获取下一个要执行的任务
        
        Returns:
            Task: 下一个任务
        """
        # 筛选待处理的任务
        todo_tasks = [task for task in self.tasks.values() if task.status == Task.STATUS_TODO]
        
        if not todo_tasks:
            logger.info("没有待处理的任务")
            return None
        
        # 检查依赖关系
        executable_tasks = []
        for task in todo_tasks:
            dependencies_met = True
            
            for dep_id in task.dependencies:
                if dep_id not in self.tasks:
                    logger.warning(f"任务 {task.id} 依赖的任务 {dep_id} 不存在")
                    dependencies_met = False
                    break
                
                dep_task = self.tasks[dep_id]
                if dep_task.status != Task.STATUS_COMPLETED:
                    dependencies_met = False
                    break
            
            if dependencies_met:
                executable_tasks.append(task)
        
        if not executable_tasks:
            logger.info("没有可执行的任务（依赖未满足）")
            return None
        
        # 按优先级排序
        executable_tasks.sort(key=lambda t: t.priority, reverse=True)
        
        # 返回优先级最高的任务
        return executable_tasks[0]
    
    def execute_task(self, task_id=None):
        """执行任务
        
        Args:
            task_id: 要执行的任务ID，如果为None则自动选择下一个任务
            
        Returns:
            bool: 是否成功执行
        """
        # 获取要执行的任务
        task = None
        if task_id:
            if task_id in self.tasks:
                task = self.tasks[task_id]
            else:
                logger.warning(f"任务不存在: {task_id}")
                return False
        else:
            task = self.get_next_task()
        
        if not task:
            logger.warning("没有可执行的任务")
            return False
        
        # 更新任务状态为进行中
        self.update_task(task.id, status=Task.STATUS_IN_PROGRESS)
        
        try:
            # 向助理发送任务
            logger.info(f"执行任务: {task}")
            
            # 构建任务消息
            message = f"请执行以下任务：{task.description}"
            
            # 发送消息并等待响应
            self.assistant.send_message(message)
            response = self.assistant.wait_for_response(timeout=300)  # 等待5分钟
            
            if response:
                # 更新任务状态为已完成
                self.update_task(task.id, status=Task.STATUS_COMPLETED, result=response[:500])
                logger.info(f"任务 {task.id} 已完成")
                return True
            else:
                # 更新任务状态为失败
                self.update_task(task.id, status=Task.STATUS_FAILED, result="助理未响应")
                logger.warning(f"任务 {task.id} 执行失败: 助理未响应")
                return False
        
        except Exception as e:
            # 更新任务状态为失败
            self.update_task(task.id, status=Task.STATUS_FAILED, result=str(e))
            logger.error(f"任务 {task.id} 执行出错: {e}")
            return False
    
    def run_all_tasks(self):
        """执行所有待处理的任务"""
        while True:
            task = self.get_next_task()
            if not task:
                break
            
            self.execute_task(task.id)
            
            # 等待一段时间再执行下一个任务
            time.sleep(2)
        
        logger.info("所有任务已执行完毕")
    
    def update_markdown(self):
        """更新Markdown文件"""
        try:
            # 准备Markdown内容
            content = "# 任务状态\n\n"
            content += f"更新时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n"
            
            # 按状态分组
            status_groups = {
                Task.STATUS_IN_PROGRESS: [],
                Task.STATUS_TODO: [],
                Task.STATUS_COMPLETED: [],
                Task.STATUS_FAILED: []
            }
            
            for task in self.tasks.values():
                status_groups[task.status].append(task)
            
            # 进行中的任务
            content += "## 进行中的任务\n\n"
            if status_groups[Task.STATUS_IN_PROGRESS]:
                content += "| ID | 描述 | 优先级 | 更新时间 |\n"
                content += "| --- | --- | --- | --- |\n"
                for task in status_groups[Task.STATUS_IN_PROGRESS]:
                    content += f"| {task.id} | {task.description} | {task.priority} | {task.updated_at} |\n"
            else:
                content += "暂无进行中的任务\n"
            
            content += "\n"
            
            # 待处理的任务
            content += "## 待处理的任务\n\n"
            if status_groups[Task.STATUS_TODO]:
                content += "| ID | 描述 | 优先级 | 依赖 |\n"
                content += "| --- | --- | --- | --- |\n"
                for task in sorted(status_groups[Task.STATUS_TODO], key=lambda t: t.priority, reverse=True):
                    deps = ", ".join(task.dependencies) if task.dependencies else "无"
                    content += f"| {task.id} | {task.description} | {task.priority} | {deps} |\n"
            else:
                content += "暂无待处理的任务\n"
            
            content += "\n"
            
            # 已完成的任务
            content += "## 已完成的任务\n\n"
            if status_groups[Task.STATUS_COMPLETED]:
                content += "| ID | 描述 | 完成时间 |\n"
                content += "| --- | --- | --- |\n"
                for task in sorted(status_groups[Task.STATUS_COMPLETED], key=lambda t: t.updated_at, reverse=True):
                    content += f"| {task.id} | {task.description} | {task.updated_at} |\n"
            else:
                content += "暂无已完成的任务\n"
            
            content += "\n"
            
            # 失败的任务
            content += "## 失败的任务\n\n"
            if status_groups[Task.STATUS_FAILED]:
                content += "| ID | 描述 | 失败原因 | 失败时间 |\n"
                content += "| --- | --- | --- | --- |\n"
                for task in status_groups[Task.STATUS_FAILED]:
                    reason = task.result or "未知原因"
                    content += f"| {task.id} | {task.description} | {reason} | {task.updated_at} |\n"
            else:
                content += "暂无失败的任务\n"
            
            # 创建目录（如果不存在）
            os.makedirs(os.path.dirname(self.output_md), exist_ok=True)
            
            # 写入Markdown文件
            with open(self.output_md, 'w', encoding='utf-8') as f:
                f.write(content)
            
            logger.info(f"已更新Markdown文件: {self.output_md}")
            return True
        
        except Exception as e:
            logger.error(f"更新Markdown失败: {e}")
            return False
    
    def parse_tasks_from_markdown(self, markdown_file):
        """从Markdown文件解析任务
        
        Args:
            markdown_file: Markdown文件路径
            
        Returns:
            bool: 是否成功解析
        """
        if not os.path.exists(markdown_file):
            logger.warning(f"Markdown文件不存在: {markdown_file}")
            return False
        
        try:
            with open(markdown_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 解析待处理的任务
            todo_pattern = r"## 待处理的任务\s+\|\s*ID\s*\|\s*描述\s*\|\s*优先级\s*\|\s*依赖\s*\|\s+\|\s*---\s*\|\s*---\s*\|\s*---\s*\|\s*---\s*\|(.*?)(?=##|\Z)"
            todo_match = re.search(todo_pattern, content, re.DOTALL)
            
            if todo_match:
                todo_content = todo_match.group(1).strip()
                task_pattern = r"\|\s*(\w+)\s*\|\s*(.*?)\s*\|\s*(\d+)\s*\|\s*(.*?)\s*\|"
                
                for match in re.finditer(task_pattern, todo_content):
                    task_id = match.group(1).strip()
                    description = match.group(2).strip()
                    priority = int(match.group(3).strip())
                    deps_str = match.group(4).strip()
                    
                    # 解析依赖
                    dependencies = []
                    if deps_str and deps_str != "无":
                        dependencies = [dep.strip() for dep in deps_str.split(",")]
                    
                    # 如果任务不存在，则添加
                    if task_id not in self.tasks:
                        task = Task(
                            id=task_id,
                            description=description,
                            status=Task.STATUS_TODO,
                            priority=priority,
                            dependencies=dependencies
                        )
                        self.tasks[task_id] = task
                    else:
                        # 更新现有任务
                        task = self.tasks[task_id]
                        task.description = description
                        task.priority = priority
                        task.dependencies = dependencies
            
            # 解析进行中的任务
            in_progress_pattern = r"## 进行中的任务\s+\|\s*ID\s*\|\s*描述\s*\|\s*优先级\s*\|\s*更新时间\s*\|\s+\|\s*---\s*\|\s*---\s*\|\s*---\s*\|\s*---\s*\|(.*?)(?=##|\Z)"
            in_progress_match = re.search(in_progress_pattern, content, re.DOTALL)
            
            if in_progress_match:
                in_progress_content = in_progress_match.group(1).strip()
                task_pattern = r"\|\s*(\w+)\s*\|\s*(.*?)\s*\|\s*(\d+)\s*\|\s*(.*?)\s*\|"
                
                for match in re.finditer(task_pattern, in_progress_content):
                    task_id = match.group(1).strip()
                    description = match.group(2).strip()
                    priority = int(match.group(3).strip())
                    updated_at = match.group(4).strip()
                    
                    # 如果任务不存在，则添加
                    if task_id not in self.tasks:
                        task = Task(
                            id=task_id,
                            description=description,
                            status=Task.STATUS_IN_PROGRESS,
                            priority=priority
                        )
                        task.updated_at = updated_at
                        self.tasks[task_id] = task
                    else:
                        # 更新现有任务
                        task = self.tasks[task_id]
                        task.description = description
                        task.status = Task.STATUS_IN_PROGRESS
                        task.priority = priority
                        task.updated_at = updated_at
            
            # 保存任务
            self._save_tasks()
            
            logger.info(f"从Markdown文件 {markdown_file} 解析了 {len(self.tasks)} 个任务")
            return True
        
        except Exception as e:
            logger.error(f"解析Markdown文件失败: {e}")
            return False

def main():
    parser = argparse.ArgumentParser(description="任务管理器")
    parser.add_argument("--tasks", "-t", default="tasks.json", help="任务文件路径")
    parser.add_argument("--output", "-o", default="docs/cursor_running.md", help="输出的Markdown文件路径")
    parser.add_argument("--window", "-w", default="Visual Studio Code", help="助理窗口标题")
    parser.add_argument("--add", "-a", help="添加新任务")
    parser.add_argument("--priority", "-p", type=int, default=0, help="任务优先级")
    parser.add_argument("--execute", "-e", help="执行指定任务")
    parser.add_argument("--run-all", "-r", action="store_true", help="执行所有待处理的任务")
    parser.add_argument("--import", "-i", dest="import_file", help="从Markdown文件导入任务")
    parser.add_argument("--update", "-u", help="更新任务状态 (格式: task_id:status)")
    
    args = parser.parse_args()
    
    # 创建任务管理器
    manager = TaskManager(
        tasks_file=args.tasks,
        output_md=args.output,
        assistant_window=args.window
    )
    
    # 导入任务
    if args.import_file:
        manager.parse_tasks_from_markdown(args.import_file)
    
    # 添加任务
    if args.add:
        manager.add_task(args.add, priority=args.priority)
    
    # 更新任务状态
    if args.update:
        try:
            task_id, status = args.update.split(":", 1)
            manager.update_task(task_id, status=status)
        except ValueError:
            logger.error(f"无效的更新格式: {args.update}，应为 task_id:status")
    
    # 执行任务
    if args.execute:
        manager.execute_task(args.execute)
    
    # 执行所有任务
    if args.run_all:
        manager.run_all_tasks()
    
    # 如果没有指定操作，则更新Markdown
    if not (args.add or args.execute or args.run_all or args.update or args.import_file):
        manager.update_markdown()

if __name__ == "__main__":
    main() 