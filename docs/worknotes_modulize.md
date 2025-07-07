# 工作笔记 modulize

本文档包含工作流 modulize 的上下文、经验和工作会话笔记，以保持AI会话之间的连续性。

## 工作流信息
- 工作ID: modulize
- 创建时间: 2024-12-19
- 关联计划: [工作计划文档](workplan_modulize.md)
- 特别注意：不要做历史纪录，只更新最后结果！

## 会话

### 会话：2024-12-19

#### 上下文
- 项目基于 PRD.md 的三层架构设计：Layer 1 (Loader) -> Layer 2 (Runtime/.native) -> Layer 3 (Program/.astc)
- src/core/ 是模块化底层，包含12个功能模块
- 模块系统采用极简主义设计：Module 结构只有 name, state, error, init(), cleanup(), resolve()
- module_module.c 作为"上帝模块"管理所有其他模块

#### 挑战
- 挑战1：架构理解错误 - 最初误解为静态注册机制，实际应为动态.native文件加载
- 挑战2：模块加载机制不完整 - 需要实现真正的.native文件动态加载
- 挑战3：符号解析机制缺失 - 需要从.native文件导出表解析符号
- 挑战4：PRD.md 建议合并编译相关模块 (astc2native, c2astc, codegen)

#### 重要发现
- **架构纠正**: 模块系统应该是动态加载.native文件的机制，类似import，而非静态注册
- **三层架构**: Layer 1 (Loader) -> Layer 2 (.native模块) -> Layer 3 (.astc程序)
- **动态加载**: 使用mmap()映射.native文件，解析NATV格式，提取导出符号表

## 知识库

### 系统架构
- **动态加载架构**: 基于.native文件的动态模块加载系统
- **NATV文件格式**: magic="NATV", 包含头部、代码段、数据段、导出表
- **符号解析**: 从导出表动态解析符号地址 = 基地址 + 代码段偏移 + 符号偏移
- **内存映射**: 使用mmap()将.native文件映射到内存，支持执行权限
- **模块管理**: 支持最多64个动态加载模块，自动管理生命周期
- **符号缓存**: 哈希表缓存已解析符号，提高性能

### 关键组件
- module_module.c：模块管理器，系统核心
- memory_module.c：内存管理，所有模块的基础依赖
- astc_module.c：ASTC字节码处理
- native_module.c：.native格式处理，支持跨架构
- vm_module.c：虚拟机核心，Layer 2 的关键组件

### 重要模式
- 依赖注入模式：模块通过 module_load() 和 module_resolve() 获取依赖
- 符号缓存模式：提高符号解析性能
- 内存池模式：不同用途使用不同内存池，便于管理和优化

### 待添加模块
- FFI 模块：Foreign Function Interface，支持跨语言调用
- AOT 模块：Ahead-of-Time 编译，将 ASTC 编译为原生代码

## 参考资料

- PRD.md：项目需求文档，三层架构设计
- workflow.md：工作流程文档
- src/core/module.h：模块系统核心接口
- src/core/astc.h：ASTC 数据结构定义 