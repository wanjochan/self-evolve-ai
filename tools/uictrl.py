#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
UI控制API服务器
基于FastAPI构建，提供与桌面GUI交互的端点
"""

import os
import sys
import json
import base64
import subprocess
from typing import List, Dict, Optional, Any, Union
from enum import Enum

import uvicorn
from fastapi import FastAPI, HTTPException, Response, Query, Path, Body
from fastapi.responses import JSONResponse
from pydantic import BaseModel, Field

# Windows平台特定导入
import ctypes
from ctypes import wintypes
import win32gui
import win32con
import win32process
import win32api
from PIL import ImageGrab

# 创建FastAPI应用
app = FastAPI(
    title="UI控制API",
    description="提供与桌面GUI交互的API端点",
    version="1.0.0",
)

# 数据模型定义
class WindowBasic(BaseModel):
    """基本窗口信息模型"""
    id: str
    title: str

class Position(BaseModel):
    """位置坐标模型"""
    x: int
    y: int

class Size(BaseModel):
    """尺寸模型"""
    width: int
    height: int

class WindowDetail(WindowBasic):
    """详细窗口信息模型"""
    position: Position
    size: Size
    process_id: int

class WindowControlAction(str, Enum):
    """窗口控制动作枚举"""
    MOVE = "move"
    RESIZE = "resize"
    CLOSE = "close"
    MINIMIZE = "minimize"
    MAXIMIZE = "maximize"
    RESTORE = "restore"

class WindowControlRequest(BaseModel):
    """窗口控制请求模型"""
    action: WindowControlAction
    payload: Optional[Dict[str, Any]] = None

class MouseAction(str, Enum):
    """鼠标动作枚举"""
    MOVE = "move"
    CLICK = "click"
    DOUBLE_CLICK = "double_click"
    RIGHT_CLICK = "right_click"

class MouseRequest(BaseModel):
    """鼠标请求模型"""
    action: MouseAction
    x: int
    y: int

class KeyboardAction(str, Enum):
    """键盘动作枚举"""
    TYPE_TEXT = "type_text"
    PRESS_KEYS = "press_keys"

class KeyboardPayload(BaseModel):
    """键盘操作负载模型"""
    text: Optional[str] = None
    keys: Optional[List[str]] = None

class KeyboardRequest(BaseModel):
    """键盘请求模型"""
    action: KeyboardAction
    payload: KeyboardPayload

class ExecuteCommandRequest(BaseModel):
    """执行命令请求模型"""
    command: str
    args: Optional[List[str]] = []
    async_exec: bool = Field(True, alias="async")

class ApiResponse(BaseModel):
    """API响应模型"""
    status: str
    message: str

# 窗口操作辅助函数
def enum_windows() -> List[WindowBasic]:
    """枚举所有顶层窗口"""
    result = []
    
    def callback(hwnd, _):
        if win32gui.IsWindowVisible(hwnd) and win32gui.GetWindowText(hwnd):
            result.append(WindowBasic(
                id=str(hwnd),
                title=win32gui.GetWindowText(hwnd)
            ))
        return True
    
    win32gui.EnumWindows(callback, None)
    return result

def get_window_details(hwnd: int) -> WindowDetail:
    """获取窗口详细信息"""
    # 获取窗口位置和大小
    rect = win32gui.GetWindowRect(hwnd)
    left, top, right, bottom = rect
    
    # 获取进程ID
    _, process_id = win32process.GetWindowThreadProcessId(hwnd)
    
    return WindowDetail(
        id=str(hwnd),
        title=win32gui.GetWindowText(hwnd),
        position=Position(x=left, y=top),
        size=Size(width=right-left, height=bottom-top),
        process_id=process_id
    )

def capture_window_screenshot(hwnd: int) -> bytes:
    """捕获窗口截图"""
    # 获取窗口位置和大小
    rect = win32gui.GetWindowRect(hwnd)
    left, top, right, bottom = rect
    
    # 截取窗口图像
    screenshot = ImageGrab.grab(bbox=(left, top, right, bottom))
    
    # 转换为PNG字节
    import io
    img_byte_arr = io.BytesIO()
    screenshot.save(img_byte_arr, format='PNG')
    return img_byte_arr.getvalue()

def control_window(hwnd: int, action: WindowControlAction, payload: Optional[Dict[str, Any]] = None) -> str:
    """控制窗口"""
    if action == WindowControlAction.MOVE:
        if not payload or "x" not in payload or "y" not in payload:
            raise ValueError("Move action requires x and y coordinates")
        win32gui.SetWindowPos(hwnd, 0, payload["x"], payload["y"], 0, 0, 
                            win32con.SWP_NOSIZE | win32con.SWP_NOZORDER)
        return "Window moved successfully"
    
    elif action == WindowControlAction.RESIZE:
        if not payload or "width" not in payload or "height" not in payload:
            raise ValueError("Resize action requires width and height")
        rect = win32gui.GetWindowRect(hwnd)
        win32gui.SetWindowPos(hwnd, 0, rect[0], rect[1], 
                            payload["width"], payload["height"], 
                            win32con.SWP_NOMOVE | win32con.SWP_NOZORDER)
        return "Window resized successfully"
    
    elif action == WindowControlAction.CLOSE:
        win32gui.PostMessage(hwnd, win32con.WM_CLOSE, 0, 0)
        return "Window close command sent"
    
    elif action == WindowControlAction.MINIMIZE:
        win32gui.ShowWindow(hwnd, win32con.SW_MINIMIZE)
        return "Window minimized successfully"
    
    elif action == WindowControlAction.MAXIMIZE:
        win32gui.ShowWindow(hwnd, win32con.SW_MAXIMIZE)
        return "Window maximized successfully"
    
    elif action == WindowControlAction.RESTORE:
        win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
        return "Window restored successfully"
    
    else:
        raise ValueError(f"Unsupported action: {action}")

def execute_command(command: str, args: List[str] = None, async_exec: bool = True) -> Dict[str, Any]:
    """执行命令"""
    if args is None:
        args = []
    
    full_command = [command] + args
    
    if async_exec:
        # 异步执行
        process = subprocess.Popen(full_command)
        return {
            "status": "started",
            "pid": process.pid,
            "command": " ".join(full_command)
        }
    else:
        # 同步执行
        result = subprocess.run(full_command, capture_output=True, text=True)
        return {
            "status": "completed",
            "returncode": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
            "command": " ".join(full_command)
        }

def send_mouse_input(hwnd: int, action: MouseAction, x: int, y: int) -> str:
    """发送鼠标输入"""
    # 获取窗口客户区域相对于屏幕的位置
    rect = win32gui.GetWindowRect(hwnd)
    client_rect = win32gui.GetClientRect(hwnd)
    
    # 计算客户区域的左上角在屏幕上的坐标
    left, top, _, _ = rect
    
    # 计算鼠标在屏幕上的绝对坐标
    screen_x = left + x
    screen_y = top + y
    
    # 设置鼠标位置
    win32api.SetCursorPos((screen_x, screen_y))
    
    if action == MouseAction.MOVE:
        return f"Mouse moved to ({x}, {y})"
    
    elif action == MouseAction.CLICK:
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        return f"Mouse clicked at ({x}, {y})"
    
    elif action == MouseAction.DOUBLE_CLICK:
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0, 0, 0)
        return f"Mouse double-clicked at ({x}, {y})"
    
    elif action == MouseAction.RIGHT_CLICK:
        win32api.mouse_event(win32con.MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0)
        win32api.mouse_event(win32con.MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0)
        return f"Mouse right-clicked at ({x}, {y})"
    
    else:
        raise ValueError(f"Unsupported mouse action: {action}")

def send_keyboard_input(hwnd: int, action: KeyboardAction, payload: KeyboardPayload) -> str:
    """发送键盘输入到指定窗口
    
    注意：此函数不需要窗口在前台即可工作
    """
    import win32con
    import win32api
    import time
    
    def send_char(char: str):
        """发送单个字符到窗口"""
        # 获取虚拟键码
        vk = win32api.VkKeyScan(char)
        # 获取扫描码
        scan = win32api.MapVirtualKey(vk & 0xff, 0)
        
        # 发送按键按下事件
        win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, vk, 0x00000001 | (scan << 16))
        # 发送字符事件
        win32api.PostMessage(hwnd, win32con.WM_CHAR, ord(char), 0x00000001 | (scan << 16))
        # 发送按键释放事件
        win32api.PostMessage(hwnd, win32con.WM_KEYUP, vk, 0xC0000001 | (scan << 16))
    
    if action == KeyboardAction.TYPE_TEXT:
        if not payload.text:
            raise ValueError("Type_text action requires text payload")
        
        # 发送每个字符
        for char in payload.text:
            send_char(char)
            time.sleep(0.01)  # 小延迟，确保输入顺序正确
            
        return f"Text sent to window {hwnd}"
    
    elif action == KeyboardAction.PRESS_KEYS:
        if not payload.keys:
            raise ValueError("Press_keys action requires keys payload")
        
        # 发送组合键
        for key in payload.keys:
            # 这里简化处理，实际应该处理特殊键
            vk = win32api.VkKeyScan(key[0].lower() if key.isalpha() else key)
            scan = win32api.MapVirtualKey(vk & 0xff, 0)
            
            # 发送按键按下
            win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, vk, 0x00000001 | (scan << 16))
            
        # 发送按键释放（逆序）
        for key in reversed(payload.keys):
            vk = win32api.VkKeyScan(key[0].lower() if key.isalpha() else key)
            scan = win32api.MapVirtualKey(vk & 0xff, 0)
            win32api.PostMessage(hwnd, win32con.WM_KEYUP, vk, 0xC0000001 | (scan << 16))
            
        return f"Keys {', '.join(payload.keys)} sent to window {hwnd}"
    
    else:
        raise ValueError(f"Unsupported keyboard action: {action}")

# API路由实现
@app.get("/")
def read_root():
    """根路由，返回API信息"""
    return {
        "name": "UI控制API",
        "version": "1.0.0",
        "description": "提供与桌面GUI交互的API端点",
        "docs_url": "/docs",
        "redoc_url": "/redoc"
    }

@app.get("/windows", response_model=List[WindowBasic])
def get_windows():
    """获取所有窗口列表"""
    return enum_windows()

@app.get("/windows/{window_id}", response_model=WindowDetail)
def get_window(window_id: str = Path(..., description="窗口的唯一标识符")):
    """获取指定窗口的详细信息"""
    try:
        hwnd = int(window_id)
        if not win32gui.IsWindow(hwnd):
            raise HTTPException(status_code=404, detail="Window not found")
        return get_window_details(hwnd)
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid window_id format")

@app.get("/windows/{window_id}/screenshot")
def get_window_screenshot(window_id: str = Path(..., description="窗口的唯一标识符")):
    """获取指定窗口的截图"""
    try:
        hwnd = int(window_id)
        if not win32gui.IsWindow(hwnd):
            raise HTTPException(status_code=404, detail="Window not found")
        
        screenshot_data = capture_window_screenshot(hwnd)
        return Response(content=screenshot_data, media_type="image/png")
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid window_id format")
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Screenshot failed: {str(e)}")

@app.post("/windows/{window_id}/control", response_model=ApiResponse)
def control_window_endpoint(
    request: WindowControlRequest,
    window_id: str = Path(..., description="窗口的唯一标识符")
):
    """控制窗口"""
    try:
        hwnd = int(window_id)
        if not win32gui.IsWindow(hwnd):
            raise HTTPException(status_code=404, detail="Window not found")
        
        message = control_window(hwnd, request.action, request.payload)
        return ApiResponse(status="success", message=message)
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Control operation failed: {str(e)}")

@app.post("/execute")
def execute_command_endpoint(request: ExecuteCommandRequest):
    """执行命令"""
    try:
        result = execute_command(request.command, request.args, request.async_exec)
        return JSONResponse(status_code=202, content=result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Command execution failed: {str(e)}")

@app.post("/windows/{window_id}/mouse", response_model=ApiResponse)
def mouse_control(
    request: MouseRequest,
    window_id: str = Path(..., description="窗口的唯一标识符")
):
    """鼠标控制"""
    try:
        hwnd = int(window_id)
        if not win32gui.IsWindow(hwnd):
            raise HTTPException(status_code=404, detail="Window not found")
        
        message = send_mouse_input(hwnd, request.action, request.x, request.y)
        return ApiResponse(status="success", message=message)
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Mouse operation failed: {str(e)}")

@app.post("/windows/{window_id}/keyboard", response_model=ApiResponse)
def keyboard_control(
    request: KeyboardRequest,
    window_id: str = Path(..., description="窗口的唯一标识符")
):
    """键盘控制"""
    try:
        hwnd = int(window_id)
        if not win32gui.IsWindow(hwnd):
            raise HTTPException(status_code=404, detail="Window not found")
        
        message = send_keyboard_input(hwnd, request.action, request.payload)
        return ApiResponse(status="success", message=message)
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Keyboard operation failed: {str(e)}")

# 主入口
if __name__ == "__main__":
    uvicorn.run("uictrl:app", host="127.0.0.1", port=9091, reload=True)
