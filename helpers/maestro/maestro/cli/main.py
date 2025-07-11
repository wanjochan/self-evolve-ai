import argparse
import json
import sys
import time
from pathlib import Path
from typing import List, Dict
import logging
import win32gui
import win32process

from ..core.ui_detector import UIDetector
from ..core.window_monitor import WindowMonitor
from ..core.window_capture import WindowCapture
from ..core.input_controller import InputController
from ..core.ui_types import UIElement

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

def analyze_window(args):
    """Analyze a single window and print results"""
    detector = UIDetector(weights_dir=args.weights_dir)
    elements = detector.analyze_window(args.window_title)
    
    if elements is None:
        print(f"Error: Could not find window '{args.window_title}'")
        sys.exit(1)
        
    # Convert to JSON
    results = [element_to_dict(e) for e in elements]
    
    if args.output:
        # Save to file
        with open(args.output, 'w') as f:
            json.dump(results, f, indent=2)
        print(f"Results saved to {args.output}")
    else:
        # Print to stdout
        print(json.dumps(results, indent=2))

def monitor_windows(args):
    """Monitor multiple windows continuously"""
    def print_update(window_title: str, elements: List[UIElement]):
        results = [element_to_dict(e) for e in elements]
        print(f"\nUpdate for window '{window_title}':")
        print(json.dumps(results, indent=2))
    
    monitor = WindowMonitor(
        window_titles=args.window_titles,
        update_interval=args.interval
    )
    
    # Add callback for updates
    monitor.add_callback(print_update)
    
    try:
        monitor.start()
        print(f"Monitoring windows: {', '.join(args.window_titles)}")
        print("Press Ctrl+C to stop...")
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        monitor.stop()
        print("\nMonitoring stopped")

def capture_window(args):
    """Capture a window screenshot"""
    capture = WindowCapture()
    if not capture.find_window(args.window_title):
        print(f"Error: Could not find window '{args.window_title}'")
        sys.exit(1)
    
    image = capture.capture()
    if image is None:
        print(f"Error: Failed to capture window '{args.window_title}'")
        sys.exit(1)
    
    output_path = args.output or f"{args.window_title.replace(' ', '_')}.png"
    image.save(output_path)
    print(f"Screenshot saved to {output_path}")

def click_element(args):
    """Click on a UI element in a window"""
    detector = UIDetector(weights_dir=args.weights_dir)
    elements = detector.analyze_window(args.window_title)
    
    if elements is None:
        print(f"Error: Could not find window '{args.window_title}'")
        sys.exit(1)
    
    # Find element by type
    matching_elements = [e for e in elements if e.type.value.lower() == args.element_type.lower()]
    
    if not matching_elements:
        print(f"Error: No elements of type '{args.element_type}' found")
        sys.exit(1)
    
    # Sort by confidence
    matching_elements.sort(key=lambda e: e.confidence, reverse=True)
    
    # Get best match
    element = matching_elements[0]
    
    # Click on element
    controller = InputController()
    controller.click_element(element, button=args.button, double=args.double)
    print(f"Clicked on {element.type.value} at {element.center}")

def list_windows():
    """列出所有可见窗口"""
    windows = []
    
    def enum_windows_callback(hwnd, results):
        if win32gui.IsWindowVisible(hwnd):
            window_text = win32gui.GetWindowText(hwnd)
            if window_text:
                try:
                    _, pid = win32process.GetWindowThreadProcessId(hwnd)
                    windows.append((hwnd, window_text, pid))
                except Exception as e:
                    windows.append((hwnd, window_text, "Unknown"))
        return True
    
    win32gui.EnumWindows(enum_windows_callback, None)
    return windows

def analyze_window_title(window_title):
    """分析指定标题的窗口"""
    windows = list_windows()
    matching_windows = [w for w in windows if window_title.lower() in w[1].lower()]
    
    if not matching_windows:
        print(f"没有找到标题包含 '{window_title}' 的窗口")
        return
    
    print(f"找到 {len(matching_windows)} 个匹配窗口:")
    for hwnd, title, pid in matching_windows:
        print(f"HWND: {hwnd}, PID: {pid}, 标题: {title}")

def main():
    """CLI主入口"""
    parser = argparse.ArgumentParser(description="Maestro CLI工具")
    subparsers = parser.add_subparsers(dest="command", help="可用命令")
    
    # list命令
    list_parser = subparsers.add_parser("list", help="列出所有窗口")
    
    # analyze命令
    analyze_parser = subparsers.add_parser("analyze", help="分析指定窗口")
    analyze_parser.add_argument("window_title", help="窗口标题")
    
    # Monitor command
    monitor_parser = subparsers.add_parser("monitor",
                                         help="Monitor multiple windows")
    monitor_parser.add_argument("window_titles", nargs="+",
                              help="Titles of windows to monitor")
    monitor_parser.add_argument("-i", "--interval", type=float, default=0.5,
                              help="Update interval in seconds")
    
    # Capture command
    capture_parser = subparsers.add_parser("capture",
                                         help="Capture window screenshot")
    capture_parser.add_argument("window_title",
                              help="Title of window to capture")
    capture_parser.add_argument("-o", "--output",
                              help="Output file path")
    
    # Click command
    click_parser = subparsers.add_parser("click",
                                       help="Click on UI element")
    click_parser.add_argument("window_title",
                            help="Title of window")
    click_parser.add_argument("element_type",
                            help="Type of element to click (button, link, etc)")
    click_parser.add_argument("-b", "--button", default="left",
                            choices=["left", "right", "middle"],
                            help="Mouse button to use")
    click_parser.add_argument("-d", "--double", action="store_true",
                            help="Perform double-click")
    
    args = parser.parse_args()
    
    if args.command == "list":
        windows = list_windows()
        print(f"找到 {len(windows)} 个窗口:")
        for hwnd, title, pid in windows:
            print(f"HWND: {hwnd}, PID: {pid}, 标题: {title}")
    
    elif args.command == "analyze":
        analyze_window_title(args.window_title)
    
    elif args.command == "monitor":
        monitor_windows(args)
    
    elif args.command == "capture":
        capture_window(args)
    
    elif args.command == "click":
        click_element(args)
    
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
