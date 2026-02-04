# 61 - 多模态模型部署

> **适用版本**: v1.25 - v1.32 | **参考**: [CLIP](https://github.com/openai/CLIP)

## 一、多模态模型对比

| 模型 | 模态 | 参数量 | 显存 | 适用场景 |
|-----|------|-------|------|---------|
| **CLIP** | 图像+文本 | 400M | 4GB | 图像检索/分类 |
| **LLaVA** | 图像+文本 | 7B-13B | 18-26GB | 视觉问答 |
| **Whisper** | 音频+文本 | 1.5B | 6GB | 语音识别 |
| **ImageBind** | 6模态 | 1.2B | 8GB | 跨模态检索 |
| **GPT-4V** | 图像+文本 | 未知 | API | 通用视觉理解 |

## 二、CLIP部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: clip-service
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: clip
        image: pytorch/pytorch:2.1.0-cuda12.1
        command:
        - python
        - serve_clip.py
        resources:
          limits:
            nvidia.com/gpu: 1
            memory: "8Gi"
---
# serve_clip.py
from transformers import CLIPProcessor, CLIPModel
from fastapi import FastAPI
import torch

app = FastAPI()
model = CLIPModel.from_pretrained("openai/clip-vit-base-patch32")
processor = CLIPProcessor.from_pretrained("openai/clip-vit-base-patch32")

@app.post("/embed/image")
def embed_image(image_url: str):
    image = load_image(image_url)
    inputs = processor(images=image, return_tensors="pt")
    embeddings = model.get_image_features(**inputs)
    return embeddings.tolist()

@app.post("/embed/text")
def embed_text(text: str):
    inputs = processor(text=[text], return_tensors="pt")
    embeddings = model.get_text_features(**inputs)
    return embeddings.tolist()
```

## 三、LLaVA视觉问答

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llava-vqa
spec:
  template:
    spec:
      containers:
      - name: llava
        image: liuhaotian/llava:v1.5-13b
        resources:
          limits:
            nvidia.com/gpu: 1
            memory: "32Gi"
---
# 调用示例
curl -X POST http://llava-service/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "llava-v1.5-13b",
    "messages": [
      {
        "role": "user",
        "content": [
          {"type": "text", "text": "What is in this image?"},
          {"type": "image_url", "image_url": "https://example.com/image.jpg"}
        ]
      }
    ]
  }'
```

## 四、Whisper语音识别

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: whisper-asr
spec:
  replicas: 5
  template:
    spec:
      containers:
      - name: whisper
        image: pytorch/pytorch:2.1.0-cuda12.1
        command:
        - python
        - whisper_server.py
        resources:
          limits:
            nvidia.com/gpu: 1
---
# whisper_server.py
import whisper
from fastapi import FastAPI, File, UploadFile

app = FastAPI()
model = whisper.load_model("large-v3")

@app.post("/transcribe")
async def transcribe(audio: UploadFile = File(...)):
    result = model.transcribe(audio.file)
    return {
        "text": result["text"],
        "language": result["language"]
    }
```

## 五、性能优化

| 优化项 | 方法 | 效果 |
|--------|------|------|
| **批处理** | 批量处理图像/音频 | 吞吐↑5x |
| **TensorRT** | 模型编译优化 | 速度↑3x |
| **量化** | INT8量化 | 显存↓50% |
| **缓存** | 缓存embeddings | 命中率20% |

## 六、成本分析

**CLIP图像检索 (1M图像库, 1000 QPS):**
- GPU: 10×T4 = $1,200/月
- 存储: 1M×512维×4字节 = 2GB = $0.05/月
- 总成本: ~$1,200/月

**Whisper语音转录 (100并发):**
- GPU: 5×T4 = $600/月
- Spot优化: $180/月 (节省70%)

## 七、监控指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: multimodal-alerts
spec:
  groups:
  - name: clip
    rules:
    - alert: HighEmbeddingLatency
      expr: histogram_quantile(0.99, rate(clip_embedding_duration_seconds_bucket[5m])) > 1
      annotations:
        summary: "CLIP embedding延迟>1秒"
```

## 八、最佳实践

1. **模型选择**:
   - 图像检索: CLIP ViT-B/32
   - 视觉问答: LLaVA-1.5-13B
   - 语音识别: Whisper Large-v3

2. **批处理配置**:
   - CLIP: 批次64
   - Whisper: 批次16
   - LLaVA: 批次4

3. **缓存策略**:
   - 图像embeddings: Redis缓存1小时
   - 热门查询: 缓存24小时

---
**相关**: [116-LLM Serving](../116-llm-serving-architecture.md) | **版本**: transformers 4.36+
