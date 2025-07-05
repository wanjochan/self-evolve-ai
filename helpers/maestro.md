# Maestro CLI 智能VSCode自动化项目

## 📋 项目需求

### 核心目标
让VSCode中的"self-evolve-ai"窗口能够持续工作，当AI助手询问"Would you like me to keep going?"时自动回复"continue"，避免工作被中断。

### 具体需求
1. **智能检测**: 能够识别VSCode窗口中是否出现需要回复的提示
2. **自动响应**: 检测到提示时自动发送"continue"回复
3. **无干扰操作**: 不需要激活窗口，后台操作
4. **智能判断**: 只在真正需要时才回复，避免误操作
5. **持续监控**: 能够长时间稳定运行

## 🎯 当前已实现功能

### 1. Maestro CLI工具 (`maestro_cli.py`)
- ✅ **窗口检测**: 可以列出所有窗口
- ✅ **窗口截图**: 可以捕获指定窗口的截图
- ✅ **UI元素检测**: 使用OmniParser检测UI元素位置和类型
- ✅ **键盘输入**: 支持发送文本和按键（包括--no-activate模式）
- ✅ **鼠标操作**: 支持点击操作

#### 主要命令
```bash
# 列出所有窗口
python maestro_cli.py list

# 详细分析窗口（包含截图和UI检测）
python maestro_cli.py detail "窗口标题" -s -o output.json

# 发送键盘输入（不激活窗口）
python maestro_cli.py keyboard "窗口标题" type "continue" --no-activate
python maestro_cli.py keyboard "窗口标题" key "Return" --no-activate

# 鼠标点击
python maestro_cli.py mouse "窗口标题" click 100 200
```

### 2. UI检测器改进 (`ui_ctrl_v2/ui_detector.py`)
- ✅ **智能类型推断**: 基于位置、大小、宽高比智能推断UI元素类型
- ✅ **多种元素类型**: 支持button、icon、text、link、tab、menu等类型
- ✅ **OmniParser集成**: 使用Microsoft OmniParser模型进行UI检测

#### 检测效果
- 从原来的93个元素全部识别为"button"
- 改进为97个元素分为5种类型：46个图标、29个按钮、16个文本、5个链接、1个标签页

### 3. 智能分析工具
- ✅ **窗口内容分析**: 可以分析VSCode窗口的布局和内容
- ✅ **UI元素统计**: 统计各类型UI元素的分布
- ✅ **状态推断**: 基于UI元素推断窗口当前状态

## ❌ 当前存在的问题

### 1. 文本识别能力不足
- **问题**: OmniParser的icon_detect模型只能检测UI元素位置，无法读取文本内容
- **影响**: 无法识别"Would you like me to keep going?"等具体文本提示
- **现状**: 只能基于UI元素的位置和类型进行推断，准确性有限

### 2. 缺少OCR或文本识别
- **问题**: 没有集成OCR功能来读取界面上的文本
- **需要**: 能够识别界面中的具体文字内容
- **选项**: 
  - 集成OmniParser的icon_caption模型
  - 使用外部OCR库（如easyocr、pytesseract）
  - 接入大模型视觉API（GPT-4V、Claude Vision）

### 3. 智能判断逻辑不完善
- **问题**: 当前只能基于启发式规则进行判断
- **需要**: 更智能的上下文理解和决策逻辑
- **建议**: 接入大模型进行语义分析

## 🔧 需要完善的功能

### 1. 文本识别模块
```python
# 需要实现的功能
def extract_text_from_screenshot(image_path):
    """从截图中提取文本内容"""
    # 选项1: 使用OmniParser icon_caption模型
    # 选项2: 使用OCR库
    # 选项3: 调用大模型视觉API
    pass

def detect_continue_prompt(text_content):
    """检测是否包含需要回复continue的提示"""
    prompts = [
        "Would you like me to keep going?",
        "Should I continue?",
        "Continue?",
        "Do you want me to proceed?"
    ]
    # 实现文本匹配逻辑
    pass
```

### 2. 智能监控循环
```python
# 需要实现的功能
def intelligent_monitor():
    """智能监控VSCode窗口"""
    while True:
        # 1. 检查窗口是否存在
        # 2. 捕获窗口截图
        # 3. 提取文本内容
        # 4. 检测是否需要回复
        # 5. 执行自动回复
        # 6. 等待下次检查
        pass
```

### 3. 配置和日志系统
- **配置文件**: 支持自定义监控间隔、目标窗口、回复内容等
- **日志记录**: 详细记录监控活动、检测结果、操作历史
- **错误处理**: 完善的异常处理和恢复机制

## 🚀 推荐的解决方案

### 方案1: 大模型视觉分析（推荐）
```python
# 使用GPT-4V或Claude Vision分析截图
def analyze_with_ai_vision(screenshot_path):
    """使用大模型分析截图内容"""
    # 1. 将截图转换为base64
    # 2. 构建分析提示
    # 3. 调用AI API
    # 4. 解析返回结果
    pass
```

**优势**: 
- 智能理解上下文
- 准确识别各种提示格式
- 可以处理复杂的界面状态

### 方案2: OCR + 规则匹配
```python
# 使用OCR提取文本，然后规则匹配
def ocr_based_detection(screenshot_path):
    """基于OCR的文本检测"""
    # 1. 使用easyocr或pytesseract提取文本
    # 2. 清理和预处理文本
    # 3. 使用正则表达式匹配关键词
    # 4. 返回检测结果
    pass
```

**优势**:
- 成本较低
- 响应速度快
- 不依赖外部API

### 方案3: OmniParser完整流程
```python
# 使用OmniParser的完整功能
def omniparser_full_analysis(screenshot_path):
    """使用OmniParser完整分析"""
    # 1. icon_detect检测UI元素位置
    # 2. icon_caption生成元素描述
    # 3. 基于描述判断是否需要回复
    pass
```

## 📁 项目文件结构

```
helpers/
├── maestro/
│   ├── maestro_cli.py          # 主CLI工具
│   └── maestro/
│       └── core/
│           ├── ui_detector.py  # UI检测器
│           └── window_capture.py # 窗口捕获
├── ui_ctrl_v2/
│   ├── ui_detector.py          # 改进的UI检测器
│   └── ui_types.py            # UI元素类型定义
├── weights/
│   └── icon_detect/           # OmniParser模型文件
├── analyze_ui_types.py        # UI类型分析工具
├── analyze_vscode_content.py  # VSCode内容分析
├── intelligent_vscode_solution.py # 智能解决方案演示
└── maestro.md                 # 本文档
```

## 🎯 下一步行动计划

### 优先级1: 文本识别
1. 选择并实现文本识别方案（推荐大模型视觉分析）
2. 测试文本提取的准确性
3. 实现关键词检测逻辑

### 优先级2: 智能监控
1. 实现完整的监控循环
2. 添加配置文件支持
3. 完善错误处理和日志

### 优先级3: 优化和测试
1. 性能优化（减少资源占用）
2. 稳定性测试（长时间运行）
3. 准确性调优（减少误判）

## 💡 技术建议

1. **API密钥管理**: 如果使用大模型API，需要安全地管理API密钥
2. **频率控制**: 避免过于频繁的检测，建议10-30秒间隔
3. **资源优化**: 截图和分析比较耗资源，需要优化
4. **容错机制**: 网络异常、API限制等情况的处理
5. **用户控制**: 提供启动/停止、暂停/恢复等控制功能

## 🔗 相关资源

- [OmniParser GitHub](https://github.com/microsoft/OmniParser)
- [OmniParser HuggingFace](https://huggingface.co/microsoft/OmniParser-v2.0)
- [YOLO Ultralytics](https://github.com/ultralytics/ultralytics)
- [EasyOCR](https://github.com/JaidedAI/EasyOCR)
- [OpenAI GPT-4V API](https://platform.openai.com/docs/guides/vision)

---

**最后更新**: 2025-01-05
**状态**: 需要完善文本识别功能
**优先级**: 高
