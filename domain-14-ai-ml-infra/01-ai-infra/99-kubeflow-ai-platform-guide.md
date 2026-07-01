---
title: Kubeflow AI 平台部署与实践指南
description: '# Kubeflow AI 平台部署与实践指南'
category: ai-infra
tags:
- k8s
- ai
- gpu
- ml
- training
- inference
- prometheus
- grafana
- istio
- harbor
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
estimated_read_time: 5min
intent_queries:
- Kubeflow AI 平台部署与实践指南 是什么
- 如何 Kubeflow AI 平台部署与实践指南
- Kubernetes 11 ai infra 最佳实践
trigger_keywords:
- Kubeflow
- AI
- 平台部署与实践指南
- ai
- infra
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gpu-scheduling-basics
- logging-basics
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

# [[Kubeflow|Kubeflow]] AI 平台部署与实践指南

> **适用版本**: Kubeflow v1.10.0  
> **最后更新**: 2026-04-24  
> **难度**: 高级

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、核心组件架构](#一核心组件架构)
- [二、部署方式](#二部署方式)
- [三、Notebook 工作空间](#三notebook-工作空间)
- [四、Pipelines 工作流编排](#四pipelines-工作流编排)
- [五、Katib 超参数调优](#五katib-超参数调优)
- [六、Training Operator 分布式训练](#六training-operator-分布式训练)
- [七、[[KServe|KServe]] 模型服务集成](#七kserve-模型服务集成)
- [八、多租户与隔离](#八多租户与隔离)
- [九、生产环境 checklist](#九生产环境-checklist)

---

<!-- chunk: 一、核心组件架构 -->
## 一、核心组件架构

```
Kubeflow 平台
├── Central Dashboard (统一入口)
├── Notebooks (Jupyter / VSCode 工作空间)
├── Pipelines (基于 Argo Workflows 的 ML 流水线)
│   └── SDK: kfp
├── Katib (超参数调优 / AutoML / NAS)
├── Training Operator (分布式训练作业)
│   ├── TFJob (TensorFlow)
│   ├── PyTorchJob (PyTorch)
│   ├── MPIJob (MPI)
│   └── XGBoostJob
├── KServe (模型推理服务, 可选独立)
└── Manifests (统一安装配置)
```

---

<!-- chunk: 二、部署方式 -->
## 二、部署方式

### 2.1 Manifests 安装 (官方推荐)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 设置环境变量
export KUBEFLOW_VERSION=v1.10.0

# 下载并安装
wget https://github.com/kubeflow/manifests/archive/refs/tags/${KUBEFLOW_VERSION}.tar.gz
tar -xzf ${KUBEFLOW_VERSION}.tar.gz
cd manifests-${KUBEFLOW_VERSION}

# 完整安装 (包含所有组件)
while ! kustomize build example | kubectl apply -f -; do
  echo "Retrying to apply resources..."
  sleep 20
done
```

### 2.2 组件选择性安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 仅安装核心 + Pipelines + Training
kustomize build apps/pipeline/upstream | kubectl apply -f -
kustomize build apps/training-operator/upstream | kubectl apply -f -
```

### 2.3 重要前置条件

| 组件 | 要求 |
|:---|:---|
| K8s 版本 | v1.29+ |
| Storage | 默认 StorageClass (PVC) |
| [[Ingress|Ingress]] | Istio / NGINX / 云厂商 LB |
| GPU (可选) | NVIDIA GPU Operator 预装 |
| 资源 | 至少 8C16G 控制平面节点 |

---

<!-- chunk: 三、Notebook 工作空间 -->
## 三、Notebook 工作空间

```yaml
apiVersion: kubeflow.org/v1
kind: Notebook
metadata:
  name: data-science-workspace
  namespace: kubeflow-user-example-com
spec:
  template:
    spec:
      containers:
      - name: notebook
        image: kubeflownotebookswg/jupyter-scipy:v1.10.0
        resources:
          requests:
            cpu: "1"
            memory: 4Gi
          limits:
            cpu: "4"
            memory: 16Gi
            nvidia.com/gpu: "1"  # GPU 工作空间
        volumeMounts:
        - name: workspace
          mountPath: /home/jovyan
      volumes:
      - name: workspace
        persistentVolumeClaim:
          claimName: workspace-pvc
```

**常用镜像**
- `jupyter-scipy`: 基础科学计算
- `jupyter-pytorch`: PyTorch + CUDA
- `jupyter-tensorflow`: TensorFlow + CUDA
- `jupyter-pytorch-full`: PyTorch + 常用 ML 库

---

<!-- chunk: 四、Pipelines 工作流编排 -->
## 四、Pipelines 工作流编排

### 4.1 Python SDK 定义流水线

```python
from kfp import dsl
from kfp import client

@dsl.component(base_image="python:3.11")
def preprocess_data(input_path: str, output_path: str):
    import pandas as pd
    df = pd.read_csv(input_path)
    df = df.dropna()
    df.to_csv(output_path, index=False)

@dsl.component(base_image="pytorch/pytorch:2.4.0-cuda12.1-cudnn9-runtime")
def train_model(data_path: str, model_path: str, epochs: int):
    import torch
    # 训练逻辑
    torch.save(model.state_dict(), model_path)

@dsl.pipeline(name="ml-training-pipeline")
def my_pipeline(
    input_data: str = "s3://bucket/raw-data.csv",
    epochs: int = 10
):
    preprocess_task = preprocess_data(
        input_path=input_data,
        output_path="/tmp/processed.csv"
    )
    train_task = train_model(
        data_path=preprocess_task.outputs["output_path"],
        model_path="/tmp/model.pt",
        epochs=epochs
    )

# 提交运行
kfp_client = client.Client(host="http://ml-pipeline.kubeflow:8888")
run = kfp_client.create_run_from_pipeline_func(
    my_pipeline,
    arguments={"input_data": "s3://mybucket/data.csv", "epochs": 20}
)
```

---

<!-- chunk: 五、Katib 超参数调优 -->
## 五、Katib 超参数调优

```yaml
apiVersion: kubeflow.org/v1beta1
kind: Experiment
metadata:
  name: hyperparameter-tuning
  namespace: kubeflow
spec:
  objective:
    type: maximize
    goal: 0.99
    objectiveMetricName: accuracy
  algorithm:
    algorithmName: random
  parallelTrialCount: 3
  maxTrialCount: 12
  maxFailedTrialCount: 3
  parameters:
  - name: learning_rate
    parameterType: double
    feasibleSpace:
      min: "0.001"
      max: "0.1"
  - name: batch_size
    parameterType: int
    feasibleSpace:
      min: "16"
      max: "128"
  trialTemplate:
    primaryContainerName: training-container
    trialParameters:
    - name: learningRate
      description: Learning rate for the training model
      reference: learning_rate
    - name: batchSize
      description: Batch size for the training model
      reference: batch_size
    trialSpec:
      apiVersion: batch/v1
      kind: Job
      spec:
        template:
          spec:
            containers:
            - name: training-container
              image: my-registry/training:latest
              command:
              - python
              - train.py
              - --lr=${trialParameters.learningRate}
              - --batch-size=${trialParameters.batchSize}
            restartPolicy: Never
```

---

<!-- chunk: 六、Training Operator 分布式训练 -->
## 六、Training Operator 分布式训练

### 6.1 PyTorchJob (DDP)

```yaml
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: pytorch-ddp-training
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: my-registry/pytorch-train:latest
            command:
            - python
            - -m
            - torch.distributed.run
            - --nproc_per_node=4
            - train.py
            resources:
              limits:
                nvidia.com/gpu: 4
    Worker:
      replicas: 3
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: my-registry/pytorch-train:latest
            command:
            - python
            - -m
            - torch.distributed.run
            - --nproc_per_node=4
            - train.py
            resources:
              limits:
                nvidia.com/gpu: 4
```

---

<!-- chunk: 七、KServe 模型服务集成 -->
## 七、KServe 模型服务集成

```yaml
# 在 Kubeflow 中集成 KServe
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: sklearn-iris
  namespace: kubeflow-user-example-com
  annotations:
    sidecar.istio.io/inject: "false"
spec:
  predictor:
    model:
      modelFormat:
        name: sklearn
      storageUri: "pvc://model-pvc/sklearn/iris"
```

---

<!-- chunk: 八、多租户与隔离 -->
## 八、多租户与隔离

### 8.1 Profile (命名空间 + RBAC)

```yaml
apiVersion: kubeflow.org/v1
kind: Profile
metadata:
  name: team-data-science
spec:
  owner:
    kind: User
    name: data-lead@example.com
  resourceQuotaSpec:
    hard:
      cpu: "100"
      memory: 500Gi
      nvidia.com/gpu: "20"
      requests.storage: 2Ti
```

---

<!-- chunk: 九、生产环境 checklist -->
## 九、生产环境 checklist

| 检查项 | 要求 |
|:---|:---|
| 持久化存储 | PVC + 备份策略 |
| GPU 节点隔离 | Taints/Tolerations + Node Selector |
| 网络策略 | 限制 Notebook 访问范围 |
| 资源配额 | Profile 级别限制 |
| 镜像安全 | Harbor + cosign 签名验证 |
| 日志收集 | Fluent Bit → Loki/Elasticsearch |
| 监控告警 | Prometheus + Grafana |
| 流水线安全 | 最小权限 ServiceAccount |
| 数据隔离 | S3/OSS Bucket 按团队隔离 |
| 成本追踪 | OpenCost 按 Namespace 归因 |

---

<!-- chunk: 参考链接 -->
## 参考链接

- [Kubeflow 官方文档](https://www.kubeflow.org/docs/)
- [Kubeflow Manifests](https://github.com/kubeflow/manifests)
- [KServe 文档](https://kserve.github.io/website/latest/)
- [Kubeflow Pipelines SDK](https://kubeflow-pipelines.readthedocs.io/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-11-ai-infra MOC
- [[domain-14-ai-ml-infra/README.md|Domain-11: AI基础设施]]
- Domain-11 AI 基础设施 — 开源项目索引
- AI 基础设施架构
- 132 - AI/ML工作负载运维 (AI/ML Workloads Operations)
- GPU 调度与管理
- GPU监控与可观测性
- 分布式训练框架
- AI数据处理Pipeline与特征工程
- AI实验管理与MLOps平台
- AutoML与超参数调优
- AI模型注册中心与版本管理

## See Also

- 36-ai-platform-observability-enhanced
- 37-agent-sandbox-security
- 01-ai-infrastructure-overview
- 02-ai-ml-workloads

## Related

- [[domain-19-landscape-references/topic-index/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]

```