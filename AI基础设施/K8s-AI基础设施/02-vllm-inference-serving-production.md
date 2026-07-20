---
title: "vLLM 生产推理服务部署完整指南"
description: "vLLM 在 Kubernetes 上的生产级部署：PagedAttention、Tensor Parallel、量化推理、性能调优与监控"
summary: "覆盖 vLLM 核心架构（PagedAttention/Continuous Batching/Tensor Parallel）、K8s 多 GPU 部署、AWQ/GPTQ/FP8 量化、性能参数调优、Prometheus 监控与 OOM/CUDA 故障排查的完整生产实践"
category: AI基础设施
tags:
- vllm
- inference
- llm-serving
- paged-attention
- tensor-parallel
- quantization
- kubernetes
- gpu
- prometheus
- hpa
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
- "vLLM 如何在 K8s 上生产部署"
- "vLLM 性能调优参数有哪些"
- "vLLM OOM 和 CUDA error 如何排查"
trigger_keywords:
- vllm
- paged-attention
- tensor-parallel
- llm-inference
- quantization
- continuous-batching
prerequisites:
- kubectl-basics
- helm-basics
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

# vLLM 生产推理服务部署完整指南

## 概述

vLLM 是当前 LLM 推理领域性能最优的开源引擎之一，其核心创新 PagedAttention 将 KV Cache 管理类比操作系统虚拟内存分页，消除了传统连续内存分配导致的显存碎片化问题，使 GPU 显存利用率从 50-60% 提升至 90% 以上。结合 Continuous Batching（持续批处理）和 Tensor Parallel（张量并行）技术，vLLM 能够在单节点多 GPU 环境下实现极高的推理吞吐量。

本文聚焦 vLLM 在 Kubernetes 上的生产级部署，覆盖从基础 Deployment 到多 GPU Tensor Parallel、量化推理、性能调优、监控告警和故障排查的完整链路。关于模型部署的整体架构，参见 [[AI基础设施/基础设施/10-model-deployment-serving]]；GPU 调度基础参见 [[概念/gpu-scheduling-ai-workloads]]。

## 架构与核心概念

### vLLM 核心架构

```
┌─────────────────────────────────────────────────────────────┐
│                      vLLM Engine                             │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐    ┌─────────────────────────────────┐ │
│  │  API Server     │    │  LLM Engine                      │ │
│  │  (OpenAI API)   │───▶│  ┌───────────┐ ┌─────────────┐  │ │
│  │  /v1/completions│    │  │ Scheduler │ │ Model Runner │  │ │
│  │  /v1/chat       │    │  │ (Continuous│ │ (PagedAttn) │  │ │
│  └─────────────────┘    │  │  Batching) │ │             │  │ │
│                          │  └───────────┘ └─────────────┘  │ │
│                          │  ┌───────────────────────────┐  │ │
│                          │  │ KV Cache Manager           │  │ │
│                          │  │ (Block-level allocation)   │  │ │
│                          │  └───────────────────────────┘  │ │
│                          └─────────────────────────────────┘ │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  Worker Processes (Tensor Parallel)                      │ │
│  │  GPU 0 ◄──NCCL──► GPU 1 ◄──NCCL──► GPU 2 ◄──► GPU 3   │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

**PagedAttention**：将 KV Cache 切分为固定大小的 Block（默认 16 tokens），按需分配物理显存块，类似 OS 页表机制。支持 Copy-on-Write，Beam Search 和 Parallel Sampling 场景下可共享 KV Cache 块。

**Continuous Batching**：不同于传统 Static Batching 需等待一个 batch 全部完成，Continuous Batching 在每个 decode step 动态插入新请求、移除已完成请求，最大化 GPU 利用率。

**Tensor Parallel**：将模型的 Attention 和 MLP 层按列/行切分到多块 GPU，通过 AllReduce 通信同步中间结果，适用于单卡显存不足以容纳完整模型的场景。

### 关键性能参数

| 参数 | 默认值 | 生产推荐 | 说明 |
|------|--------|---------|------|
| `max_num_seqs` | 256 | 64-128 | 最大并发序列数，影响显存占用 |
| `gpu_memory_utilization` | 0.90 | 0.85-0.92 | GPU 显存使用比例上限 |
| `max_model_len` | auto | 按业务设定 | 最大上下文长度，直接影响 KV Cache 大小 |
| `swap_space` | 0 | 4-16 GB | CPU 交换空间，防止突发 OOM |
| `enforce_eager` | false | false | 禁用 CUDA Graph（调试用） |
| `tensor_parallel_size` | 1 | GPU 数量 | 张量并行度 |
| `max_num_batched_tokens` | auto | 2048-8192 | 单步最大 token 数 |
| `enable_prefix_caching` | false | true | 前缀缓存，加速共享 system prompt |

## 生产部署

### 基础 Deployment + Service

🟡 中风险：创建新的工作负载和服务。

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-llama3-70b
  namespace: ai-serving
  labels:
    app: vllm-llama3-70b
    model: llama-3-70b-instruct
spec:
  replicas: 2
  selector:
    matchLabels:
      app: vllm-llama3-70b
  template:
    metadata:
      labels:
        app: vllm-llama3-70b
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8000"
        prometheus.io/path: "/metrics"
    spec:
      containers:
      - name: vllm
        image: vllm/vllm-openai:v0.6.3
        args:
        - --model=/models/llama-3-70b-instruct
        - --tensor-parallel-size=4
        - --max-model-len=8192
        - --max-num-seqs=64
        - --gpu-memory-utilization=0.90
        - --swap-space=8
        - --enable-prefix-caching
        - --dtype=auto
        - --disable-log-requests
        ports:
        - containerPort: 8000
          name: http
          protocol: TCP
        resources:
          limits:
            nvidia.com/gpu: 4
            memory: "128Gi"
          requests:
            nvidia.com/gpu: 4
            memory: "64Gi"
            cpu: "16"
        env:
        - name: NCCL_DEBUG
          value: "WARN"
        - name: VLLM_LOGGING_LEVEL
          value: "WARNING"
        - name: HF_TOKEN
          valueFrom:
            secretKeyRef:
              name: hf-token-secret
              key: token
        volumeMounts:
        - name: model-storage
          mountPath: /models
        - name: shm
          mountPath: /dev/shm
        readinessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 120
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
        livenessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 180
          periodSeconds: 30
          timeoutSeconds: 10
          failureThreshold: 5
        lifecycle:
          preStop:
            exec:
              command: ["sh", "-c", "sleep 15"]
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-pvc-llama3-70b
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
      terminationGracePeriodSeconds: 60
---
apiVersion: v1
kind: Service
metadata:
  name: vllm-llama3-70b-svc
  namespace: ai-serving
spec:
  selector:
    app: vllm-llama3-70b
  ports:
  - port: 80
    targetPort: 8000
    protocol: TCP
  type: ClusterIP
```

### HPA 自动伸缩

🟡 中风险：创建 HPA 策略，可能触发 Pod 扩缩容。

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: vllm-llama3-70b-hpa
  namespace: ai-serving
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: vllm-llama3-70b
  minReplicas: 2
  maxReplicas: 8
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 120
      policies:
      - type: Pods
        value: 1
        periodSeconds: 300
    scaleDown:
      stabilizationWindowSeconds: 600
      policies:
      - type: Pods
        value: 1
        periodSeconds: 600
  metrics:
  - type: Pods
    pods:
      metric:
        name: vllm_num_requests_waiting
      target:
        type: AverageValue
        averageValue: "10"
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

### 量化部署（AWQ/GPTQ/FP8）

量化是降低显存占用、提升吞吐的关键手段。vLLM 原生支持 AWQ、GPTQ、FP8 等量化格式。

```bash
# AWQ 4-bit 量化部署（70B 模型仅需 ~40GB 显存，2x A100 即可）
# 🟡 中风险
python -m vllm.entrypoints.openai.api_server \
  --model /models/llama-3-70b-instruct-awq \
  --quantization awq \
  --tensor-parallel-size=2 \
  --max-model-len=4096 \
  --gpu-memory-utilization=0.92

# FP8 量化（H100/H200 原生支持，精度损失最小）
python -m vllm.entrypoints.openai.api_server \
  --model /models/llama-3-70b-instruct-fp8 \
  --quantization fp8 \
  --tensor-parallel-size=2 \
  --max-model-len=8192

# GPTQ 量化
python -m vllm.entrypoints.openai.api_server \
  --model /models/llama-3-70b-instruct-gptq \
  --quantization gptq \
  --tensor-parallel-size=2
```

### 多 GPU Tensor Parallel 配置要点

Tensor Parallel 要求 GPU 之间通过 NVLink 或高速互联连接，跨 NUMA 节点的 PCIe 连接会严重降低通信效率。

```yaml
# 关键配置：确保 Pod 内 GPU 拓扑最优
spec:
  containers:
  - name: vllm
    env:
    - name: NVIDIA_VISIBLE_DEVICES
      value: "all"
    - name: NCCL_P2P_LEVEL
      value: "NVL"  # 仅使用 NVLink 进行 P2P 通信
    - name: NCCL_IB_DISABLE
      value: "1"    # 单节点内禁用 InfiniBand
    resources:
      limits:
        nvidia.com/gpu: 4  # 必须与 --tensor-parallel-size 一致
```

## 运维操作

### 健康检查与流量管理

🟢 低风险/只读。

```bash
# 检查 vLLM 服务健康状态
curl -s http://vllm-llama3-70b-svc.ai-serving.svc/health | jq .

# 查看模型信息
curl -s http://vllm-llama3-70b-svc.ai-serving.svc/v1/models | jq .

# 发送测试请求
curl -s http://vllm-llama3-70b-svc.ai-serving.svc/v1/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "/models/llama-3-70b-instruct",
    "prompt": "Hello, world",
    "max_tokens": 50,
    "temperature": 0.7
  }' | jq .

# 检查 Pod 资源使用
kubectl top pods -n ai-serving -l app=vllm-llama3-70b
```

### 监控指标（Prometheus）

vLLM 内置 Prometheus metrics endpoint（`/metrics`），关键指标包括：

```bash
# 🟢 查看 vLLM 暴露的指标
kubectl exec -it <vllm-pod> -n ai-serving -- curl -s localhost:8000/metrics | grep vllm

# 关键指标：
# vllm_num_requests_running   - 当前正在处理的请求数
# vllm_num_requests_waiting   - 排队等待的请求数（HPA 核心指标）
# vllm_gpu_cache_usage_perc   - GPU KV Cache 使用率
# vllm_avg_generation_throughput_toks_per_s - 生成吞吐（tokens/s）
# vllm_e2e_request_latency_seconds - 端到端延迟分布
# vllm_time_to_first_token_seconds - TTFT（首 token 延迟）
```

### 滚动更新与模型切换

🔴 高风险：模型切换会导致短暂服务中断（除非配置了足够的 replicas）。

```bash
# 滚动更新到新模型版本
kubectl set image deployment/vllm-llama3-70b \
  vllm=vllm/vllm-openai:v0.7.0 \
  -n ai-serving

# 或使用 kubectl patch 更新模型路径
kubectl patch deployment vllm-llama3-70b -n ai-serving --type='json' \
  -p='[{"op":"replace","path":"/spec/template/spec/containers/0/args/0","value":"--model=/models/llama-3.1-70b-instruct"}]'

# 监控滚动更新进度
kubectl rollout status deployment/vllm-llama3-70b -n ai-serving --timeout=600s
```

## 故障排查

### OOM（Out of Memory）

```bash
# 🟢 Step 1: 确认 OOM 类型
kubectl describe pod <vllm-pod> -n ai-serving | grep -A 5 "Last State"
# "OOMKilled" → 容器内存超限
# CUDA OOM → GPU 显存不足（看 Pod 日志）

# 🟢 Step 2: 检查 GPU 显存使用
kubectl exec -it <vllm-pod> -n ai-serving -- nvidia-smi

# 🟢 Step 3: 检查 vLLM 日志中的 OOM 信息
kubectl logs <vllm-pod> -n ai-serving | grep -i "out of memory\|OOM\|CUDA error"

# 修复方案：
# 1. 降低 gpu_memory_utilization（0.90 → 0.85）
# 2. 减小 max_model_len（减少 KV Cache 预分配）
# 3. 减小 max_num_seqs（减少并发序列）
# 4. 增加 swap_space（CPU 内存兜底）
# 5. 使用量化模型减少权重显存占用
```

### CUDA Error / NCCL 通信失败

```bash
# 🟢 检查 CUDA 错误
kubectl logs <vllm-pod> -n ai-serving | grep -i "cuda\|nccl"

# 常见 CUDA 错误：
# "CUDA error: device-side assert triggered" → 模型与 GPU 架构不兼容
# "NCCL WARN Cuda failure" → GPU 间通信问题
# "CUDA error: an illegal memory access" → 显存越界，通常是 bug

# 🟡 临时修复：重启 Pod
kubectl delete pod <vllm-pod> -n ai-serving

# 排查 NCCL 问题
kubectl exec -it <vllm-pod> -n ai-serving -- nvidia-smi topo -m
# 确认 GPU 间连接类型为 NV# (NVLink) 而非 PIX/PHB (PCIe)
```

### 吞吐下降

```bash
# 🟢 检查当前吞吐指标
curl -s localhost:8000/metrics | grep "vllm_avg_generation_throughput"

# 🟢 检查是否有请求积压
curl -s localhost:8000/metrics | grep "vllm_num_requests_waiting"

# 🟢 检查 GPU 利用率
kubectl exec -it <vllm-pod> -n ai-serving -- nvidia-smi dmon -s u -d 5

# 常见原因：
# 1. GPU 利用率低 → max_num_seqs 过小，增大并发
# 2. GPU 利用率 100% 但吞吐低 → 检查是否有长序列拖慢 batch
# 3. 显存接近满载 → KV Cache 不足，触发 preemption
# 4. 网络延迟 → 检查 Service/Ingress 层
```

## 最佳实践

1. **显存规划公式**：模型权重显存 + KV Cache 显存 + 激活值显存 < GPU 显存 × gpu_memory_utilization。70B FP16 模型约需 140GB 权重显存，4x A100 80GB 配置下 KV Cache 可用约 140GB。

2. **Shared Memory 必须配置**：Tensor Parallel 使用 NCCL 通信依赖 `/dev/shm`，K8s 默认仅 64MB，必须挂载 `emptyDir.medium: Memory` 并设置足够大小（建议 16Gi+）。

3. **Readiness Probe 延迟**：大模型加载需要 2-5 分钟，`initialDelaySeconds` 必须设置足够大（120-300s），否则 Pod 会被反复杀死重启。

4. **Prefix Caching**：对于共享 system prompt 的场景（如客服机器人），启用 `--enable-prefix-caching` 可显著减少重复计算，TTFT 降低 50%+。

5. **优雅停机**：配置 `preStop` hook（sleep 15s）确保 Service Endpoint 摘除后存量请求处理完毕，避免 502 错误。

6. **模型存储**：使用 ReadWriteMany PVC（如 CephFS、NFS）存储模型文件，避免每个 Pod 独立下载。模型预热可使用 initContainer。

7. ** Gang Scheduling**：多 GPU Pod 需要所有 GPU 同时就绪，建议配合 [[概念/gang-scheduling]] 避免资源碎片化导致的调度死锁。

## Related

- [[AI基础设施/基础设施/10-model-deployment-serving]]
- [[概念/gpu-scheduling-ai-workloads]]
- [[概念/gang-scheduling]]
- [[AI基础设施/基础设施/04-gpu-monitoring-dcgm]]
- [[故障诊断/]]
