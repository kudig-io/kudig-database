---
title: AI 推理服务最佳实践研究
summary: 深入研究 Kubernetes 上 AI/ML 推理服务的部署、弹性伸缩、模型管理和性能优化实践，覆盖 vLLM、Triton、KServe 等推理框架的生产实践。
category: research
tags:
- research
- ai-ml-infra
- inference
- llm
- serving
- autoscaling
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# AI 推理服务最佳实践研究

## 研究背景

随着 LLM（大语言模型）在生产环境的大规模部署，AI 推理服务的架构和运维与传统 Web 服务有本质不同：

- **资源消耗极高**：单个 LLM 推理实例需要 16-80GB GPU 显存
- **冷启动慢**：模型加载到 GPU 需 30-120 秒，传统 HPA 无法应对突发流量
- **批处理复杂**：请求需要动态 batch 以提高 GPU 利用率，但增加延迟
- **多模型管理**：A/B 测试、灰度发布需要在同一集群管理多个模型版本
- **成本敏感**：GPU 成本占 AI 基础设施 TCO 的 70%+

## 核心问题

1. vLLM、NVIDIA Triton、KServe 三大推理框架的架构差异和适用场景是什么？
2. 如何设计 LLM 推理服务的弹性伸缩策略以平衡延迟和成本？
3. 多模型版本管理（金丝雀、A/B、流量切分）在 K8s 上如何实现？
4. 推理服务的 SLO 定义和监控体系应该如何设计？

## 调研发现

### 发现一：推理框架对比

| 维度 | vLLM | NVIDIA Triton | KServe |
|------|------|--------------|--------|
| **定位** | LLM 专用推理引擎 | 通用推理服务器 | K8s 原生推理平台 |
| **PagedAttention** | ✅ 原生 | ✅ (TensorRT-LLM) | 取决于后端 |
| **Continuous Batching** | ✅ 原生 | ✅ | 取决于后端 |
| **多模型管理** | 单模型/实例 | 多模型/实例 | 多模型/实例 |
| **多框架后端** | PyTorch only | TensorRT/PyTorch/ONNX/TF | 任意（通过 InferenceService） |
| **K8s 原生** | 需要包装 | 需要包装 | ✅ CRD 原生 |
| **弹性伸缩** | 自定义 | 自定义 | ✅ 内置 Knative |
| **Canary/A-B** | 需 Istio | 需 Istio | ✅ 内置 |
| **模型仓库** | 本地/HF | 本地/AWS S3 | S3/GCS/MinIO |
| **推荐场景** | LLM 推理 | 多框架多模型 | K8s 原生平台化 |

### 发现二：弹性伸缩策略设计

传统 HPA 基于 CPU/内存的伸缩策略不适合 LLM 推理。推荐的多层伸缩策略：

```
Layer 1: 请求队列长度触发（实时伸缩）
  → 自定义指标: vllm:num_requests_waiting > 0
  → 缩放速度: 快（每个副本处理 ~10-50 req/s）
  → 挑战: 冷启动延迟（30-120s 加载模型）

Layer 2: GPU 利用率触发（资源优化）
  → 自定义指标: DCGM GPU 利用率 > 70% 或 < 30%
  → 作用: 防止过度缩容和资源浪费

Layer 3: 预测性伸缩（流量预热）
  → 基于历史流量模式预加载模型
  → Cron-based: 每日高峰前 30 分钟扩容
  → 降低冷启动影响

Layer 4: GPU 共享 + 分时调度（成本优化）
  → 低峰期使用时间分片/MPS 共享 GPU
  → 高峰期切换为独占模式
```

**KEDA + vLLM 示例配置**：

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: vllm-scaler
spec:
  scaleTargetRef:
    name: vllm-llama-7b
  minReplicaCount: 2          # 保持最少 2 副本避免冷启动
  maxReplicaCount: 20
  pollingInterval: 5          # 5 秒检查一次
  cooldownPeriod: 300         # 缩容冷却 5 分钟
  triggers:
  - type: prometheus
    metadata:
      serverAddress: http://prometheus:9090
      metricName: vllm_num_requests_waiting
      threshold: "3"          # 等待队列 > 3 时扩容
      query: |
        vllm:num_requests_waiting{model="llama-7b"}
```

### 发现三：vLLM 推理优化技术栈

vLLM 是当前最流行的开源 LLM 推理引擎，其核心优化技术：

| 技术 | 原理 | 性能影响 |
|------|------|---------|
| **PagedAttention** | 将 KV Cache 按 Page 管理（类似虚拟内存），消除显存碎片 | 吞吐 +2-4x |
| **Continuous Batching** | 动态组装 batch，不等当前 batch 全部完成就插入新请求 | 吞吐 +4-8x |
| **Speculative Decoding** | 小模型先生成候选 token，大模型验证 | 延迟 -30-50% |
| **Tensor Parallelism** | 将模型切分到多 GPU 并行推理 | 支持 >70B 模型 |
| **Quantization (AWQ/GPTQ)** | INT8/INT4 量化，减少显存占用 | 显存 -50-75% |
| **Prefix Caching** | 缓存系统 prompt 的 KV，减少重复计算 | TTFT -40-60% |

**生产推荐配置（LLaMA-2 70B, 2 × A100 80GB）**：

```bash
python -m vllm.entrypoints.openai.api_server \
  --model meta-llama/Llama-2-70b-chat-hf \
  --tensor-parallel-size 2 \      # 2 GPU 张量并行
  --gpu-memory-utilization 0.90 \ # 使用 90% 显存
  --max-model-len 4096 \          # 最大序列长度
  --enable-prefix-caching \       # 启用前缀缓存
  --quantization awq \            # AWQ 量化
  --swap-space 16 \               # KV Cache swap 到 CPU（GB）
  --disable-log-requests          # 生产环境关闭请求日志
```

### 发现四：多模型版本管理

**KServe InferenceService 的 Canary 部署**：

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama-chat
spec:
  predictor:
    canaryTrafficPercent: 10      # 10% 流量到新版本
    model:
      modelFormat:
        name: vLLM
      storageUri: s3://models/llama-v2
      resources:
        limits:
          nvidia.com/gpu: 2
    trafficSplit:                 # 精确流量切分
      "v2": 90
      "v3-canary": 10
```

**无 KServe 的方案**：
- 使用 Argo Rollouts 实现 Canary 策略
- 使用 Istio VirtualService 进行流量切分
- 使用 Seldon Core 进行多模型路由

### 发现五：SLO 定义与监控

AI 推理服务的 SLO 应分层定义：

| SLO 层级 | 指标 | 目标 | 报警条件 |
|---------|------|------|---------|
| **可用性** | 请求成功率 | ≥ 99.9% | 5xx 率 > 0.1% |
| **首 Token 延迟 (TTFT)** | P99 TTFT | ≤ 500ms | TTFT P99 > 1s |
| **端到端延迟** | P99 完成时间 | ≤ 3s | P99 > 5s |
| **每 Token 延迟** | P99 inter-token | ≤ 50ms | P99 > 100ms |
| **队列等待** | 平均等待时间 | ≤ 100ms | > 500ms |
| **GPU 利用率** | GPU 使用率 | 60-80% | < 30% 或 > 90% |
| **批处理效率** | 平均 batch size | ≥ 4 | < 2（资源浪费） |

**关键 Prometheus 指标**：

```promql
# 首 Token 延迟 P99
histogram_quantile(0.99, rate(vllm_time_to_first_token_bucket[5m]))

# 吞吐量（tokens/s）
rate(vllm_total_output_tokens[1m])

# 排队等待中的请求数
vllm_num_requests_waiting

# KV Cache 使用率
vllm_gpu_cache_usage_perc

# 批处理大小分布
histogram_quantile(0.50, vllm_batch_size_bucket)
```

## 结论与建议

1. **vLLM 是 LLM 推理的首选引擎**：PagedAttention + Continuous Batching 的组合提供了最高的吞吐量。
2. **弹性伸缩必须基于自定义指标**：CPU/内存指标对 LLM 推理无意义，应使用队列长度和 TTFT。
3. **冷启动是最大运维挑战**：通过保持最小副本数 + 预测性伸缩来缓解。
4. **KServe 提供了最 K8s 原生的多模型管理**：但 vLLM + Argo Rollouts 也是可行的轻量方案。
5. **推理 SLO 需要重新定义**：TTFT 比 P99 延迟更能反映用户体验。
6. **GPU 成本优化是持续主题**：量化 + GPU 共享 + 弹性伸缩三者结合可降低 60%+ 成本。

## 参考资料

- vLLM 文档: https://docs.vllm.ai/
- NVIDIA Triton: https://docs.nvidia.com/deeplearning/triton-inference-server/
- KServe: https://kserve.github.io/website/
- [[AI基础设施/03-inference-serving/|推理服务目录]]
- [[AI基础设施/02-gpu-scheduling/|GPU 调度目录]]
- [[研究/gpu-sharing-scheduling.md|GPU 共享调度研究]]

## Related

- [[综合/gpu-scheduling-cost.md|GPU 调度 × 成本优化]]
- [[概念/autoscaling-strategies.md|自动伸缩策略]]
- [[可观测性/index.md|可观测性目录]]
