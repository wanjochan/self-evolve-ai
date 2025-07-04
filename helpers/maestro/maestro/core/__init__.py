"""
Maestro Core - UI detection and control components
"""

from .window_capture import WindowCapture
from .ui_detector import UIDetector
from .window_monitor import WindowMonitor
from .ui_types import UIElement, ElementType
from .process_manager import ProcessManager
from .input_controller import InputController

__all__ = [
    'WindowCapture', 
    'UIDetector', 
    'WindowMonitor', 
    'UIElement', 
    'ElementType',
    'ProcessManager',
    'InputController'
] 