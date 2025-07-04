"""
Maestro - UI自动化工具
"""

__version__ = "0.1.0"

from .core.window_capture import WindowCapture
from .core.ui_detector import UIDetector
from .core.window_monitor import WindowMonitor
from .core.ui_types import UIElement, ElementType
from .core.process_manager import ProcessManager
from .core.input_controller import InputController

__all__ = [
    'WindowCapture', 
    'UIDetector', 
    'WindowMonitor', 
    'UIElement', 
    'ElementType',
    'ProcessManager',
    'InputController'
] 