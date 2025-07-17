# AI 工作流

本文档概述了项目中 AI 会话的标准工作流程。

## 工作流程图

```mermaid
flowchart TD
    Start["会话开始"] --> TaskType{"任务类型?"}
    TaskType -->|标准任务| CheckWorkId{"确定work_id"}
    TaskType -->|微任务| QuickMode["快速模式"]
    TaskType -->|紧急任务| UrgentMode["紧急模式"]

    QuickMode --> SimpleExecute["直接执行"]
    SimpleExecute --> QuickLog["简要记录结果"]
    QuickLog --> End["结束会话"]

    UrgentMode --> UrgentExecute["优先执行关键步骤"]
    UrgentExecute --> PostActionDoc["事后补充文档"]
    PostActionDoc --> End

    CheckWorkId --> |已知work_id| ReadDocs["强制重新阅读文档<br/>workflow.md<br/>workplan_{work_id}.md<br/>worknotes_{work_id}.md"]
    CheckWorkId --> |新工作流| CreateWorkId["创建新work_id"]
    CreateWorkId --> ReadTemplates["强制阅读模板文档<br/>workplan_template.md<br/>worknotes_template.md<br/>workflow.md"]
    ReadTemplates --> InitDocs["初始化工作流文档"]
    InitDocs --> ReadDocs

    ReadDocs --> CheckInput{"用户有新留言?"}

    CheckInput -->|有| EvaluateInput["评估用户留言"]
    EvaluateInput --> UpdateNeeded{"评估是否需要更新文档?"}
    UpdateNeeded -->|是| UpdateDocs["更新工作文档"]
    UpdateNeeded -->|否| ExecutePlan["执行当前计划"]

    CheckInput -->|无| CheckCompletion{"工作计划已彻底完成?"}
    CheckCompletion -->|是| FinalUpdate["更新文档并结束"]
    CheckCompletion -->|否| UpdateNeeded

    UpdateDocs --> ExecutePlan
    ExecutePlan --> CheckParallel{"是否有并行任务?"}
    CheckParallel -->|是| SplitTasks["拆分并行任务"]
    SplitTasks --> ExecuteParallel["并行执行任务"]
    ExecuteParallel --> WaitComplete["等待所有任务完成"]
    WaitComplete --> MergeResults["合并执行结果"]
    MergeResults --> CheckSuccess{"验证是否成功?"}

    CheckParallel -->|否| NormalExecute["常规执行并验证"]
    NormalExecute --> CheckSuccess
    CheckSuccess -->|成功| UpdateProgress["更新进度文档"]
    CheckSuccess -->|失败| HandleError["处理错误"]
    HandleError --> RetryDecision{"是否重试?"}
    RetryDecision -->|是| ExecutePlan
    RetryDecision -->|否| UpdateFailedStatus["更新失败状态"]
    UpdateFailedStatus --> NextCycle["结束当前回合"]
    
    UpdateProgress --> SubmitProgress["提交且合并到远程"]
    SubmitProgress --> NextCycle
    NextCycle --> ReadDocs

    FinalUpdate --> UpdateFinalStatus["更新最终状态"]
    UpdateFinalStatus --> End

```

## 工作流程说明

1. **会话开始**：每个新的 AI 会话从这里开始

2. **任务类型判断**：
   - 标准任务：需要完整工作流程的常规任务
   - 微任务：简单、短期、无需详细记录的小任务
   - 紧急任务：需要立即执行的高优先级任务

3. **微任务处理**：
   - 直接执行：无需复杂的计划和文档
   - 简要记录：只记录关键结果，不创建详细文档
   - 快速结束：完成后直接结束会话

4. **紧急任务处理**：
   - 优先执行：立即执行关键步骤，不等待文档更新
   - 事后补充：任务完成后再补充必要的文档
   - 快速结束：完成后直接结束会话

5. **标准任务处理**：
   - **确定work_id**：
     - 如果是已知工作流，使用现有的work_id
     - 如果是新工作流，创建新的work_id并初始化相关文档
   - **阅读模板文档**：
     - 阅读 workplan_template.md（工作计划模板）
     - 阅读 worknotes_template.md（工作笔记模板）
     - 阅读 workflow.md（工作流程说明）
     - 确保新创建的文档遵循统一的格式和标准
   - **初始化工作流文档**：
    - 基于模板创建 workplan_{work_id}.md
    - 基于模板创建 worknotes_{work_id}.md
    - 添加新工作流，状态设为 `ACTIVE`
   - **阅读文档**：
     - 阅读 workflow.md（工作流程说明）
     - 阅读 workplan_{work_id}.md（任务非线性分解、动态规划、细节描述）
     - 阅读 worknotes_{work_id}.md（上下文和经验）
   - **检查用户输入**：
     - 如有用户新输入，评估输入内容
     - 如无新输入，检查工作计划完成状态
     - 如用户请求暂停，保存当前状态并暂停工作流
   - **有用户输入时**：
     - 评估是否需要更新 workplan_{work_id}.md 和 worknotes_{work_id}.md
     - 如需要则更新文档
     - 执行当前计划
   - **无用户输入时**：
     - 如工作计划已完成，更新文档并结束会话
     - 如工作计划未完成，直接执行当前计划
   - **执行计划**：
     - 根据 workplan_{work_id}.md 执行下一步
     - 检查是否有标记为 `[PARALLEL]` 的任务组（可选）
     - **重要**：脚本或代码创建后必须实际执行并验证结果
     - **禁止**：不允许仅创建脚本/代码就标记任务为完成

   - **更新进度**：
     - 更新 workplan_{work_id}.md 的进度（仅在验证成功后）
     - 更新 worknotes_{work_id}.md 的上下文和经验
     - 记录遇到的问题和解决方案
   - **提交进度**：
     - 将更新的文档和代码变更提交到版本控制系统
     - 推送到远程仓库以保持同步
     - 确保工作成果得到持久化保存
     - 便于团队协作和进度跟踪
   - **循环完成**：
     - 结束当前回合，返回阅读文档开始下一循环

## 任务完成标准

为确保工作质量，任务只有在满足以下条件时才能标记为完成：

1. **代码/脚本创建**：代码或脚本已经创建并通过基本语法检查
2. **实际执行**：代码或脚本已被实际执行（非模拟执行）
3. **结果验证**：执行结果已被验证符合预期要求
4. **问题修复**：执行过程中发现的所有问题都已修复
5. **文档更新**：相关文档已更新，包括进度、经验和注意事项
6. **时间记录**：记录任务实际执行时间，用于后续估算优化

不满足以上全部条件的任务必须标记为"进行中"而非"已完成"。

## 执行效率优化

### 时间管理
- 每个任务开始前估算预期执行时间
- 记录实际执行时间，建立时间估算数据库
- 对超时任务进行分析和优化

### 资源管理
- 监控系统资源使用情况
- 对资源密集型任务进行调度优化
- 避免同时执行多个高资源消耗任务

此工作流确保任务持续推进，同时保持文档更新并适应用户输入。支持多工作流并行处理，通过唯一的work_id区分不同工作流的文档和状态。同时通过不同的任务模式，适应各种复杂度和紧急程度的工作场景。

## 工作流状态追踪

每个工作流的状态将在 `workplan_{work_id}.md` 中进行集中追踪：

### 状态类型
- `ACTIVE` - 活跃执行中
- `PAUSED` - 已暂停
- `COMPLETED` - 已完成
- `FAILED` - 执行失败

### 状态记录格式
```
{work_id} | [状态] | [当前任务] | [进度百分比]
```

### 状态详情记录
每个状态还应包含：
- **执行日志**：关键操作的详细记录
- **错误信息**：失败时的具体错误描述
- **性能指标**：执行时间、资源使用等
- **依赖关系**：任务间的依赖状态

### 状态更新时机
- 工作流初始化时
- 每个主要阶段完成时
- 任务状态变更时
- 会话结束时

## 并行任务处理（可选）

对于需要并行处理的复杂任务：

### 任务并行化
- 在 `workplan_{work_id}.md` 中使用 `[PARALLEL]` 标记可并行执行的任务组
- 示例：
  ```
  - T1 [50%] 核心功能开发 [PARALLEL]
    - T1.1 [100%] 组件A开发
    - T1.2 [75%] 组件B开发
  ```

### 并行任务管理
- 每个并行任务应有明确的输入和预期输出
- 并行任务之间应尽量减少依赖
- 在 `worknotes_{work_id}.md` 中记录每个并行分支的执行情况

## 工作流优化

### 持续改进
- **定期回顾**：分析工作流执行效率，识别改进点
- **模板更新**：基于实际使用情况优化模板文档
- **流程简化**：移除冗余步骤，优化关键路径