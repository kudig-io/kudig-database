---
title: "SGLang 与 LMDeploy 推理引擎"
description: "SGLang（RadixAttention/结构化生成）与 LMDeploy（TurboMind/W4A16）推理引擎的架构、K8s 部署与性能对比"
summary: "深入对比 SGLang 与 LMDeploy 两大高性能推理引擎：SGLang RadixAttention 前缀缓存、结构化生成、多模态支持；LMDeploy TurboMind 内核、W4A16 量化、Persistent Batch；K8s 生产部署方案、与 vLLM 性能对比、适用场景分析与故障排查"
category: AI基础设施
tags:
- sglang
- lmdeploy
- inference
- vllm
- radixattention
- turbomind
- quantization
- kubernetes
- gpu
- llm-serving
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
estimated_read_time: 20min
intent_queries:
- "SGLang 和 vLLM 哪个性能更好"
- "LMDeploy 如何在 K8s 上部署"
- "SGLang RadixAttention 是什么原理"
trigger_keywords:
- SGLang
- LMDeploy
- TurboMind
- RadixAttention
- 推理引擎
prerequisites:
- kubectl-basics
- helm-basics
- gpu-scheduling-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# SGLang 与 LMDeploy 推理引擎

## 概述

在 LLM 推理引擎生态中，vLLM 凭借 PagedAttention 率先确立了开源推理引擎的标杆地位。但随着生产场景对吞吐量、延迟、结构化输出、多模态支持的需求不断升级，SGLang 和 LMDeploy 作为新一代推理引擎，分别在各自的优势领域展现出显著的性能优势。

SGLang 由 UC Berkeley LMSYS 团队开发，核心创新是 RadixAttention 前缀缓存机制和原生结构化生成支持，在多轮对话和 Agent 场景中吞吐量可达 vLLM 的 2-5 倍。LMDeploy 由上海人工智能实验室（InternLM 团队）开发，核心是 TurboMind 推理内核和 W4A16 量化技术，在中文模型和量化推理场景中表现突出。

本文深入解析两大引擎的架构设计、K8s 生产部署方案、性能对比及适用场景选型。

## 架构与核心概念

### SGLang 架构

SGLang 的核心设计目标是最大化 KV Cache 复用率和结构化生成效率：

**RadixAttention（基数树注意力）**：
- 使用 Radix Tree（基数树）数据结构管理所有请求的 KV Cache
- 共享前缀的请求（如多轮对话的 system prompt、few-shot examples）自动复用已计算的 KV Cache
- 前缀匹配是 O(prefix_length) 的树遍历，无需重新计算
- 在多轮对话场景中，第 2 轮及之后的请求只需计算增量 token 的 KV

**结构化生成引擎**：
- 原生集成 JSON Schema / Regex 约束解码
- 使用压缩有限状态机（Compressed FSM）驱动 token 采样
- 相比 vLLM 的 outlines 集成，SGLang 的结构化生成开销更低（< 5% 吞吐损失）

**调度器设计**：
- 支持 chunked prefill（分块预填充），避免长 prompt 阻塞 decode 请求
- 支持 jump-forward decoding（跳跃前向解码），对确定性 token 跳过计算
- 多级调度优先级：prefill > decode > preempted

```
┌─────────────────────────────────────────────────────┐
│                  SGLang Runtime                       │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │  Radix Tree │  │  Scheduler   │  │ Structured│  │
│  │  KV Cache   │  │  (Chunked    │  │ Generation│  │
│  │  Manager    │  │   Prefill)   │  │ Engine    │  │
│  └──────┬──────┘  └──────┬───────┘  └─────┬─────┘  │
│         │                │                 │        │
│  ┌──────▼────────────────▼─────────────────▼─────┐  │
│  │           FlashInfer Attention Kernel          │  │
│  │         (CUDA / Triton Backend)                │  │
│  └───────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────┐  │
│  │     Model Weights (FP16 / FP8 / AWQ)         │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### LMDeploy 架构

LMDeploy 的核心设计目标是极致的量化推理效率和国产硬件适配：

**TurboMind 推理内核**：
- 自研 CUDA 推理内核，针对 Transformer 架构深度优化
- 支持 Persistent Batch（持久化批处理），避免 batch 重组开销
- 支持 split-k attention，在 decode 阶段提升 memory-bound 操作效率
- 支持 W4A16（4-bit 权重，16-bit 激活）和 W8A8 量化

**量化技术栈**：
- 支持 GPTQ、AWQ、SmoothQuant 等主流量化方案
- W4A16 量化下模型体积减少 75%，推理速度提升 2-3 倍
- 量化精度损失 < 1%（在 MMLU/C-Eval 等基准上）

**多模态支持**：
- 支持 InternVL、LLaVA、Qwen-VL 等视觉语言模型
- 视觉编码器与语言模型解耦，支持独立批处理

```
┌─────────────────────────────────────────────────────┐
│                 LMDeploy Runtime                      │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │  TurboMind  │  │  Persistent  │  │  Quant    │  │
│  │  Kernel     │  │  Batch       │  │  Engine   │  │
│  │  (CUDA)     │  │  Scheduler   │  │(W4A16/W8A8)│ │
│  └──────┬──────┘  └──────┬───────┘  └─────┬─────┘  │
│         │                │                 │        │
│  ┌──────▼────────────────▼─────────────────▼─────┐  │
│  │         Attention + FFN Fused Kernels          │  │
│  │    (Split-K Attention / Flash Attention)       │  │
│  └───────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────┐  │
│  │   Quantized Weights (INT4/INT8 + FP16 Act)   │  │
│  └───────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

## 生产部署

### SGLang K8s 部署

🟡 **中风险** — SGLang 推理服务 Deployment（单卡 A100）：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: sglang-llama3-8b
  namespace: ai-inference
  labels:
    app: sglang-llama3
    engine: sglang
    version: v0.4.1
spec:
  replicas: 2
  selector:
    matchLabels:
      app: sglang-llama3
  template:
    metadata:
      labels:
        app: sglang-llama3
        engine: sglang
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "30000"
        prometheus.io/path: "/metrics"
    spec:
      containers:
      - name: sglang
        image: lmsysorg/sglang:v0.4.1-cu124
        command: ["python", "-m", "sglang.launch_server"]
        args:
        - --model-path=/models/llama3-8b-instruct
        - --port=30000
        - --host=0.0.0.0
        - --tp=1                          # Tensor Parallelism
        - --mem-fraction-static=0.88      # GPU 显存分配比例
        - --max-running-requests=256      # 最大并发请求数
        - --chunked-prefill-size=8192     # 分块预填充大小
        - --enable-radix-cache            # 启用 RadixAttention
        - --schedule-policy=lpm           # Longest Prefix Match 调度
        - --log-level=info
        ports:
        - containerPort: 30000
          name: http
          protocol: TCP
        resources:
          limits:
            nvidia.com/gpu: "1"
            memory: "64Gi"
          requests:
            nvidia.com/gpu: "1"
            memory: "48Gi"
            cpu: "8"
        volumeMounts:
        - name: model-storage
          mountPath: /models
          readOnly: true
        - name: shm
          mountPath: /dev/shm
        readinessProbe:
          httpGet:
            path: /health
            port: 30000
          initialDelaySeconds: 90
          periodSeconds: 10
          failureThreshold: 30
        livenessProbe:
          httpGet:
            path: /health
            port: 30000
          initialDelaySeconds: 120
          periodSeconds: 30
          failureThreshold: 5
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-store-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: "16Gi"
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
      tolerations:
      - key: nvidia.com/gpu
        operator: Exists
        effect: NoSchedule
---
apiVersion: v1
kind: Service
metadata:
  name: sglang-llama3-svc
  namespace: ai-inference
spec:
  selector:
    app: sglang-llama3
  ports:
  - port: 8000
    targetPort: 30000
    protocol: TCP
  type: ClusterIP
```

### LMDeploy K8s 部署

🟡 **中风险** — LMDeploy 推理服务 Deployment（W4A16 量化）：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: lmdeploy-internlm2-20b
  namespace: ai-inference
  labels:
    app: lmdeploy-internlm2
    engine: lmdeploy
spec:
  replicas: 2
  selector:
    matchLabels:
      app: lmdeploy-internlm2
  template:
    metadata:
      labels:
        app: lmdeploy-internlm2
        engine: lmdeploy
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "23333"
    spec:
      containers:
      - name: lmdeploy
        image: openmmlab/lmdeploy:v0.6.4-cu12
        command: ["lmdeploy", "serve", "api_server"]
        args:
        - /models/internlm2-chat-20b-awq
        - --server-port=23333
        - --backend=turbomind             # 使用 TurboMind 内核
        - --tp=1                          # Tensor Parallelism
        - --session-len=8192              # 最大会话长度
        - --max-batch-size=64             # 最大批处理大小
        - --cache-max-entry-count=0.9     # KV Cache 显存占比
        - --quant-policy=4                # W4A16 量化
        - --model-format=awq              # AWQ 量化格式
        ports:
        - containerPort: 23333
          name: http
          protocol: TCP
        resources:
          limits:
            nvidia.com/gpu: "1"
            memory: "48Gi"
          requests:
            nvidia.com/gpu: "1"
            memory: "32Gi"
            cpu: "8"
        volumeMounts:
        - name: model-storage
          mountPath: /models
          readOnly: true
        - name: shm
          mountPath: /dev/shm
        readinessProbe:
          httpGet:
            path: /health
            port: 23333
          initialDelaySeconds: 60
          periodSeconds: 10
          failureThreshold: 30
        livenessProbe:
          httpGet:
            path: /health
            port: 23333
          initialDelaySeconds: 90
          periodSeconds: 30
          failureThreshold: 5
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-store-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: "8Gi"
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
      tolerations:
      - key: nvidia.com/gpu
        operator: Exists
        effect: NoSchedule
---
apiVersion: v1
kind: Service
metadata:
  name: lmdeploy-internlm2-svc
  namespace: ai-inference
spec:
  selector:
    app: lmdeploy-internlm2
  ports:
  - port: 8000
    targetPort: 23333
    protocol: TCP
  type: ClusterIP
```

### SGLang 多卡 Tensor Parallelism 部署

🟡 **中风险** — 70B 模型多卡部署（4x A100）：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: sglang-llama3-70b
  namespace: ai-inference
spec:
  replicas: 1
  selector:
    matchLabels:
      app: sglang-llama3-70b
  template:
    metadata:
      labels:
        app: sglang-llama3-70b
    spec:
      containers:
      - name: sglang
        image: lmsysorg/sglang:v0.4.1-cu124
        command: ["python", "-m", "sglang.launch_server"]
        args:
        - --model-path=/models/llama3-70b-instruct
        - --port=30000
        - --host=0.0.0.0
        - --tp=4                          # 4 卡 Tensor Parallelism
        - --mem-fraction-static=0.90
        - --max-running-requests=128
        - --chunked-prefill-size=4096
        - --enable-radix-cache
        - --dp=1                          # Data Parallelism
        - --trust-remote-code
        resources:
          limits:
            nvidia.com/gpu: "4"
            memory: "256Gi"
          requests:
            nvidia.com/gpu: "4"
            memory: "200Gi"
            cpu: "32"
        volumeMounts:
        - name: model-storage
          mountPath: /models
          readOnly: true
        - name: shm
          mountPath: /dev/shm
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-store-large-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: "64Gi"
      nodeSelector:
        nvidia.com/gpu.count: "8"         # 选择 8 卡节点
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
```

## 运维操作

### 性能基准测试

🟢 **只读** — 使用 SGLang 内置 benchmark 工具：

```bash
# SGLang 吞吐量基准测试
python -m sglang.bench_serving \
  --backend sglang \
  --host sglang-llama3-svc.ai-inference.svc \
  --port 8000 \
  --dataset-name sharegpt \
  --num-prompts 1000 \
  --request-rate 50 \
  --output-file results_sglang.json

# LMDeploy 基准测试（使用 OpenAI 兼容 API）
python -m sglang.bench_serving \
  --backend openai \
  --host lmdeploy-internlm2-svc.ai-inference.svc \
  --port 8000 \
  --model internlm2-chat-20b \
  --dataset-name sharegpt \
  --num-prompts 1000 \
  --request-rate 50 \
  --output-file results_lmdeploy.json

# 对比 vLLM 基线
python -m sglang.bench_serving \
  --backend openai \
  --host vllm-llama3-svc.ai-inference.svc \
  --port 8000 \
  --model llama3-8b-instruct \
  --dataset-name sharegpt \
  --num-prompts 1000 \
  --request-rate 50 \
  --output-file results_vllm.json
```

### 运行时指标监控

🟢 **只读** — 查看引擎运行时状态：

```bash
# SGLang 服务器状态
curl -s http://sglang-llama3-svc.ai-inference.svc:8000/get_server_info | jq .

# SGLang Prometheus 指标
curl -s http://sglang-llama3-svc.ai-inference.svc:8000/metrics | grep -E "sglang:(num_running|num_waiting|token_usage|cache_hit)"

# LMDeploy 服务状态
curl -s http://lmdeploy-internlm2-svc.ai-inference.svc:8000/v1/models | jq .

# 查看 Pod GPU 使用情况
kubectl top pods -n ai-inference -l engine=sglang
kubectl exec -n ai-inference deploy/sglang-llama3 -- nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total --format=csv
```

### 模型热更新

🔴 **高风险** — 滚动更新模型版本（会导致短暂服务中断）：

```bash
# 更新 SGLang 模型路径（触发滚动更新）
kubectl set image deployment/sglang-llama3-8b \
  sglang=lmsysorg/sglang:v0.4.2-cu124 \
  -n ai-inference

# 或使用 kubectl patch 更新模型参数
kubectl patch deployment sglang-llama3-8b -n ai-inference --type json \
  -p='[{"op":"replace","path":"/spec/template/spec/containers/0/args/0","value":"--model-path=/models/llama3-8b-instruct-v2"}]'

# 监控滚动更新进度
kubectl rollout status deployment/sglang-llama3-8b -n ai-inference --timeout=600s
```

## 故障排查

### SGLang Radix Cache 命中率低

**现象**：多轮对话场景中 TTFT（Time To First Token）未如预期降低，cache hit rate < 30%。

**排查步骤**：

```bash
# 🟢 查看 Radix Cache 统计
curl -s http://sglang-llama3-svc:8000/metrics | grep "sglang:cache"

# 🟢 检查调度策略是否为 lpm
kubectl get deploy sglang-llama3-8b -n ai-inference -o jsonpath='{.spec.template.spec.containers[0].args}' | jq .
```

**修复方案**：确保 `--schedule-policy=lpm`（Longest Prefix Match）已启用；检查请求是否携带一致的 system prompt 前缀；增大 `--mem-fraction-static` 为 KV Cache 留更多空间。

### LMDeploy OOM（显存不足）

**现象**：Pod 启动后立即 CrashLoopBackOff，日志显示 `CUDA out of memory`。

**排查步骤**：

```bash
# 🟢 查看 Pod 日志
kubectl logs -n ai-inference -l app=lmdeploy-internlm2 --tail=50

# 🟢 检查节点 GPU 显存状态
kubectl exec -n ai-inference deploy/lmdeploy-internlm2 -- nvidia-smi
```

**修复方案**：降低 `--cache-max-entry-count`（如从 0.9 降到 0.8）；减小 `--session-len`；确认 AWQ 量化模型格式正确（非 FP16 原始权重）；检查是否有其他进程占用 GPU 显存。

### 推理延迟突增

**现象**：P99 延迟从 2s 突增到 15s+，但 QPS 未明显变化。

**排查步骤**：

```bash
# 🟢 检查 GPU 温度和功耗（是否触发降频）
kubectl exec -n ai-inference deploy/sglang-llama3 -- nvidia-smi --query-gpu=temperature.gpu,power.draw,clocks.sm --format=csv

# 🟢 查看当前排队请求数
curl -s http://sglang-llama3-svc:8000/metrics | grep "num_waiting"

# 🟢 检查是否有长 prompt 请求阻塞
curl -s http://sglang-llama3-svc:8000/get_server_info | jq '.internal_states'
```

**修复方案**：启用 chunked prefill 避免长 prompt 阻塞；设置 `--max-running-requests` 限制并发；检查 GPU 是否因温度过高降频（> 85°C）。

## 最佳实践

### 引擎性能对比（Llama3-8B, A100 80GB, 单卡）

| 指标 | SGLang v0.4.1 | LMDeploy v0.6.4 (W4A16) | vLLM v0.6.3 |
|------|--------------|------------------------|-------------|
| 吞吐量 (tokens/s) | ~4,200 | ~5,800 (量化) / ~3,500 (FP16) | ~3,200 |
| TTFT P50 (ms) | 180 | 220 | 250 |
| TTFT P99 (ms) | 450 | 520 | 680 |
| 多轮对话加速比 | 2.5x (RadixAttention) | 1.2x | 1.0x (baseline) |
| 结构化生成开销 | < 5% | 不支持原生 | ~15% (outlines) |
| 显存占用 (8B FP16) | ~18GB | ~6GB (W4A16) / ~18GB | ~18GB |
| 多模态支持 | 支持 | 支持 | 支持 |

### 适用场景选型

| 场景 | 推荐引擎 | 理由 |
|------|---------|------|
| 多轮对话 / Agent | SGLang | RadixAttention 前缀缓存大幅降低 TTFT |
| 结构化输出 (JSON/SQL) | SGLang | 原生结构化生成，开销极低 |
| 高吞吐 batch 推理 | LMDeploy (W4A16) | 量化后吞吐量最高 |
| 显存受限环境 | LMDeploy (W4A16) | 4-bit 量化节省 75% 显存 |
| 中文模型 (InternLM/Qwen) | LMDeploy | 同团队优化，适配最佳 |
| 通用 OpenAI API 兼容 | vLLM / SGLang | 生态最完善 |
| 多模态 (VLM) | SGLang / LMDeploy | 两者均支持，按需选择 |

### 生产部署建议

1. **模型存储**：使用共享 PVC（如 NFS/CSI）存储模型权重，避免每个 Pod 重复下载（参考 [[AI基础设施/基础设施/06-ai-data-pipeline.md|AI 数据管道]]）
2. **共享内存**：必须挂载 `/dev/shm`（emptyDir Memory），否则 NCCL 通信会失败
3. **健康检查**：`initialDelaySeconds` 至少设为模型加载时间的 1.5 倍（7B 约 90s，70B 约 300s）
4. **自动伸缩**：结合 [[AI基础设施/K8s-AI基础设施/13-model-serving-autoscaling-keda.md|推理服务自动伸缩]] 配置基于排队数的 KEDA 伸缩
5. **GPU 监控**：部署 DCGM Exporter 监控 GPU 温度、功耗、利用率（参考 [[AI基础设施/基础设施/04-gpu-monitoring-dcgm.md|GPU 监控 DCGM]]）
6. **版本固定**：生产环境固定引擎镜像版本，避免自动升级引入不兼容变更

## Related

- [[AI基础设施/基础设施/17-llm-inference-serving.md|LLM 推理服务]]
- [[AI基础设施/基础设施/18-llm-serving-architecture.md|LLM Serving 架构]]
- [[AI基础设施/基础设施/19-llm-quantization.md|LLM 量化]]
- [[AI基础设施/K8s-AI基础设施/13-model-serving-autoscaling-keda.md|推理服务自动伸缩]]
- [[AI基础设施/基础设施/04-gpu-monitoring-dcgm.md|GPU 监控 DCGM]]
