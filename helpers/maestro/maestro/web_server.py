import json
import logging
from typing import List, Dict, Optional
from flask import Flask, request, jsonify
import threading
import time

from .core.ui_detector import UIDetector
from .core.window_monitor import WindowMonitor
from .core.window_capture import WindowCapture, WindowInfo
from .core.input_controller import InputController
from .core.ui_types import UIElement, ElementType

# 设置日志
logging.basicConfig(level=logging.INFO, 
                    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("maestro.web")

app = Flask(__name__)

# 全局对象
detector = None
window_capture = None
input_controller = None
monitor = None
monitored_windows = set()
window_elements = {}

def element_to_dict(element: UIElement) -> Dict:
    """Convert UIElement to dictionary for JSON serialization"""
    return {
        "type": element.type.value,
        "bbox": element.bbox,
        "confidence": element.confidence,
        "center": element.center,
        "width": element.width,
        "height": element.height
    }

def window_info_to_dict(window: WindowInfo) -> Dict:
    """Convert WindowInfo to dictionary"""
    return {
        "hwnd": window.hwnd,
        "title": window.title,
        "rect": window.rect,
        "width": window.width,
        "height": window.height,
        "is_visible": window.is_visible,
        "is_minimized": window.is_minimized
    }

def update_elements(window_title: str, elements: List[UIElement]):
    """Callback for window monitor updates"""
    global window_elements
    window_elements[window_title] = elements
    logger.debug(f"Updated elements for {window_title}: {len(elements)} elements")

@app.route('/api/windows', methods=['GET'])
def list_windows():
    """List all visible windows"""
    global window_capture
    windows = window_capture.find_all_windows()
    return jsonify({
        "status": "success",
        "windows": [window_info_to_dict(w) for w in windows]
    })

@app.route('/api/windows/<window_title>/analyze', methods=['GET'])
def analyze_window(window_title):
    """Analyze a window and return UI elements"""
    global detector
    elements = detector.analyze_window(window_title)
    
    if elements is None:
        return jsonify({
            "status": "error",
            "message": f"Window '{window_title}' not found"
        }), 404
        
    return jsonify({
        "status": "success",
        "window_title": window_title,
        "elements": [element_to_dict(e) for e in elements]
    })

@app.route('/api/windows/<window_title>/capture', methods=['GET'])
def capture_window(window_title):
    """Capture a window screenshot and return base64 image"""
    global window_capture
    
    if not window_capture.find_window(window_title):
        return jsonify({
            "status": "error",
            "message": f"Window '{window_title}' not found"
        }), 404
    
    image = window_capture.capture()
    if image is None:
        return jsonify({
            "status": "error",
            "message": f"Failed to capture window '{window_title}'"
        }), 500
    
    # Save to temporary file
    import tempfile
    import base64
    from io import BytesIO
    
    buffer = BytesIO()
    image.save(buffer, format="PNG")
    img_str = base64.b64encode(buffer.getvalue()).decode('utf-8')
    
    return jsonify({
        "status": "success",
        "window_title": window_title,
        "image": f"data:image/png;base64,{img_str}"
    })

@app.route('/api/windows/<window_title>/click', methods=['POST'])
def click_element(window_title):
    """Click on a UI element in a window"""
    global detector, input_controller
    
    data = request.json
    if not data:
        return jsonify({
            "status": "error",
            "message": "Missing request body"
        }), 400
    
    element_type = data.get('element_type')
    button = data.get('button', 'left')
    double = data.get('double', False)
    
    if not element_type:
        return jsonify({
            "status": "error",
            "message": "Missing element_type parameter"
        }), 400
    
    elements = detector.analyze_window(window_title)
    if elements is None:
        return jsonify({
            "status": "error",
            "message": f"Window '{window_title}' not found"
        }), 404
    
    # Find element by type
    matching_elements = [e for e in elements if e.type.value.lower() == element_type.lower()]
    
    if not matching_elements:
        return jsonify({
            "status": "error",
            "message": f"No elements of type '{element_type}' found"
        }), 404
    
    # Sort by confidence
    matching_elements.sort(key=lambda e: e.confidence, reverse=True)
    
    # Get best match
    element = matching_elements[0]
    
    # Click on element
    input_controller.click_element(element, button=button, double=double)
    
    return jsonify({
        "status": "success",
        "window_title": window_title,
        "element": element_to_dict(element),
        "action": f"{'double ' if double else ''}clicked with {button} button"
    })

@app.route('/api/windows/<window_title>/monitor', methods=['POST'])
def start_monitoring(window_title):
    """Start monitoring a window"""
    global monitor, monitored_windows
    
    if window_title in monitored_windows:
        return jsonify({
            "status": "success",
            "message": f"Already monitoring window '{window_title}'"
        })
    
    monitor.add_window(window_title)
    monitored_windows.add(window_title)
    
    return jsonify({
        "status": "success",
        "message": f"Started monitoring window '{window_title}'",
        "monitored_windows": list(monitored_windows)
    })

@app.route('/api/windows/<window_title>/monitor', methods=['DELETE'])
def stop_monitoring(window_title):
    """Stop monitoring a window"""
    global monitor, monitored_windows
    
    if window_title not in monitored_windows:
        return jsonify({
            "status": "error",
            "message": f"Not monitoring window '{window_title}'"
        }), 404
    
    monitor.remove_window(window_title)
    monitored_windows.remove(window_title)
    
    return jsonify({
        "status": "success",
        "message": f"Stopped monitoring window '{window_title}'",
        "monitored_windows": list(monitored_windows)
    })

@app.route('/api/monitor/status', methods=['GET'])
def monitor_status():
    """Get monitoring status"""
    global monitored_windows, window_elements
    
    return jsonify({
        "status": "success",
        "monitored_windows": list(monitored_windows),
        "elements": {
            window: [element_to_dict(e) for e in elements]
            for window, elements in window_elements.items()
        }
    })

def init_server(weights_dir: str = "weights"):
    """Initialize server components"""
    global detector, window_capture, input_controller, monitor
    
    # Initialize components
    detector = UIDetector(weights_dir=weights_dir)
    window_capture = WindowCapture()
    input_controller = InputController()
    
    # Initialize monitor
    monitor = WindowMonitor(update_interval=1.0)
    monitor.add_callback(update_elements)
    monitor.start()
    
    logger.info("Server components initialized")

def run_server(host: str = "127.0.0.1", port: int = 5000, weights_dir: str = "weights"):
    """Run the web server"""
    init_server(weights_dir)
    app.run(host=host, port=port, debug=False)
    
if __name__ == "__main__":
    run_server()
