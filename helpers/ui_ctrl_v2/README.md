# UI Control V2

A fast UI layout detection tool using YOLO and OmniParser for Windows applications.

## Features
- Fast window screenshot capture (even for background windows)
- YOLO-based UI element detection
- OmniParser integration for UI element understanding
- Memory-efficient processing
- Real-time layout monitoring capability

## Setup

1. Create and activate conda environment:
```bash
conda create -n omni python=3.12 -y
conda activate omni
```

2. Install dependencies:
```bash
pip install -r requirements.txt
```

3. Download required models:
```bash
mkdir -p weights
huggingface-cli download microsoft/OmniParser-v2.0 icon_detect/model.pt --local-dir weights
huggingface-cli download microsoft/OmniParser-v2.0 icon_detect/model.yaml --local-dir weights
```

## Usage

Basic usage:
```python
from ui_ctrl_v2 import WindowCapture, UIDetector

# Initialize
detector = UIDetector()

# Capture and analyze a specific window
window_title = "Notepad"
elements = detector.analyze_window(window_title)

# Print detected elements
for elem in elements:
    print(f"Found {elem.type} at {elem.bbox} with confidence {elem.confidence}")
```

For continuous monitoring:
```python
from ui_ctrl_v2 import WindowMonitor

monitor = WindowMonitor(["Notepad", "Chrome"])
monitor.start()  # Starts monitoring in background
# ... do other things ...
monitor.stop()
``` 