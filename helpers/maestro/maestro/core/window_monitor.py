import threading
import time
import logging
from typing import List, Dict, Optional, Callable, Set, Tuple
from collections import defaultdict

from .ui_detector import UIDetector
from .ui_types import UIElement
from .window_capture import WindowInfo

logger = logging.getLogger("maestro.monitor")

class ElementChangeEvent:
    """Event representing a change in UI elements"""
    ADDED = "added"
    REMOVED = "removed"
    CHANGED = "changed"
    
    def __init__(self, event_type: str, window_title: str, 
                 elements: List[UIElement], previous_elements: List[UIElement] = None):
        self.type = event_type
        self.window_title = window_title
        self.elements = elements
        self.previous_elements = previous_elements or []
        self.timestamp = time.time()

class WindowMonitor:
    def __init__(self, window_titles: List[str] = None, update_interval: float = 0.5,
                 auto_discover: bool = False):
        """
        Initialize window monitor
        
        Args:
            window_titles: List of window titles to monitor
            update_interval: Time between updates in seconds
            auto_discover: Whether to automatically discover and monitor new windows
        """
        self.window_titles = window_titles or []
        self.update_interval = update_interval
        self.auto_discover = auto_discover
        self.detector = UIDetector()
        
        self._running = False
        self._thread: Optional[threading.Thread] = None
        self._lock = threading.Lock()
        self._latest_elements: Dict[str, List[UIElement]] = defaultdict(list)
        self._previous_elements: Dict[str, List[UIElement]] = defaultdict(list)
        self._callbacks: List[Callable[[str, List[UIElement]], None]] = []
        self._change_callbacks: List[Callable[[ElementChangeEvent], None]] = []
        self._monitored_windows: Set[str] = set(window_titles) if window_titles else set()
        
    def start(self):
        """Start monitoring windows in background"""
        if self._running:
            return
            
        self._running = True
        self._thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self._thread.start()
        logger.info(f"Started monitoring {len(self._monitored_windows)} windows")
        
    def stop(self):
        """Stop monitoring windows"""
        self._running = False
        if self._thread:
            self._thread.join()
            self._thread = None
            logger.info("Stopped monitoring")
            
    def add_window(self, window_title: str):
        """Add a window to monitor"""
        with self._lock:
            self._monitored_windows.add(window_title)
            
    def remove_window(self, window_title: str):
        """Remove a window from monitoring"""
        with self._lock:
            self._monitored_windows.discard(window_title)
            if window_title in self._latest_elements:
                del self._latest_elements[window_title]
            if window_title in self._previous_elements:
                del self._previous_elements[window_title]
    
    def get_latest_elements(self, window_title: str) -> List[UIElement]:
        """Get latest detected elements for a window"""
        with self._lock:
            return self._latest_elements[window_title].copy()
            
    def get_monitored_windows(self) -> Set[str]:
        """Get set of currently monitored window titles"""
        with self._lock:
            return self._monitored_windows.copy()
            
    def add_callback(self, callback: Callable[[str, List[UIElement]], None]):
        """Add callback for when elements are updated
        
        Callback receives window_title and list of elements
        """
        self._callbacks.append(callback)
        
    def add_change_callback(self, callback: Callable[[ElementChangeEvent], None]):
        """Add callback for element change events
        
        Callback receives ElementChangeEvent object
        """
        self._change_callbacks.append(callback)
        
    def _detect_changes(self, window_title: str, 
                       current: List[UIElement], 
                       previous: List[UIElement]) -> List[ElementChangeEvent]:
        """Detect changes between current and previous elements"""
        events = []
        
        # Simple heuristic based on element type and position
        # In a real implementation, you might want a more sophisticated approach
        
        # Find added elements
        added = []
        for curr_elem in current:
            found = False
            for prev_elem in previous:
                if (curr_elem.type == prev_elem.type and 
                    self._bbox_iou(curr_elem.bbox, prev_elem.bbox) > 0.7):
                    found = True
                    break
            if not found:
                added.append(curr_elem)
                
        if added:
            events.append(ElementChangeEvent(
                ElementChangeEvent.ADDED,
                window_title,
                added
            ))
            
        # Find removed elements
        removed = []
        for prev_elem in previous:
            found = False
            for curr_elem in current:
                if (curr_elem.type == prev_elem.type and 
                    self._bbox_iou(curr_elem.bbox, prev_elem.bbox) > 0.7):
                    found = True
                    break
            if not found:
                removed.append(prev_elem)
                
        if removed:
            events.append(ElementChangeEvent(
                ElementChangeEvent.REMOVED,
                window_title,
                removed
            ))
            
        # Find changed elements (position or size changed)
        changed = []
        for curr_elem in current:
            for prev_elem in previous:
                if (curr_elem.type == prev_elem.type and 
                    0.3 < self._bbox_iou(curr_elem.bbox, prev_elem.bbox) < 0.7):
                    changed.append(curr_elem)
                    break
                    
        if changed:
            events.append(ElementChangeEvent(
                ElementChangeEvent.CHANGED,
                window_title,
                changed,
                previous
            ))
            
        return events
    
    @staticmethod
    def _bbox_iou(box1: Tuple[int, int, int, int], 
                 box2: Tuple[int, int, int, int]) -> float:
        """Calculate IoU between two bounding boxes"""
        # box format: (x1, y1, x2, y2)
        x1_1, y1_1, x2_1, y2_1 = box1
        x1_2, y1_2, x2_2, y2_2 = box2
        
        # Calculate intersection area
        x_left = max(x1_1, x1_2)
        y_top = max(y1_1, y1_2)
        x_right = min(x2_1, x2_2)
        y_bottom = min(y2_1, y2_2)
        
        if x_right < x_left or y_bottom < y_top:
            return 0.0
            
        intersection_area = (x_right - x_left) * (y_bottom - y_top)
        
        # Calculate union area
        box1_area = (x2_1 - x1_1) * (y2_1 - y1_1)
        box2_area = (x2_2 - x1_2) * (y2_2 - y1_2)
        union_area = box1_area + box2_area - intersection_area
        
        return intersection_area / union_area if union_area > 0 else 0.0
    
    def _discover_windows(self):
        """Discover new windows to monitor"""
        if not self.auto_discover:
            return
            
        windows = self.detector.window_capture.find_all_windows()
        with self._lock:
            for window in windows:
                # 只添加标题不为空且不在忽略列表中的窗口
                if (window.title and window.title not in self._monitored_windows and
                    window.width > 100 and window.height > 100):
                    self._monitored_windows.add(window.title)
                    logger.info(f"Auto-discovered window: {window.title}")
    
    def _monitor_loop(self):
        """Main monitoring loop"""
        while self._running:
            # 可能需要发现新窗口
            self._discover_windows()
            
            # 获取当前监控的窗口列表
            with self._lock:
                windows_to_monitor = list(self._monitored_windows)
                
            for title in windows_to_monitor:
                elements = self.detector.analyze_window(title)
                if elements:
                    with self._lock:
                        # 保存先前的元素状态
                        if title in self._latest_elements:
                            self._previous_elements[title] = self._latest_elements[title].copy()
                        
                        # 更新最新元素
                        self._latest_elements[title] = elements
                        
                        # 检测变化
                        if title in self._previous_elements:
                            changes = self._detect_changes(
                                title, 
                                self._latest_elements[title],
                                self._previous_elements[title]
                            )
                            
                            # 调用变化回调
                            for event in changes:
                                for callback in self._change_callbacks:
                                    try:
                                        callback(event)
                                    except Exception as e:
                                        logger.error(f"Error in change callback: {e}")
                    
                    # 调用常规回调
                    for callback in self._callbacks:
                        try:
                            callback(title, elements)
                        except Exception as e:
                            logger.error(f"Error in callback: {e}")
                            
            time.sleep(self.update_interval) 