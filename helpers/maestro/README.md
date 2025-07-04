# Maestro CLI工具

简单的窗口管理和UI自动化工具。

## 功能

- 列出所有可见窗口
- 详细分析指定窗口，包括UI元素检测
- 控制窗口状态（激活、最小化、最大化、恢复、关闭）
- 执行鼠标操作（移动、点击）
- 执行键盘操作（输入文本、按键、组合键）

## 使用方法

### 列出所有窗口

```
python maestro_cli.py list
```

这将显示所有可见窗口的HWND、PID和标题。

### 详细分析指定窗口

```
python maestro_cli.py detail "窗口标题" [-o 输出文件.json] [-s] [-f]
```

参数说明：
- `窗口标题`: 要分析的窗口标题（部分匹配）
- `-o, --output`: 可选，将分析结果保存到指定的JSON文件
- `-s, --save-screenshot`: 可选，保存窗口截图
- `-f, --fast`: 可选，快速模式，减少输出并加快分析速度

这将对标题包含指定文本的窗口进行详细分析：
1. 显示窗口的基本信息
2. 捕获窗口截图（但默认不保存）
3. 使用YOLO和OmniParser模型检测UI元素
4. 默认将结果输出到标准输出，可选择保存到JSON文件

### 控制窗口状态

```
python maestro_cli.py window "窗口标题" <操作>
```

参数说明：
- `窗口标题`: 要控制的窗口标题（部分匹配）
- `操作`: 窗口操作，可选值：
  - `activate`: 激活窗口
  - `minimize`: 最小化窗口
  - `maximize`: 最大化窗口
  - `restore`: 恢复窗口
  - `close`: 关闭窗口

### 执行鼠标操作

```
python maestro_cli.py mouse "窗口标题" <操作> [-x X坐标] [-y Y坐标] [-b 按钮] [-d]
```

参数说明：
- `窗口标题`: 要操作的窗口标题（部分匹配）
- `操作`: 鼠标操作，可选值：
  - `click`: 点击指定位置
  - `move`: 移动鼠标到指定位置
  - `current`: 获取当前鼠标位置
- `-x`: X坐标（click和move操作需要）
- `-y`: Y坐标（click和move操作需要）
- `-b, --button`: 鼠标按钮，可选值：left（默认）、right、middle
- `-d, --double`: 双击

### 执行键盘操作

```
python maestro_cli.py keyboard "窗口标题" <操作> <按键>
```

参数说明：
- `窗口标题`: 要操作的窗口标题（部分匹配）
- `操作`: 键盘操作，可选值：
  - `type`: 输入文本
  - `key`: 按下特定按键
  - `hotkey`: 按下组合键（使用+连接，如ctrl+c）
- `按键`: 要输入的文本、按键或组合键

## 示例

```
# 列出所有窗口
python maestro_cli.py list

# 详细分析窗口（快速模式）
python maestro_cli.py detail "Cursor" -f

# 保存分析结果和截图
python maestro_cli.py detail "Cursor" -o cursor_analysis.json -s

# 激活窗口
python maestro_cli.py window "Notepad" activate

# 在窗口中点击
python maestro_cli.py mouse "Notepad" click -x 100 -y 100

# 在窗口中输入文本
python maestro_cli.py keyboard "Notepad" type "Hello World"

# 在窗口中按下组合键
python maestro_cli.py keyboard "Notepad" hotkey "ctrl+s"
```

## 依赖

- Python 3.6+
- pywin32
- ui_ctrl_v2 (可选，用于UI元素检测和高级输入控制)
  - ultralytics (YOLO)
  - Pillow
  - numpy 