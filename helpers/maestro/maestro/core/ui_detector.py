import os
from pathlib import Path
from typing import List, Optional, Dict, Any, Tuple
from ultralytics import YOLO
from PIL import Image
import numpy as np
import logging
import torch
from transformers import AutoProcessor, AutoModelForCausalLM
import cv2

from .ui_types import UIElement, ElementType
from .window_capture import WindowCapture, WindowInfo

logger = logging.getLogger("maestro.detector")

class UIDetector:
    def __init__(self, weights_dir: str = "weights", conf_threshold: float = 0.25, enable_ocr: bool = True, enable_caption: bool = True):
        self.weights_dir = Path(weights_dir)
        self.conf_threshold = conf_threshold
        self.enable_ocr = enable_ocr
        self.enable_caption = enable_caption
        
        # 加载YOLO模型
        self.model = self._load_model()
        self.window_capture = WindowCapture()
        
        # 加载OCR模型
        self.ocr = None
        if self.enable_ocr:
            self._load_ocr()
        
        # 加载caption模型
        self.caption_model = None
        self.caption_processor = None
        if self.enable_caption:
            self._load_caption_model()
    
    def _load_model(self) -> YOLO:
        """加载YOLO模型用于UI元素检测"""
        model_path = self.weights_dir / "icon_detect" / "model.pt"
        if not model_path.exists():
            raise FileNotFoundError(f"Model weights not found at {model_path}")
        logger.info(f"Loading model from {model_path}")
        return YOLO(str(model_path))
    
    def _load_ocr(self):
        """加载PaddleOCR模型"""
        try:
            from paddleocr import PaddleOCR
            logger.info("Loading PaddleOCR model...")
            self.ocr = PaddleOCR(use_angle_cls=True, lang='ch', use_gpu=False, show_log=False)
            logger.info("PaddleOCR model loaded successfully")
        except ImportError:
            logger.warning("PaddleOCR not installed. Text recognition will be disabled.")
            self.enable_ocr = False
    
    def _load_caption_model(self):
        """加载icon_caption模型"""
        try:
            caption_path = self.weights_dir / "icon_caption_florence"
            if not caption_path.exists():
                caption_path = self.weights_dir / "icon_caption"
                if not caption_path.exists():
                    logger.warning(f"Caption model not found at {caption_path}, caption will be disabled")
                    self.enable_caption = False
                    return
            
            logger.info(f"Loading caption model from {caption_path}")
            self.caption_processor = AutoProcessor.from_pretrained(str(caption_path))
            self.caption_model = AutoModelForCausalLM.from_pretrained(str(caption_path))
            
            # 如果有GPU，将模型移到GPU上
            if torch.cuda.is_available():
                self.caption_model = self.caption_model.to("cuda")
            
            logger.info("Caption model loaded successfully")
        except Exception as e:
            logger.warning(f"Failed to load caption model: {e}")
            self.enable_caption = False
    
    def analyze_image(self, image: Image.Image, conf: float = None) -> List[UIElement]:
        """分析图像并返回检测到的UI元素"""
        if conf is None:
            conf = self.conf_threshold
            
        # 转换PIL Image为numpy数组
        if isinstance(image, Image.Image):
            np_image = np.array(image)
        else:
            np_image = image
            image = Image.fromarray(np_image)
            
        # 运行YOLO预测
        results = self.model.predict(np_image, imgsz=640, conf=conf)
        
        # 转换结果为UIElements
        elements = []
        for r in results:
            boxes = r.boxes
            for box in boxes:
                # 获取坐标
                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy().astype(int)
                
                # 获取类别和置信度
                cls = box.cls[0].cpu().numpy().item()
                conf = box.conf[0].cpu().numpy().item()
                
                # 映射类别索引到ElementType
                element_type = self._map_class_to_type(int(cls))
                
                # 创建UIElement
                element = UIElement(
                    type=element_type,
                    bbox=(x1, y1, x2, y2),
                    confidence=conf
                )
                
                # 提取元素区域图像
                element_image = np_image[y1:y2, x1:x2]
                
                # 如果启用了OCR，尝试识别文本
                if self.enable_ocr and self.ocr is not None:
                    try:
                        # 转换为PIL图像
                        element_pil = Image.fromarray(element_image)
                        ocr_result = self.ocr.ocr(np.array(element_pil), cls=True)
                        
                        # 提取文本
                        if ocr_result and ocr_result[0]:
                            texts = []
                            for line in ocr_result[0]:
                                if line[1][0]:  # 文本内容
                                    texts.append(line[1][0])
                            
                            if texts:
                                element.text = " ".join(texts)
                                logger.debug(f"OCR text: {element.text}")
                    except Exception as e:
                        logger.warning(f"OCR error: {e}")
                
                # 如果启用了caption模型，生成元素描述
                if self.enable_caption and self.caption_model is not None and element_image.size > 0:
                    try:
                        # 转换为PIL图像
                        element_pil = Image.fromarray(element_image)
                        
                        # 准备输入
                        inputs = self.caption_processor(images=element_pil, return_tensors="pt")
                        
                        # 如果有GPU，将输入移到GPU上
                        if torch.cuda.is_available():
                            inputs = {k: v.to("cuda") for k, v in inputs.items()}
                        
                        # 生成描述
                        with torch.no_grad():
                            generated_ids = self.caption_model.generate(
                                pixel_values=inputs["pixel_values"],
                                max_new_tokens=50,
                                do_sample=False
                            )
                        
                        # 解码生成的描述
                        generated_text = self.caption_processor.batch_decode(generated_ids, skip_special_tokens=True)[0]
                        element.description = generated_text.strip()
                        logger.debug(f"Caption: {element.description}")
                    except Exception as e:
                        logger.warning(f"Caption error: {e}")
                
                elements.append(element)
                
        logger.debug(f"Detected {len(elements)} UI elements")
        return elements
    
    def analyze_window(self, window_title: str) -> Optional[List[UIElement]]:
        """捕获并分析特定窗口"""
        if not self.window_capture.find_window(window_title):
            logger.warning(f"Window '{window_title}' not found")
            return None
            
        image = self.window_capture.capture()
        if image is None:
            logger.warning(f"Failed to capture window '{window_title}'")
            return None
            
        return self.analyze_image(image)
        
    def analyze_window_by_handle(self, hwnd: int) -> Optional[List[UIElement]]:
        """使用窗口句柄分析窗口"""
        self.window_capture.set_window_handle(hwnd)
        image = self.window_capture.capture()
        if image is None:
            logger.warning(f"Failed to capture window with handle {hwnd}")
            return None
            
        return self.analyze_image(image)
        
    def analyze_all_windows(self) -> Dict[str, List[UIElement]]:
        """分析所有可见窗口"""
        windows = self.window_capture.find_all_windows()
        results = {}
        
        for window in windows:
            if window.width > 50 and window.height > 50:  # 忽略太小的窗口
                self.window_capture.set_window_handle(window.hwnd)
                elements = self.analyze_window_by_handle(window.hwnd)
                if elements:
                    results[window.title] = elements
                    
        return results
    
    def find_element_by_type(self, elements: List[UIElement], 
                           element_type: ElementType) -> List[UIElement]:
        """查找特定类型的元素"""
        return [e for e in elements if e.type == element_type]
    
    def find_element_by_position(self, elements: List[UIElement], 
                               x: int, y: int) -> Optional[UIElement]:
        """查找特定位置的元素"""
        for element in elements:
            if element.contains_point(x, y):
                return element
        return None
    
    def find_element_by_size(self, elements: List[UIElement], 
                           min_width: int = None, min_height: int = None,
                           max_width: int = None, max_height: int = None) -> List[UIElement]:
        """查找符合特定尺寸约束的元素"""
        result = []
        for element in elements:
            if ((min_width is None or element.width >= min_width) and
                (min_height is None or element.height >= min_height) and
                (max_width is None or element.width <= max_width) and
                (max_height is None or element.height <= max_height)):
                result.append(element)
        return result
    
    def find_element_by_text(self, elements: List[UIElement], text: str, 
                          case_sensitive: bool = False) -> List[UIElement]:
        """根据文本内容查找元素"""
        result = []
        for element in elements:
            if hasattr(element, 'text') and element.text:
                element_text = element.text if case_sensitive else element.text.lower()
                search_text = text if case_sensitive else text.lower()
                if search_text in element_text:
                    result.append(element)
        return result
    
    def find_element_by_description(self, elements: List[UIElement], description: str, 
                                 case_sensitive: bool = False) -> List[UIElement]:
        """根据描述查找元素"""
        result = []
        for element in elements:
            if hasattr(element, 'description') and element.description:
                element_desc = element.description if case_sensitive else element.description.lower()
                search_desc = description if case_sensitive else description.lower()
                if search_desc in element_desc:
                    result.append(element)
        return result
    
    @staticmethod
    def _map_class_to_type(class_idx: int) -> ElementType:
        """映射YOLO类别索引到ElementType"""
        # 映射表可以根据模型输出类别进行扩展
        class_map = {
            0: ElementType.BUTTON,
            1: ElementType.LINK,
            2: ElementType.MENU,
            3: ElementType.CHECKBOX,
            4: ElementType.RADIO,
            5: ElementType.DROPDOWN,
            6: ElementType.INPUT,
            7: ElementType.TAB,
            8: ElementType.ICON,
            9: ElementType.TEXT
        }
        return class_map.get(class_idx, ElementType.UNKNOWN) 