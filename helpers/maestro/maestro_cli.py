"""
Maestro CLI工具 - 简化版
"""

import argparse
import win32gui
import win32process

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

def analyze_window(window_title):
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
    
    args = parser.parse_args()
    
    if args.command == "list":
        windows = list_windows()
        print(f"找到 {len(windows)} 个窗口:")
        for hwnd, title, pid in windows:
            print(f"HWND: {hwnd}, PID: {pid}, 标题: {title}")
    
    elif args.command == "analyze":
        analyze_window(args.window_title)
    
    else:
        parser.print_help()

if __name__ == "__main__":
    main() 