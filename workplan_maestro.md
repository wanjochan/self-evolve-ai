# Maestro macOS 适配工作计划

## 项目概述

Maestro 是一个 RPA（Robotic Process Automation）工具，专注于窗口管理和 UI 自动化，目前仅支持 Windows 平台。本项目旨在扩展 Maestro，使其能够在 macOS 上运行，同时保持现有 API 的兼容性。

## 当前状态

Maestro 当前实现严重依赖 Windows API（win32gui, win32api, win32con）和 ctypes 调用，主要用于：
- 窗口检测和激活
- 屏幕截图和 UI 元素识别
- 鼠标和键盘输入模拟
- 剪贴板操作

## 目标

1. 设计并实现跨平台抽象层
2. 为 macOS 实现窗口管理、屏幕捕获、输入控制等功能
3. 重构现有代码以使用新的抽象层
4. 确保现有脚本和工作流程在 macOS 上能够正常运行
5. 更新文档和示例

## 工作计划

### 阶段 1: 设计抽象层（预计 2 天）

- [x] 分析当前架构和依赖
- [x] 识别 Windows 特定代码
- [x] 确定需要修改的关键组件
- [x] 设计平台无关的接口
  - [x] 窗口管理接口
  - [x] 屏幕捕获接口
  - [x] 输入控制接口
  - [x] 剪贴板操作接口
- [x] 规划 Windows 实现适配
- [x] 设计 macOS 实现方案

### 阶段 2: 实现 macOS 支持（预计 5 天）

- [x] 研究并选择 macOS API 方案（pyobjc, Quartz 等）
- [x] 实现窗口管理功能
  - [x] 窗口查找
  - [x] 窗口激活
  - [x] 窗口位置和大小操作
- [x] 实现屏幕捕获功能
  - [x] 窗口截图
  - [x] 区域截图
- [x] 实现鼠标控制功能
  - [x] 鼠标移动
  - [x] 鼠标点击
  - [x] 鼠标拖拽
- [x] 实现键盘输入功能
  - [x] 文本输入
  - [x] 键盘快捷键
  - [x] 特殊键操作
- [x] 实现剪贴板操作功能
  - [x] 剪贴板读取
  - [x] 剪贴板写入

### 阶段 3: 重构现有代码（预计 3 天）

- [ ] 重构 input_controller.py
  - [ ] 实现平台检测
  - [ ] 重构为使用抽象接口
  - [ ] 确保向后兼容性
- [ ] 重构 window_capture.py
  - [ ] 实现平台检测
  - [ ] 重构为使用抽象接口
- [ ] 更新 maestro_core.py
  - [ ] 更新以使用新的抽象接口
  - [ ] 添加平台特定的处理逻辑
- [ ] 更新 CLI 接口
  - [ ] 更新命令行参数处理
  - [ ] 添加平台特定的选项

### 阶段 4: 测试与优化（预计 3 天）

- [ ] 编写单元测试
  - [ ] 核心功能测试
  - [ ] 平台特定实现测试
- [ ] 进行集成测试
  - [ ] 测试完整工作流程
  - [ ] 测试跨平台兼容性
- [ ] 性能优化
  - [ ] 识别性能瓶颈
  - [ ] 优化资源使用
- [ ] 错误处理改进
  - [ ] 增强错误检测
  - [ ] 改进错误消息

### 阶段 5: 文档与发布（预计 2 天）

- [ ] 更新安装说明
  - [ ] 更新依赖要求
  - [ ] 添加 macOS 特定安装步骤
- [ ] 更新使用文档
  - [ ] 更新 API 文档
  - [ ] 添加平台特定注意事项
- [ ] 创建示例脚本
  - [ ] 跨平台示例
  - [ ] macOS 特定示例

## 技术方案

### 抽象层设计

将创建以下抽象接口：

```python
class WindowManager:
    def find_window(self, window_title: str) -> bool:
        """Find window by title and store its handle"""
        pass
        
    def get_window_rect(self) -> Tuple[int, int, int, int]:
        """Get window rectangle coordinates"""
        pass
        
    # 其他窗口管理方法...

class ScreenCapture:
    def capture(self) -> Optional[Image.Image]:
        """Capture window content as PIL Image"""
        pass
        
    # 其他屏幕捕获方法...

class InputController:
    def move_mouse(self, x: int, y: int) -> None:
        """Move mouse cursor to specified coordinates"""
        pass
        
    def click(self, x: int, y: int, button: str = "left") -> None:
        """Perform mouse click at specified coordinates"""
        pass
        
    def type_text(self, text: str) -> None:
        """Type text using keyboard"""
        pass
        
    # 其他输入控制方法...

class ClipboardManager:
    def get_text(self) -> str:
        """Get text from clipboard"""
        pass
        
    def set_text(self, text: str) -> None:
        """Set text to clipboard"""
        pass
```

### macOS 实现方案

将使用以下技术实现 macOS 支持：

1. **窗口管理**：使用 pyobjc 和 Quartz 访问 macOS 窗口管理 API
   ```python
   from AppKit import NSWorkspace, NSScreen
   from Quartz import CGWindowListCopyWindowInfo, kCGWindowListOptionOnScreenOnly
   ```

2. **屏幕捕获**：使用 Quartz 的屏幕捕获功能
   ```python
   from Quartz import CGWindowListCreateImage, CGRectNull, kCGWindowListOptionOnScreenOnly
   ```

3. **输入控制**：使用 pyobjc 和 Quartz 事件
   ```python
   from Quartz import CGEventCreateMouseEvent, CGEventPost, kCGEventMouseMoved
   ```

4. **剪贴板操作**：使用 pyperclip 或 AppKit
   ```python
   from AppKit import NSPasteboard, NSStringPboardType
   ```

### 平台检测

使用以下方法进行平台检测：

```python
import platform

def is_windows():
    return platform.system() == "Windows"

def is_macos():
    return platform.system() == "Darwin"
```

## 依赖项

macOS 实现将需要以下依赖：

- pyobjc: 访问 macOS Objective-C API
- pyobjc-framework-Quartz: 屏幕捕获和事件处理
- pyobjc-framework-AppKit: UI 和窗口管理
- pillow: 图像处理（已有）
- numpy: 数据处理（已有）
- pyperclip: 剪贴板操作（已有）

## 注意事项

1. **macOS 安全限制**：
   - 需要用户授予辅助功能权限
   - 可能需要在沙盒外运行

2. **API 差异**：
   - Windows 和 macOS 的窗口管理方式有根本差异
   - macOS 使用 bundle ID 和进程 ID 而非窗口句柄
   - 输入模拟机制不同

3. **兼容性**：
   - 确保现有脚本不需要修改即可在 macOS 上运行
   - 处理平台特定的边缘情况

## 进度跟踪

- 开始日期：2025-07-14
- 预计完成日期：2025-07-29
- 当前阶段：重构现有代码
