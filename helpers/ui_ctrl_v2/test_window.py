from .window_capture import WindowCapture
from .ui_detector import UIDetector
import json
import os

def main():
    # 获取当前脚本的目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    # 获取项目根目录
    root_dir = os.path.dirname(os.path.dirname(os.path.dirname(current_dir)))
    # 构建权重文件路径
    weights_dir = os.path.join(root_dir, "weights")
    
    # 初始化检测器
    detector = UIDetector(weights_dir=weights_dir)
    
    # 测试VSCode窗口
    window_title = "Visual Studio Code"
    print(f"Testing window: {window_title}")
    
    elements = detector.analyze_window(window_title)
    
    if elements is None:
        print(f"Could not find window: {window_title}")
        return
        
    # 转换结果为JSON
    results = []
    for element in elements:
        results.append({
            "type": element.type.value,
            "bbox": element.bbox,
            "confidence": element.confidence,
            "center": element.center,
            "width": element.width,
            "height": element.height
        })
    
    # 打印结果
    print(json.dumps(results, indent=2))

if __name__ == "__main__":
    main() 