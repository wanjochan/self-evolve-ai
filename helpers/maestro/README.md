# Maestro

Maestro 是一个高级UI控制和自动化系统，基于YOLO和OmniParser进行UI元素检测，提供命令行和Web API接口。

## 功能特点

- 快速窗口截图捕获（包括后台窗口）
- 基于YOLO的UI元素检测
- 实时窗口监控能力
- 自动化鼠标和键盘控制
- 提供命令行和Web API接口

## 安装

1. 创建并激活conda环境：
```bash
conda create -n maestro python=3.12 -y
conda activate maestro
```

2. 安装依赖：
```bash
pip install -r requirements.txt
```

3. 下载所需模型：
```bash
mkdir -p weights/icon_detect
# 下载模型文件到weights目录
```

## 命令行使用

### 分析窗口UI元素

```bash
python -m maestro.cli.main analyze "窗口标题"
```

### 监控窗口

```bash
python -m maestro.cli.main monitor "窗口标题1" "窗口标题2"
```

### 捕获窗口截图

```bash
python -m maestro.cli.main capture "窗口标题" -o screenshot.png
```

### 点击UI元素

```bash
python -m maestro.cli.main click "窗口标题" button
```

## Web API使用

启动Web服务器：

```bash
python -m maestro.web_server
```

### API端点

- `GET /api/windows` - 列出所有可见窗口
- `GET /api/windows/{window_title}/analyze` - 分析窗口UI元素
- `GET /api/windows/{window_title}/capture` - 捕获窗口截图
- `POST /api/windows/{window_title}/click` - 点击窗口中的UI元素
- `POST /api/windows/{window_title}/monitor` - 开始监控窗口
- `DELETE /api/windows/{window_title}/monitor` - 停止监控窗口
- `GET /api/monitor/status` - 获取监控状态

### 示例

点击窗口中的按钮：

```bash
curl -X POST http://localhost:5000/api/windows/Notepad/click \
  -H "Content-Type: application/json" \
  -d '{"element_type": "button", "button": "left", "double": false}'
```

## 编程使用

```python
from maestro import UIDetector, WindowCapture, InputController

# 初始化
detector = UIDetector()
capture = WindowCapture()
controller = InputController()

# 分析窗口
window_title = "记事本"
elements = detector.analyze_window(window_title)

# 点击按钮
for elem in elements:
    if elem.type.value == "button":
        controller.click_element(elem)
        break
```

## 注意事项

- 确保已安装所需的模型文件
- 某些窗口可能需要管理员权限才能进行操作
- 在Windows上使用时，可能需要以管理员身份运行程序 