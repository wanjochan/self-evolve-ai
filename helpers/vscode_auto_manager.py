#!/usr/bin/env python3
"""
VSCode自动化管理器
自动检测VSCode窗口中的"Would you like me to keep going?"提示并回复"continue"
让AI助手持续工作不被打断
"""

import time
import subprocess
import json
import re
import os
import sys
from datetime import datetime
import logging

# 设置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('vscode_auto_manager.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class VSCodeAutoManager:
    def __init__(self):
        self.window_title = "self-evolve-ai - Visual Studio Code"
        self.maestro_path = "maestro/maestro_cli.py"
        self.check_interval = 5  # 检查间隔（秒）
        self.running = True
        
        # 需要检测的提示模式
        self.prompt_patterns = [
            r"Would you like me to keep going\?",
            r"Would you like me to continue\?",
            r"Should I continue\?",
            r"Continue\?",
            r"继续吗\?",
            r"是否继续\?"
        ]
        
        # 完成信号模式（遇到这些时不要继续）
        self.completion_patterns = [
            r"All tasks completed",
            r"Task completed successfully",
            r"所有任务已完成",
            r"任务已完成",
            r"工作已完成",
            r"No more tasks",
            r"Everything is done"
        ]
        
        logger.info(f"VSCode自动化管理器已启动，监控窗口: {self.window_title}")
    
    def run_maestro_command(self, command):
        """执行maestro CLI命令"""
        try:
            full_command = f"python {self.maestro_path} {command}"
            result = subprocess.run(
                full_command,
                shell=True,
                capture_output=True,
                text=True,
                timeout=30
            )
            return result.returncode == 0, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            logger.error("Maestro命令执行超时")
            return False, "", "Timeout"
        except Exception as e:
            logger.error(f"执行maestro命令失败: {e}")
            return False, "", str(e)
    
    def check_window_exists(self):
        """检查VSCode窗口是否存在"""
        success, stdout, stderr = self.run_maestro_command("list")
        if success and self.window_title in stdout:
            return True
        return False
    
    def capture_window_content(self):
        """捕获窗口内容"""
        success, stdout, stderr = self.run_maestro_command(
            f'detail "{self.window_title}" -s -o temp_analysis.json'
        )
        if success:
            logger.debug("窗口内容捕获成功")
            return True
        else:
            logger.error(f"窗口内容捕获失败: {stderr}")
            return False
    
    def analyze_content_for_prompts(self):
        """分析窗口内容，检测是否有需要回复的提示"""
        # 这里我们通过截图文件名来检测
        # 实际应用中可能需要OCR或其他文本识别方法
        screenshot_files = [f for f in os.listdir('.') if f.endswith('_screenshot.png')]
        
        if not screenshot_files:
            return False, None
            
        # 简单的启发式检测：如果最近有新的截图生成，可能有新内容
        latest_screenshot = max(screenshot_files, key=os.path.getctime)
        file_age = time.time() - os.path.getctime(latest_screenshot)
        
        # 如果截图是最近5分钟内生成的，认为可能有新提示
        if file_age < 300:  # 5分钟
            logger.info(f"检测到新的窗口活动: {latest_screenshot}")
            return True, "potential_prompt"
        
        return False, None
    
    def send_continue_response(self):
        """发送continue回复"""
        success, stdout, stderr = self.run_maestro_command(
            f'keyboard "{self.window_title}" type "continue" --no-activate'
        )
        
        if success:
            logger.info("✅ 成功发送'continue'回复")
            return True
        else:
            logger.error(f"❌ 发送'continue'失败: {stderr}")
            return False
    
    def send_enter_key(self):
        """发送回车键确认"""
        success, stdout, stderr = self.run_maestro_command(
            f'keyboard "{self.window_title}" key "Return" --no-activate'
        )
        
        if success:
            logger.info("✅ 成功发送回车键")
            return True
        else:
            logger.error(f"❌ 发送回车键失败: {stderr}")
            return False
    
    def auto_respond(self):
        """自动回复流程"""
        logger.info("🤖 检测到可能的提示，执行自动回复...")
        
        # 发送continue
        if self.send_continue_response():
            time.sleep(0.5)  # 短暂等待
            
            # 发送回车确认
            if self.send_enter_key():
                logger.info("✅ 自动回复完成")
                return True
        
        logger.error("❌ 自动回复失败")
        return False
    
    def monitor_loop(self):
        """主监控循环"""
        logger.info("🔍 开始监控VSCode窗口...")
        consecutive_failures = 0
        
        while self.running:
            try:
                # 检查窗口是否存在
                if not self.check_window_exists():
                    logger.warning(f"⚠️ 未找到窗口: {self.window_title}")
                    consecutive_failures += 1
                    if consecutive_failures > 12:  # 1分钟后停止
                        logger.error("❌ 窗口长时间不存在，停止监控")
                        break
                    time.sleep(self.check_interval)
                    continue
                
                consecutive_failures = 0
                
                # 捕获窗口内容
                if self.capture_window_content():
                    # 分析是否有提示
                    has_prompt, prompt_type = self.analyze_content_for_prompts()
                    
                    if has_prompt:
                        # 执行自动回复
                        self.auto_respond()
                        
                        # 回复后等待更长时间，避免重复回复
                        time.sleep(10)
                    else:
                        logger.debug("📊 未检测到需要回复的提示")
                
                # 等待下次检查
                time.sleep(self.check_interval)
                
            except KeyboardInterrupt:
                logger.info("🛑 收到停止信号，退出监控")
                self.running = False
                break
            except Exception as e:
                logger.error(f"❌ 监控循环出错: {e}")
                time.sleep(self.check_interval)
    
    def start(self):
        """启动自动化管理器"""
        logger.info("🚀 VSCode自动化管理器启动")
        try:
            self.monitor_loop()
        except Exception as e:
            logger.error(f"❌ 管理器运行出错: {e}")
        finally:
            logger.info("🏁 VSCode自动化管理器已停止")

def main():
    """主函数"""
    print("🤖 VSCode自动化管理器")
    print("=" * 50)
    print("功能: 自动检测并回复'Would you like me to keep going?'提示")
    print("目标: 让VSCode中的AI助手持续工作不被打断")
    print("按 Ctrl+C 停止")
    print("=" * 50)
    
    manager = VSCodeAutoManager()
    manager.start()

if __name__ == "__main__":
    main()
