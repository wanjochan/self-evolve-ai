# Self-Evolving AI

## 项目愿景

自举的AI系统，自我进化能力。采用三层架构（Loader + Runtime + Program），脱离对外部编译器依赖，集成AI驱动代码进化功能

## 重要文档

- docs/PRD.md  产品文档
- docs/plan.md 主力工作智能工作文档
- docs/cursor_running.md 任务执行状态跟踪
- docs/{agentName}.md 其它智能助理评估文档

## VibeTeam 智能助理团队：

- augment+vscode：工程化推进 PRD.md，是团队的主力；//目前augment的工程化能力不错
- rovodev-atlassian：不断审阅和评估更新 docs/revodev.md
- gemini-cli-google：不断审阅和评估更新 docs/gemini.md
- claude-code：不断审阅和评估更新 docs/claude.md
- 类似地，cursor,windsurf等 docs/{agentName}.md

![workmode](README.jpg)

## 工具

### Maestro 智能助理窗口管理工具

Maestro是一个强大的窗口管理和UI自动化工具，可以帮助您分析和控制Windows窗口，特别是与智能助理进行交互。

主要功能：
- 智能助理窗口对话管理
- 任务自动化执行和状态跟踪
- UI元素检测和交互
- 窗口控制和自动化操作

使用方法：
```bash
# 基本窗口分析
python helpers/maestro/maestro_cli.py detail "Visual Studio Code"

# 发送消息给智能助理
python helpers/maestro/assistant_manager.py --message "请分析这段代码"

# 任务管理和自动化执行
python helpers/maestro/task_manager.py --run-all

# 运行演示
python helpers/maestro/demo.py
```

详细文档请查看 [helpers/maestro/README.md](helpers/maestro/README.md)

## ref

rovodev: https://community.atlassian.com/forums/Rovo-for-Software-Teams-Beta/Introducing-Rovo-Dev-CLI-AI-Powered-Development-in-your-terminal/ba-p/3043623

# JUST FOR FUN
