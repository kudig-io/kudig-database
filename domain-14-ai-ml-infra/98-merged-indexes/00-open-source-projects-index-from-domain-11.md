---
title: Domain-11 AI 基础设施 — 开源项目索引
description: '- [四、GPU 调度与管理](#四gpu-调度与管理)'
category: ai-infra
tags:
- k8s
- ai
- gpu
- ml
- training
- inference
- scheduler
- prometheus
- grafana
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
estimated_read_time: 5min
intent_queries:
- Domain-11 AI 基础设施 — 开源项目索引 是什么
- 如何 Domain-11 AI 基础设施 — 开源项目索引
- Kubernetes 11 ai infra 最佳实践
trigger_keywords:
- Domain-11
- AI
- 基础设施
- 开源项目索引
- ai
- infra
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
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
cross_refs:
- type: domain
  path: ../domain-02-workloads-applications/
  label: '相关知识域: domain-02-workloads-applications'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/go.md
  label: '速查卡: go'
created: "2026-05-23"
---

# Domain-11 AI 基础设施 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Kubeflow v1.10 / KServe v0.15 / Fluid v1.0

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、Kubeflow (CNCF Incubating)](#二kubeflow-cncf-incubating)
- [三、KServe (CNCF Incubating)](#三kserve-cncf-incubating)
- [四、GPU 调度与管理](#四gpu-调度与管理)
- [五、数据编排与缓存](#五数据编排与缓存)
- [六、分布式训练框架](#六分布式训练框架)
- [七、AI 流水线与实验管理](#七ai-流水线与实验管理)
- [八、版本兼容矩阵](#八版本兼容矩阵)
- [九、AI 基础设施架构选型](#九ai-基础设施架构选型)

---

<!-- chunk: 一、核心项目总览 -->
## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Kubeflow** | ML 工作流平台 | Incubating | v1.10.0 | 14k+ | Apache-2.0 |
| **KServe** | 模型推理服务 | Incubating | v0.15.0 | 3k+ | Apache-2.0 |
| **Fluid** | 数据集缓存编排 | Incubating | v1.0.6 | 1.5k+ | Apache-2.0 |
| **KubeRay** | Ray on K8s | 非 CNCF | v1.3.0 | 2k+ | Apache-2.0 |
| **Volcano** | 批处理调度器 | 非 CNCF | v1.11.0 | 4k+ | Apache-2.0 |
| **NVIDIA GPU Operator** | GPU 驱动与设备插件 | NVIDIA | v24.9.0 | 2k+ | Apache-2.0 |
| **NVIDIA Device Plugin** | K8s GPU 设备发现 | NVIDIA | v0.17.0 | 3k+ | Apache-2.0 |
| **DCGM Exporter** | GPU 指标导出 | NVIDIA | v4.1.0 | 1k+ | Apache-2.0 |
| **Triton Inference Server** | 多框架推理服务 | NVIDIA | v2.55.0 | 7k+ | BSD-3 |
| **vLLM** | LLM 推理加速 | 非 CNCF | v0.11.0 | 45k+ | Apache-2.0 |
| **MLflow** | ML 生命周期管理 | 非 CNCF | v2.21.0 | 19k+ | Apache-2.0 |
| **BentoML** | 模型服务框架 | 非 CNCF | v1.4.0 | 7k+ | Apache-2.0 |
| **Seldon Core** | ML 部署平台 | 非 CNCF | v1.18.0 | 4k+ | Apache-2.0 |
| **Apache Airflow** | 工作流编排 | Apache | v2.10.0 | 39k+ | Apache-2.0 |
| **Kueue** | K8s 作业队列管理 | K8s SIG | v0.11.0 | 1.5k+ | Apache-2.0 |

---

<!-- chunk: 二、Kubeflow (CNCF Incubating) -->
## 二、Kubeflow (CNCF Incubating)

### 2.1 核心组件

```yaml
# Kubeflow 平台组件
- Central Dashboard: 统一入口
- Notebooks: Jupyter/VSCode 工作空间
- Pipelines: 基于 Argo Workflows 的 ML 流水线
- Katib: 超参数调优与神经架构搜索 (NAS)
- Training Operator: 分布式训练作业管理 (TFJob/PyTorchJob/XGBoostJob/MPIJob)
- KServe: 模型服务 (已独立，可选集成)
- Manifests: 统一安装配置
```

### 2.2 v1.10 更新要点

- Training Operator v1 稳定
- 改进的认证与多用户隔离
- 组件版本对齐 K8s v1.29+

**GitHub**: https://github.com/kubeflow/kubeflow
**文档**: https://www.kubeflow.org/docs/

---

<!-- chunk: 三、KServe (CNCF Incubating) -->
## 三、KServe (CNCF Incubating)

### 3.1 云原生模型推理平台

> **2025.09 新晋 CNCF Incubating 项目**

```yaml
# 核心特性
- 标准化 InferenceService CRD
- 自动缩放 (包括 GPU 感知的 scale-from-zero)
- 金丝雀发布与 A/B 测试
- 多框架支持 (TensorFlow, PyTorch, ONNX, Triton, vLLM, HuggingFace)
- 解释性服务 (Explainability)
- 模型可观测性 (Prometheus 指标)
```

### 3.2 架构

```
InferenceService
├── Predictor (必需): 核心推理服务
├── Transformer (可选): 请求/响应转换
├── Explainer (可选): 模型解释
└── Route: 流量分配策略
```

```yaml
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

**GitHub**: https://github.com/kserve/kserve
**文档**: https://kserve.github.io/website/latest/

---

<!-- chunk: 四、GPU 调度与管理 -->
## 四、GPU 调度与管理

### 4.1 NVIDIA GPU Operator

```yaml
# 自动部署组件
- NVIDIA 驱动 (Driver)
- NVIDIA Container Toolkit
- Device Plugin (GPU 发现与分配)
- DCGM Exporter (GPU 监控)
- GPU Feature Discovery (GPU 特性标签)
- MIG Manager (Multi-Instance GPU)
```

### 4.2 MIG (Multi-Instance GPU)

- A100/H100 支持物理分区
- K8s 中通过 device-plugin 暴露为独立资源
- `nvidia.com/gpu` vs `nvidia.com/mig-1g.5gb`

### 4.3 GPU 时间切片 (Time-slicing)

- 软件级 GPU 共享
- 适用于开发/测试环境
- 生产环境推荐 MIG 或 MPS

### 4.4 其他 GPU 方案

| 方案 | 适用场景 | 厂商 |
|:---|:---|:---|
| AMD GPU Operator | AMD GPU | AMD |
| Intel GPU Plugin | Intel Arc/Data Center GPU | Intel |
| Alibaba Cloud GPU Share | GPU 共享调度 | 阿里云 |
|HAMi | 异构 AI 算力虚拟化 | 开源 |

---

<!-- chunk: 五、数据编排与缓存 -->
## 五、数据编排与缓存

### 5.1 Fluid (CNCF Incubating)

```yaml
# 核心能力
- Dataset CRD: 将数据集抽象为 K8s 资源
- Runtime: 缓存引擎 (Alluxio, JuiceFS, Vineyard)
- 数据预热与亲和调度
- 数据集弹性伸缩
```

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: imagenet
spec:
  mounts:
  - mountPoint: oss://mybucket/imagenet/
    name: imagenet
---
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: imagenet
spec:
  replicas: 2
  tieredstore:
    levels:
    - mediumtype: MEM
      path: /dev/shm
      quota: 50Gi
```

**GitHub**: https://github.com/fluid-cloudnative/fluid

---

<!-- chunk: 六、分布式训练框架 -->
## 六、分布式训练框架

### 6.1 Training Operator (Kubeflow)

| 作业类型 | 框架 | 适用场景 |
|:---|:---|:---|
| TFJob | TensorFlow | Parameter Server / MultiWorkerMirroredStrategy |
| PyTorchJob | PyTorch | DDP / FSDP |
| XGBoostJob | XGBoost | 分布式梯度提升 |
| MPIJob | MPI (OpenMPI/MPICH) | 高性能计算 |
| PaddleJob | PaddlePaddle | 百度深度学习框架 |

### 6.2 KubeRay

- Ray 集群的 K8s 原生管理
- 支持 Ray Train (分布式训练)、Ray Serve (模型服务)、Ray Tune (超参搜索)
- 自动头节点发现、Worker 弹性伸缩

**GitHub**: https://github.com/ray-project/kuberay

### 6.3 Volcano

- 批处理调度器，替代默认 scheduler
- Gang Scheduling (All-or-Nothing)
- 队列与优先级
- 支持 TensorFlow、PyTorch、MPI、Spark

**GitHub**: https://github.com/volcano-sh/volcano

---

<!-- chunk: 七、AI 流水线与实验管理 -->
## 七、AI 流水线与实验管理

### 7.1 MLflow

- 实验追踪 (Experiments & Runs)
- 模型注册中心 (Model Registry)
- 模型服务 (MLflow Model Serving)
- 项目打包 (MLflow Projects)

### 7.2 Apache Airflow

- 通用工作流编排 (非 K8s 原生)
- KubernetesPodOperator 在 K8s 上运行任务
- 与 Kubeflow Pipelines 互补

### 7.3 BentoML / Yatai

- 模型服务标准化 (Bento)
- 自适应批处理 (Adaptive Batching)
- Yatai: BentoML 的 K8s 部署平台

---

<!-- chunk: 八、版本兼容矩阵 -->
## 八、版本兼容矩阵

| 组件 | K8s v1.31 | v1.32 | v1.33 | 备注 |
|:---|:---|:---|:---|:---|
| Kubeflow v1.10 | ✅ | ✅ | ⚠️ 验证 | 组件众多 |
| KServe v0.15 | ✅ | ✅ | ✅ | Knative 依赖 |
| Fluid v1.0 | ✅ | ✅ | ✅ | Runtime 兼容 |
| KubeRay v1.3 | ✅ | ✅ | ✅ | Ray v2.40+ |
| Volcano v1.11 | ✅ | ✅ | ✅ | 替代 Scheduler |
| GPU Operator v24.9 | ✅ | ✅ | ✅ | NVIDIA 驱动绑定 |
| Kueue v0.11 | ✅ | ✅ | ✅ | 原生作业队列 |

---

<!-- chunk: 九、AI 基础设施架构选型 -->
## 九、AI 基础设施架构选型

```
┌─────────────────────────────────────────────────────────────┐
│                 AI on K8s 参考架构                            │
└─────────────────────────────────────────────────────────────┘

训练层
  ├── Kubeflow Training Operator ──► TFJob/PyTorchJob/MPIJob
  ├── KubeRay ──► Ray 分布式训练与超参搜索
  ├── Volcano ──► Gang Scheduling 保障 All-Reduce
  └── GPU Operator ──► 驱动/DevicePlugin/DCGM

数据层
  ├── Fluid + Alluxio/JuiceFS ──► 数据集缓存与亲和调度
  ├── PVC + CSI ──► 高性能并行文件系统
  └── 对象存储 (S3/OSS) ──► 原始数据湖

推理层
  ├── KServe ──► 标准化模型服务 (Serverless/KNative)
  ├── Triton ──► 多框架高性能推理
  ├── vLLM ──► LLM 推理加速 (PagedAttention)
  └── BentoML ──► 自适应批处理服务

管理层
  ├── Kubeflow Pipelines ──► ML 工作流编排
  ├── MLflow ──► 实验追踪与模型注册
  ├── Katib ──► 自动超参优化
  └── Prometheus + Grafana ──► GPU/训练指标监控

调度层
  ├── Kueue ──► 原生 K8s 作业队列与配额
  ├── Volcano ──► 批处理专用调度
  └── 节点亲和 / Pod 拓扑分布 ──► 数据局部性
```

---

<!-- chunk: 参考链接 -->
## 参考链接

- [Kubeflow 官方文档](https://www.kubeflow.org/docs/)
- [KServe 文档](https://kserve.github.io/website/latest/)
- [Fluid 文档](https://fluid-cloudnative.github.io/docs/)
- [NVIDIA GPU Operator](https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/overview.html)
- [KubeRay 文档](https://docs.ray.io/en/latest/cluster/kubernetes/index.html)
- [Volcano 文档](https://volcano.sh/en/docs/)
- [CNCF AI 白皮书](https://github.com/cncf/tag-runtime/blob/main/whitepapers/ai-ml-platforms/index.md)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-11-ai-infra MOC
- [[domain-14-ai-ml-infra/README|Domain-11: AI基础设施]]
- AI 基础设施架构
- 132 - AI/ML工作负载运维 (AI/ML Workloads Operations)
- GPU 调度与管理
- GPU监控与可观测性
- 分布式训练框架
- AI数据处理Pipeline与特征工程
- AI实验管理与MLOps平台
- AutoML与超参数调优
- AI模型注册中心与版本管理
- AI模型部署与生命周期管理
