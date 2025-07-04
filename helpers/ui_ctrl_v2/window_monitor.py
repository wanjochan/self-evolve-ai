import threading
import time
from typing import List, Dict, Optional, Callable
from collections import defaultdict

from .ui_detector import UIDetector
from .ui_types import UIElement

class WindowMonitor:
    def __init__(self, window_titles: List[str], update_interval: float = 0.5):
        """
        Initialize window monitor
        
        Args:
            window_titles: List of window titles to monitor
            update_interval: Time between updates in seconds
        """
        self.window_titles = window_titles
        self.update_interval = update_interval
        self.detector = UIDetector()
        
        self._running = False
        self._thread: Optional[threading.Thread] = None
        self._lock = threading.Lock()
        self._latest_elements: Dict[str, List[UIElement]] = defaultdict(list)
        self._callbacks: List[Callable[[str, List[UIElement]], None]] = []
        
    def start(self):
        """Start monitoring windows in background"""
        if self._running:
            return
            
        self._running = True
        self._thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self._thread.start()
        
    def stop(self):
        """Stop monitoring windows"""
        self._running = False
        if self._thread:
            self._thread.join()
            self._thread = None
            
    def get_latest_elements(self, window_title: str) -> List[UIElement]:
        """Get latest detected elements for a window"""
        with self._lock:
            return self._latest_elements[window_title].copy()
            
    def add_callback(self, callback: Callable[[str, List[UIElement]], None]):
        """Add callback for when elements are updated
        
        Callback receives window_title and list of elements
        """
        self._callbacks.append(callback)
        
    def _monitor_loop(self):
        """Main monitoring loop"""
        while self._running:
            for title in self.window_titles:
                elements = self.detector.analyze_window(title)
                if elements:
                    with self._lock:
                        self._latest_elements[title] = elements
                    
                    # Call callbacks
                    for callback in self._callbacks:
                        try:
                            callback(title, elements)
                        except Exception as e:
                            print(f"Error in callback: {e}")
                            
            time.sleep(self.update_interval) 