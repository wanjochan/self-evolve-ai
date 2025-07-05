# Maestro 工具

Maestro是一个强大的窗口管理和UI自动化工具，专为与智能助理进行交互而设计，现已优化为更高效的版本。

## 主要特点

- **高效内存管理**：减少文件操作，使用内存缓存分析结果
- **简化的API**：统一且简洁的接口，易于使用
- **交互式操作**：支持交互模式，方便调试和探索
- **批处理功能**：支持从文件批量执行命令
- **性能优化**：减少不必要的截图和分析，提高响应速度

## 安装依赖

```bash
pip install pywin32 pillow numpy pyperclip
```

## 快速开始

### 发送消息

```bash
python maestro.py send "你好，请帮我分析这段代码"
```

### 执行任务

```bash
python maestro.py task "优化这段代码的性能"
```

### 交互模式

```bash
python maestro.py interact --wait
```

### 点击操作

```bash
python maestro.py click 100 200
```

### 按键操作

```bash
python maestro.py key --key enter
python maestro.py key --hotkey "ctrl+c"
```

### 批处理

```bash
python maestro.py batch messages.txt --wait
```

## 命令参考

### 全局参数

- `--window`, `-w`: 指定窗口标题（默认: "Visual Studio Code"）
- `--debug`, `-d`: 启用调试模式

### 子命令

#### send - 发送消息

```bash
python maestro.py send "消息内容"
```

#### task - 执行任务

```bash
python maestro.py task "任务描述" --timeout 30
```

参数:
- `--timeout`, `-t`: 等待响应的超时时间（秒）

#### interact - 交互模式

```bash
python maestro.py interact --wait
```

参数:
- `--wait`, `-t`: 等待响应
- `--timeout`, `-o`: 等待响应的超时时间（秒）

#### click - 点击位置

```bash
python maestro.py click 100 200 --button right --double
```

参数:
- `x`: X坐标
- `y`: Y坐标
- `--button`, `-b`: 鼠标按钮（left/right/middle）
- `--double`, `-d`: 双击

#### key - 按键

```bash
python maestro.py key --key enter
python maestro.py key --hotkey "ctrl+c"
```

参数:
- `--key`, `-k`: 按键名称
- `--hotkey`, `-h`: 组合键

#### batch - 批处理

```bash
python maestro.py batch messages.txt --wait --interval 2.0
```

参数:
- `file`: 批处理文件
- `--wait`, `-t`: 等待响应
- `--timeout`, `-o`: 等待响应的超时时间（秒）
- `--interval`, `-i`: 消息间隔时间（秒）

## 编程接口

### 基本用法

```python
from maestro_core import MaestroCore, send_message, execute_task

# 快速发送消息
send_message("你好，请帮我分析这段代码")

# 执行任务
execute_task("优化这段代码的性能")

# 创建实例并使用更多功能
maestro = MaestroCore(window_title="Visual Studio Code")
maestro.click(100, 200)
maestro.press_key("enter")
maestro.press_hotkey("ctrl+c")
```

### 主要方法

- `send_message(message)`: 发送消息
- `execute_task(task_description)`: 执行任务
- `click(x, y, button="left", double_click=False)`: 点击指定位置
- `press_key(key)`: 按下按键
- `press_hotkey(hotkey)`: 按下组合键
- `wait_for_response(timeout=30)`: 等待响应

## 批处理文件格式

批处理文件是一个文本文件，每行一条消息。以`#`开头的行会被视为注释。

示例:
```
# 这是一个注释
你好，请帮我分析这段代码
def hello():
    print("Hello, World!")
请优化这段代码
```

## 注意事项

- 窗口必须可见且可访问
- 某些功能可能需要管理员权限
- 不同窗口的UI元素位置可能不同，可能需要调整 