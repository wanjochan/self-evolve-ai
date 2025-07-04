from window_capture import WindowCapture
import time

def main():
    # 创建窗口捕获器
    cap = WindowCapture()
    
    # 尝试捕获记事本窗口
    print("请打开一个记事本窗口...")
    time.sleep(2)
    
    if cap.find_window("记事本"):
        print("找到记事本窗口!")
        # 捕获并保存截图
        if cap.capture_to_file("notepad_capture.png"):
            print("截图已保存到 notepad_capture.png")
        else:
            print("截图失败")
    else:
        print("未找到记事本窗口")

if __name__ == "__main__":
    main() 