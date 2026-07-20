---
title: "AI 推理应用模式"
description: "生产级 AI 推理服务：模型服务化、请求批处理、模型版本管理、A/B 测试、降级策略与成本控制"
summary: "覆盖 Kubernetes 上 AI 推理应用的完整生产实践，包括模型服务化架构（vLLM/Triton/TGI）、动态批处理、模型版本管理与灰度发布、A/B 测试框架、推理降级策略、GPU 成本优化和推理服务可观测性。"
category: 应用模式
tags:
- patterns
- ai-inference
- model-serving
- gpu
- ab-testing
- cost-optimization
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 应用开发者
- SRE
- 架构师
estimated_read_time: 20min
intent_queries:
- "AI 推理服务如何在 K8s 上生产部署"
- "模型版本管理和 A/B 测试怎么做"
- "推理服务降级和成本控制策略"
trigger_keywords:
- AI 推理
- 模型服务
- vLLM
- A/B 测试
- 降级策略
- GPU 成本
prerequisites:
- kubectl-basics
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

# AI 推理应用模式

> **适用范围**: Kubernetes v1.28–v1.32 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

## 概述

AI 推理服务是将训练好的模型部署为在线 API 的过程，是 AI 价值落地的最后一公里。与传统微服务不同，AI 推理服务面临独特挑战：模型体积大（数 GB 到数百 GB）导致启动慢、GPU 资源昂贵且稀缺、推理延迟对批处理敏感、模型版本迭代频繁需要灰度验证、单次推理成本远高于传统 API 调用。

本文覆盖 AI 推理应用从模型服务化、请求批处理、版本管理、A/B 测试到降级策略和成本控制的完整生产实践。相关内容可参见 [[gpu-workload-scheduling-patterns]]、[[app-resilience-circuit-breaker]]、[[progressive-delivery-patterns]]。

---

## 模式定义与适用场景

### 推理服务框架对比

| 框架 | 适用模型 | 批处理 | GPU 利用 | 生态 | 适用场景 |
|------|---------|--------|---------|------|---------|
| **vLLM** | LLM (Transformer) | Continuous Batching | 极高（PagedAttention） | OpenAI 兼容 | LLM 对话/生成 |
| **TGI** | LLM (HuggingFace) | Continuous Batching | 高 | HF 生态 | HF 模型快速部署 |
| **Triton** | 通用（多框架） | Dynamic Batching | 高 | NVIDIA 全栈 | 多模型混合部署 |
| **TorchServe** | PyTorch 模型 | Dynamic Batching | 中高 | PyTorch 生态 | CV/NLP 通用 |
| **BentoML** | 通用 | Adaptive Batching | 中 | Python 生态 | 快速原型到生产 |
| **Ray Serve** | 通用 + LLM | 自定义 | 高 | Ray 生态 | 复杂推理管线 |

### 推理模式分类

| 模式 | 延迟要求 | 吞吐要求 | GPU 策略 | 典型场景 |
|------|---------|---------|---------|---------|
| **实时推理** | < 100ms | 中 | 独占 GPU | 搜索排序、推荐 |
| **近实时推理** | < 2s | 高 | 共享/批处理 | LLM 对话 |
| **流式推理** | 首 Token < 500ms | 中 | 独占 | LLM 流式生成 |
| **批量推理** | 分钟-小时 | 极高 | 填充空闲 GPU | 数据标注、离线生成 |
| **边缘推理** | < 50ms | 低 | 小模型/CPU | IoT、移动端 |

---

## 架构设计

### AI 推理服务分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                    API 网关层                                  │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  认证 / 限流 / 路由 / A/B 分流 / 请求队列             │   │
│  └──────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    推理路由层                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                  │
│  │ Model    │  │ Version  │  │ Fallback │                  │
│  │ Router   │  │ Selector │  │ Manager  │                  │
│  └──────────┘  └──────────┘  └──────────┘                  │
├─────────────────────────────────────────────────────────────┤
│                    推理引擎层                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ vLLM/TGI     │  │ Triton       │  │ TorchServe   │      │
│  │ (LLM 推理)   │  │ (多模型)     │  │ (CV/NLP)     │      │
│  │ GPU: A100×2  │  │ GPU: L4×1   │  │ GPU: T4×1   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
├─────────────────────────────────────────────────────────────┤
│                    模型存储层                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                  │
│  │ Model    │  │ Shared   │  │ Object   │                  │
│  │ Registry │  │ PVC/CSI  │  │ Storage  │                  │
│  └──────────┘  └──────────┘  └──────────┘                  │
├─────────────────────────────────────────────────────────────┤
│                    可观测层                                    │
│  推理延迟 / Token 吞吐 / GPU 利用率 / 队列深度 / 成本         │
└─────────────────────────────────────────────────────────────┘
```

### 模型版本灰度发布流程

```
新模型版本发布流程：

1. 模型注册 → Model Registry (版本化存储)
2. 离线评估 → Benchmark (准确率/延迟/吞吐)
3. 影子部署 → Shadow Mode (不影响响应，对比输出)
4. 灰度放量 → 5% → 25% → 50% → 100%
5. 全量切换 → 旧版本保留 7 天（快速回滚）
6. 清理下线 → 释放旧版本 GPU 资源
```

---

## K8s 实现

### vLLM 推理服务部署

```yaml
# 🟡 中风险：GPU 推理服务配置影响在线 AI 功能
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llm-inference-qwen2
  namespace: ai-serving
  labels:
    app.kubernetes.io/name: llm-inference
    app.kubernetes.io/version: "qwen2-7b-v1.2"
    kudig.io/model-name: "qwen2-7b"
    kudig.io/model-version: "v1.2"
spec:
  replicas: 4
  selector:
    matchLabels:
      app.kubernetes.io/name: llm-inference
      kudig.io/model-version: "v1.2"
  template:
    metadata:
      labels:
        app.kubernetes.io/name: llm-inference
        kudig.io/model-name: "qwen2-7b"
        kudig.io/model-version: "v1.2"
      annotations:
        # 禁止 Istio sidecar（推理服务延迟敏感）
        sidecar.istio.io/inject: "false"
    spec:
      priorityClassName: inference-critical
      terminationGracePeriodSeconds: 120  # 等待进行中推理完成
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
        workload-type: inference
      tolerations:
        - key: "workload"
          operator: "Equal"
          value: "inference"
          effect: "NoSchedule"
      containers:
        - name: vllm
          image: registry.internal/ai/vllm-openai:v0.5.4
          args:
            - "--model=/models/qwen2-7b-instruct"
            - "--tensor-parallel-size=1"
            - "--max-model-len=8192"
            - "--gpu-memory-utilization=0.92"
            - "--max-num-seqs=64"
            - "--max-num-batched-tokens=16384"
            - "--enable-prefix-caching"
            - "--disable-log-requests"
            - "--port=8000"
          ports:
            - containerPort: 8000
              name: http
          resources:
            limits:
              nvidia.com/gpu: 1
              cpu: "12"
              memory: "64Gi"
            requests:
              nvidia.com/gpu: 1
              cpu: "8"
              memory: "32Gi"
          env:
            - name: VLLM_USAGE_STATS
              value: "0"
            - name: HF_HUB_OFFLINE
              value: "1"
          # 启动探针：模型加载可能需要 2-5 分钟
          startupProbe:
            httpGet:
              path: /health
              port: 8000
            initialDelaySeconds: 30
            periodSeconds: 10
            failureThreshold: 30  # 最多等 5 分钟
          readinessProbe:
            httpGet:
              path: /health
              port: 8000
            periodSeconds: 10
            timeoutSeconds: 5
            failureThreshold: 3
          livenessProbe:
            httpGet:
              path: /health
              port: 8000
            periodSeconds: 30
            timeoutSeconds: 10
            failureThreshold: 3
          volumeMounts:
            - name: model-storage
              mountPath: /models
              readOnly: true
            - name: shm
              mountPath: /dev/shm
      volumes:
        - name: model-storage
          persistentVolumeClaim:
            claimName: model-qwen2-7b-pvc
        - name: shm
          emptyDir:
            medium: Memory
            sizeLimit: "16Gi"
---
# Service
apiVersion: v1
kind: Service
metadata:
  name: llm-inference-qwen2
  namespace: ai-serving
  labels:
    app.kubernetes.io/name: llm-inference
spec:
  selector:
    app.kubernetes.io/name: llm-inference
  ports:
    - port: 80
      targetPort: 8000
      name: http
```

### 模型版本 A/B 测试（Istio 流量分割）

```yaml
# 🟡 中风险：流量分割影响推理结果
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: llm-inference-ab-test
  namespace: ai-serving
spec:
  hosts:
    - llm-inference.ai-serving.svc.cluster.local
  http:
    # A/B 测试：基于请求 Header 分流
    - name: model-v2-canary
      match:
        - headers:
            x-model-version:
              exact: "v2"
        # 或基于用户 ID 哈希分流 10%
        - headers:
            x-user-id:
              regex: ".*[0-9]$"  # 简化示例
      route:
        - destination:
            host: llm-inference-qwen2-v2
            port:
              number: 80
          weight: 100
    # 默认路由：稳定版
    - name: model-v1-stable
      route:
        - destination:
            host: llm-inference-qwen2
            port:
              number: 80
          weight: 90
        - destination:
            host: llm-inference-qwen2-v2
            port:
              number: 80
          weight: 10  # 10% 流量到新版本
      # 超时：LLM 推理允许更长超时
      timeout: 60s
      retries:
        attempts: 1  # LLM 推理不重试（非幂等）
        perTryTimeout: 60s
---
# 新版本 Deployment（灰度）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llm-inference-qwen2-v2
  namespace: ai-serving
  labels:
    app.kubernetes.io/name: llm-inference
    kudig.io/model-version: "v2.0-rc1"
    kudig.io/canary: "true"
spec:
  replicas: 1  # 灰度期间少量副本
  selector:
    matchLabels:
      app.kubernetes.io/name: llm-inference
      kudig.io/model-version: "v2.0-rc1"
  template:
    metadata:
      labels:
        app.kubernetes.io/name: llm-inference
        kudig.io/model-version: "v2.0-rc1"
    spec:
      priorityClassName: inference-critical
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
      containers:
        - name: vllm
          image: registry.internal/ai/vllm-openai:v0.5.4
          args:
            - "--model=/models/qwen2-7b-instruct-v2"
            - "--tensor-parallel-size=1"
            - "--max-model-len=8192"
            - "--gpu-memory-utilization=0.92"
          resources:
            limits:
              nvidia.com/gpu: 1
              cpu: "12"
              memory: "64Gi"
          startupProbe:
            httpGet:
              path: /health
              port: 8000
            failureThreshold: 30
            periodSeconds: 10
          volumeMounts:
            - name: model-storage
              mountPath: /models
      volumes:
        - name: model-storage
          persistentVolumeClaim:
            claimName: model-qwen2-7b-v2-pvc
```

### 推理降级策略配置

```yaml
# 🟡 中风险：降级配置影响 AI 功能可用性
apiVersion: v1
kind: ConfigMap
metadata:
  name: inference-fallback-config
  namespace: ai-serving
data:
  fallback.yaml: |
    # 推理降级策略
    fallback_chain:
      # 主模型：Qwen2-7B（高质量）
      - name: qwen2-7b
        endpoint: llm-inference-qwen2.ai-serving.svc
        timeout: 30s
        max_tokens: 2048
        
      # 降级 1：小模型（低延迟）
      - name: qwen2-1.5b
        endpoint: llm-inference-qwen2-small.ai-serving.svc
        timeout: 10s
        max_tokens: 1024
        trigger:
          - primary_error_rate > 0.1
          - primary_p99_latency > 10s
          - primary_queue_depth > 100
        
      # 降级 2：规则引擎（无 GPU 依赖）
      - name: rule-engine
        endpoint: rule-engine-fallback.ai-serving.svc
        timeout: 5s
        trigger:
          - all_gpu_services_unavailable
        
      # 降级 3：缓存响应
      - name: cached-response
        type: cache
        cache_ttl: 3600s
        trigger:
          - all_services_unavailable
        response_template:
          content: "AI 服务暂时不可用，请稍后重试。"
          metadata:
            degraded: true
            fallback_level: 3
    
    # 降级恢复
    recovery:
      check_interval: 30s
      success_threshold: 5  # 连续 5 次成功才恢复
      gradual_restore: true  # 逐步恢复流量
```

---

## 生产配置示例

### 推理服务 HPA（基于 GPU 利用率 + 队列深度）

```yaml
# 🟡 中风险：HPA 配置影响推理服务容量和成本
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: llm-inference-hpa
  namespace: ai-serving
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: llm-inference-qwen2
  minReplicas: 2   # 最少 2 个（高可用）
  maxReplicas: 12  # 最多 12 个（成本上限）
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 120
      policies:
        - type: Pods
          value: 2
          periodSeconds: 180  # 每 3 分钟最多扩 2 个（GPU 启动慢）
    scaleDown:
      stabilizationWindowSeconds: 900  # 缩容冷却 15 分钟
      policies:
        - type: Pods
          value: 1
          periodSeconds: 600
  metrics:
    # GPU 利用率
    - type: Pods
      pods:
        metric:
          name: vllm_gpu_cache_usage_perc
        target:
          type: AverageValue
          averageValue: "80"
    # 请求队列深度
    - type: Pods
      pods:
        metric:
          name: vllm_num_requests_waiting
        target:
          type: AverageValue
          averageValue: "20"
    # Token 生成吞吐
    - type: Pods
      pods:
        metric:
          name: vllm_generation_tokens_total
        target:
          type: AverageValue
          averageValue: "5000"
```

### 推理服务可观测性

```yaml
# 🟢 低风险：监控配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ai-inference-alerts
  namespace: monitoring
spec:
  groups:
    - name: ai-inference
      rules:
        # 推理延迟告警
        - alert: InferenceHighLatency
          expr: |
            histogram_quantile(0.99,
              sum(rate(vllm_time_to_first_token_seconds_bucket{namespace="ai-serving"}[5m])) by (le, model_name)
            ) > 5
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "模型 {{ $labels.model_name }} P99 首 Token 延迟 > 5s"

        # GPU 显存告警
        - alert: GPUMemoryNearFull
          expr: |
            vllm_gpu_cache_usage_perc{namespace="ai-serving"} > 0.95
          for: 3m
          labels:
            severity: critical
          annotations:
            summary: "GPU KV Cache 使用率 > 95%，可能 OOM"

        # 推理错误率
        - alert: InferenceHighErrorRate
          expr: |
            sum(rate(vllm_request_failure_total{namespace="ai-serving"}[5m]))
            /
            sum(rate(vllm_request_total{namespace="ai-serving"}[5m])) > 0.05
          for: 3m
          labels:
            severity: critical
          annotations:
            summary: "推理错误率 > 5%"

        # 队列堆积
        - alert: InferenceQueueBacklog
          expr: |
            vllm_num_requests_waiting{namespace="ai-serving"} > 50
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "推理请求队列堆积 > 50"
```

---

## 运维要点

### 推理服务状态检查

```bash
# 🟢 低风险：查看推理 Pod 状态
kubectl get pods -n ai-serving -l app.kubernetes.io/name=llm-inference -o wide

# 🟢 低风险：查看 GPU 使用情况
kubectl exec -n ai-serving deploy/llm-inference-qwen2 -- nvidia-smi

# 🟢 低风险：查看 vLLM 服务指标
kubectl exec -n ai-serving deploy/llm-inference-qwen2 -- \
  curl -s http://localhost:8000/metrics | grep -E "vllm_(num_requests|gpu_cache|generation_tokens)"

# 🟢 低风险：测试推理服务
kubectl exec -n ai-serving deploy/llm-inference-qwen2 -- \
  curl -s http://localhost:8000/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{"model":"qwen2-7b","messages":[{"role":"user","content":"hello"}],"max_tokens":10}'

# 🟢 低风险：查看模型加载日志
kubectl logs -n ai-serving -l app.kubernetes.io/name=llm-inference --tail=50 | grep -i "model\|loaded\|error"
```

### 成本控制策略

| 策略 | 节省比例 | 延迟影响 | 实现复杂度 |
|------|---------|---------|-----------|
| 动态批处理（Continuous Batching） | 吞吐 +3-5x | 略增 | 框架内置 |
| 量化（INT8/INT4） | GPU 内存 -50% | 极小 | 中 |
| 混合精度（FP16/BF16） | 吞吐 +2x | 无 | 低 |
| 潮汐伸缩（夜间缩容） | 成本 -40% | 冷启动 | 低 |
| 小模型路由（简单问题用小模型） | 成本 -60% | 降低 | 中 |
| KV Cache 复用（Prefix Caching） | 吞吐 +30% | 降低 | 框架内置 |
| Spot GPU 实例（批量推理） | 成本 -70% | 中断风险 | 中 |

### 模型版本管理

```bash
# 🟢 低风险：查看当前部署的模型版本
kubectl get deploy -n ai-serving -l app.kubernetes.io/name=llm-inference \
  -o custom-columns=NAME:.metadata.name,VERSION:.metadata.labels.kudig\.io/model-version

# 🟡 中风险：灰度发布新模型版本（调整流量权重）
kubectl patch virtualservice llm-inference-ab-test -n ai-serving \
  --type merge -p '{"spec":{"http":[{"name":"model-v1-stable","route":[{"destination":{"host":"llm-inference-qwen2"},"weight":75},{"destination":{"host":"llm-inference-qwen2-v2"},"weight":25}]}]}}'

# 🔴 高风险：回滚到旧版本（100% 流量切回）
kubectl patch virtualservice llm-inference-ab-test -n ai-serving \
  --type merge -p '{"spec":{"http":[{"name":"model-v1-stable","route":[{"destination":{"host":"llm-inference-qwen2"},"weight":100}]}]}}'
```

---

## 反模式

### 反模式 1：推理服务无 Startup Probe

```yaml
# ❌ 错误：大模型加载需要 3-5 分钟，默认 liveness 会 Kill Pod
livenessProbe:
  httpGet:
    path: /health
  initialDelaySeconds: 30  # 远远不够
```

**后果**：Pod 在模型加载完成前被 Kill，进入 CrashLoopBackOff，永远无法启动。

**修正**：使用 `startupProbe`，`failureThreshold × periodSeconds > 模型加载时间`。参见 [[gpu-workload-scheduling-patterns]]。

### 反模式 2：推理请求无超时

**后果**：长文本生成可能耗时 60s+，无超时导致连接池耗尽，网关 502。

**修正**：Gateway 层设置 60s 超时，应用层设置 `max_tokens` 限制生成长度，流式输出避免长等待。

### 反模式 3：模型文件放在容器镜像中

```dockerfile
# ❌ 错误：20GB 模型打入镜像
COPY model/ /models/qwen2-7b/
```

**后果**：镜像 20GB+，拉取时间 10 分钟+，每次模型更新都要重新构建推送镜像，节点磁盘爆炸。

**修正**：模型存储在共享 PVC/对象存储，通过 Init Container 或 CSI 驱动挂载。

### 反模式 4：A/B 测试无质量评估

**后果**：新模型版本延迟更低但质量下降（幻觉增加），仅看延迟指标会错误地全量切换。

**修正**：A/B 测试必须同时评估延迟、吞吐和质量指标（用户反馈、自动评估分数）。参见 [[progressive-delivery-patterns]]。

### 反模式 5：无降级策略

**后果**：GPU 节点故障或模型 OOM 时，AI 功能完全不可用，影响核心业务流程。

**修正**：多级降级链（大模型→小模型→规则引擎→缓存），配合 Circuit Breaker 自动切换。参见 [[app-resilience-circuit-breaker]]。

---

## Related

- [[gpu-workload-scheduling-patterns]] — GPU 工作负载调度模式
- [[app-resilience-circuit-breaker]] — 应用弹性与熔断模式
- [[progressive-delivery-patterns]] — 渐进式交付生产模式
- [[app-observability-patterns]] — 应用可观测性模式
- [[cost-optimization-finops]] — 成本优化与 FinOps
- [[serverless-event-driven-patterns]] — Serverless 与事件驱动模式
