import os
from pathlib import Path
from typing import List, Optional, Dict, Any, Tuple
from ultralytics import YOLO
from PIL import Image
import numpy as np
import logging

from .ui_types import UIElement, ElementType
from .window_capture import WindowCapture, WindowInfo

logger = logging.getLogger("maestro.detector")

class UIDetector:
    def __init__(self, weights_dir: str = "weights", conf_threshold: float = 0.25):
        self.weights_dir = Path(weights_dir)
        self.conf_threshold = conf_threshold
        self.model = self._load_model()
        self.window_capture = WindowCapture()
        
    def _load_model(self) -> YOLO:
        """Load YOLO model with OmniParser weights"""
        model_path = self.weights_dir / "icon_detect" / "model.pt"
        if not model_path.exists():
            raise FileNotFoundError(f"Model weights not found at {model_path}")
        logger.info(f"Loading model from {model_path}")
        return YOLO(str(model_path))
    
    def analyze_image(self, image: Image.Image, conf: float = None) -> List[UIElement]:
        """Analyze image and return detected UI elements"""
        if conf is None:
            conf = self.conf_threshold
            
        # Convert PIL Image to numpy array if needed
        if isinstance(image, Image.Image):
            image = np.array(image)
            
        # Run YOLO prediction
        results = self.model.predict(image, imgsz=640, conf=conf)
        
        # Convert results to UIElements
        elements = []
        for r in results:
            boxes = r.boxes
            for box in boxes:
                # Get coordinates
                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy().astype(int)
                
                # Get class and confidence
                cls = box.cls[0].cpu().numpy().item()
                conf = box.conf[0].cpu().numpy().item()
                
                # Map class index to ElementType
                element_type = self._map_class_to_type(int(cls))
                
                # Create UIElement
                element = UIElement(
                    type=element_type,
                    bbox=(x1, y1, x2, y2),
                    confidence=conf
                )
                elements.append(element)
                
        logger.debug(f"Detected {len(elements)} UI elements")
        return elements
    
    def analyze_window(self, window_title: str) -> Optional[List[UIElement]]:
        """Capture and analyze a specific window"""
        if not self.window_capture.find_window(window_title):
            logger.warning(f"Window '{window_title}' not found")
            return None
            
        image = self.window_capture.capture()
        if image is None:
            logger.warning(f"Failed to capture window '{window_title}'")
            return None
            
        return self.analyze_image(image)
        
    def analyze_window_by_handle(self, hwnd: int) -> Optional[List[UIElement]]:
        """Analyze window using its handle"""
        self.window_capture.set_window_handle(hwnd)
        image = self.window_capture.capture()
        if image is None:
            logger.warning(f"Failed to capture window with handle {hwnd}")
            return None
            
        return self.analyze_image(image)
        
    def analyze_all_windows(self) -> Dict[str, List[UIElement]]:
        """Analyze all visible windows"""
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
        """Find elements of a specific type"""
        return [e for e in elements if e.type == element_type]
    
    def find_element_by_position(self, elements: List[UIElement], 
                               x: int, y: int) -> Optional[UIElement]:
        """Find element at a specific position"""
        for element in elements:
            if element.contains_point(x, y):
                return element
        return None
    
    def find_element_by_size(self, elements: List[UIElement], 
                           min_width: int = None, min_height: int = None,
                           max_width: int = None, max_height: int = None) -> List[UIElement]:
        """Find elements within specific size constraints"""
        result = []
        for element in elements:
            if ((min_width is None or element.width >= min_width) and
                (min_height is None or element.height >= min_height) and
                (max_width is None or element.width <= max_width) and
                (max_height is None or element.height <= max_height)):
                result.append(element)
        return result
    
    @staticmethod
    def _map_class_to_type(class_idx: int) -> ElementType:
        """Map YOLO class index to ElementType"""
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
            8: ElementType.ICON
        }
        return class_map.get(class_idx, ElementType.UNKNOWN) 