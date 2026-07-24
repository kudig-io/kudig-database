---
title: AI Agent 运维模式
description: '# AI Agent 运维模式'
summary: '# AI Agent 运维模式'
category: synthesis
tags:
- ai-agent
- mcp
- ops
- observability
- reliability
- gpu
- serverless
- kserve
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI Agent 运维模式 是什么
- 如何 AI Agent 运维模式
trigger_keywords:
- AI
- Agent
- 运维模式
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# AI Agent 运维模式

## 概述

AI Agent 运维模式涵盖了 LLM 推理服务、AI Agent 应用和 MLOps 在 Kubernetes 上的部署、扩缩容、可观测性和可靠性实践。随着大模型在生产环境的大规模部署，AI 工作负载的运维已形成独特的模式体系，与传统微服务运维有显著差异。

## 推理服务部署模式

### 模式 1：专用推理集群

```
专用推理集群:
  → 独立的 GPU 节点池（带 nvidia.com/gpu 资源）
  → 高资源隔离，GPU 不与其他工作负载共享
  → 适合大规模推理（>1000 QPS）
  → 通过节点 taint 确保仅推理 Pod 调度

优势: 性能稳定，GPU 利用率高
劣势: 成本高，GPU 资源可能闲置
```

```yaml
# 专用 GPU 节点池配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-inference
spec:
  replicas: 4
  template:
    spec:
      nodeSelector:
        node.kubernetes.io/instance-type: g5.12xlarge
      tolerations:
        - key: nvidia.com/gpu
          operator: Exists
      containers:
        - name: vllm
          image: vllm/vllm-openai:latest
          resources:
            limits:
              nvidia.com/gpu: 1
            requests:
              nvidia.com/gpu: 1
```

### 模式 2：混合部署

```
混合部署:
  → GPU 节点同时运行推理和训练
  → 资源分时复用（推理白天高峰，训练夜间）
  → 适合中小规模团队
  → 通过优先级和抢占实现资源切换

优势: GPU 利用率最大化（从 30% → 70%+）
劣势: 调度复杂，需要精细的优先级控制
```

### 模式 3：Serverless 推理

```yaml
# KServe InferenceService: scale-to-zero
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama-7b-serverless
spec:
  predictor:
    minReplicas: 0                 # 允许缩容到零
    maxReplicas: 10
    scaleTarget: 5                 # 每副本处理 5 并发
    scaleMetric: concurrency
    containers:
      - name: vllm
        image: vllm/vllm-openai:latest
        resources:
          limits:
            nvidia.com/gpu: 1
```

**适用场景**：波动性负载、内部工具、开发/测试环境。冷启动延迟（镜像拉取 + 模型加载）是主要挑战。

## Agent 可观测性

AI Agent 的可观测性需要扩展到模型质量维度：

```
关键指标:
├── 推理延迟
│   ├── TTFT (Time To First Token) — 首 token 延迟
│   └── TBT (Time Between Tokens) — 生成速度
├── 吞吐量
│   ├── tokens/sec — 单 GPU 生成速率
│   └── requests/sec — 并发处理能力
├── 质量
│   ├── 错误率 — 5xx + 超时
│   ├── 幻觉检测 — 模型输出可信度评分
│   └── 用户满意度 — 反馈评分/Thumb up-down ratio
├── 成本
│   ├── per 1K tokens — 单位推理成本
│   └── GPU 小时成本 — 资源利用率折算
└── 系统
    ├── KV Cache 命中率 — 推理效率
    └── Queue 等待时间 — 请求积压
```

## 模型版本管理

### 金丝雀发布策略

```
金丝雀发布:
  v1.0: 90% 流量（稳定版模型）
  v1.1: 10% 流量（候选版模型）

评估指标:
  - 准确率变化（A/B 测试）
  - 延迟变化（TTFT/TBT 对比）
  - 成本变化（GPU 利用率差异）
  - 用户反馈（满意度评分）
```

```yaml
# KServe Canary 部署
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llm-service
spec:
  predictor:
    canaryTrafficPercent: 10         # 10% 流量到 canary
    containers:
      - name: stable
        image: model:v1.0
      - name: canary
        image: model:v1.1
```

## GPU 弹性扩缩容

```yaml
# GPU 推理服务 HPA（基于自定义指标）
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: inference-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: vllm-inference
  minReplicas: 2
  maxReplicas: 20
  metrics:
    - type: Pods
      pods:
        metric:
          name: vllm_requests_waiting     # 等待中的请求数
        target:
          type: AverageValue
          averageValue: 2                    # 每副本平均等待 < 2 个请求
```

## 最佳实践

- **使用 vLLM/TGI 替代原始模型服务**：vLLM 的 PagedAttention 可将吞吐量提升 2-4 倍，KV Cache 管理大幅减少显存浪费
- **推理服务使用自定义指标扩缩容**：不要用 CPU 利用率（推理是 GPU-bound），使用请求队列深度或并发数作为扩缩容信号
- **模型预加载到节点**：使用 DaemonSet 在 GPU 节点预加载模型权重，避免每次 Pod 启动时拉取大文件
- **实施请求限流和优先级队列**：LLM 推理延迟高，突发流量容易导致队列积压——配置限流保护
- **模型版本灰度与 A/B 测试**：新模型必须通过金丝雀验证，监控用户满意度和幻觉率

## 常见陷阱

- **GPU 冷启动延迟**：模型加载（权重从磁盘到 GPU 显存）可能需要数分钟，scale-to-zero 场景需评估冷启动可接受性
- **KV Cache OOM**：长上下文请求会消耗大量 KV Cache 显存，导致 OOM——需要配置最大序列长度限制和请求超时
- **推理和训练资源争抢**：混合部署时如果不设优先级和抢占，训练任务可能挤占推理资源——必须配置 PriorityClass

## 相关 Domain

- AI基础设施/01-ai-infrastructure-overview
- 应用模式/05-ai-ml-patterns/01-ml-serving-patterns

## 相关页面

- [[概念/ai-ml-observability.md|AI/ML 可观测性]] — GPU 监控与推理 SLO
- [[概念/dynamic-resource-allocation.md|动态资源分配]] — GPU 资源声明
- [[概念/canary-deployment.md|金丝雀发布]] — 模型版本灰度

## Related

- [[STRUCTURE|KUDIG-DATABASE 目录结构规范]]


<!-- risk-assessed -->
