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
        # 由于模型只有一个类别，我们把所有检测都当作按钮
        return ElementType.BUTTON 