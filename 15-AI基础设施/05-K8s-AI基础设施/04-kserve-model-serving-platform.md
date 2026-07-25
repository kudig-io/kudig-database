---
title: "KServe 模型服务平台生产部署"
description: "KServe 生产级部署：InferenceService、自动伸缩、金丝雀发布、多运行时支持与故障排查"
summary: "覆盖 KServe 架构（Control Plane/Data Plane）、安装依赖链（cert-manager + Istio/Kourier）、InferenceService 多运行时部署（vLLM/Triton/SKLearn）、KPA 与 K8s HPA 自动伸缩、Scale-to-Zero、金丝雀发布与模型解释性的完整生产实践"
category: AI基础设施
tags:
- kserve
- inferenceservice
- model-serving
- knative
- istio
- autoscaling
- canary
- serverless
- kubernetes
- ml-platform
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
- "KServe 如何在生产环境部署"
- "InferenceService 如何配置自动伸缩"
- "KServe 金丝雀发布怎么做"
trigger_keywords:
- kserve
- inferenceservice
- knative
- model-serving
- scale-to-zero
- canary-rollout
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

# KServe 模型服务平台生产部署

## 概述

KServe 是 Kubernetes 原生的模型服务平台，提供标准化的模型推理、转换和解释能力。它通过 InferenceService CRD 抽象了模型部署的复杂性，支持多种推理运行时（vLLM、Triton、TorchServe、SKLearn、XGBoost 等），并集成了 Knative 实现 Serverless 自动伸缩（含 Scale-to-Zero）和 Istio/Kourier 实现流量管理。

KServe 的核心价值在于：统一的模型部署接口、生产级自动伸缩、金丝雀发布、模型版本管理和可观测性。对于需要管理数十到数百个模型的平台团队，KServe 提供了比裸 Deployment 更高层次的抽象和运维能力。关于模型部署的基础概念，参见 [[15-AI基础设施/01-基础设施/10-model-deployment-serving]]；GPU 调度参见 [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads]]。

## 架构与核心概念

### KServe 架构分层

```
┌─────────────────────────────────────────────────────────────────┐
│                        Control Plane                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────┐  │
│  │ KServe           │  │ Knative Serving  │  │ Istio /      │  │
│  │ Controller       │  │ Controller       │  │ Kourier      │  │
│  │ (InferenceService│  │ (Revision/Route/ │  │ (Ingress/    │  │
│  │  Reconciler)     │  │  Configuration)  │  │  Gateway)    │  │
│  └──────────────────┘  └──────────────────┘  └──────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                        Data Plane                                │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  InferenceService                                          │ │
│  │  ┌──────────┐    ┌──────────────┐    ┌──────────────────┐ │ │
│  │  │ Predictor│───▶│ Transformer  │───▶│ Explainer        │ │ │
│  │  │ (vLLM/   │    │ (Pre/Post    │    │ (SHAP/LIME/      │ │ │
│  │  │  Triton/ │    │  Processing) │    │  Alibi)          │ │ │
│  │  │  SKLearn)│    │              │    │                  │ │ │
│  │  └──────────┘    └──────────────┘    └──────────────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

**Control Plane 组件**：
- **KServe Controller**：Watch InferenceService CR，编排 Knative Service / RawDeployment、VirtualService、HorizontalPodAutoscaler 等子资源。
- **Knative Serving**：提供 Revision 管理、流量分割、Scale-to-Zero 能力。
- **Istio/Kourier**：提供 Ingress Gateway、流量路由、mTLS。

**Data Plane 组件**：
- **Predictor**：核心推理容器，运行实际模型。
- **Transformer**：请求预处理和响应后处理（可选）。
- **Explainer**：模型解释性服务（可选），支持 SHAP、LIME、Alibi。

### 部署模式对比

| 模式 | 适用场景 | 自动伸缩 | Scale-to-Zero | 依赖 |
|------|---------|---------|--------------|------|
| Serverless (Knative) | 请求量波动大、多模型共享集群 | KPA (并发/RPS) | 支持 | Knative + Istio/Kourier |
| RawDeployment | 稳定负载、低延迟要求 | K8s HPA (CPU/自定义) | 不支持 | 无额外依赖 |
| ModelMesh | 大量小模型、多租户 | 自动 | 部分支持 | etcd + ModelMesh Controller |

## 生产部署

### 安装依赖链

🔴 高风险：安装集群级组件（cert-manager、Istio、Knative），可能影响现有服务和网络配置。

```bash
# Step 1: 安装 cert-manager（KServe Webhook 需要 TLS 证书）
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.15.0/cert-manager.yaml
kubectl wait --for=condition=Available deployment/cert-manager-webhook -n cert-manager --timeout=120s

# Step 2: 安装 Istio（或 Kourier 作为轻量替代）
# 方案 A: Istio（功能完整，适合已有 Service Mesh 的集群）
istioctl install --set profile=minimal -y
kubectl wait --for=condition=Available deployment/istiod -n istio-system --timeout=120s

# 方案 B: Kourier（轻量级，仅需 Ingress 功能）
kubectl apply -f https://github.com/knative/net-kourier/releases/download/knative-v1.14.0/kourier.yaml

# Step 3: 安装 Knative Serving
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.14.0/serving-crds.yaml
kubectl apply -f https://github.com/knative/serving/releases/download/knative-v1.14.0/serving-core.yaml
kubectl wait --for=condition=Available deployment/controller -n knative-serving --timeout=120s

# Step 4: 安装 KServe
kubectl apply -f https://github.com/kserve/kserve/releases/download/v0.13.0/kserve.yaml
kubectl apply -f https://github.com/kserve/kserve/releases/download/v0.13.0/kserve-cluster-resources.yaml
kubectl wait --for=condition=Available deployment/kserve-controller-manager -n kserve --timeout=120s
```

### InferenceService 部署（vLLM Runtime）

🟡 中风险：创建新的推理服务。

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama3-8b-vllm
  namespace: ai-serving
  annotations:
    serving.kserve.io/deploymentMode: Serverless
spec:
  predictor:
    minReplicas: 1
    maxReplicas: 4
    scaleTarget: 10  # 每 Pod 目标并发数
    scaleMetric: concurrency
    containers:
    - name: kserve-container
      image: vllm/vllm-openai:v0.6.3
      args:
      - --model=/models/llama-3-8b-instruct
      - --max-model-len=4096
      - --gpu-memory-utilization=0.90
      - --disable-log-requests
      resources:
        limits:
          nvidia.com/gpu: 1
          memory: "32Gi"
        requests:
          nvidia.com/gpu: 1
          memory: "16Gi"
          cpu: "4"
      volumeMounts:
      - name: model-storage
        mountPath: /models
      readinessProbe:
        httpGet:
          path: /health
          port: 8000
        initialDelaySeconds: 90
        periodSeconds: 10
    volumes:
    - name: model-storage
      persistentVolumeClaim:
        claimName: model-pvc-llama3-8b
```

### InferenceService 部署（Triton Runtime）

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: bert-triton
  namespace: ai-serving
spec:
  predictor:
    triton:
      runtimeVersion: "24.05-py3"
      resources:
        limits:
          nvidia.com/gpu: 1
          memory: "16Gi"
        requests:
          nvidia.com/gpu: 1
          memory: "8Gi"
      storageUri: "s3://model-bucket/bert-base/"
      env:
      - name: TRITON_MODEL_REPOSITORY
        value: "/mnt/models"
    minReplicas: 1
    maxReplicas: 3
```

### InferenceService 部署（SKLearn Runtime - CPU）

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: sklearn-iris
  namespace: ai-serving
spec:
  predictor:
    sklearn:
      runtimeVersion: "v0.13.0"
      storageUri: "gs://kfserving-examples/models/sklearn/1.0/model"
      resources:
        requests:
          cpu: "1"
          memory: "2Gi"
        limits:
          cpu: "2"
          memory: "4Gi"
    minReplicas: 1
    maxReplicas: 5
    scaleTarget: 50
    scaleMetric: concurrency
```

### 金丝雀发布

🟡 中风险：修改流量分配比例。

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama3-8b-vllm
  namespace: ai-serving
spec:
  predictor:
    # 新版本（Canary）- 分配 20% 流量
    canaryTrafficPercent: 20
    containers:
    - name: kserve-container
      image: vllm/vllm-openai:v0.7.0  # 新版本
      args:
      - --model=/models/llama-3.1-8b-instruct  # 新模型
      - --max-model-len=8192
      resources:
        limits:
          nvidia.com/gpu: 1
    # 旧版本配置保持不变（KServe 自动保留上一个 Revision）
```

```bash
# 🟢 验证金丝雀流量分配
kubectl get inferenceservice llama3-8b-vllm -n ai-serving -o yaml | grep -A 5 "traffic"

# 🟢 查看 Revision 列表
kubectl get revisions -n ai-serving -l serving.kserve.io/inferenceservice=llama3-8b-vllm

# 🔴 回滚：将 canaryTrafficPercent 设为 0 或删除新版本配置
kubectl patch inferenceservice llama3-8b-vllm -n ai-serving \
  --type='merge' -p '{"spec":{"predictor":{"canaryTrafficPercent":0}}}'
```

## 运维操作

### 自动伸缩配置

**KPA（Knative Pod Autoscaler）- Serverless 模式**：

```yaml
# 基于并发数的自动伸缩
metadata:
  annotations:
    autoscaling.knative.dev/class: "kpa.autoscaling.knative.dev"
    autoscaling.knative.dev/metric: "concurrency"
    autoscaling.knative.dev/target: "10"  # 每 Pod 目标 10 并发
    autoscaling.knative.dev/min-scale: "1"
    autoscaling.knative.dev/max-scale: "8"
    autoscaling.knative.dev/scale-down-delay: "5m"  # 缩容延迟
    autoscaling.knative.dev/window: "60s"  # 指标窗口
```

**K8s HPA - RawDeployment 模式**：

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama3-8b-raw
  namespace: ai-serving
  annotations:
    serving.kserve.io/deploymentMode: RawDeployment
    serving.kserve.io/autoscalerClass: "hpa"
    serving.kserve.io/metrics: "gpu_utilization"
    serving.kserve.io/targetUtilizationPercentage: "70"
spec:
  predictor:
    minReplicas: 2
    maxReplicas: 6
    containers:
    - name: kserve-container
      image: vllm/vllm-openai:v0.6.3
      resources:
        limits:
          nvidia.com/gpu: 1
```

### Scale-to-Zero 配置

```bash
# 🟡 配置 Knative 全局 Scale-to-Zero 策略
kubectl patch configmap config-autoscaler -n knative-serving --type='merge' \
  -p '{"data":{"scale-to-zero-grace-period":"30s","stable-window":"60s","enable-scale-to-zero":"true"}}'

# 注意：GPU 模型 Scale-to-Zero 后冷启动时间较长（模型重新加载 2-5 分钟）
# 生产 GPU 推理服务建议 minReplicas >= 1，仅 CPU 模型使用 Scale-to-Zero
```

### 监控与日志

🟢 低风险/只读。

```bash
# 查看 InferenceService 状态
kubectl get inferenceservice -n ai-serving
# NAME              URL   READY   PREV   LATEST   AGE
# llama3-8b-vllm    http://...  True    80     20      5d

# 查看推理服务 URL
kubectl get inferenceservice llama3-8b-vllm -n ai-serving -o jsonpath='{.status.url}'

# 发送推理请求（通过 Istio Ingress Gateway）
curl -v http://llama3-8b-vllm.ai-serving.example.com/v1/completions \
  -H "Content-Type: application/json" \
  -H "Host: llama3-8b-vllm.ai-serving.example.com" \
  -d '{"model": "llama-3-8b-instruct", "prompt": "Hello", "max_tokens": 50}'

# 查看 KServe Controller 日志（排查 Reconcile 问题）
kubectl logs -n kserve -l control-plane=kserve-controller-manager --tail=50

# 查看 Knative Revision 状态
kubectl get revisions -n ai-serving -o wide
kubectl get pods -n ai-serving -l serving.kserve.io/inferenceservice=llama3-8b-vllm
```

### 模型解释性（Explainer）

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: sklearn-iris-explain
  namespace: ai-serving
spec:
  predictor:
    sklearn:
      storageUri: "gs://kfserving-examples/models/sklearn/1.0/model"
  explainer:
    alibi:
      type: "AnchorTabular"
      storageUri: "gs://kfserving-examples/models/sklearn/1.0/explainer"
      resources:
        requests:
          cpu: "1"
          memory: "2Gi"
```

```bash
# 🟢 调用 Explainer 获取模型解释
curl http://sklearn-iris-explain.ai-serving.example.com/v1/models/sklearn-iris:explain \
  -d '{"instances": [[6.8, 2.8, 4.8, 1.4]]}'
```

## 故障排查

### InferenceService 不就绪

```bash
# 🟢 Step 1: 检查 InferenceService 状态和条件
kubectl get inferenceservice <name> -n ai-serving -o yaml | grep -A 20 "status:"

# 🟢 Step 2: 检查 Knative Revision 状态
kubectl get revisions -n ai-serving -l serving.kserve.io/inferenceservice=<name>
kubectl describe revision <revision-name> -n ai-serving

# 🟢 Step 3: 检查 Pod 事件
kubectl get pods -n ai-serving -l serving.kserve.io/inferenceservice=<name>
kubectl describe pod <pod-name> -n ai-serving

# 常见原因：
# 1. 镜像拉取失败 → 检查 imagePullSecrets 和镜像地址
# 2. GPU 资源不足 → 检查节点 GPU 可用量
# 3. 模型下载失败 → 检查 storageUri 和存储凭证
# 4. Readiness Probe 超时 → 增大 initialDelaySeconds
# 5. Knative 配置错误 → 检查 config-features ConfigMap
```

### 流量路由异常

```bash
# 🟢 检查 VirtualService（Istio 模式）
kubectl get virtualservice -n ai-serving
kubectl get virtualservice <name> -n ai-serving -o yaml

# 🟢 检查 Knative Route
kubectl get routes -n ai-serving
kubectl get route <name> -n ai-serving -o yaml

# 🟢 检查 Ingress Gateway
kubectl get gateway -n istio-system
kubectl logs -n istio-system -l app=istio-ingressgateway --tail=50

# 常见问题：
# 1. 502 Bad Gateway → Pod 未就绪或端口不匹配
# 2. 404 Not Found → Host header 不正确
# 3. 金丝雀流量不生效 → 检查 canaryTrafficPercent 和 Revision 状态
```

### Scale-to-Zero 后无法唤醒

```bash
# 🟢 检查 Knative Activator 日志
kubectl logs -n knative-serving -l app=activator --tail=50

# 🟢 检查 Pod 是否正在创建
kubectl get pods -n ai-serving -w

# 常见原因：
# 1. 模型加载时间超过请求超时 → 增大 Knative revision-timeout-seconds
# 2. GPU 节点无可用资源 → Pod Pending
# 3. PVC 挂载失败 → 检查 StorageClass 和 PV 状态
```

## 最佳实践

1. **GPU 推理服务避免 Scale-to-Zero**：大模型加载耗时数分钟，Scale-to-Zero 后首次请求会超时。GPU 服务设置 `minReplicas: 1`，仅 CPU 轻量模型使用 Scale-to-Zero 节省成本。

2. **RawDeployment 用于延迟敏感场景**：Knative 的 Activator 组件增加约 5-10ms 网络跳数。对于 P99 延迟要求 < 50ms 的场景，使用 RawDeployment 模式绕过 Knative 数据面。

3. **模型存储统一化**：使用 S3/GCS/MinIO 作为模型存储后端，配合 `storageUri` 实现模型版本管理。避免将模型 bake 进镜像（镜像过大、更新慢）。

4. **资源配额与 LimitRange**：为 AI Serving Namespace 设置 ResourceQuota，防止单个模型占满集群 GPU。配合 [[22-概念/07-调度与资源/dynamic-resource-allocation]] 实现更精细的资源管理。

5. **多集群模型分发**：大规模场景下使用 KServe ModelMesh 或 ClusterServingRuntime 实现跨集群模型分发和负载均衡。

6. **可观测性三支柱**：Metrics（Prometheus + Grafana）、Logging（结构化 JSON 日志）、Tracing（Istio 分布式追踪）缺一不可。GPU 监控参见 [[15-AI基础设施/01-基础设施/04-gpu-monitoring-dcgm]]。

7. **安全加固**：启用 Istio mTLS 加密服务间通信；使用 NetworkPolicy 限制推理服务仅接受 Gateway 流量；模型存储使用 IAM 认证而非静态凭证。

## Related

- [[15-AI基础设施/01-基础设施/10-model-deployment-serving]]
- [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads]]
- [[22-概念/07-调度与资源/dynamic-resource-allocation]]
- [[15-AI基础设施/01-基础设施/04-gpu-monitoring-dcgm]]
- [[05-网络/01-K8s网络核心/]]
