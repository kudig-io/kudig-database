# LLM模型Serving架构与推理优化

## 一、LLM推理架构全景

```
┌──────────────────────────────────────────────────────────────────────────┐
│                        LLM Serving完整架构                                │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  ┌────────────┐       ┌────────────┐       ┌────────────┐               │
│  │  负载均衡   │──────▶│  路由层     │──────▶│  推理引擎   │               │
│  │ Istio/Nginx│       │ KServe     │       │ vLLM/TGI   │               │
│  └────────────┘       │ Seldon Core│       │ TensorRT   │               │
│       │               └────────────┘       │ Triton     │               │
│       │                     │              └────────────┘               │
│       ▼                     ▼                    │                       │
│  ┌────────────┐       ┌────────────┐            ▼                       │
│  │  认证鉴权   │       │  模型管理   │       ┌────────────┐               │
│  │  API Key   │       │  版本控制   │       │  GPU调度    │               │
│  │  OAuth2    │       │  A/B测试   │       │  MIG/MPS   │               │
│  └────────────┘       └────────────┘       │  Time-Slice│               │
│                                             └────────────┘               │
│  ┌────────────┐       ┌────────────┐       ┌────────────┐               │
│  │  请求队列   │──────▶│  批处理层   │──────▶│  缓存层     │               │
│  │  优先级     │       │ Continuous │       │  Redis     │               │
│  │  限流       │       │  Batching  │       │  KV Cache  │               │
│  └────────────┘       └────────────┘       └────────────┘               │
│                                                                            │
│  ┌────────────────────────────────────────────────────────┐              │
│  │              可观测性层                                 │              │
│  │  • Prometheus指标  • Jaeger链路追踪  • 日志聚合        │              │
│  └────────────────────────────────────────────────────────┘              │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## 二、vLLM高性能推理引擎

### 2.1 vLLM核心技术

**PagedAttention机制：**
- 将KV Cache分页存储，类似操作系统虚拟内存
- 显存利用率提升至95%（传统方式仅60%）
- 支持动态批处理，吞吐量提升10-20倍

**Continuous Batching：**
- 传统批处理：等待批次内所有请求完成
- Continuous Batching：请求完成立即替换新请求，GPU利用率最大化

### 2.2 vLLM部署配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: vllm-config
  namespace: ai-platform
data:
  start_server.sh: |
    #!/bin/bash
    python -m vllm.entrypoints.openai.api_server \
      --model /models/llama-2-13b \
      --tensor-parallel-size 4 \
      --gpu-memory-utilization 0.95 \
      --max-num-seqs 256 \
      --max-model-len 4096 \
      --dtype float16 \
      --enforce-eager \
      --disable-log-requests \
      --trust-remote-code
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-llama2-13b
  namespace: ai-platform
spec:
  replicas: 2
  selector:
    matchLabels:
      app: vllm-server
      model: llama2-13b
  template:
    metadata:
      labels:
        app: vllm-server
        model: llama2-13b
    spec:
      # 绑定到GPU节点
      nodeSelector:
        nvidia.com/gpu.product: NVIDIA-A100-SXM4-80GB
      
      # 反亲和性：不同节点提高可用性
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - vllm-server
              topologyKey: kubernetes.io/hostname
      
      containers:
      - name: vllm-server
        image: vllm/vllm-openai:v0.3.0
        command: ["/bin/bash", "/config/start_server.sh"]
        ports:
        - containerPort: 8000
          name: http
          protocol: TCP
        
        env:
        - name: HF_HOME
          value: "/models/.cache"
        - name: CUDA_VISIBLE_DEVICES
          value: "0,1,2,3"  # 4卡张量并行
        
        # GPU资源请求
        resources:
          requests:
            cpu: "16"
            memory: "64Gi"
            nvidia.com/gpu: 4
          limits:
            cpu: "32"
            memory: "128Gi"
            nvidia.com/gpu: 4
        
        # 健康检查
        livenessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 120
          periodSeconds: 30
          timeoutSeconds: 10
        
        readinessProbe:
          httpGet:
            path: /v1/models
            port: 8000
          initialDelaySeconds: 60
          periodSeconds: 10
        
        volumeMounts:
        - name: model-storage
          mountPath: /models
          readOnly: true
        - name: config
          mountPath: /config
        - name: shm
          mountPath: /dev/shm
      
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: llama2-13b-pvc
      - name: config
        configMap:
          name: vllm-config
          defaultMode: 0755
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: 32Gi  # 共享内存用于张量并行通信
---
apiVersion: v1
kind: Service
metadata:
  name: vllm-llama2-13b-svc
  namespace: ai-platform
spec:
  type: ClusterIP
  ports:
  - port: 8000
    targetPort: 8000
    protocol: TCP
    name: http
  selector:
    app: vllm-server
    model: llama2-13b
```

### 2.3 vLLM客户端调用

```python
# OpenAI兼容API调用
from openai import OpenAI

client = OpenAI(
    base_url="http://vllm-llama2-13b-svc.ai-platform.svc.cluster.local:8000/v1",
    api_key="dummy-key"  # vLLM默认不需要真实key
)

# 流式生成
stream = client.chat.completions.create(
    model="llama-2-13b",
    messages=[
        {"role": "system", "content": "You are a helpful assistant."},
        {"role": "user", "content": "Explain quantum computing in simple terms."}
    ],
    temperature=0.7,
    max_tokens=512,
    stream=True
)

for chunk in stream:
    if chunk.choices[0].delta.content:
        print(chunk.choices[0].delta.content, end="", flush=True)

# 批量推理（非流式）
responses = client.completions.create(
    model="llama-2-13b",
    prompt=[
        "Translate to French: Hello",
        "Translate to Spanish: Hello",
        "Translate to German: Hello"
    ],
    max_tokens=50,
    temperature=0.3
)

for resp in responses.choices:
    print(resp.text)
```

### 2.4 vLLM性能优化

```python
# vLLM高级配置
"""
关键参数调优：

1. --tensor-parallel-size 4
   • 模型跨4张GPU切分
   • 适用于单模型无法放入单卡的情况
   • 通信开销：NVLink <5%, PCIe ~20%

2. --gpu-memory-utilization 0.95
   • KV Cache使用95% GPU显存
   • 更高值=更多并发，但可能OOM
   • 推荐：A100 80GB用0.95，A10 24GB用0.90

3. --max-num-seqs 256
   • 最大并发序列数（批量大小）
   • 决定吞吐量上限
   • 受限于GPU显存和模型大小

4. --max-model-len 4096
   • 最大上下文长度
   • 影响KV Cache显存占用
   • Llama2支持4096，可设为2048节省显存

5. --dtype float16
   • 推理精度：float16（FP16）或bfloat16
   • FP16比FP32快2倍，显存减半
   • 精度损失<1%

6. --enforce-eager
   • 禁用CUDA Graph优化
   • 调试时启用，生产环境移除
   • CUDA Graph可再提速10-15%
"""

# 性能监控
import requests
response = requests.get("http://vllm-server:8000/metrics")
print(response.text)
# 关键指标：
# - vllm:num_requests_running（当前请求数）
# - vllm:gpu_cache_usage_perc（KV Cache使用率）
# - vllm:time_to_first_token（TTFT）
# - vllm:time_per_output_token（TPOT）
```

---

## 三、Text Generation Inference (TGI)

### 3.1 TGI特性

Hugging Face官方推理框架，专为生成式模型优化。

**核心优势：**
- Flash Attention 2集成（速度提升2倍）
- Paged Attention支持
- 量化推理（GPTQ、AWQ、bitsandbytes）
- 原生支持Hugging Face Hub

### 3.2 TGI部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: tgi-mistral-7b
  namespace: ai-platform
spec:
  replicas: 3
  selector:
    matchLabels:
      app: tgi-server
      model: mistral-7b
  template:
    metadata:
      labels:
        app: tgi-server
        model: mistral-7b
    spec:
      containers:
      - name: tgi
        image: ghcr.io/huggingface/text-generation-inference:1.4.0
        args:
        - --model-id
        - mistralai/Mistral-7B-Instruct-v0.2
        - --num-shard
        - "1"
        - --max-concurrent-requests
        - "128"
        - --max-input-length
        - "4096"
        - --max-total-tokens
        - "8192"
        - --dtype
        - float16
        - --quantize
        - bitsandbytes-nf4  # 4-bit量化
        
        ports:
        - containerPort: 80
          name: http
        
        env:
        - name: HUGGING_FACE_HUB_TOKEN
          valueFrom:
            secretKeyRef:
              name: hf-token
              key: token
        - name: MAX_BATCH_TOTAL_TOKENS
          value: "1048576"  # 1M tokens批处理
        - name: CUDA_VISIBLE_DEVICES
          value: "0"
        
        resources:
          requests:
            cpu: "8"
            memory: "32Gi"
            nvidia.com/gpu: 1
          limits:
            cpu: "16"
            memory: "64Gi"
            nvidia.com/gpu: 1
        
        volumeMounts:
        - name: cache
          mountPath: /data
        - name: shm
          mountPath: /dev/shm
      
      volumes:
      - name: cache
        emptyDir: {}
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: 16Gi
---
# HorizontalPodAutoscaler实现自动扩缩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: tgi-mistral-7b-hpa
  namespace: ai-platform
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: tgi-mistral-7b
  minReplicas: 2
  maxReplicas: 10
  metrics:
  # 基于自定义指标扩缩容
  - type: Pods
    pods:
      metric:
        name: tgi_queue_size
      target:
        type: AverageValue
        averageValue: "10"  # 队列>10时扩容
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # 5分钟稳定期
      policies:
      - type: Percent
        value: 50  # 每次最多缩容50%
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0  # 立即扩容
      policies:
      - type: Percent
        value: 100  # 每次最多扩容100%
        periodSeconds: 30
```

### 3.3 TGI量化推理

```bash
# GPTQ 4-bit量化推理
docker run --gpus all \
  -p 8080:80 \
  -e MODEL_ID=TheBloke/Llama-2-13B-chat-GPTQ \
  -e QUANTIZE=gptq \
  -e MAX_INPUT_LENGTH=4096 \
  -e MAX_TOTAL_TOKENS=8192 \
  ghcr.io/huggingface/text-generation-inference:1.4.0

# 量化效果对比
# Llama-2-13B:
# - FP16: 26GB显存, 30 tokens/s
# - INT8: 13GB显存, 28 tokens/s (速度-7%)
# - INT4(GPTQ): 7GB显存, 25 tokens/s (速度-17%)
# - NF4(bitsandbytes): 7GB显存, 22 tokens/s (速度-27%)
```

---

## 四、NVIDIA Triton Inference Server

### 4.1 Triton多模型Serving

Triton支持同时部署多个模型，共享GPU资源。

```
┌─────────────────────────────────────┐
│       Triton Inference Server       │
├─────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐          │
│  │ Model 1 │  │ Model 2 │          │
│  │ (BERT)  │  │ (GPT-2) │  ...     │
│  └─────────┘  └─────────┘          │
│        │            │               │
│  ┌─────────────────────────┐       │
│  │   Dynamic Batching      │       │
│  │   GPU资源调度            │       │
│  └─────────────────────────┘       │
│              GPU                    │
└─────────────────────────────────────┘
```

### 4.2 Triton模型仓库结构

```bash
model_repository/
├── llama2_13b/
│   ├── config.pbtxt          # 模型配置
│   └── 1/                    # 版本1
│       └── model.plan        # TensorRT引擎
├── bert_base/
│   ├── config.pbtxt
│   └── 1/
│       └── model.onnx
└── gpt2/
    ├── config.pbtxt
    └── 1/
        ├── model.py          # Python Backend
        └── model_weights/
```

**config.pbtxt示例（Llama2-13B）：**
```protobuf
name: "llama2_13b"
backend: "tensorrtllm"
max_batch_size: 128

# 动态批处理配置
dynamic_batching {
  preferred_batch_size: [8, 16, 32]
  max_queue_delay_microseconds: 5000
}

# 实例组配置
instance_group [
  {
    count: 2  # 2个模型实例
    kind: KIND_GPU
    gpus: [0, 1]  # 绑定GPU 0和1
  }
]

# 输入输出
input [
  {
    name: "input_ids"
    data_type: TYPE_INT32
    dims: [-1]  # 动态长度
  },
  {
    name: "attention_mask"
    data_type: TYPE_INT32
    dims: [-1]
  }
]

output [
  {
    name: "output_ids"
    data_type: TYPE_INT32
    dims: [-1]
  }
]

# 模型参数
parameters: {
  key: "max_tokens"
  value: { string_value: "512" }
}
parameters: {
  key: "temperature"
  value: { string_value: "0.7" }
}
```

### 4.3 Triton Kubernetes部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: triton-inference-server
  namespace: ai-platform
spec:
  replicas: 2
  selector:
    matchLabels:
      app: triton-server
  template:
    metadata:
      labels:
        app: triton-server
    spec:
      containers:
      - name: triton
        image: nvcr.io/nvidia/tritonserver:23.12-py3
        command:
        - tritonserver
        - --model-repository=s3://models/triton_repository
        - --model-control-mode=poll
        - --repository-poll-secs=60
        - --log-verbose=1
        - --metrics-port=8002
        
        ports:
        - containerPort: 8000
          name: http
        - containerPort: 8001
          name: grpc
        - containerPort: 8002
          name: metrics
        
        env:
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: s3-credentials
              key: access-key-id
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: s3-credentials
              key: secret-access-key
        - name: LD_LIBRARY_PATH
          value: "/opt/tritonserver/backends/tensorrtllm:/usr/local/cuda/lib64"
        
        resources:
          requests:
            cpu: "8"
            memory: "32Gi"
            nvidia.com/gpu: 2
          limits:
            cpu: "16"
            memory: "64Gi"
            nvidia.com/gpu: 2
        
        livenessProbe:
          httpGet:
            path: /v2/health/live
            port: 8000
          initialDelaySeconds: 90
          periodSeconds: 30
        
        readinessProbe:
          httpGet:
            path: /v2/health/ready
            port: 8000
          initialDelaySeconds: 60
          periodSeconds: 10
---
# ServiceMonitor采集Triton指标
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: triton-metrics
  namespace: ai-platform
spec:
  selector:
    matchLabels:
      app: triton-server
  endpoints:
  - port: metrics
    interval: 15s
    path: /metrics
```

### 4.4 Triton客户端调用

```python
import tritonclient.http as httpclient
import numpy as np

# 初始化客户端
client = httpclient.InferenceServerClient(
    url="triton-server.ai-platform.svc.cluster.local:8000"
)

# 检查模型状态
if client.is_model_ready("llama2_13b"):
    print("模型已就绪")

# 准备输入
input_ids = np.array([[1, 2, 3, 4, 5]], dtype=np.int32)
attention_mask = np.array([[1, 1, 1, 1, 1]], dtype=np.int32)

inputs = [
    httpclient.InferInput("input_ids", input_ids.shape, "INT32"),
    httpclient.InferInput("attention_mask", attention_mask.shape, "INT32")
]
inputs[0].set_data_from_numpy(input_ids)
inputs[1].set_data_from_numpy(attention_mask)

# 指定输出
outputs = [httpclient.InferRequestedOutput("output_ids")]

# 推理请求
response = client.infer(
    model_name="llama2_13b",
    inputs=inputs,
    outputs=outputs,
    request_id="123"
)

# 获取结果
output_ids = response.as_numpy("output_ids")
print(f"Generated IDs: {output_ids}")

# 查询模型统计信息
stats = client.get_inference_statistics(model_name="llama2_13b")
print(stats)
```

---

## 五、TensorRT-LLM加速

### 5.1 模型转换为TensorRT引擎

```bash
# 1. 安装TensorRT-LLM
pip install tensorrt-llm==0.7.1

# 2. 转换Llama2-7B模型
git clone https://github.com/NVIDIA/TensorRT-LLM.git
cd TensorRT-LLM/examples/llama

# 下载原始权重
huggingface-cli download meta-llama/Llama-2-7b-hf --local-dir ./llama-2-7b-hf

# 转换为TensorRT-LLM格式（FP16）
python convert_checkpoint.py \
  --model_dir ./llama-2-7b-hf \
  --output_dir ./llama-2-7b-trtllm \
  --dtype float16

# 构建TensorRT引擎（单GPU）
trtllm-build \
  --checkpoint_dir ./llama-2-7b-trtllm \
  --output_dir ./llama-2-7b-engine \
  --gemm_plugin float16 \
  --max_batch_size 128 \
  --max_input_len 2048 \
  --max_output_len 512 \
  --max_beam_width 1

# 构建TensorRT引擎（4-GPU张量并行）
trtllm-build \
  --checkpoint_dir ./llama-2-7b-trtllm \
  --output_dir ./llama-2-7b-engine-tp4 \
  --gemm_plugin float16 \
  --max_batch_size 256 \
  --max_input_len 4096 \
  --max_output_len 1024 \
  --tp_size 4 \
  --workers 4

# 性能对比（Llama2-7B, A100 80GB）：
# PyTorch FP16:           45 tokens/s
# vLLM FP16:             180 tokens/s  (4x)
# TensorRT-LLM FP16:     280 tokens/s  (6.2x)
# TensorRT-LLM INT8:     420 tokens/s  (9.3x)
```

### 5.2 TensorRT-LLM推理脚本

```python
import tensorrt_llm
from tensorrt_llm.runtime import ModelRunner

# 加载TensorRT引擎
runner = ModelRunner.from_dir(
    engine_dir="./llama-2-7b-engine",
    rank=0  # 多GPU时指定rank
)

# 推理
input_text = "Once upon a time"
output = runner.generate(
    input_text,
    max_new_tokens=512,
    temperature=0.7,
    top_p=0.9,
    top_k=50,
    repetition_penalty=1.1
)

print(output)
```

---

## 六、KServe模型服务编排

### 6.1 InferenceService定义

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama2-13b-inference
  namespace: ai-platform
spec:
  predictor:
    minReplicas: 2
    maxReplicas: 10
    
    # 自动扩缩容配置
    scaleTarget: 80  # 目标并发请求数
    scaleMetric: concurrency
    
    containers:
    - name: kserve-container
      image: vllm/vllm-openai:v0.3.0
      command:
      - python
      - -m
      - vllm.entrypoints.openai.api_server
      args:
      - --model=/mnt/models/llama-2-13b
      - --tensor-parallel-size=4
      - --dtype=float16
      
      resources:
        requests:
          cpu: "16"
          memory: "64Gi"
          nvidia.com/gpu: 4
        limits:
          cpu: "32"
          memory: "128Gi"
          nvidia.com/gpu: 4
      
      ports:
      - containerPort: 8000
        protocol: TCP
      
      volumeMounts:
      - name: model-volume
        mountPath: /mnt/models
    
    volumes:
    - name: model-volume
      persistentVolumeClaim:
        claimName: llama2-13b-pvc
  
  # Transformer预处理（可选）
  transformer:
    containers:
    - name: transformer
      image: myregistry/text-preprocessor:latest
      env:
      - name: PREDICTOR_HOST
        value: "llama2-13b-inference-predictor-default"
---
# 流量分割（金丝雀发布）
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama2-13b-canary
  namespace: ai-platform
spec:
  predictor:
    canaryTrafficPercent: 10  # 10%流量到新版本
    minReplicas: 1
    containers:
    - name: kserve-container
      image: vllm/vllm-openai:v0.3.1  # 新版本
      # ... 其他配置同上
```

### 6.2 KServe请求路由

```python
import requests
import json

# InferenceService自动生成的URL
url = "http://llama2-13b-inference.ai-platform.example.com/v1/chat/completions"

headers = {
    "Content-Type": "application/json"
}

payload = {
    "model": "llama-2-13b",
    "messages": [
        {"role": "user", "content": "What is Kubernetes?"}
    ],
    "temperature": 0.7,
    "max_tokens": 256
}

response = requests.post(url, headers=headers, json=payload)
result = response.json()
print(result['choices'][0]['message']['content'])
```

---

## 七、推理性能优化技术

### 7.1 KV Cache优化

```python
"""
KV Cache原理：
Transformer解码时，每个token生成需要访问之前所有token的K和V。
缓存这些K/V可以避免重复计算。

显存占用计算（Llama2-13B）：
- 单个token的KV: 2 * num_layers * hidden_size * 2 (K+V) * 2 bytes (FP16)
- Llama2-13B: 2 * 40 * 5120 * 2 * 2 = 1.6 MB/token
- 4096 context: 1.6 MB * 4096 = 6.5 GB
- 批次32: 6.5 GB * 32 = 208 GB (超出A100 80GB!)

PagedAttention解决方案：
- 分页存储KV，页大小16 tokens
- 非连续内存分配，类似OS虚拟内存
- 共享KV Cache（相同前缀的请求共享）
"""

# vLLM自动管理KV Cache，无需手动配置
# 显存分配：
# - 模型权重: ~26GB (FP16)
# - KV Cache: 80GB * 0.95 - 26GB = 50GB
# - 支持并发: 50GB / (1.6MB * 4096) ≈ 8个长上下文请求
```

### 7.2 Flash Attention 2

```python
"""
Flash Attention优化：
- 传统Attention: O(N^2)内存，N是序列长度
- Flash Attention: 分块计算，减少HBM访问
- Flash Attention 2: 进一步优化，速度提升2倍

性能对比（Llama2-7B, A100）：
Context Length | Standard | Flash Attn | Flash Attn 2
     512       |  120ms   |   40ms     |    25ms
    2048       |  480ms   |  160ms     |   100ms
    4096       | 1920ms   |  640ms     |   400ms
"""

# TGI默认启用Flash Attention 2
# vLLM需手动编译支持：
# pip install vllm[flashinfer]
```

### 7.3 Speculative Decoding

```python
"""
投机解码（Speculative Decoding）：
1. 使用小模型快速生成多个候选token
2. 大模型并行验证候选token
3. 接受正确的token，拒绝错误的

加速比：
- GPT-2-XL (1.5B) + GPT-2 (117M): 2.3x
- Llama2-70B + Llama2-7B: 2.1x
- 适用于推理密集型任务
"""

# vLLM支持Speculative Decoding（实验性）
from vllm import LLM, SamplingParams

llm = LLM(
    model="meta-llama/Llama-2-70b-hf",
    speculative_model="meta-llama/Llama-2-7b-hf",  # draft model
    num_speculative_tokens=5,
    use_v2_block_manager=True
)

prompts = ["Explain quantum computing"]
sampling_params = SamplingParams(temperature=0.7, max_tokens=512)
outputs = llm.generate(prompts, sampling_params)
```

---

## 八、成本优化策略

### 8.1 GPU共享方案对比

| 方案 | 隔离性 | 显存利用率 | 适用场景 | 实现复杂度 |
|-----|-------|-----------|---------|-----------|
| **Time-Slicing** | 弱（时间片） | 低（60%） | 开发测试 | 低 |
| **MPS** | 中（进程隔离） | 中（75%） | 小模型推理 | 中 |
| **MIG** | 强（硬件隔离） | 高（90%） | 生产多租户 | 高（仅A100/H100） |
| **vGPU** | 强（虚拟化） | 高（85%） | 云服务商 | 高（需License） |

### 8.2 MIG配置实践

```bash
# A100 80GB MIG配置
# 切分为 3 * 3g.40gb 实例

# 启用MIG模式
sudo nvidia-smi -mig 1

# 创建GPU实例
sudo nvidia-smi mig -cgi 19,19,19 -C

# 验证
nvidia-smi mig -lgi
# +-------------------------------------------------------+
# | GPU instance profiles:                                |
# | GPU   GI ID  Profile                   Placement      |
# |        ID                                Start:Size   |
# |=======================================================|
# |   0    0      MIG 3g.40gb                   0:4       |
# |        1      MIG 3g.40gb                   4:4       |
# |        2      MIG 3g.40gb                   8:4       |
# +-------------------------------------------------------+

# Kubernetes设备插件识别MIG
kubectl get node gpu-node-01 -o yaml | grep nvidia.com/mig
#  nvidia.com/mig-3g.40gb: 3
```

```yaml
# Pod使用MIG实例
apiVersion: v1
kind: Pod
metadata:
  name: llama2-7b-mig
spec:
  containers:
  - name: inference
    image: vllm/vllm-openai:v0.3.0
    resources:
      limits:
        nvidia.com/mig-3g.40gb: 1  # 请求一个MIG实例
```

### 8.3 Spot实例混合部署

```yaml
# Karpenter自动扩容配置
apiVersion: karpenter.sh/v1alpha5
kind: Provisioner
metadata:
  name: gpu-spot-provisioner
spec:
  requirements:
  - key: karpenter.sh/capacity-type
    operator: In
    values: ["spot", "on-demand"]
  - key: node.kubernetes.io/instance-type
    operator: In
    values: ["g5.12xlarge", "p4d.24xlarge"]
  - key: nvidia.com/gpu
    operator: Exists
  
  # Spot实例权重（优先使用）
  weight: 100
  
  # Spot中断处理
  ttlSecondsAfterEmpty: 30
  ttlSecondsUntilExpired: 604800  # 7天
  
  limits:
    resources:
      nvidia.com/gpu: 100
---
# 关键服务使用On-Demand，非关键使用Spot
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-llama2-70b-prod
spec:
  template:
    spec:
      nodeSelector:
        karpenter.sh/capacity-type: on-demand  # 生产环境
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-llama2-7b-dev
spec:
  template:
    spec:
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: karpenter.sh/capacity-type
                operator: In
                values: ["spot"]  # 开发环境优先Spot
```

**成本节省：**
- Spot实例折扣：60-90% off On-Demand价格
- MIG提升利用率：3个7B模型共享A100，成本降低65%
- 推理优化（vLLM）：吞吐10倍提升 = 所需GPU数降低90%

---

## 九、监控与可观测性

### 9.1 关键推理指标

```yaml
# Prometheus告警规则
groups:
- name: llm_inference
  interval: 15s
  rules:
  - alert: HighInferenceLatency
    expr: histogram_quantile(0.99, rate(tgi_request_duration_seconds_bucket[5m])) > 5
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "LLM推理P99延迟 > 5秒"
      description: "模型: {{ $labels.model }}, 当前P99: {{ $value }}s"
  
  - alert: HighQueueSize
    expr: vllm_num_requests_waiting > 50
    for: 3m
    labels:
      severity: warning
    annotations:
      summary: "vLLM请求队列积压 > 50"
      description: "考虑扩容实例"
  
  - alert: LowGPUUtilization
    expr: avg_over_time(DCGM_FI_DEV_GPU_UTIL[10m]) < 30
    for: 15m
    labels:
      severity: info
    annotations:
      summary: "GPU利用率 < 30%，资源浪费"
  
  - alert: OOMRisk
    expr: (vllm_gpu_cache_usage_perc > 95)
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "KV Cache使用率 > 95%，OOM风险"
```

### 9.2 Grafana Dashboard

```json
{
  "dashboard": {
    "title": "LLM Inference Performance",
    "panels": [
      {
        "title": "Requests Per Second",
        "targets": [
          {
            "expr": "rate(tgi_request_count[5m])",
            "legendFormat": "{{model}}"
          }
        ]
      },
      {
        "title": "Time to First Token (TTFT)",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(vllm_time_to_first_token_seconds_bucket[5m]))",
            "legendFormat": "P95"
          },
          {
            "expr": "histogram_quantile(0.99, rate(vllm_time_to_first_token_seconds_bucket[5m]))",
            "legendFormat": "P99"
          }
        ]
      },
      {
        "title": "Tokens Per Second (Throughput)",
        "targets": [
          {
            "expr": "rate(vllm_num_generation_tokens_total[1m])",
            "legendFormat": "{{pod}}"
          }
        ]
      },
      {
        "title": "KV Cache Usage",
        "targets": [
          {
            "expr": "vllm_gpu_cache_usage_perc",
            "legendFormat": "GPU {{gpu_id}}"
          }
        ]
      },
      {
        "title": "Cost Per 1M Tokens",
        "targets": [
          {
            "expr": "(sum(rate(container_cpu_usage_seconds_total{pod=~\"vllm.*\"}[1h])) * 0.05 + sum(nvidia_gpu_duty_cycle{pod=~\"vllm.*\"}) / 100 * 2.5) / (rate(vllm_num_generation_tokens_total[1h]) / 1000000)",
            "legendFormat": "Cost"
          }
        ]
      }
    ]
  }
}
```

---

## 十、生产环境Checklist

### 10.1 部署前检查

- [ ] **模型选择**：根据任务选择合适模型大小
  - 简单任务：7B模型（Mistral-7B, Llama2-7B）
  - 复杂推理：13B-70B（Llama2-70B, Mixtral-8x7B）
- [ ] **量化策略**：权衡精度与性能
  - 生产环境：FP16或INT8（GPTQ）
  - 资源受限：INT4（AWQ, NF4）
- [ ] **推理框架**：
  - 通用：vLLM（最佳吞吐）
  - HF生态：TGI（原生集成）
  - 多模型：Triton（企业级）
  - 极致性能：TensorRT-LLM（NVIDIA GPU）
- [ ] **GPU配置**：
  - 单模型大小 > 单卡显存：张量并行
  - 多模型共享GPU：MIG或Time-Slicing
  - 成本敏感：Spot实例 + 自动扩缩容
- [ ] **高可用**：
  - 最少2副本
  - 跨可用区部署
  - 健康检查与自动重启
- [ ] **可观测性**：
  - Prometheus指标采集
  - Grafana Dashboard
  - 分布式追踪（Jaeger）
  - 日志聚合（ELK/Loki）

### 10.2 性能基准

| 模型 | 框架 | GPU | 批次大小 | 吞吐量(tokens/s) | P99延迟(ms) | 成本($/1M tokens) |
|-----|------|-----|---------|----------------|------------|------------------|
| Llama2-7B | vLLM | A100 40GB | 128 | 4800 | 850 | $0.12 |
| Llama2-7B | TensorRT-LLM | A100 40GB | 256 | 7200 | 620 | $0.08 |
| Llama2-13B | vLLM | A100 80GB | 64 | 2400 | 1200 | $0.28 |
| Llama2-70B | vLLM | 4×A100 80GB | 32 | 1200 | 2400 | $1.20 |
| Mistral-7B | TGI | A10G 24GB | 64 | 2200 | 980 | $0.18 |

---

## 十一、故障排查

### 11.1 常见问题

**问题1：OOM (Out of Memory)**
```
错误：CUDA out of memory
原因：KV Cache或模型权重超出GPU显存

解决方案：
1. 降低--gpu-memory-utilization（从0.95→0.90）
2. 减少--max-num-seqs并发数
3. 降低--max-model-len上下文长度
4. 使用量化（INT8/INT4）
5. 增加张量并行GPU数量
```

**问题2：推理吞吐量低**
```
症状：GPU利用率 < 50%

排查步骤：
1. 检查批次大小：max_batch_size是否过小
2. 检查队列深度：请求是否足够（<10并发无法发挥批处理优势）
3. 检查CPU瓶颈：tokenization是否成为瓶颈
4. 检查网络带宽：模型从S3加载速度
5. 启用CUDA Graph：vLLM移除--enforce-eager

优化：
- 增加max_num_seqs到256+
- 使用Triton Dynamic Batching
- 客户端批量请求而非单个请求
```

**问题3：首Token延迟高（TTFT）**
```
症状：TTFT > 3秒

原因：
1. Prefill阶段计算量大（长上下文）
2. 批处理导致等待
3. 模型加载到GPU慢

优化：
- 使用FlashAttention 2
- 分离Prefill和Decode服务
- 预热模型（启动时发送dummy请求）
- 增加优先级队列（付费用户优先）
```

### 11.2 日志分析

```bash
# vLLM调试日志
kubectl logs -f vllm-pod --namespace ai-platform | grep -E "ERROR|WARNING|OOM"

# 关键日志示例：
# [WARNING] KV cache is full. The request will be blocked until some requests finish.
#   → 需要扩容或降低并发

# [ERROR] CUDA out of memory. Tried to allocate 20.00 GiB
#   → OOM，调整配置

# [INFO] Avg prompt throughput: 1234.5 tokens/s, generation: 45.6 tokens/s
#   → 性能基准参考
```

---

**相关表格：**
- [111-AI基础设施架构](./01-ai-infrastructure.md)
- [112-分布式训练框架](./05-distributed-training-frameworks.md)
- [113-AI模型注册中心](./09-model-registry.md)
- [114-GPU监控与可观测性](./04-gpu-monitoring.md)
- [115-AI数据处理Pipeline](./06-ai-data-pipeline.md)

**版本信息：**
- vLLM: v0.3.0+
- Text Generation Inference: v1.4.0+
- Triton Inference Server: 23.12+
- TensorRT-LLM: v0.7.0+
- KServe: v0.11+
- Kubernetes: v1.27+
