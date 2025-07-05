from dataclasses import dataclass
from enum import Enum
from typing import Tuple, Optional

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
    image: Optional[bytes] = None
    
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