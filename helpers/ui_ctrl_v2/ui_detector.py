import os
from pathlib import Path
from typing import List, Optional
from ultralytics import YOLO
from PIL import Image
import numpy as np

from .ui_types import UIElement, ElementType
from .window_capture import WindowCapture

class UIDetector:
    def __init__(self, weights_dir: str = "weights"):
        self.weights_dir = Path(weights_dir)
        self.model = self._load_model()
        self.window_capture = WindowCapture()
        
    def _load_model(self) -> YOLO:
        """Load YOLO model with OmniParser weights"""
        model_path = self.weights_dir / "icon_detect" / "model.pt"
        if not model_path.exists():
            raise FileNotFoundError(f"Model weights not found at {model_path}")
        return YOLO(str(model_path))
    
    def analyze_image(self, image: Image.Image, conf: float = 0.25) -> List[UIElement]:
        """Analyze image and return detected UI elements"""
        # Get original image dimensions before conversion
        image_width, image_height = image.size

        # Convert PIL Image to numpy array if needed
        if isinstance(image, Image.Image):
            image_array = np.array(image)
        else:
            image_array = image

        # Run YOLO prediction
        results = self.model.predict(image_array, imgsz=640, conf=conf)

        # Convert results to UIElements
        elements = []
        for r in results:
            boxes = r.boxes
            for box in boxes:
                # Get coordinates
                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy().astype(int)

                # Get class and confidence
                cls = box.cls[0].cpu().numpy().item()
                conf_score = box.conf[0].cpu().numpy().item()

                # 智能推断UI元素类型
                element_type = self._infer_element_type(
                    (x1, y1, x2, y2), conf_score, image_width, image_height
                )

                # Create UIElement
                element = UIElement(
                    type=element_type,
                    bbox=(x1, y1, x2, y2),
                    confidence=conf_score
                )
                elements.append(element)

        return elements
    
    def analyze_window(self, window_title: str) -> Optional[List[UIElement]]:
        """Capture and analyze a specific window"""
        if not self.window_capture.find_window(window_title):
            return None
            
        image = self.window_capture.capture()
        if image is None:
            return None
            
        return self.analyze_image(image)
    
    @staticmethod
    def _map_class_to_type(class_idx: int) -> ElementType:
        """Map YOLO class index to ElementType"""
        # OmniParser的icon_detect模型只有一个类别: 'icon'
        # 我们返回UNKNOWN，让后续的智能推断来确定具体类型
        return ElementType.UNKNOWN

    def _infer_element_type(self, bbox: tuple, confidence: float, image_width: int, image_height: int) -> ElementType:
        """基于位置、大小等特征智能推断UI元素类型"""
        x1, y1, x2, y2 = bbox
        width = x2 - x1
        height = y2 - y1
        area = width * height
        aspect_ratio = width / height if height > 0 else 1

        # 计算相对位置
        rel_x = x1 / image_width if image_width > 0 else 0
        rel_y = y1 / image_height if image_height > 0 else 0
        rel_width = width / image_width if image_width > 0 else 0
        rel_height = height / image_height if image_height > 0 else 0

        # 基于位置推断类型
        # 1. 顶部区域 (y < 10%) - 可能是菜单、标题栏、标签页
        if rel_y < 0.1:
            if rel_width > 0.8:  # 横跨大部分宽度
                return ElementType.MENU
            elif aspect_ratio > 3:  # 很宽的元素
                return ElementType.TAB
            else:
                return ElementType.BUTTON

        # 2. 左侧边栏 (x < 10%, height > 20%)
        elif rel_x < 0.1 and rel_height > 0.2:
            if aspect_ratio < 0.5:  # 很高的元素
                return ElementType.MENU
            else:
                return ElementType.BUTTON

        # 3. 底部状态栏 (y > 90%)
        elif rel_y > 0.9:
            return ElementType.TEXT  # 状态栏通常是文本信息

        # 4. 基于大小推断
        elif area < 1000:  # 小元素
            if aspect_ratio > 2:
                return ElementType.LINK  # 可能是链接
            else:
                return ElementType.ICON  # 小图标

        # 5. 基于宽高比推断
        elif aspect_ratio > 4:  # 很宽的元素
            if rel_width > 0.5:
                return ElementType.INPUT  # 可能是输入框
            else:
                return ElementType.LINK

        # 6. 中等大小的方形元素
        elif 0.7 <= aspect_ratio <= 1.3 and 1000 <= area <= 10000:
            return ElementType.BUTTON

        # 7. 大面积元素
        elif area > 50000:
            return ElementType.TEXT  # 可能是文本区域

        # 默认返回按钮
        return ElementType.BUTTON