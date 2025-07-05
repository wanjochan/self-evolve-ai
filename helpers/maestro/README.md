# Maestro CLI工具

Maestro是一个简化的窗口管理和UI自动化工具，可以帮助您分析和控制Windows窗口。

## 功能

- 列出所有可见窗口
- 详细分析窗口内容和UI元素
- 控制窗口状态（激活、最小化、最大化等）
- 执行鼠标操作（点击、移动等）
- 执行键盘操作（输入文本、按键等）

## 安装依赖

```bash
pip install -r requirements.txt
```

## 使用方法

### 列出所有窗口

```bash
python maestro_cli.py list
```

### 详细分析窗口

```bash
# 通过窗口标题分析
python maestro_cli.py detail "窗口标题"

# 通过窗口HWND分析
python maestro_cli.py detail 12345 -t hwnd

# 通过进程PID分析
python maestro_cli.py detail 6789 -t pid

# 保存分析结果到JSON文件
python maestro_cli.py detail "窗口标题" -o result.json

# 保存窗口截图
python maestro_cli.py detail "窗口标题" -s

# 快速模式（减少输出并加快分析速度）
python maestro_cli.py detail "窗口标题" -f

# 安静模式（减少输出）
python maestro_cli.py detail "窗口标题" -q
```

### 控制窗口状态

```bash
# 激活窗口
python maestro_cli.py window "窗口标题" activate

# 最小化窗口
python maestro_cli.py window "窗口标题" minimize

# 最大化窗口
python maestro_cli.py window "窗口标题" maximize

# 恢复窗口
python maestro_cli.py window "窗口标题" restore

# 关闭窗口
python maestro_cli.py window "窗口标题" close
```

### 执行鼠标操作

```bash
# 在指定位置点击
python maestro_cli.py mouse "窗口标题" click -x 100 -y 200

# 右键点击
python maestro_cli.py mouse "窗口标题" click -x 100 -y 200 -b right

# 双击
python maestro_cli.py mouse "窗口标题" click -x 100 -y 200 -d

# 移动鼠标到指定位置
python maestro_cli.py mouse "窗口标题" move -x 100 -y 200

# 获取当前鼠标位置
python maestro_cli.py mouse "窗口标题" current
```

### 执行键盘操作

```bash
# 输入文本
python maestro_cli.py keyboard "窗口标题" type "要输入的文本"

# 按下按键
python maestro_cli.py keyboard "窗口标题" key "enter"

# 按下组合键
python maestro_cli.py keyboard "窗口标题" hotkey "ctrl+c"
```

## 参数说明

### detail命令参数

- `window_identifier`: 窗口标识符（标题、HWND或PID）
- `-t, --type`: 标识符类型，可选值为`title`、`hwnd`、`pid`，默认为`title`
- `-o, --output`: 将分析结果保存到JSON文件
- `-s, --save-screenshot`: 保存窗口截图
- `-f, --fast`: 快速模式，减少输出并加快分析速度
- `-q, --quiet`: 安静模式，减少输出

### window命令参数

- `window_title`: 窗口标题
- `action`: 窗口操作，可选值为`activate`、`minimize`、`maximize`、`restore`、`close`

### mouse命令参数

- `window_title`: 窗口标题
- `action`: 鼠标操作，可选值为`click`、`move`、`current`
- `-x`: X坐标
- `-y`: Y坐标
- `-b, --button`: 鼠标按钮，可选值为`left`、`right`、`middle`，默认为`left`
- `-d, --double`: 双击

### keyboard命令参数

- `window_title`: 窗口标题
- `action`: 键盘操作，可选值为`type`、`key`、`hotkey`
- `keys`: 要输入的文本、按键或组合键

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