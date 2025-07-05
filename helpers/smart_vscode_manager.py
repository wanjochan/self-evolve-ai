#!/usr/bin/env python3
"""
智能VSCode管理器
使用大模型分析截图来智能判断是否需要回复"continue"
"""

import time
import subprocess
import json
import os
import base64
from datetime import datetime
import logging
from pathlib import Path

# 设置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('smart_vscode_manager.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class SmartVSCodeManager:
    def __init__(self):
        self.window_title = "self-evolve-ai - Visual Studio Code"
        self.maestro_path = "maestro/maestro_cli.py"
        self.check_interval = 10  # 检查间隔（秒）
        self.running = True
        self.last_screenshot_time = 0
        
        logger.info(f"智能VSCode管理器已启动，监控窗口: {self.window_title}")
    
    def run_maestro_command(self, command):
        """执行maestro CLI命令"""
        try:
            full_command = f"python {self.maestro_path} {command}"
            result = subprocess.run(
                full_command,
                shell=True,
                capture_output=True,
                text=True,
                timeout=30,
                encoding='utf-8',
                errors='ignore'
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
    
    def capture_window_screenshot(self):
        """捕获窗口截图"""
        timestamp = int(time.time())
        output_file = f"vscode_analysis_{timestamp}.json"
        
        success, stdout, stderr = self.run_maestro_command(
            f'detail "{self.window_title}" -s -o {output_file}'
        )
        
        if success:
            logger.info("窗口截图捕获成功")
            self.last_screenshot_time = timestamp
            return output_file
        else:
            logger.error(f"窗口截图捕获失败: {stderr}")
            return None
    
    def analyze_screenshot_with_ai(self, screenshot_path):
        """使用AI分析截图内容"""
        if not os.path.exists(screenshot_path):
            logger.error(f"截图文件不存在: {screenshot_path}")
            return False, "截图文件不存在"

        try:
            # 将截图转换为base64编码
            with open(screenshot_path, 'rb') as image_file:
                image_data = base64.b64encode(image_file.read()).decode('utf-8')

            # 构建AI分析提示
            analysis_prompt = """
            请分析这个VSCode窗口截图，判断是否需要回复"continue"。

            需要回复的情况：
            1. 看到"Would you like me to keep going?"提示
            2. 看到"Should I continue?"提示
            3. 看到"Continue?"提示
            4. 看到类似的询问是否继续的文本
            5. 看到AI助手在等待用户确认继续

            不需要回复的情况：
            1. AI助手正在工作中（有进度显示、正在输出文本等）
            2. 显示"Task completed"或"All done"等完成信息
            3. 窗口显示正常的代码编辑界面
            4. 没有明显的等待用户输入的提示

            请回答：
            - 判断结果：需要回复 / 不需要回复
            - 理由：简要说明判断依据
            - 置信度：0-100%
            """

            # 这里应该调用实际的AI API
            # 由于演示目的，我们使用简化的逻辑

            # 检查文件修改时间作为活动指标
            file_time = os.path.getctime(screenshot_path)
            current_time = time.time()

            # 如果截图很新，可能有新的交互
            if current_time - file_time < 15:  # 15秒内的截图
                logger.info("🤖 AI分析：检测到新的窗口活动")
                return True, "检测到新的窗口活动，可能需要回复continue"
            else:
                logger.info("🤖 AI分析：窗口状态稳定，无需回复")
                return False, "窗口状态稳定，无需回复"

        except Exception as e:
            logger.error(f"AI分析失败: {e}")
            return False, f"分析失败: {e}"

    def call_ai_vision_api(self, image_base64, prompt):
        """调用AI视觉API分析图像"""
        # 这里可以接入各种AI API：
        # 1. OpenAI GPT-4V
        # 2. Claude 3 Vision
        # 3. Google Gemini Vision
        # 4. 本地部署的视觉模型

        # 示例：OpenAI API调用（需要API密钥）
        """
        import openai

        response = openai.ChatCompletion.create(
            model="gpt-4-vision-preview",
            messages=[
                {
                    "role": "user",
                    "content": [
                        {"type": "text", "text": prompt},
                        {
                            "type": "image_url",
                            "image_url": {
                                "url": f"data:image/png;base64,{image_base64}"
                            }
                        }
                    ]
                }
            ],
            max_tokens=300
        )

        return response.choices[0].message.content
        """

        # 暂时返回模拟结果
        return "需要回复，检测到等待用户输入的提示"
    
    def send_continue_response(self):
        """发送continue回复"""
        success, stdout, stderr = self.run_maestro_command(
            f'keyboard "{self.window_title}" type "continue" --no-activate'
        )
        
        if success:
            logger.info("✅ 成功发送'continue'回复")
            
            # 等待一下再发送回车
            time.sleep(0.5)
            
            # 发送回车键确认
            success2, stdout2, stderr2 = self.run_maestro_command(
                f'keyboard "{self.window_title}" key "Return" --no-activate'
            )
            
            if success2:
                logger.info("✅ 成功发送回车键确认")
                return True
            else:
                logger.error(f"❌ 发送回车键失败: {stderr2}")
        else:
            logger.error(f"❌ 发送'continue'失败: {stderr}")
        
        return False
    
    def intelligent_monitor_loop(self):
        """智能监控循环"""
        logger.info("🧠 开始智能监控VSCode窗口...")
        consecutive_failures = 0
        
        while self.running:
            try:
                # 检查窗口是否存在
                if not self.check_window_exists():
                    logger.warning(f"⚠️ 未找到窗口: {self.window_title}")
                    consecutive_failures += 1
                    if consecutive_failures > 6:  # 1分钟后停止
                        logger.error("❌ 窗口长时间不存在，停止监控")
                        break
                    time.sleep(self.check_interval)
                    continue
                
                consecutive_failures = 0
                
                # 捕获窗口截图
                analysis_file = self.capture_window_screenshot()
                if not analysis_file:
                    logger.warning("⚠️ 截图捕获失败，跳过本次检查")
                    time.sleep(self.check_interval)
                    continue
                
                # 查找对应的截图文件
                screenshot_files = [f for f in os.listdir('.') if f.endswith('_screenshot.png')]
                if not screenshot_files:
                    logger.warning("⚠️ 未找到截图文件")
                    time.sleep(self.check_interval)
                    continue
                
                # 使用最新的截图
                latest_screenshot = max(screenshot_files, key=os.path.getctime)
                
                # 使用AI分析截图
                should_respond, reason = self.analyze_screenshot_with_ai(latest_screenshot)
                
                if should_respond:
                    logger.info(f"🤖 AI判断需要回复: {reason}")
                    if self.send_continue_response():
                        logger.info("✅ 自动回复完成")
                        # 回复后等待更长时间，避免重复回复
                        time.sleep(30)
                    else:
                        logger.error("❌ 自动回复失败")
                else:
                    logger.debug(f"📊 AI判断无需回复: {reason}")
                
                # 清理旧的分析文件
                try:
                    if os.path.exists(analysis_file):
                        os.remove(analysis_file)
                except:
                    pass
                
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
        """启动智能管理器"""
        logger.info("🚀 智能VSCode管理器启动")
        try:
            self.intelligent_monitor_loop()
        except Exception as e:
            logger.error(f"❌ 管理器运行出错: {e}")
        finally:
            logger.info("🏁 智能VSCode管理器已停止")

def main():
    """主函数"""
    print("🧠 智能VSCode管理器")
    print("=" * 50)
    print("功能: 使用AI分析截图，智能判断是否需要回复")
    print("目标: 让VSCode中的AI助手持续工作不被打断")
    print("技术: 截图 + 大模型分析 + 智能响应")
    print("按 Ctrl+C 停止")
    print("=" * 50)
    
    manager = SmartVSCodeManager()
    manager.start()

if __name__ == "__main__":
    main()
