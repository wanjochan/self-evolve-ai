#!/bin/bash

# 检查是否有人类消息或指示
echo "=== 检查人类消息 ==="
echo "检查时间: $(date)"
echo "当前工作目录: $(pwd)"

# 检查是否有新的指示文件
if [ -f "human_message.txt" ]; then
    echo "发现人类消息文件:"
    cat human_message.txt
    exit 1
fi

# 检查是否有停止指示
if [ -f "STOP" ] || [ -f "stop" ]; then
    echo "发现停止指示"
    exit 1
fi

# 检查git状态
if command -v git >/dev/null 2>&1; then
    echo "Git状态检查:"
    git status --porcelain | head -5
fi

echo "✅ 没有发现人类消息，继续工作"
exit 0
