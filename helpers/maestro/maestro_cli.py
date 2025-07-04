"""
Maestro CLI工具 - 简化版
"""

import argparse
import win32gui
import win32process
import win32con
import win32api
import sys
import os
import json
import time
from pathlib import Path

# 导入ui_ctrl_v2中的相关模块
sys.path.append(str(Path(__file__).parent.parent))  # 添加helpers目录到路径
try:
    from ui_ctrl_v2.ui_detector import UIDetector
    from ui_ctrl_v2.window_capture import WindowCapture
    from ui_ctrl_v2.input_controller import InputController
    UI_CTRL_V2_AVAILABLE = True
except ImportError:
    print("警告: ui_ctrl_v2模块不可用，部分功能将受限")
    UI_CTRL_V2_AVAILABLE = False

# 全局变量
_detector = None
_window_capture = None
_input_controller = None

def get_detector(weights_dir=None):
    """获取或初始化UI检测器"""
    global _detector
    if _detector is None and UI_CTRL_V2_AVAILABLE:
        if weights_dir is None:
            weights_dir = Path(__file__).parent.parent.parent / "weights"
            if not weights_dir.exists():
                weights_dir = Path(__file__).parent.parent / "weights"
        _detector = UIDetector(str(weights_dir))
    return _detector

def get_window_capture():
    """获取或初始化窗口捕获器"""
    global _window_capture
    if _window_capture is None and UI_CTRL_V2_AVAILABLE:
        _window_capture = WindowCapture()
    return _window_capture

def get_input_controller():
    """获取或初始化输入控制器"""
    global _input_controller
    if _input_controller is None and UI_CTRL_V2_AVAILABLE:
        _input_controller = InputController()
    return _input_controller

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

def find_window(window_title):
    """查找指定标题的窗口"""
    windows = list_windows()
    matching_windows = [w for w in windows if window_title.lower() in w[1].lower()]
    return matching_windows[0] if matching_windows else None

def detail_window(window_title, output_file=None, save_screenshot=False, fast_mode=False):
    """详细分析指定标题的窗口"""
    # 首先列出匹配的窗口
    windows = list_windows()
    matching_windows = [w for w in windows if window_title.lower() in w[1].lower()]
    
    if not matching_windows:
        print(f"没有找到标题包含 '{window_title}' 的窗口")
        return
    
    print(f"找到 {len(matching_windows)} 个匹配窗口:")
    for hwnd, title, pid in matching_windows:
        print(f"HWND: {hwnd}, PID: {pid}, 标题: {title}")
    
    # 使用第一个匹配的窗口进行详细分析
    hwnd, title, pid = matching_windows[0]
    print(f"\n对窗口 '{title}' (HWND: {hwnd}) 进行详细分析:")
    
    # 如果ui_ctrl_v2可用，使用它进行UI元素检测
    if UI_CTRL_V2_AVAILABLE:
        try:
            # 捕获窗口截图
            window_capture = get_window_capture()
            window_capture.set_window_handle(hwnd)
            
            # 快速模式下减少输出
            if fast_mode:
                window_capture.verbose = False
                
            image = window_capture.capture()
            
            if image:
                screenshot_path = None
                # 只有在指定保存截图时才保存
                if save_screenshot:
                    screenshot_path = f"{window_title.replace(' ', '_')}_screenshot.png"
                    image.save(screenshot_path)
                    print(f"窗口截图已保存到: {screenshot_path}")
                
                # 使用UIDetector分析UI元素
                detector = get_detector()
                
                # 快速模式下使用更低的置信度阈值和更快的推理设置
                conf_threshold = 0.4 if fast_mode else 0.25
                elements = detector.analyze_image(image, conf=conf_threshold)
                
                if elements:
                    print(f"\n检测到 {len(elements)} 个UI元素:")
                    
                    # 转换为可序列化的字典
                    elements_data = []
                    for i, elem in enumerate(elements):
                        # 将numpy.int64转换为普通int
                        x1, y1, x2, y2 = map(int, elem.bbox)
                        center_x, center_y = map(int, elem.center)
                        width = int(elem.width)
                        height = int(elem.height)
                        
                        elem_dict = {
                            "id": i + 1,
                            "type": elem.type.value,
                            "position": {
                                "x1": x1,
                                "y1": y1,
                                "x2": x2,
                                "y2": y2,
                                "center_x": center_x,
                                "center_y": center_y
                            },
                            "size": {
                                "width": width,
                                "height": height
                            },
                            "confidence": float(elem.confidence)
                        }
                        elements_data.append(elem_dict)
                        print(f"元素 {i+1}: 类型={elem.type.value}, 位置=({x1}, {y1}, {x2}, {y2}), 置信度={elem.confidence:.2f}")
                    
                    # 如果指定了输出文件，保存分析结果
                    if output_file:
                        result = {
                            "window": {
                                "title": title,
                                "hwnd": hwnd,
                                "pid": pid
                            },
                            "elements": elements_data
                        }
                        
                        # 如果保存了截图，添加截图路径
                        if screenshot_path:
                            result["screenshot"] = screenshot_path
                        
                        with open(output_file, 'w', encoding='utf-8') as f:
                            json.dump(result, f, ensure_ascii=False, indent=2)
                        print(f"\n分析结果已保存到: {output_file}")
                else:
                    print("未检测到UI元素")
            else:
                print("无法捕获窗口截图")
        except Exception as e:
            print(f"分析过程中出错: {e}")
    else:
        print("ui_ctrl_v2模块不可用，无法进行UI元素检测")
        
        # 获取窗口基本信息
        try:
            rect = win32gui.GetWindowRect(hwnd)
            client_rect = win32gui.GetClientRect(hwnd)
            print(f"窗口矩形: {rect}")
            print(f"客户区矩形: {client_rect}")
            print(f"窗口大小: {rect[2]-rect[0]}x{rect[3]-rect[1]}")
            print(f"客户区大小: {client_rect[2]}x{client_rect[3]}")
        except Exception as e:
            print(f"获取窗口信息出错: {e}")

def window_control(window_title, action):
    """控制窗口状态"""
    window_info = find_window(window_title)
    if not window_info:
        print(f"没有找到标题包含 '{window_title}' 的窗口")
        return
    
    hwnd, title, pid = window_info
    print(f"对窗口 '{title}' (HWND: {hwnd}) 执行 {action} 操作")
    
    try:
        if action == "activate":
            # 激活窗口
            if UI_CTRL_V2_AVAILABLE:
                input_controller = get_input_controller()
                result = input_controller.activate_window(hwnd)
                print(f"激活窗口结果: {'成功' if result else '失败'}")
            else:
                # 使用原生API
                if win32gui.IsIconic(hwnd):  # 如果窗口最小化
                    win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
                win32gui.SetForegroundWindow(hwnd)
                print("已尝试激活窗口")
                
        elif action == "minimize":
            # 最小化窗口
            win32gui.ShowWindow(hwnd, win32con.SW_MINIMIZE)
            print("已最小化窗口")
            
        elif action == "maximize":
            # 最大化窗口
            win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
            print("已最大化窗口")
            
        elif action == "restore":
            # 恢复窗口
            win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
            print("已恢复窗口")
            
        elif action == "close":
            # 关闭窗口
            win32gui.PostMessage(hwnd, win32con.WM_CLOSE, 0, 0)
            print("已发送关闭命令")
            
        else:
            print(f"不支持的操作: {action}")
    except Exception as e:
        print(f"执行窗口操作时出错: {e}")

def mouse_action(window_title, action, x=None, y=None, button="left", double=False):
    """执行鼠标操作"""
    if not UI_CTRL_V2_AVAILABLE:
        print("ui_ctrl_v2模块不可用，无法执行鼠标操作")
        return
    
    window_info = find_window(window_title)
    if not window_info:
        print(f"没有找到标题包含 '{window_title}' 的窗口")
        return
    
    hwnd, title, pid = window_info
    input_controller = get_input_controller()
    
    # 先激活窗口
    input_controller.activate_window(hwnd)
    time.sleep(0.5)  # 等待窗口激活
    
    try:
        if action == "click":
            if x is not None and y is not None:
                # 在指定位置点击
                print(f"在窗口 '{title}' 的位置 ({x}, {y}) 执行{button}键{'双' if double else '单'}击")
                input_controller.click_at_position(x, y, button, double)
            else:
                print("缺少坐标参数 x 和 y")
                
        elif action == "move":
            if x is not None and y is not None:
                # 移动鼠标到指定位置
                print(f"将鼠标移动到窗口 '{title}' 的位置 ({x}, {y})")
                input_controller.set_cursor_position(x, y)
            else:
                print("缺少坐标参数 x 和 y")
                
        elif action == "current":
            # 获取当前鼠标位置
            pos = input_controller.get_cursor_position()
            print(f"当前鼠标位置: {pos}")
            
        else:
            print(f"不支持的鼠标操作: {action}")
    except Exception as e:
        print(f"执行鼠标操作时出错: {e}")

def keyboard_action(window_title, action, keys=None):
    """执行键盘操作"""
    if not UI_CTRL_V2_AVAILABLE:
        print("ui_ctrl_v2模块不可用，无法执行键盘操作")
        return
    
    window_info = find_window(window_title)
    if not window_info:
        print(f"没有找到标题包含 '{window_title}' 的窗口")
        return
    
    hwnd, title, pid = window_info
    
    # 先激活窗口
    input_controller = get_input_controller()
    input_controller.activate_window(hwnd)
    time.sleep(0.5)  # 等待窗口激活
    
    try:
        if action == "type":
            if keys:
                # 输入文本
                print(f"在窗口 '{title}' 中输入文本: {keys}")
                input_controller.type_text(keys)
            else:
                print("缺少要输入的文本")
                
        elif action == "key":
            if keys:
                # 按下特定按键
                print(f"在窗口 '{title}' 中按下按键: {keys}")
                input_controller.press_key(keys)
            else:
                print("缺少要按下的按键")
                
        elif action == "hotkey":
            if keys:
                # 按下组合键
                key_list = keys.split('+')
                print(f"在窗口 '{title}' 中按下组合键: {keys}")
                input_controller.press_hotkey(key_list)
            else:
                print("缺少要按下的组合键")
                
        else:
            print(f"不支持的键盘操作: {action}")
    except Exception as e:
        print(f"执行键盘操作时出错: {e}")

def main():
    """CLI主入口"""
    parser = argparse.ArgumentParser(description="Maestro CLI工具")
    subparsers = parser.add_subparsers(dest="command", help="可用命令")
    
    # list命令
    list_parser = subparsers.add_parser("list", help="列出所有窗口")
    
    # detail命令
    detail_parser = subparsers.add_parser("detail", help="详细分析指定窗口")
    detail_parser.add_argument("window_title", help="窗口标题")
    detail_parser.add_argument("-o", "--output", help="将分析结果保存到JSON文件")
    detail_parser.add_argument("-s", "--save-screenshot", action="store_true", help="保存窗口截图")
    detail_parser.add_argument("-f", "--fast", action="store_true", help="快速模式，减少输出并加快分析速度")
    
    # window命令
    window_parser = subparsers.add_parser("window", help="控制窗口状态")
    window_parser.add_argument("window_title", help="窗口标题")
    window_parser.add_argument("action", choices=["activate", "minimize", "maximize", "restore", "close"], 
                             help="窗口操作")
    
    # mouse命令
    mouse_parser = subparsers.add_parser("mouse", help="执行鼠标操作")
    mouse_parser.add_argument("window_title", help="窗口标题")
    mouse_parser.add_argument("action", choices=["click", "move", "current"], help="鼠标操作")
    mouse_parser.add_argument("-x", type=int, help="X坐标")
    mouse_parser.add_argument("-y", type=int, help="Y坐标")
    mouse_parser.add_argument("-b", "--button", choices=["left", "right", "middle"], default="left", 
                            help="鼠标按钮")
    mouse_parser.add_argument("-d", "--double", action="store_true", help="双击")
    
    # keyboard命令
    keyboard_parser = subparsers.add_parser("keyboard", help="执行键盘操作")
    keyboard_parser.add_argument("window_title", help="窗口标题")
    keyboard_parser.add_argument("action", choices=["type", "key", "hotkey"], help="键盘操作")
    keyboard_parser.add_argument("keys", help="要输入的文本、按键或组合键")
    
    args = parser.parse_args()
    
    if args.command == "list":
        windows = list_windows()
        print(f"找到 {len(windows)} 个窗口:")
        for hwnd, title, pid in windows:
            print(f"HWND: {hwnd}, PID: {pid}, 标题: {title}")
    
    elif args.command == "detail":
        detail_window(args.window_title, args.output, args.save_screenshot, args.fast)
    
    elif args.command == "window":
        window_control(args.window_title, args.action)
    
    elif args.command == "mouse":
        mouse_action(args.window_title, args.action, args.x, args.y, args.button, args.double)
    
    elif args.command == "keyboard":
        keyboard_action(args.window_title, args.action, args.keys)
    
    else:
        parser.print_help()

if __name__ == "__main__":
    main() 