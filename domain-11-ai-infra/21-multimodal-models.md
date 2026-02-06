# 21 - 多模态模型融合与部署

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **参考**: [CLIP](https://github.com/openai/CLIP) | [LLaVA](https://github.com/haotian-liu/LLaVA) | [Whisper](https://github.com/openai/whisper) | [ImageBind](https://github.com/facebookresearch/ImageBind)

## 一、多模态AI架构全景

### 1.1 多模态融合架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        Multimodal AI Fusion Architecture                            │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Modality Encoders                                   │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │ Vision      │  │ Audio       │  │ Text        │  │ Thermal     │          │  │
│  │  │ Encoder     │  │ Encoder     │  │ Encoder     │  │ Encoder     │          │  │
│  │  │ (ViT/ResNet)│  │ (Wav2Vec)   │  │ (BERT)      │  │ (CNN)       │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                        Cross-modal Alignment Layer                            │  │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐  │  │
│  │  │                              Projection Heads                           │  │  │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐                │  │  │
│  │  │  │ Vision   │  │ Audio    │  │ Text     │  │ Thermal  │                │  │  │
│  │  │  │ Projector│  │ Projector│  │ Projector│  │ Projector│                │  │  │
│  │  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘                │  │  │
│  │  └─────────────────────────────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                         Unified Embedding Space                               │  │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐  │  │
│  │  │                              Joint Embedding                            │  │  │
│  │  │  Dimension: 512-1024                                                    │  │  │
│  │  │  Normalized: Cosine Similarity                                          │  │  │
│  │  └─────────────────────────────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 多模态模型对比矩阵

| 模型 | 模态 | 参数量 | 显存需求 | 推理延迟 | 适用场景 | 技术特点 |
|------|------|--------|----------|----------|----------|----------|
| **CLIP** | 图像+文本 | 400M-1.2B | 2-8GB | 50-200ms | 图像检索、零样本分类 | 对比学习、双编码器 |
| **LLaVA** | 图像+文本 | 7B-34B | 15-60GB | 200-800ms | 视觉问答、图像理解 | 指令微调、多模态LLM |
| **Whisper** | 音频+文本 | 150M-1.5B | 1-6GB | 100-500ms | 语音识别、翻译 | 编码器-解码器、多任务 |
| **ImageBind** | 6模态 | 1.2B | 8-12GB | 150-300ms | 跨模态检索、内容理解 | 统一嵌入空间、零样本迁移 |
| **BLIP-2** | 图像+文本 | 7B-175B | 20-300GB | 300-1500ms | 图像生成、视觉对话 | Q-Former、大型语言模型 |
| **AudioLDM** | 音频+文本 | 1.5B | 10-15GB | 500-2000ms | 音频生成、音效合成 | 扩散模型、条件生成 |

## 一、多模态模型对比

| 模型 | 模态 | 参数量 | 显存 | 适用场景 |
|-----|------|-------|------|---------|
| **CLIP** | 图像+文本 | 400M | 4GB | 图像检索/分类 |
| **LLaVA** | 图像+文本 | 7B-13B | 18-26GB | 视觉问答 |
| **Whisper** | 音频+文本 | 1.5B | 6GB | 语音识别 |
| **ImageBind** | 6模态 | 1.2B | 8GB | 跨模态检索 |
| **GPT-4V** | 图像+文本 | 未知 | API | 通用视觉理解 |

## 二、CLIP图像文本匹配系统

### 2.1 CLIP生产级部署架构

```yaml
# clip-production-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: clip-service-production
  namespace: multimodal
spec:
  replicas: 6
  selector:
    matchLabels:
      app: clip-service
  template:
    metadata:
      labels:
        app: clip-service
        version: v1.2.0
    spec:
      containers:
      - name: clip-inference
        image: nvidia/pytorch:23.10-py3
        command:
        - python
        - /app/clip_server.py
        env:
        - name: MODEL_NAME
          value: "openai/clip-vit-large-patch14"
        - name: BATCH_SIZE
          value: "64"
        - name: MAX_CONCURRENT_REQUESTS
          value: "100"
        - name: CACHE_TTL_SECONDS
          value: "3600"
        ports:
        - containerPort: 8000
          name: http
        resources:
          requests:
            cpu: "2"
            memory: "8Gi"
            nvidia.com/gpu: "1"
          limits:
            cpu: "4"
            memory: "16Gi"
            nvidia.com/gpu: "1"
        volumeMounts:
        - name: model-cache
          mountPath: /root/.cache/huggingface
        - name: shared-memory
          mountPath: /dev/shm
        livenessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 60
          periodSeconds: 30
        readinessProbe:
          httpGet:
            path: /ready
            port: 8000
          initialDelaySeconds: 30
          periodSeconds: 10
      volumes:
      - name: model-cache
        emptyDir: {}
      - name: shared-memory
        emptyDir:
          medium: Memory
          sizeLimit: 2Gi
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: nvidia.com/gpu.product
                operator: In
                values:
                - A10G
                - T4
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - clip-service
              topologyKey: kubernetes.io/hostname
---
apiVersion: v1
kind: Service
metadata:
  name: clip-service
  namespace: multimodal
spec:
  selector:
    app: clip-service
  ports:
  - port: 80
    targetPort: 8000
  type: ClusterIP
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: clip-hpa
  namespace: multimodal
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: clip-service-production
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Pods
    pods:
      metric:
        name: queue_depth
      target:
        type: AverageValue
        averageValue: "50"
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
```

### 2.2 CLIP服务核心实现

```python
# clip_server.py
import torch
from transformers import CLIPProcessor, CLIPModel
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import asyncio
import time
from prometheus_client import Counter, Histogram, Gauge, generate_latest
import numpy as np
from PIL import Image
import io
import base64
import redis.asyncio as redis
import logging

# Prometheus指标
REQUEST_COUNT = Counter('clip_requests_total', 'Total CLIP requests', ['endpoint', 'status'])
REQUEST_LATENCY = Histogram('clip_request_duration_seconds', 'CLIP request latency', ['endpoint'])
GPU_UTILIZATION = Gauge('clip_gpu_utilization_percent', 'GPU utilization')
CACHE_HIT_RATE = Gauge('clip_cache_hit_rate', 'Cache hit rate')

app = FastAPI(title="CLIP Multimodal Service")

class ImageTextRequest(BaseModel):
    image_data: str  # base64 encoded
    texts: list[str]
    return_embeddings: bool = False

class SearchResult(BaseModel):
    similarities: list[float]
    best_match: str
    best_similarity: float
    embeddings: dict = None

class CLIPService:
    def __init__(self):
        self.device = "cuda" if torch.cuda.is_available() else "cpu"
        self.model = CLIPModel.from_pretrained("openai/clip-vit-large-patch14").to(self.device)
        self.processor = CLIPProcessor.from_pretrained("openai/clip-vit-large-patch14")
        self.cache = redis.Redis(host='redis-master', port=6379, decode_responses=True)
        self.logger = logging.getLogger(__name__)
        
    async def encode_image(self, image_data: str) -> torch.Tensor:
        """编码图像"""
        try:
            # 解码base64图像
            image_bytes = base64.b64decode(image_data)
            image = Image.open(io.BytesIO(image_bytes)).convert('RGB')
            
            # 预处理
            inputs = self.processor(images=image, return_tensors="pt").to(self.device)
            
            # 编码
            with torch.no_grad():
                image_features = self.model.get_image_features(**inputs)
                image_features = image_features / image_features.norm(dim=-1, keepdim=True)
            
            return image_features.cpu()
        except Exception as e:
            self.logger.error(f"Image encoding failed: {e}")
            raise HTTPException(status_code=400, detail="Invalid image data")
    
    async def encode_texts(self, texts: list[str]) -> torch.Tensor:
        """编码文本"""
        try:
            inputs = self.processor(text=texts, return_tensors="pt", padding=True).to(self.device)
            
            with torch.no_grad():
                text_features = self.model.get_text_features(**inputs)
                text_features = text_features / text_features.norm(dim=-1, keepdim=True)
            
            return text_features.cpu()
        except Exception as e:
            self.logger.error(f"Text encoding failed: {e}")
            raise HTTPException(status_code=400, detail="Text encoding failed")
    
    async def search_similarities(self, image_features: torch.Tensor, text_features: torch.Tensor) -> list[float]:
        """计算相似度"""
        # 计算余弦相似度
        similarities = torch.matmul(image_features, text_features.T)
        return similarities.squeeze().tolist()

# 初始化服务
clip_service = CLIPService()

@app.on_event("startup")
async def startup_event():
    """服务启动初始化"""
    # 预热模型
    dummy_image = torch.randn(1, 3, 224, 224).to(clip_service.device)
    dummy_text = clip_service.processor(text=["warmup"], return_tensors="pt", padding=True).to(clip_service.device)
    
    with torch.no_grad():
        clip_service.model.get_image_features(pixel_values=dummy_image)
        clip_service.model.get_text_features(**dummy_text)
    
    clip_service.logger.info("CLIP service initialized and warmed up")

@app.get("/health")
async def health_check():
    return {"status": "healthy", "device": clip_service.device}

@app.get("/ready")
async def readiness_check():
    try:
        # 简单的就绪检查
        torch.cuda.synchronize() if torch.cuda.is_available() else None
        return {"status": "ready"}
    except Exception:
        raise HTTPException(status_code=503, detail="Service not ready")

@app.post("/search", response_model=SearchResult)
@REQUEST_LATENCY.labels(endpoint='search').time()
async def search_similarities(request: ImageTextRequest):
    """图像-文本相似度搜索"""
    start_time = time.time()
    
    try:
        # 编码图像和文本
        image_features = await clip_service.encode_image(request.image_data)
        text_features = await clip_service.encode_texts(request.texts)
        
        # 计算相似度
        similarities = await clip_service.search_similarities(image_features, text_features)
        
        # 找到最佳匹配
        best_idx = np.argmax(similarities)
        result = SearchResult(
            similarities=similarities,
            best_match=request.texts[best_idx],
            best_similarity=float(similarities[best_idx]),
            embeddings={
                "image_embedding": image_features.squeeze().tolist(),
                "text_embeddings": text_features.tolist()
            } if request.return_embeddings else None
        )
        
        REQUEST_COUNT.labels(endpoint='search', status='success').inc()
        return result
        
    except Exception as e:
        REQUEST_COUNT.labels(endpoint='search', status='error').inc()
        clip_service.logger.error(f"Search failed: {e}")
        raise HTTPException(status_code=500, detail=str(e))
    
    finally:
        duration = time.time() - start_time
        REQUEST_LATENCY.labels(endpoint='search').observe(duration)

@app.get("/metrics")
async def metrics():
    """Prometheus指标端点"""
    return generate_latest()

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
```

### 2.3 CLIP生产运维最佳实践

```yaml
# clip-monitoring-config.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: clip-monitoring-rules
  namespace: multimodal
spec:
  groups:
  - name: clip.rules
    rules:
    # 性能指标
    - alert: HighClipLatency
      expr: histogram_quantile(0.99, rate(clip_request_duration_seconds_bucket[5m])) > 0.5
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "CLIP P99延迟超过500ms"
        description: "CLIP服务响应时间异常，请检查GPU资源和模型状态"
    
    - alert: LowClipAccuracy
      expr: |
        avg(clip_similarity_scores) < 0.7
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "CLIP相似度得分偏低"
        description: "模型准确性下降，可能需要重新训练或调整"
    
    # 资源指标
    - alert: HighGPUMemoryUsage
      expr: |
        avg(clip_gpu_memory_utilization_percent) > 90
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "CLIP GPU内存使用率过高"
        description: "GPU内存接近上限，可能导致OOM错误"
    
    - alert: LowCacheHitRate
      expr: |
        clip_cache_hit_rate < 0.6
      for: 15m
      labels:
        severity: info
      annotations:
        summary: "CLIP缓存命中率偏低"
        description: "缓存效率不佳，考虑调整缓存策略或增加缓存容量"

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: clip-grafana-dashboard
  namespace: monitoring
data:
  dashboard.json: |
    {
      "dashboard": {
        "title": "CLIP Multimodal Service Monitoring",
        "panels": [
          {
            "title": "Request Rate and Latency",
            "type": "graph",
            "targets": [
              {
                "expr": "rate(clip_requests_total[1m])",
                "legendFormat": "{{status}}"
              },
              {
                "expr": "histogram_quantile(0.95, rate(clip_request_duration_seconds_bucket[5m]))",
                "legendFormat": "P95 Latency"
              }
            ]
          },
          {
            "title": "GPU Utilization",
            "type": "gauge",
            "targets": [
              {
                "expr": "avg(clip_gpu_utilization_percent)",
                "legendFormat": "GPU Utilization %"
              }
            ]
          },
          {
            "title": "Cache Performance",
            "type": "graph",
            "targets": [
              {
                "expr": "clip_cache_hit_rate",
                "legendFormat": "Hit Rate"
              },
              {
                "expr": "rate(redis_cache_hits_total[1m])",
                "legendFormat": "Cache Hits/sec"
              }
            ]
          },
          {
            "title": "Similarity Score Distribution",
            "type": "heatmap",
            "targets": [
              {
                "expr": "clip_similarity_scores",
                "legendFormat": "Scores"
              }
            ]
          }
        ]
      }
    }
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
