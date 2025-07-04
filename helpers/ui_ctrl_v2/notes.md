# 激活 conda 环境
conda activate omni

# 克隆仓库 & 环境准备
git clone https://github.com/microsoft/OmniParser.git
cd OmniParser
conda create -n omni python=3.12 -y
conda activate omni

# 安装依赖 + CLI 工具
pip install -r requirements.txt
pip install ultralytics
pip install -U "huggingface_hub[cli]"

# 下载模型权重到 weights 目录
mkdir -p weights
for f in icon_detect/{model.pt,model.yaml} icon_caption/{config.json,generation_config.json,model.safetensors}; do
  huggingface-cli download microsoft/OmniParser-v2.0 "$f" --local-dir weights
done
mv weights/icon_caption weights/icon_caption_florence

# 启动 Gradio demo 查看按钮/链接/菜单检测效果
python gradio_demo.py

# do screendump of specified windows(since ui_ctrl_v1)
//dump screenshot.png

# try do screendump with yolo + omniparser-model
python - << 'EOF'
from ultralytics import YOLO
model = YOLO('weights/icon_detect/model.pt')
results = model.predict('screenshot.png', imgsz=640, conf=0.25)
results.print()
results.show()  # 或 results.save()
EOF

# future:
- dump image in memory to predict;
- maybe 'monitor' screendump stream to keep the layout as new as possible (parallel working on all main windows?)
- the ai-assistent help human-master to do mouse/keyboard with the latest full layout