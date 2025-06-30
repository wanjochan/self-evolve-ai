# UI控制工具集重构计划

## 1. 背景与目标

### 1.1 背景
当前的UI控制工具集基于Windows UI Automation (UIA)技术，用于获取窗口界面结构树信息，但存在性能问题，特别是在dump_tree和dump_vscode_tree等方法上。在vibe coding氛围编程时代，需要一个高效的工具集来协调不同的智能助理。

### 1.2 目标
重构UI控制工具集，实现更快速地获取窗口界面结构树信息，包括窗口类型、ID、坐标、长宽、内容等，以支持中控智能助理对多智能体会话的协调。

## 2. 现状分析

### 2.1 现有架构
- `uia_module.py`: UI Automation的封装，提供组件树扫描、内容提取等功能
- `uictrl.py`: UI控制工具，提供CLI和Web API接口
- `test_uia_performance.py`: 性能测试脚本

### 2.2 性能瓶颈
1. **递归遍历效率低**: 当前使用递归方式遍历UI树，深度过大时性能下降严重
2. **同步API调用**: UIA API调用为同步操作，阻塞主线程
3. **缓存机制不完善**: 现有缓存策略未充分利用UI树的静态特性
4. **数据结构冗余**: 返回的结构树包含过多冗余信息

## 3. 技术路线选择

### 3.1 可选技术路线
1. **优化现有UIA实现**
   - 优点: 兼容性好，无需额外依赖
   - 缺点: UIA本身存在性能限制

2. **Win32 API直接调用**
   - 优点: 性能更高，更底层的控制
   - 缺点: 实现复杂，跨应用兼容性挑战

3. **混合方案: Win32 API + UIA**
   - 优点: 结合两者优势，对不同窗口类型使用最适合的技术
   - 缺点: 实现复杂度增加

4. **Chrome DevTools Protocol (CDP)**
   - 优点: 对Electron应用(VSCode, Cursor等)有更好的支持
   - 缺点: 仅适用于基于Chromium的应用

### 3.2 推荐技术路线
采用**混合方案**，结合Win32 API和优化后的UIA:
- 使用Win32 API快速获取窗口基本信息和层次结构
- 对特定应用(如VSCode, Cursor)使用针对性优化
- 保留UIA作为后备方案，处理复杂控件

## 4. 架构设计

### 4.1 核心组件
1. **WindowTreeDumper**: 快速窗口树采集器
   - 使用Win32 API实现高效窗口遍历
   - 支持并行处理和增量更新

2. **SpecializedDumpers**: 特定应用优化采集器
   - ChromiumDumper: 针对Electron应用
   - VSCodeDumper: 针对VSCode特化
   - CursorDumper: 针对Cursor IDE特化

3. **UITreeCache**: 树结构缓存管理
   - 实现增量更新机制
   - 支持部分树更新

### 4.2 数据结构
```python
@dataclass
class UINode:
    id: str                    # 节点唯一标识
    type: str                  # 节点类型
    name: str                  # 节点名称
    rect: Rect                 # 位置和大小
    attributes: Dict[str, Any] # 附加属性
    children: List["UINode"]   # 子节点
    content: Optional[str]     # 文本内容
```

### 4.3 接口设计
1. **命令行接口**
```
uictrl dump-tree [--window-id ID] [--max-depth N] [--format json|yaml] [--output FILE]
uictrl find-element [--window-id ID] [--name NAME] [--type TYPE] [--content CONTENT]
uictrl monitor-changes [--window-id ID] [--interval MS] [--output FILE]
```

2. **编程接口**
```python
# 快速获取窗口树
tree = uictrl.dump_window_tree(window_id, max_depth=5)

# 查找特定元素
elements = uictrl.find_elements(window_id, name="button", type="Button")

# 监控变化
for change in uictrl.monitor_changes(window_id, interval_ms=500):
    print(f"Changed: {change}")
```

## 5. 性能优化策略

### 5.1 并行处理
- 使用线程池并行处理子树
- 对大型窗口树进行分区处理

### 5.2 增量更新
- 记录上次扫描状态
- 只更新变化的部分

### 5.3 智能缓存
- 缓存静态UI元素
- 使用LRU策略管理缓存大小

### 5.4 采样和过滤
- 支持采样模式，只获取关键节点
- 提供过滤机制，减少无关节点

## 6. 实现路线图

### 6.1 阶段一: 基础框架 (1-2天)
- 设计核心数据结构
- 实现Win32 API基础调用
- 创建基本CLI框架

### 6.2 阶段二: 快速树采集 (2-3天)
- 实现WindowTreeDumper
- 添加并行处理支持
- 优化基本性能

### 6.3 阶段三: 特化优化 (3-4天)
- 实现ChromiumDumper
- 添加VSCode和Cursor特化支持
- 集成增量更新机制

### 6.4 阶段四: 工具化和集成 (1-2天)
- 完善CLI接口
- 添加Web API支持
- 编写文档和示例

## 7. 测试与验证

### 7.1 性能基准测试
- 与现有实现对比
- 测量大型窗口树的处理时间
- 测量内存使用情况

### 7.2 功能测试
- 验证树结构正确性
- 测试特定应用支持
- 验证增量更新准确性

### 7.3 集成测试
- 与中控智能助理集成测试
- 多应用场景测试

## 8. 风险与挑战

### 8.1 已识别风险
1. Win32 API对现代应用支持有限
2. 不同版本Windows兼容性问题
3. Electron应用内部DOM访问限制

### 8.2 缓解策略
1. 保留UIA作为后备方案
2. 添加版本检测和适配
3. 探索CDP等替代技术

## 9. 结论与建议

建议采用混合技术路线，结合Win32 API的高性能和UIA的兼容性，同时针对特定应用进行优化。重点关注并行处理和增量更新机制，以显著提升性能。

初步估计，通过重构可以将窗口树获取性能提升5-10倍，同时保持或提高数据准确性。
