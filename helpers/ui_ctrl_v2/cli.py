import argparse
import json
import sys
from pathlib import Path
from typing import List, Dict

from .ui_detector import UIDetector
from .window_monitor import WindowMonitor
from .types import UIElement

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
            pass
    except KeyboardInterrupt:
        monitor.stop()
        print("\nMonitoring stopped")

def main():
    parser = argparse.ArgumentParser(description="UI Control V2 CLI")
    parser.add_argument("--weights-dir", default="weights",
                      help="Directory containing model weights")
    
    subparsers = parser.add_subparsers(dest="command", required=True)
    
    # Analyze command
    analyze_parser = subparsers.add_parser("analyze",
                                         help="Analyze a single window")
    analyze_parser.add_argument("window_title",
                              help="Title of window to analyze")
    analyze_parser.add_argument("-o", "--output",
                              help="Save results to JSON file")
    
    # Monitor command
    monitor_parser = subparsers.add_parser("monitor",
                                         help="Monitor multiple windows")
    monitor_parser.add_argument("window_titles", nargs="+",
                              help="Titles of windows to monitor")
    monitor_parser.add_argument("-i", "--interval", type=float, default=0.5,
                              help="Update interval in seconds")
    
    args = parser.parse_args()
    
    if args.command == "analyze":
        analyze_window(args)
    elif args.command == "monitor":
        monitor_windows(args)

if __name__ == "__main__":
    main() 