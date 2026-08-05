---
title: "推理服务自动伸缩"
description: "基于 KEDA/Knative 的 LLM 推理服务自动伸缩：队列驱动、GPU 利用率驱动、scale-to-zero 与冷启动优化"
summary: "深入解析推理服务自动伸缩策略：KEDA Prometheus/Queue Scaler 配置、vLLM pending requests 指标、DCGM GPU 利用率伸缩、KServe Knative concurrency target 与 scale-to-zero、自定义 Metrics Server、伸缩抖动与冷启动故障排查"
category: AI基础设施
tags:
- keda
- autoscaling
- inference
- knative
- kserve
- gpu
- vllm
- dcgm
- hpa
- scale-to-zero
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
- "推理服务如何自动伸缩"
- "KEDA 如何配置 GPU 推理伸缩"
- "KServe scale-to-zero 冷启动怎么优化"
trigger_keywords:
- KEDA
- autoscaling
- scale-to-zero
- inference
- Knative
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
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

# 推理服务自动伸缩

## 概述

LLM 推理服务的负载特征与传统 Web 服务截然不同。传统微服务的 QPS 曲线相对平滑，而推理服务面临突发性流量（如产品发布后的流量洪峰）、长尾请求（单次生成可能耗时 30 秒以上）、以及 batch 与 realtime 请求混合的复杂场景。静态副本数配置要么导致 GPU 资源浪费（低峰期），要么导致请求排队超时（高峰期）。

自动伸缩是 AI 基础设施成本与性能平衡的核心杠杆。本文覆盖从 KEDA 事件驱动伸缩、基于 vLLM 请求队列的伸缩、DCGM GPU 利用率伸缩、到 KServe Knative 原生 scale-to-zero 的完整技术栈，并提供生产环境的故障排查方法论。

## 架构与核心概念

### 推理负载特征分析

推理负载可分为三大类，每类对伸缩策略的要求不同：

| 负载类型 | 特征 | 延迟要求 | 推荐伸缩信号 |
|---------|------|---------|-------------|
| Realtime（实时对话） | 突发、短连接、用户敏感 | P99 < 3s | 并发数 / 队列深度 |
| Batch（离线生成） | 大批量、可排队、延迟不敏感 | 分钟级 | 队列长度 |
| Long-tail（长文本生成） | 单请求耗时长、GPU 占用久 | 10-60s | GPU 利用率 + 排队数 |

### 伸缩架构总览

```
┌─────────────────────────────────────────────────────────┐
│                    伸缩控制平面                            │
│  ┌─────────┐  ┌──────────┐  ┌───────────────────────┐  │
│  │  KEDA   │  │   HPA    │  │  Knative Autoscaler   │  │
│  │ Operator│  │Controller│  │  (KPA)                │  │
│  └────┬────┘  └────┬─────┘  └───────────┬───────────┘  │
│       │             │                    │              │
│  ┌────▼─────────────▼────────────────────▼───────────┐  │
│  │              Metrics Pipeline                      │  │
│  │  Prometheus ← DCGM Exporter / vLLM /metrics       │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│                    数据平面                               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │ vLLM Pod │  │ vLLM Pod │  │ vLLM Pod │  ...        │
│  │ (GPU x1) │  │ (GPU x1) │  │ (GPU x1) │             │
│  └──────────┘  └──────────┘  └──────────┘             │
└─────────────────────────────────────────────────────────┘
```

## 生产部署

### KEDA 部署与配置

KEDA（Kubernetes Event-Driven Autoscaling）作为 HPA 的增强层，支持从 Prometheus、消息队列、自定义指标等多种 Scaler 驱动伸缩。

🟡 **中风险** — 部署 KEDA Operator 到集群：

```bash
# 使用 Helm 部署 KEDA（生产推荐固定版本）
helm repo add kedacore https://kedacore.github.io/charts
helm repo update

helm install keda kedacore/keda \
  --namespace keda \
  --create-namespace \
  --version 2.16.1 \
  --set resources.operator.limits.memory=512Mi \
  --set resources.metricServer.limits.memory=256Mi \
  --set prometheus.metricServer.enabled=true \
  --set prometheus.metricServer.port=8080
```

### 基于 vLLM Pending Requests 的队列伸缩

vLLM 暴露 `/metrics` 端点，其中 `vllm:num_requests_waiting` 指标反映当前排队等待处理的请求数。这是推理服务最直接的背压信号。

🟡 **中风险** — 创建 ScaledObject 基于 vLLM 队列深度伸缩：

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: vllm-llama3-scaledobject
  namespace: ai-inference
  labels:
    app: vllm-llama3
    team: ml-platform
spec:
  scaleTargetRef:
    name: vllm-llama3-deployment
  pollingInterval: 15          # 指标采集间隔（秒）
  cooldownPeriod: 300          # 缩容冷却期（秒），避免抖动
  minReplicaCount: 1
  maxReplicaCount: 8
  advanced:
    horizontalPodAutoscalerConfig:
      behavior:
        scaleUp:
          stabilizationWindowSeconds: 30
          policies:
          - type: Pods
            value: 2
            periodSeconds: 60
        scaleDown:
          stabilizationWindowSeconds: 300
          policies:
          - type: Pods
            value: 1
            periodSeconds: 120
  triggers:
  - type: prometheus
    metadata:
      serverAddress: http://prometheus-server.monitoring.svc:9090
      metricName: vllm_num_requests_waiting
      query: |
        sum(vllm:num_requests_waiting{namespace="ai-inference", deployment="vllm-llama3"})
      threshold: "5"           # 每 5 个排队请求扩容一个副本
      activationThreshold: "1" # 低于 1 时允许缩容到 minReplicas
```

### 基于 GPU 利用率的伸缩（DCGM Metrics）

DCGM Exporter 暴露 `DCGM_FI_DEV_GPU_UTIL` 指标，反映 GPU SM 利用率。对于 compute-bound 的推理负载，GPU 利用率是比 QPS 更准确的伸缩信号。

🟡 **中风险** — 基于 DCGM GPU 利用率的 ScaledObject：

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: vllm-gpu-util-scaledobject
  namespace: ai-inference
spec:
  scaleTargetRef:
    name: vllm-llama3-deployment
  pollingInterval: 30
  cooldownPeriod: 600
  minReplicaCount: 2
  maxReplicaCount: 16
  triggers:
  - type: prometheus
    metadata:
      serverAddress: http://prometheus-server.monitoring.svc:9090
      metricName: gpu_utilization_avg
      query: |
        avg(DCGM_FI_DEV_GPU_UTIL{namespace="ai-inference", pod=~"vllm-llama3.*"})
      threshold: "75"          # GPU 利用率超过 75% 触发扩容
      activationThreshold: "10" # 低于 10% 允许缩容
  - type: prometheus
    metadata:
      serverAddress: http://prometheus-server.monitoring.svc:9090
      metricName: vllm_num_requests_waiting
      query: |
        sum(vllm:num_requests_waiting{namespace="ai-inference", deployment="vllm-llama3"})
      threshold: "10"
```

### KServe Knative 自动伸缩

KServe 基于 Knative Serving 提供原生的自动伸缩能力，支持 scale-to-zero（无流量时缩容到零副本）和基于并发数的精确伸缩。

🟡 **中风险** — KServe InferenceService 配置 Knative 伸缩：

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama3-kserve
  namespace: ai-inference
  annotations:
    # Knative 伸缩注解
    autoscaling.knative.dev/class: "kpa.autoscaling.knative.dev"
    autoscaling.knative.dev/metric: "concurrency"
    autoscaling.knative.dev/target: "4"          # 每 Pod 目标并发数
    autoscaling.knative.dev/min-scale: "1"
    autoscaling.knative.dev/max-scale: "12"
    # Scale-to-zero 配置
    autoscaling.knative.dev/scale-to-zero-pod-retention-period: "5m"
    serving.knative.dev/progress-deadline: "10m"  # GPU 模型加载超时
spec:
  predictor:
    containers:
    - name: vllm
      image: vllm/vllm-openai:v0.6.3
      args:
      - --model=/models/llama3-8b-instruct
      - --tensor-parallel-size=1
      - --max-model-len=8192
      - --gpu-memory-utilization=0.90
      resources:
        limits:
          nvidia.com/gpu: "1"
          memory: "32Gi"
        requests:
          nvidia.com/gpu: "1"
          memory: "24Gi"
      readinessProbe:
        httpGet:
          path: /health
          port: 8000
        initialDelaySeconds: 120   # GPU 模型加载需要时间
        periodSeconds: 10
        failureThreshold: 30
```

### 自定义 Metrics Server 配置

当标准 Scaler 无法满足需求时，可部署自定义 External Metrics Adapter 将业务指标（如 token 生成速率、TTFT）暴露给 HPA。

🟡 **中风险** — 部署 Prometheus Adapter 作为 External Metrics 源：

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-adapter-config
  namespace: monitoring
data:
  config.yaml: |
    rules:
    - seriesQuery: 'vllm:num_requests_waiting{namespace!="",pod!=""}'
      resources:
        overrides:
          namespace: {resource: "namespace"}
          pod: {resource: "pod"}
      name:
        matches: "^(.*)$"
        as: "vllm_pending_requests"
      metricsQuery: 'sum(<<.Series>>{<<.LabelMatchers>>}) by (<<.GroupBy>>)'
    - seriesQuery: 'DCGM_FI_DEV_GPU_UTIL{namespace!=""}'
      resources:
        overrides:
          namespace: {resource: "namespace"}
      name:
        as: "gpu_utilization"
      metricsQuery: 'avg(<<.Series>>{<<.LabelMatchers>>}) by (<<.GroupBy>>)'
    externalRules:
    - seriesQuery: 'vllm:avg_time_to_first_token_seconds{namespace!=""}'
      resources:
        overrides:
          namespace: {resource: "namespace"}
      name:
        as: "vllm_ttft"
      metricsQuery: 'avg(<<.Series>>{<<.LabelMatchers>>}) by (<<.GroupBy>>)'
```

## 运维操作

### 查看伸缩状态

🟢 **只读** — 检查 KEDA ScaledObject 状态与当前副本数：

```bash
# 查看 ScaledObject 状态
kubectl get scaledobject -n ai-inference -o wide

# 查看 HPA 详情（KEDA 创建的 HPA）
kubectl get hpa -n ai-inference
kubectl describe hpa keda-hpa-vllm-llama3-scaledobject -n ai-inference

# 查看当前 Pod 数量与 GPU 分配
kubectl get pods -n ai-inference -l app=vllm-llama3 -o wide
kubectl top pods -n ai-inference -l app=vllm-llama3

# 查看 KEDA 指标采集日志
kubectl logs -n keda -l app=keda-operator --tail=50 | grep -i "vllm"
```

### 手动调整伸缩参数

🟡 **中风险** — 运行时调整伸缩上限（如大促前预扩容）：

```bash
# 临时提高最大副本数
kubectl patch scaledobject vllm-llama3-scaledobject -n ai-inference \
  --type merge -p '{"spec":{"maxReplicaCount":16}}'

# 临时固定最小副本数（防止缩容）
kubectl patch scaledobject vllm-llama3-scaledobject -n ai-inference \
  --type merge -p '{"spec":{"minReplicaCount":4}}'

# 暂停伸缩（维护窗口）
kubectl annotate scaledobject vllm-llama3-scaledobject -n ai-inference \
  autoscaling.keda.sh/paused-replicas="4" --overwrite
```

### 验证伸缩行为

🟢 **只读** — 压测验证伸缩响应：

```bash
# 使用 vegeta 进行阶梯式压测
echo 'POST http://vllm-llama3.ai-inference.svc:8000/v1/completions' | \
  vegeta attack -rate=50 -duration=5m -body='{"model":"llama3","prompt":"Hello","max_tokens":100}' | \
  vegeta report

# 观察 Pod 数量变化
watch -n 5 'kubectl get pods -n ai-inference -l app=vllm-llama3 --no-headers | wc -l'

# 查看 Prometheus 指标
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query?query=sum(vllm:num_requests_waiting{namespace="ai-inference"})'
```

## 故障排查

### 伸缩抖动（Flapping）

**现象**：Pod 数量在 N 和 N+1 之间频繁波动，导致 GPU 资源反复分配释放。

**根因分析**：
1. `cooldownPeriod` 设置过短，缩容后立即触发扩容
2. 指标阈值设置在负载波动区间内（如 threshold=5 但实际波动在 4-6 之间）
3. `stabilizationWindowSeconds` 未配置或过短

**排查步骤**：

```bash
# 🟢 查看 HPA 事件，确认伸缩频率
kubectl describe hpa keda-hpa-vllm-llama3-scaledobject -n ai-inference | grep -A 20 "Events"

# 🟢 查看指标历史波动
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query_range?query=sum(vllm:num_requests_waiting{namespace="ai-inference"})&start=2026-07-19T00:00:00Z&end=2026-07-19T12:00:00Z&step=60'
```

**修复方案**：增大 `cooldownPeriod` 至 300-600 秒；设置 `stabilizationWindowSeconds`；使用滞后阈值（扩容 threshold=8，缩容 activationThreshold=2）。

### 冷启动延迟

**现象**：scale-to-zero 后首次请求超时（>60s），用户体验严重下降。

**根因分析**：
1. 模型加载时间长（7B 模型约 30-60s，70B 模型约 3-5min）
2. GPU 设备分配延迟（Device Plugin Allocate 耗时）
3. 镜像拉取时间（大镜像 10-20GB）

**修复方案**：
- 使用 `min-scale: 1` 避免 scale-to-zero（生产推荐）
- 配置 `scale-to-zero-pod-retention-period` 延长缩零等待
- 使用预热容器（warm pool）预加载模型
- 利用 [[15-AI基础设施/05-K8s-AI基础设施/17-cdi-device-plugin-framework.md|CDI 与 Device Plugin 框架]] 优化设备分配

### 指标延迟导致伸缩滞后

**现象**：流量洪峰到来后 2-3 分钟才开始扩容，期间大量请求超时。

**根因分析**：
1. Prometheus scrape interval 过长（默认 30s）
2. KEDA `pollingInterval` 过长
3. 指标聚合窗口（`rate()` 的 range）过大

**修复方案**：缩短 Prometheus scrape interval 至 10-15s；KEDA `pollingInterval` 设为 10-15s；使用 `rate()` 的 1m 窗口而非 5m。

## 最佳实践

### 伸缩策略对比

| 维度 | HPA（原生） | KEDA | Knative KPA | Karpenter |
|------|------------|------|-------------|-----------|
| 伸缩信号 | CPU/Memory/Custom Metrics | 60+ Scaler（Prometheus, Queue, Cron） | 并发数/RPS | 节点级资源需求 |
| 缩容到零 | 不支持 | 支持 | 原生支持 | N/A（节点级） |
| GPU 感知 | 需自定义指标 | 支持（DCGM Scaler） | 需配置 | 支持（节点池） |
| 冷启动处理 | N/A | 有限 | retention period | 节点预热 |
| 适用场景 | 简单 CPU/Memory 伸缩 | 事件驱动、多指标组合 | Serverless 推理 | 节点容量规划 |
| 生产推荐度 | 基础场景 | 推荐 | Serverless 场景 | 配合使用 |

### 生产配置建议

1. **多指标组合伸缩**：同时使用队列深度（快速响应）和 GPU 利用率（容量保护），取 max 值驱动扩容
2. **非对称伸缩速度**：扩容快（30s 窗口，每次 +2 Pod）、缩容慢（300s 窗口，每次 -1 Pod）
3. **预热策略**：对延迟敏感的服务保持 `minReplicaCount >= 2`，避免冷启动
4. **分时段策略**：使用 KEDA Cron Trigger 在业务高峰前预扩容
5. **GPU 资源池化**：结合 [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] 实现跨模型 GPU 共享

### 监控告警配置

```yaml
# 🟢 推荐的伸缩相关告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: inference-autoscaling-alerts
  namespace: monitoring
spec:
  groups:
  - name: inference.autoscaling
    rules:
    - alert: InferenceScaleUpStuck
      expr: |
        kube_horizontalpodautoscaler_status_desired_replicas{namespace="ai-inference"}
        > kube_horizontalpodautoscaler_status_current_replicas{namespace="ai-inference"}
        for: 5m
      labels:
        severity: warning
      annotations:
        summary: "推理服务扩容卡住超过 5 分钟"
    - alert: InferenceQueueBacklog
      expr: |
        sum(vllm:num_requests_waiting{namespace="ai-inference"}) > 50
        for: 2m
      labels:
        severity: critical
      annotations:
        summary: "推理请求队列积压超过 50，需紧急扩容"
    - alert: GPUUtilizationLow
      expr: |
        avg(DCGM_FI_DEV_GPU_UTIL{namespace="ai-inference"}) < 10
        for: 30m
      labels:
        severity: info
      annotations:
        summary: "GPU 利用率持续低于 10%，考虑缩容或整合"
```

## Related

- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]]
- [[15-AI基础设施/01-基础设施/17-llm-inference-serving.md|LLM 推理服务]]
- [[15-AI基础设施/01-基础设施/04-gpu-monitoring-dcgm.md|GPU 监控 DCGM]]
- [[15-AI基础设施/05-K8s-AI基础设施/15-gpu-cost-attribution-multitenant.md|GPU 成本分摊与多租户 AI 平台]]
- [[15-AI基础设施/05-K8s-AI基础设施/18-ai-platform-architecture-reference.md|企业 AI 平台参考架构]]
