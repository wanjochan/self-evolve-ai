# VSCode UI Automation 工具集

本工具集基于Windows UI Automation (UIA) 技术，专门用于自动化VSCode中的Augment对话处理。

## 🚀 核心功能

### 1. UIA模块 (`uia_module.py`)
- **优化的UI Automation封装**，支持高效的组件树扫描
- **TextPattern支持**，能够访问Electron应用中的DOM级别文本内容
- **智能缓存机制**，提高搜索和分析性能
- **多种查找策略**，支持按名称、类型、文本内容等条件查找元素

### 2. Keep Going 自动处理器 (`vscode_keep_going_handler.py`)
- **智能检测** "Would you like me to keep going?" 提示
- **自动回复** "请继续" 并发送回车键
- **监控模式** 持续检查并自动处理
- **详细日志** 支持调试和故障排除

### 3. TextPattern 搜索测试工具 (`test_textpattern_search.py`)
- **文本搜索验证** 测试UIA文本提取功能
- **全面搜索测试** 验证多种文本模式的检测能力
- **结果保存** 支持将搜索结果保存为JSON格式

## 🛠️ 技术特点

### UI Automation 增强
- **TextPattern集成**: 使用UIA TextPattern API访问Electron应用的文本内容
- **多层级搜索**: 递归搜索UI树的多个层级
- **模式识别**: 支持Value、LegacyIAccessible等多种UIA模式
- **错误处理**: 完善的异常处理和资源清理

### 搜索优化
- **深度控制**: 可配置的搜索深度限制
- **性能优化**: 智能缓存和批量操作
- **灵活匹配**: 支持大小写敏感/不敏感搜索
- **多短语搜索**: 同时搜索多个关键词

## 📋 使用方法

### 基本用法

#### 1. 单次检查
```bash
python tools/vscode_keep_going_handler.py
```

#### 2. 监控模式
```bash
python tools/vscode_keep_going_handler.py --monitor --interval 5
```

#### 3. 详细输出
```bash
python tools/vscode_keep_going_handler.py --verbose
```

### 测试和验证

#### 1. 测试TextPattern搜索
```bash
python tools/test_textpattern_search.py --verbose
```

#### 2. 搜索自定义文本
```bash
python tools/test_textpattern_search.py --search "your text" --verbose
```

#### 3. 全面搜索测试
```bash
python tools/test_textpattern_search.py --comprehensive --verbose
```

## 🔧 配置选项

### Keep Going Handler 参数
- `--monitor, -m`: 启用监控模式
- `--interval, -i`: 设置监控间隔（秒）
- `--max-checks`: 最大检查次数
- `--verbose, -v`: 详细输出

### TextPattern 测试参数
- `--search, -s`: 搜索指定文本
- `--comprehensive, -c`: 全面搜索测试
- `--save`: 保存结果到文件
- `--verbose, -v`: 详细输出

## 📊 测试结果

### 当前验证状态
✅ **UI元素检测**: 成功检测到VSCode基本UI结构
✅ **TextPattern搜索**: 能够提取部分文本内容
✅ **Augment元素识别**: 成功找到7个Augment相关元素
✅ **自动输入功能**: 文本输入和键盘操作正常工作

### 搜索能力验证
- **"augment"**: ✅ 找到 6-7 个匹配结果
- **"file"**: ✅ 找到 1 个匹配结果  
- **"edit"**: ✅ 找到 1 个匹配结果
- **"would"**: ❌ 当前未检测到（可能文本尚未出现）

## 🚨 已知限制

### Electron应用特殊性
- **DOM访问限制**: Electron应用的某些内容可能无法通过标准UIA访问
- **动态内容**: 对话内容可能动态生成，需要实时检测
- **渲染层级**: 某些文本可能在Chrome渲染进程的深层结构中

### 解决方案
1. **增加搜索深度**: 目前支持最大8层深度搜索
2. **多模式检测**: 结合TextPattern、Value、LegacyIAccessible等多种模式
3. **监控模式**: 持续检测直到目标文本出现

## 🔄 工作流程

```
1. 初始化UIA连接
   ↓
2. 查找VSCode窗口
   ↓  
3. 获取UI根元素
   ↓
4. 递归搜索文本内容
   ↓
5. 检测"keep going"提示
   ↓
6. 自动输入"请继续"
   ↓
7. 发送回车键
```

## 🎯 未来改进

### 计划功能
- [ ] **图像识别集成**: 结合OCR技术处理无法通过UIA访问的内容
- [ ] **Chrome DevTools Protocol**: 探索CDP方式直接访问Electron DOM
- [ ] **机器学习优化**: 使用ML模型提高文本检测准确率
- [ ] **多语言支持**: 支持更多语言的"继续"指令

### 性能优化
- [ ] **并行搜索**: 多线程并行搜索不同UI分支
- [ ] **智能缓存**: 更智能的缓存策略减少重复扫描
- [ ] **增量更新**: 只检测变化的UI区域

## 📚 技术参考

- [Microsoft UI Automation](https://docs.microsoft.com/en-us/windows/win32/winauto/entry-uiauto-win32)
- [TextPattern Control Pattern](https://docs.microsoft.com/en-us/windows/win32/winauto/uiauto-implementingtextandtextrange)
- [Electron Accessibility](https://www.electronjs.org/docs/latest/tutorial/accessibility)
- [Chrome UI Automation Support](https://developer.chrome.com/blog/windows-uia-support)

## 🤝 贡献

欢迎提交Issue和Pull Request来改进这个工具集！

---

*最后更新: 2024-12-30* 