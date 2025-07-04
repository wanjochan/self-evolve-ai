import win32gui
import win32con
from window_capture import WindowCapture
import os

def list_windows():
    """列出所有可见窗口"""
    def callback(hwnd, windows):
        if win32gui.IsWindowVisible(hwnd):
            title = win32gui.GetWindowText(hwnd)
            if title:  # 只添加有标题的窗口
                rect = win32gui.GetWindowRect(hwnd)
                windows.append({
                    'hwnd': hwnd,
                    'title': title,
                    'rect': rect,
                    'style': win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
                })
        return True
    
    windows = []
    win32gui.EnumWindows(callback, windows)
    return windows

def print_window_info(window_info):
    """打印窗口详细信息"""
    print(f"\nWindow Details:")
    print(f"  Title: {window_info['title']}")
    print(f"  Handle: {window_info['hwnd']}")
    print(f"  Rectangle: {window_info['rect']}")
    print(f"  Style: 0x{window_info['style']:08x}")
    
    # 检查窗口样式
    style = window_info['style']
    styles = []
    if style & win32con.WS_VISIBLE:
        styles.append("WS_VISIBLE")
    if style & win32con.WS_DISABLED:
        styles.append("WS_DISABLED")
    if style & win32con.WS_MINIMIZE:
        styles.append("WS_MINIMIZE")
    if style & win32con.WS_MAXIMIZE:
        styles.append("WS_MAXIMIZE")
    print(f"  Style flags: {', '.join(styles)}")

def main():
    # 创建输出目录
    os.makedirs("output", exist_ok=True)
    
    print("Listing all visible windows...")
    windows = list_windows()
    
    # 查找Cursor窗口
    cursor_windows = [w for w in windows if "Cursor" in w['title']]
    
    if not cursor_windows:
        print("No Cursor windows found!")
        print("\nAvailable windows:")
        for w in windows:
            print(f"- {w['title']}")
        return
    
    # 打印所有找到的Cursor窗口信息
    print(f"\nFound {len(cursor_windows)} Cursor windows:")
    for window in cursor_windows:
        print_window_info(window)
    
    # 使用第一个找到的Cursor窗口
    target_window = cursor_windows[0]
    
    # 初始化窗口捕获
    capture = WindowCapture()
    capture.set_window_handle(target_window['hwnd'])
    
    print("\nTrying to capture window...")
    # 保存截图
    if capture.capture_to_file("output/captured_window.png"):
        print("Successfully saved window capture to output/captured_window.png")
    else:
        print("Failed to capture window")

if __name__ == "__main__":
    main() 
