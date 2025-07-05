from dataclasses import dataclass
from enum import Enum
from typing import Tuple, Optional, Dict, Any, List

class ElementType(Enum):
    BUTTON = "button"
    LINK = "link"
    MENU = "menu"
    TEXT = "text"
    ICON = "icon"
    CHECKBOX = "checkbox"
    RADIO = "radio"
    DROPDOWN = "dropdown"
    INPUT = "input"
    SLIDER = "slider"
    TAB = "tab"
    SCROLLBAR = "scrollbar"
    UNKNOWN = "unknown"

@dataclass
class UIElement:
    type: ElementType
    bbox: Tuple[int, int, int, int]  # x1, y1, x2, y2
    confidence: float
    text: Optional[str] = None
    description: Optional[str] = None
    image: Optional[bytes] = None
    attributes: Dict[str, Any] = None
    children: List['UIElement'] = None
    
    def __post_init__(self):
        if self.attributes is None:
            self.attributes = {}
        if self.children is None:
            self.children = []
    
    @property
    def center(self) -> Tuple[int, int]:
        """Returns the center point of the element"""
        x1, y1, x2, y2 = self.bbox
        return ((x2 + x1) // 2, (y2 + y1) // 2)
    
    @property
    def width(self) -> int:
        """Returns the width of the element"""
        return self.bbox[2] - self.bbox[0]
    
    @property
    def height(self) -> int:
        """Returns the height of the element"""
        return self.bbox[3] - self.bbox[1]
        
    @property
    def area(self) -> int:
        """Returns the area of the element in pixels"""
        return self.width * self.height
        
    def contains_point(self, x: int, y: int) -> bool:
        """Check if the element contains the given point"""
        x1, y1, x2, y2 = self.bbox
        return x1 <= x <= x2 and y1 <= y <= y2
        
    def overlaps(self, other: 'UIElement') -> bool:
        """Check if this element overlaps with another element"""
        x1, y1, x2, y2 = self.bbox
        ox1, oy1, ox2, oy2 = other.bbox
        
        # Check if one rectangle is to the left of the other
        if x2 < ox1 or ox2 < x1:
            return False
            
        # Check if one rectangle is above the other
        if y2 < oy1 or oy2 < y1:
            return False
            
        return True
        
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary representation"""
        return {
            "type": self.type.value,
            "bbox": self.bbox,
            "confidence": self.confidence,
            "center": self.center,
            "width": self.width,
            "height": self.height,
            "text": self.text,
            "description": self.description,
            "attributes": self.attributes,
            "children": [child.to_dict() for child in self.children] if self.children else []
        } 