import win32gui
import win32con
import json

def get_child_windows(parent_hwnd):
    """获取指定窗口的所有子窗口"""
    result = []
    
    def callback(hwnd, _):
        class_name = win32gui.GetClassName(hwnd)
        window_text = win32gui.GetWindowText(hwnd)
        rect = win32gui.GetWindowRect(hwnd)
        
        # 获取窗口样式
        style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
        ex_style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
        
        # 获取窗口可见性
        is_visible = win32gui.IsWindowVisible(hwnd)
        
        # 获取窗口启用状态
        is_enabled = win32gui.IsWindowEnabled(hwnd)
        
        result.append({
            "hwnd": hwnd,
            "class_name": class_name,
            "text": window_text,
            "rect": rect,
            "style": style,
            "ex_style": ex_style,
            "is_visible": is_visible,
            "is_enabled": is_enabled
        })
        return True
    
    win32gui.EnumChildWindows(parent_hwnd, callback, None)
    return result

def analyze_window_structure(hwnd):
    """分析窗口结构"""
    # 获取窗口基本信息
    window_info = {
        "hwnd": hwnd,
        "class_name": win32gui.GetClassName(hwnd),
        "text": win32gui.GetWindowText(hwnd),
        "rect": win32gui.GetWindowRect(hwnd),
        "is_visible": win32gui.IsWindowVisible(hwnd),
        "is_enabled": win32gui.IsWindowEnabled(hwnd)
    }
    
    # 获取子窗口
    child_windows = get_child_windows(hwnd)
    
    # 构建完整结构
    window_structure = {
        "window_info": window_info,
        "child_windows": child_windows
    }
    
    return window_structure

if __name__ == "__main__":
    # Cursor窗口的句柄
    cursor_hwnd = 52564458
    
    # 分析窗口结构
    structure = analyze_window_structure(cursor_hwnd)
    
    # 输出结构信息
    print(json.dumps(structure, indent=2))
