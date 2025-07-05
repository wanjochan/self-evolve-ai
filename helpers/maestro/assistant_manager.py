#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import time
import json
import logging
from pathlib import Path
import argparse

# 添加父目录到路径
sys.path.append(str(Path(__file__).parent))

# 导入maestro模块
from maestro.core.input_controller import InputController
from maestro.core.window_capture import WindowCapture
from maestro.core.ui_detector import UIDetector
from maestro.core.ui_types import ElementType

# 设置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger('assistant_manager')

class AssistantManager:
    """智能助理窗口管理器"""
    
    def __init__(self, window_title="Visual Studio Code", weights_dir=None):
        """初始化助理管理器
        
        Args:
            window_title: 窗口标题
            weights_dir: 模型权重目录
        """
        self.window_title = window_title
        
        # 初始化组件
        if weights_dir is None:
            weights_dir = Path(__file__).parent / "weights"
            if not weights_dir.exists():
                weights_dir = Path(__file__).parent.parent.parent / "weights"
        
        self.ui_detector = UIDetector(str(weights_dir))
        self.window_capture = WindowCapture()
        self.input_controller = InputController()
        
        # 查找窗口
        self.hwnd = self._find_window()
        if not self.hwnd:
            logger.warning(f"未找到窗口: {window_title}")
        
        # 对话区域坐标
        self.dialog_area = None
        self.input_area = None
        self.send_button = None
        
        # 初始化UI元素
        if self.hwnd:
            self._initialize_ui_elements()
    
    def _find_window(self):
        """查找窗口句柄"""
        if self.window_capture.find_window(self.window_title):
            return self.window_capture.hwnd
        return None
    
    def _initialize_ui_elements(self):
        """初始化UI元素位置"""
        logger.info("初始化UI元素...")
        
        # 捕获窗口截图
        self.window_capture.set_window_handle(self.hwnd)
        image = self.window_capture.capture()
        if image is None:
            logger.error("无法捕获窗口截图")
            return
        
        # 分析UI元素
        elements = self.ui_detector.analyze_image(image)
        if not elements:
            logger.warning("未检测到UI元素")
            return
        
        # 查找对话区域（大型文本区域）
        text_elements = self.ui_detector.find_element_by_type(elements, ElementType.TEXT)
        large_text_elements = []
        
        for elem in text_elements:
            x1, y1, x2, y2 = elem.bbox
            width = x2 - x1
            height = y2 - y1
            area = width * height
            
            # 如果面积大于一定阈值，可能是对话区域
            if area > 10000:
                large_text_elements.append((elem, area))
        
        # 按面积排序
        large_text_elements.sort(key=lambda x: x[1], reverse=True)
        
        if large_text_elements:
            # 最大的文本区域可能是对话区域
            self.dialog_area = large_text_elements[0][0].bbox
            logger.info(f"找到对话区域: {self.dialog_area}")
        
        # 查找输入区域（通常在窗口底部的文本框）
        input_elements = self.ui_detector.find_element_by_type(elements, ElementType.TEXTBOX)
        if input_elements:
            # 选择位于窗口底部的文本框
            input_elements.sort(key=lambda e: e.bbox[1], reverse=True)
            self.input_area = input_elements[0].bbox
            logger.info(f"找到输入区域: {self.input_area}")
        
        # 查找发送按钮（通常在输入框旁边的按钮）
        button_elements = self.ui_detector.find_element_by_type(elements, ElementType.BUTTON)
        if button_elements and self.input_area:
            # 查找靠近输入区域的按钮
            input_x = (self.input_area[0] + self.input_area[2]) / 2
            input_y = (self.input_area[1] + self.input_area[3]) / 2
            
            closest_button = None
            min_distance = float('inf')
            
            for button in button_elements:
                button_x = (button.bbox[0] + button.bbox[2]) / 2
                button_y = (button.bbox[1] + button.bbox[3]) / 2
                
                # 计算距离
                distance = ((button_x - input_x) ** 2 + (button_y - input_y) ** 2) ** 0.5
                
                if distance < min_distance:
                    min_distance = distance
                    closest_button = button
            
            if closest_button:
                self.send_button = closest_button.bbox
                logger.info(f"找到发送按钮: {self.send_button}")
    
    def activate_window(self):
        """激活窗口"""
        if not self.hwnd:
            logger.warning("窗口未找到，无法激活")
            return False
        
        self.input_controller.activate_window(self.hwnd)
        time.sleep(0.5)  # 等待窗口激活
        return True
    
    def send_message(self, message):
        """发送消息到助理
        
        Args:
            message: 要发送的消息
        
        Returns:
            bool: 是否成功发送
        """
        if not self.hwnd or not self.input_area:
            logger.warning("窗口或输入区域未找到，无法发送消息")
            return False
        
        # 激活窗口
        if not self.activate_window():
            return False
        
        # 点击输入区域
        input_x = (self.input_area[0] + self.input_area[2]) // 2
        input_y = (self.input_area[1] + self.input_area[3]) // 2
        self.input_controller.click(input_x, input_y)
        time.sleep(0.2)
        
        # 清空输入区域（Ctrl+A然后删除）
        self.input_controller.key_press('ctrl+a')
        time.sleep(0.1)
        self.input_controller.key_press('delete')
        time.sleep(0.1)
        
        # 输入消息
        self.input_controller.type_text(message)
        time.sleep(0.2)
        
        # 点击发送按钮或按回车键
        if self.send_button:
            send_x = (self.send_button[0] + self.send_button[2]) // 2
            send_y = (self.send_button[1] + self.send_button[3]) // 2
            self.input_controller.click(send_x, send_y)
        else:
            self.input_controller.key_press('enter')
        
        logger.info(f"已发送消息: {message}")
        return True
    
    def read_last_response(self):
        """读取最后一条助理响应
        
        Returns:
            str: 助理响应文本
        """
        if not self.hwnd or not self.dialog_area:
            logger.warning("窗口或对话区域未找到，无法读取响应")
            return None
        
        # 捕获窗口截图
        self.window_capture.set_window_handle(self.hwnd)
        image = self.window_capture.capture()
        if image is None:
            logger.error("无法捕获窗口截图")
            return None
        
        # 裁剪对话区域
        x1, y1, x2, y2 = self.dialog_area
        dialog_image = image[y1:y2, x1:x2]
        
        # 使用OCR识别文本
        if self.ui_detector.enable_ocr and self.ui_detector.ocr is not None:
            try:
                ocr_result = self.ui_detector.ocr.ocr(dialog_image, cls=True)
                
                if ocr_result and ocr_result[0]:
                    texts = []
                    for line in ocr_result[0]:
                        if line[1][0]:  # 文本内容
                            texts.append(line[1][0])
                    
                    response = " ".join(texts)
                    logger.info(f"读取到响应: {response[:50]}...")
                    return response
            except Exception as e:
                logger.error(f"OCR识别错误: {e}")
        
        logger.warning("无法读取响应文本")
        return None
    
    def wait_for_response(self, timeout=60, check_interval=1):
        """等待助理响应
        
        Args:
            timeout: 超时时间（秒）
            check_interval: 检查间隔（秒）
            
        Returns:
            str: 助理响应文本
        """
        if not self.hwnd:
            logger.warning("窗口未找到，无法等待响应")
            return None
        
        logger.info(f"等待助理响应，最多 {timeout} 秒...")
        
        # 记录初始对话内容
        initial_response = self.read_last_response()
        
        start_time = time.time()
        while time.time() - start_time < timeout:
            # 等待一段时间
            time.sleep(check_interval)
            
            # 读取当前响应
            current_response = self.read_last_response()
            
            # 如果响应发生变化，说明助理正在回复
            if current_response != initial_response and current_response:
                # 再等待一段时间，确保响应完成
                time.sleep(2)
                final_response = self.read_last_response()
                logger.info("助理已响应")
                return final_response
        
        logger.warning(f"等待响应超时（{timeout}秒）")
        return None
    
    def run_conversation(self, messages, wait_time=60):
        """运行一系列对话
        
        Args:
            messages: 要发送的消息列表
            wait_time: 每条消息的等待时间
            
        Returns:
            list: 助理响应列表
        """
        if not self.hwnd:
            logger.warning("窗口未找到，无法进行对话")
            return []
        
        responses = []
        
        for i, message in enumerate(messages):
            logger.info(f"发送消息 {i+1}/{len(messages)}: {message[:50]}...")
            
            # 发送消息
            if not self.send_message(message):
                logger.error(f"发送消息失败: {message[:50]}...")
                break
            
            # 等待响应
            response = self.wait_for_response(timeout=wait_time)
            responses.append(response)
            
            # 在消息之间等待一段时间
            if i < len(messages) - 1:
                time.sleep(2)
        
        return responses
    
    def export_conversation(self, output_file="conversation.json"):
        """导出当前对话
        
        Args:
            output_file: 输出文件路径
        """
        if not self.hwnd or not self.dialog_area:
            logger.warning("窗口或对话区域未找到，无法导出对话")
            return
        
        # 读取对话内容
        conversation = self.read_last_response()
        if not conversation:
            logger.warning("无法读取对话内容")
            return
        
        # 保存到文件
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump({"conversation": conversation}, f, ensure_ascii=False, indent=2)
        
        logger.info(f"对话已导出到: {output_file}")

def main():
    parser = argparse.ArgumentParser(description="智能助理窗口管理器")
    parser.add_argument("--window", "-w", default="Visual Studio Code", help="窗口标题")
    parser.add_argument("--message", "-m", help="要发送的消息")
    parser.add_argument("--file", "-f", help="从文件读取消息")
    parser.add_argument("--output", "-o", default="conversation.json", help="输出文件")
    parser.add_argument("--wait", "-t", type=int, default=60, help="等待响应的超时时间（秒）")
    
    args = parser.parse_args()
    
    # 创建助理管理器
    manager = AssistantManager(window_title=args.window)
    
    if not manager.hwnd:
        print(f"未找到窗口: {args.window}")
        return
    
    # 如果提供了消息，发送并等待响应
    if args.message:
        manager.send_message(args.message)
        response = manager.wait_for_response(timeout=args.wait)
        if response:
            print(f"助理响应: {response[:100]}...")
    
    # 如果提供了文件，从文件读取消息
    elif args.file:
        if not os.path.exists(args.file):
            print(f"文件不存在: {args.file}")
            return
        
        with open(args.file, 'r', encoding='utf-8') as f:
            messages = [line.strip() for line in f if line.strip()]
        
        responses = manager.run_conversation(messages, wait_time=args.wait)
        
        # 输出响应
        for i, response in enumerate(responses):
            print(f"\n响应 {i+1}:")
            if response:
                print(response[:100] + "..." if len(response) > 100 else response)
            else:
                print("无响应")
    
    # 导出对话
    manager.export_conversation(args.output)

if __name__ == "__main__":
    main() 