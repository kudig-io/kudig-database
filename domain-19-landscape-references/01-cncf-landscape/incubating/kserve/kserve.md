---
title: KServe
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- istio
- helm
- opa
- ingress
- gateway
- gpu
- nvidia
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KServe 是什么
- 如何 KServe
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KServe
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- prometheus-basics
- gpu-scheduling-basics
- policy-basics
---

title: KServe
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- istio
- helm
- opa
- ingress
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KServe 是什么
- 如何 KServe
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KServe
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# KServe

> **成熟度**: Incubating | **加入时间**: 2022-07 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kserve.github.io/website |
| **GitHub** | https://github.com/kserve/kserve |
| **许可证** | Apache-2.0 |
| **主要语言** | Python, Go |
| **CNCF 分类** | AI/ML Serving |

---

## 项目概述

KServe（前身 KFServing）是 Kubernetes 上的标准化模型推理平台。它提供无服务器推理、自动扩缩容、金丝雀部署和模型解释能力，支持 TensorFlow、PyTorch、scikit-learn、XGBoost 等主流框架。

## 核心特性

- **标准化接口**: 统一的 V1/V2 推理协议
- **多框架支持**: TensorFlow、PyTorch、Triton、ONNX、XGBoost 等
- **Serverless**: 基于 Knative 的自动扩缩容（可缩至零）
- **高级部署**: 金丝雀发布、A/B 测试、蓝绿部署
- **模型解释**: 集成 Alibi Explainer 提供可解释性
- **GPU 支持**: 自动 GPU 调度和资源管理
- **模型监控**: 请求日志、指标导出、漂移检测

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      KServe Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    InferenceService                        │ │
│  │                                                            │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │ Transformer │─▶│  Predictor  │─▶│   Explainer     │   │ │
│  │  │ (Pre/Post   │  │  (Model     │  │   (Model        │   │ │
│  │  │  Process)   │  │   Serving)  │  │   Explanation)  │   │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │ │
│  │                                                            │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                  Serving Runtimes                          │ │
│  │                                                            │ │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────────┐   │ │
│  │  │TFServing │ │TorchServe│ │  Triton  │ │ SKLearn    │   │ │
│  │  │          │ │          │ │          │ │ Server     │   │ │
│  │  └──────────┘ └──────────┘ └──────────┘ └────────────┘   │ │
│  │                                                            │ │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────────┐   │ │
│  │  │  XGBoost │ │  LightGBM│ │   ONNX   │ │   Custom   │   │ │
│  │  │  Server  │ │  Server  │ │  Runtime │ │   Runtime  │   │ │
│  │  └──────────┘ └──────────┘ └──────────┘ └────────────┘   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│          ┌───────────────────┼───────────────────┐              │
│          ▼                   ▼                   ▼              │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────┐     │
│  │   Knative   │    │   Istio/    │    │   Prometheus    │     │
│  │   Serving   │    │   Gateway   │    │   (Metrics)     │     │
│  │(Autoscaling)│    │  (Routing)  │    │                 │     │
│  └─────────────┘    └─────────────┘    └─────────────────┘     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 KServe

```bash
# 安装 KServe（需要 Knative 和 Istio/Kourier）
kubectl apply -f https://github.com/kserve/kserve/releases/download/v0.12.0/kserve.yaml

# 安装默认 ServingRuntimes
kubectl apply -f https://github.com/kserve/kserve/releases/download/v0.12.0/kserve-runtimes.yaml

# 或使用 Helm
helm install kserve oci://ghcr.io/kserve/charts/kserve \
  --namespace kserve \
  --create-namespace
```

### 部署模型

```yaml
# sklearn-iris.yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: sklearn-iris
spec:
  predictor:
    model:
      modelFormat:
        name: sklearn
      storageUri: "gs://kfserving-examples/models/sklearn/1.0/model"
```

```bash
kubectl apply -f sklearn-iris.yaml

# 查看状态
kubectl get inferenceservice sklearn-iris
```

### 发送推理请求

```bash
# 获取 Ingress 地址
INGRESS_HOST=$(kubectl get service istio-ingressgateway -n istio-system -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
SERVICE_HOSTNAME=$(kubectl get inferenceservice sklearn-iris -o jsonpath='{.status.url}' | cut -d "/" -f 3)

# V1 协议
curl -v -H "Host: ${SERVICE_HOSTNAME}" \
  "http://${INGRESS_HOST}/v1/models/sklearn-iris:predict" \
  -d '{"instances": [[6.8, 2.8, 4.8, 1.4], [6.0, 3.4, 4.5, 1.6]]}'

# V2 协议
curl -v -H "Host: ${SERVICE_HOSTNAME}" \
  "http://${INGRESS_HOST}/v2/models/sklearn-iris/infer" \
  -d '{
    "inputs": [{
      "name": "input-0",
      "shape": [2, 4],
      "datatype": "FP32",
      "data": [[6.8, 2.8, 4.8, 1.4], [6.0, 3.4, 4.5, 1.6]]
    }]
  }'
```

---

## 主流框架部署

### TensorFlow

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: tensorflow-mnist
spec:
  predictor:
    model:
      modelFormat:
        name: tensorflow
      storageUri: "gs://kfserving-examples/models/tensorflow/mnist"
      resources:
        limits:
          nvidia.com/gpu: 1
```

### PyTorch

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: pytorch-cifar10
spec:
  predictor:
    model:
      modelFormat:
        name: pytorch
      storageUri: "gs://kfserving-examples/models/pytorch/cifar10"
      protocolVersion: v2
```

### NVIDIA Triton

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: triton-bert
spec:
  predictor:
    model:
      modelFormat:
        name: triton
      storageUri: "gs://kfserving-examples/models/triton/bert"
      resources:
        limits:
          nvidia.com/gpu: 1
```

### 自定义模型

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: custom-model
spec:
  predictor:
    containers:
      - name: kserve-container
        image: my-registry/custom-model:v1
        ports:
          - containerPort: 8080
            protocol: TCP
        env:
          - name: MODEL_NAME
            value: custom-model
```

---

## 高级部署

### 金丝雀发布

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: sklearn-iris
spec:
  predictor:
    canaryTrafficPercent: 20
    model:
      modelFormat:
        name: sklearn
      storageUri: "gs://models/sklearn/v2"  # 新版本
```

### Transformer（预处理）

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: image-classifier
spec:
  transformer:
    containers:
      - name: image-transformer
        image: kserve/image-transformer:latest
        env:
          - name: STORAGE_URI
            value: "gs://models/image-transformer"
  predictor:
    model:
      modelFormat:
        name: tensorflow
      storageUri: "gs://models/resnet"
```

### Explainer（模型解释）

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: sklearn-iris-explainer
spec:
  predictor:
    model:
      modelFormat:
        name: sklearn
      storageUri: "gs://models/sklearn/iris"
  explainer:
    alibi:
      type: AnchorTabular
      storageUri: "gs://models/sklearn/iris-explainer"
```

---

## 自动扩缩容

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: sklearn-iris
  annotations:
    # 最小/最大副本
    autoscaling.knative.dev/min-scale: "1"
    autoscaling.knative.dev/max-scale: "10"
    # 扩缩容指标
    autoscaling.knative.dev/metric: "concurrency"
    autoscaling.knative.dev/target: "10"
spec:
  predictor:
    model:
      modelFormat:
        name: sklearn
      storageUri: "gs://models/sklearn/iris"
```

---

## 模型存储

```yaml
# S3 存储
spec:
  predictor:
    model:
      storageUri: "s3://my-bucket/models/sklearn"
      
# Azure Blob
spec:
  predictor:
    model:
      storageUri: "https://myaccount.blob.core.windows.net/models/sklearn"

# PVC
spec:
  predictor:
    model:
      storageUri: "pvc://my-model-pvc/sklearn"
```

### 存储凭证

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: storage-config
  annotations:
    serving.kserve.io/s3-endpoint: s3.amazonaws.com
    serving.kserve.io/s3-usehttps: "1"
type: Opaque
stringData:
  AWS_ACCESS_KEY_ID: "xxx"
  AWS_SECRET_ACCESS_KEY: "xxx"
```

---

## 监控

```yaml
# Prometheus 指标
- kserve_prediction_count
- kserve_prediction_latency_bucket
- kserve_model_ready
- kserve_model_loaded_count
```

---

## 最佳实践

1. **模型版本管理**: 使用 storageUri 路径区分版本
2. **资源配置**: 根据模型大小配置合适的 memory/GPU
3. **健康检查**: 配置 liveness/readiness probe
4. **预热**: 生产环境设置 min-scale >= 1 避免冷启动
5. **监控告警**: 监控推理延迟和错误率

---

## 参考资源

- [官方文档](https://kserve.github.io/website)
- [GitHub Repo](https://github.com/kserve/kserve)
- [模型示例](https://github.com/kserve/kserve/tree/master/docs/samples)
- [V2 推理协议](https://kserve.github.io/website/modelserving/v1beta1/v2_protocol/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[references/specialized-workloads-terms|K8s 专用工作负载术语参考]] — Cross-reference
- [[references/root-terms|K8s Root术语参考]] — Cross-reference
- [[skills/ts-ai-ml-workloads|AI/ML 工作负载排查]] — Cross-reference
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
