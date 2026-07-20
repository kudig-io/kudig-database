---
title: "AI 平台工程：内部 AI 平台设计与 MLOps 流水线"
description: "内部 AI 平台的设计原则、自助服务架构、模型生命周期管理及 MLOps 流水线构建"
summary: "从平台工程视角讲解内部 AI 平台的设计：开发者自助服务体验、模型训练-评估-部署全生命周期管理、MLOps 流水线自动化及 GPU 资源编排"
category: 平台工程
tags:
- ai-platform
- mlops
- model-lifecycle
- developer-experience
- self-service
- platform-engineering
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- AI 工程师
estimated_read_time: 20min
intent_queries:
- "如何设计内部 AI 平台"
- "MLOps 流水线怎么搭建"
- "AI 平台如何提供自助服务"
trigger_keywords:
- ai-platform
- mlops
- model-lifecycle
- self-service
- developer-experience
prerequisites:
- kubectl-basics
- platform-engineering-basics
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

# AI 平台工程

## 概述

AI 平台工程是将平台工程（Platform Engineering）理念应用于 AI/ML 工作负载的实践。传统平台工程关注"让开发者自助部署微服务"，AI 平台工程则关注"让数据科学家和 AI 工程师自助完成模型训练、评估、部署和监控的全生命周期"。

一个成熟的内部 AI 平台需要解决三个核心矛盾：
1. **灵活性 vs 标准化**：AI 研究员需要实验自由度，但平台需要标准化以降低运维成本
2. **GPU 稀缺 vs 需求爆发**：GPU 资源有限，需要智能调度和公平分配
3. **快速迭代 vs 生产稳定**：模型需要快速实验，但生产推理服务需要高可用保障

本文从平台架构、自助服务、模型生命周期、MLOps 流水线和开发者体验五个维度展开。

## 核心概念

### AI 平台架构分层

```
┌─────────────────────────────────────────────────────┐
│  Layer 5: 开发者体验（Portal / CLI / Notebook）       │
├─────────────────────────────────────────────────────┤
│  Layer 4: MLOps 流水线（训练→评估→部署→监控）         │
├─────────────────────────────────────────────────────┤
│  Layer 3: 模型服务（推理引擎 / 模型仓库 / A/B 测试）   │
├─────────────────────────────────────────────────────┤
│  Layer 2: 计算编排（GPU 调度 / 弹性伸缩 / 队列管理）   │
├─────────────────────────────────────────────────────┤
│  Layer 1: 基础设施（GPU 节点 / 存储 / 网络 / 监控）    │
└─────────────────────────────────────────────────────┘
```

### 模型生命周期

| 阶段 | 活动 | 平台能力 | 工具 |
|------|------|---------|------|
| 数据准备 | 数据收集、清洗、标注 | 数据版本管理、特征存储 | DVC, Feast, Label Studio |
| 实验 | 模型开发、超参搜索 | Notebook 环境、实验追踪 | JupyterHub, MLflow, W&B |
| 训练 | 分布式训练、Checkpoint | GPU 调度、训练框架 | Kubeflow Training, Volcano |
| 评估 | 模型评估、公平性检查 | 自动化评估流水线 | MLflow, Evidently AI |
| 部署 | 模型打包、推理服务 | 模型仓库、推理引擎 | KServe, vLLM, Triton |
| 监控 | 数据漂移、模型退化 | 指标监控、告警 | Prometheus, Evidently |
| 迭代 | 重训练、版本回滚 | 自动化重训练触发 | Argo Workflows, Airflow |

### 自助服务模型

AI 平台的自助服务核心是"Golden Path"（黄金路径）：
- **数据科学家**：一键启动 Notebook → 选择 GPU 规格 → 挂载数据集 → 开始实验
- **ML 工程师**：提交训练任务 → 自动调度 GPU → 训练完成通知 → 一键部署
- **SRE**：监控推理服务 → 自动扩缩容 → 模型版本回滚 → 成本报告

## 生产部署

### JupyterHub 自助 Notebook 环境

```yaml
# 🟡 中风险：部署 JupyterHub（AI 开发者入口）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: jupyterhub-proxy
  namespace: ai-platform
spec:
  replicas: 1
  selector:
    matchLabels:
      app: jupyterhub-proxy
  template:
    metadata:
      labels:
        app: jupyterhub-proxy
    spec:
      containers:
      - name: proxy
        image: jupyterhub/configurable-http-proxy:4.6.1
        ports:
        - containerPort: 8000
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
---
# Notebook 实例模板（由 JupyterHub Spawner 创建）
apiVersion: v1
kind: Pod
metadata:
  name: notebook-{username}
  namespace: ai-workspace
  labels:
    app: jupyter-notebook
    user: "{username}"
  annotations:
    gpu.platform.io/max-runtime-hours: "8"
    gpu.platform.io/cost-center: "{team-cost-center}"
spec:
  runtimeClassName: nvidia
  containers:
  - name: notebook
    image: registry.example.com/ai/notebook:pytorch-2.3-cuda12.3
    ports:
    - containerPort: 8888
    resources:
      requests:
        cpu: "4"
        memory: "16Gi"
        nvidia.com/gpu: "1"
      limits:
        cpu: "8"
        memory: "32Gi"
        nvidia.com/gpu: "1"
    volumeMounts:
    - name: workspace
      mountPath: /home/jovyan/work
    - name: shared-data
      mountPath: /data
      readOnly: true
  volumes:
  - name: workspace
    persistentVolumeClaim:
      claimName: notebook-{username}-pvc
  - name: shared-data
    persistentVolumeClaim:
      claimName: shared-datasets-pvc
  # 8 小时无活动自动停止
  activeDeadlineSeconds: 28800
```

### 训练任务编排（Kubeflow Training Operator）

```yaml
# 🟡 中风险：提交分布式训练任务
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llm-finetune-7b
  namespace: team-nlp
  labels:
    project: llm-finetune
    experiment-id: "exp-2026-0719"
  annotations:
    gpu.platform.io/max-runtime-hours: "24"
    gpu.platform.io/cost-center: "CC-NLP-001"
    gpu.platform.io/priority: "training"
spec:
  nprocPerNode: "8"
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        metadata:
          labels:
            job-role: master
        spec:
          priorityClassName: gpu-training
          containers:
          - name: pytorch
            image: registry.example.com/ai/training:pytorch-2.3-deepspeed
            command:
            - torchrun
            - --nproc_per_node=8
            - --nnodes=2
            - train.py
            - --model=llama-7b
            - --data=/data/sft-dataset
            - --output=/checkpoints
            - --epochs=3
            resources:
              requests:
                nvidia.com/gpu: "8"
                cpu: "32"
                memory: "256Gi"
              limits:
                nvidia.com/gpu: "8"
                cpu: "64"
                memory: "512Gi"
            volumeMounts:
            - name: checkpoints
              mountPath: /checkpoints
            - name: dataset
              mountPath: /data
          volumes:
          - name: checkpoints
            persistentVolumeClaim:
              claimName: llm-checkpoints-pvc
          - name: dataset
            persistentVolumeClaim:
              claimName: sft-dataset-pvc
    Worker:
      replicas: 1
      restartPolicy: OnFailure
      template:
        metadata:
          labels:
            job-role: worker
        spec:
          priorityClassName: gpu-training
          containers:
          - name: pytorch
            image: registry.example.com/ai/training:pytorch-2.3-deepspeed
            command:
            - torchrun
            - --nproc_per_node=8
            - --nnodes=2
            - train.py
            - --model=llama-7b
            - --data=/data/sft-dataset
            - --output=/checkpoints
            resources:
              requests:
                nvidia.com/gpu: "8"
                cpu: "32"
                memory: "256Gi"
              limits:
                nvidia.com/gpu: "8"
                cpu: "64"
                memory: "512Gi"
            volumeMounts:
            - name: checkpoints
              mountPath: /checkpoints
            - name: dataset
              mountPath: /data
          volumes:
          - name: checkpoints
            persistentVolumeClaim:
              claimName: llm-checkpoints-pvc
          - name: dataset
            persistentVolumeClaim:
              claimName: sft-dataset-pvc
```

### 模型推理服务（KServe）

```yaml
# 🟡 中风险：部署模型推理服务
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llm-7b-inference
  namespace: ai-serving
  annotations:
    serving.kserve.io/deploymentMode: Serverless
spec:
  predictor:
    minReplicas: 1
    maxReplicas: 10
    scaleTarget: 5  # 每 5 个并发请求扩容一个副本
    scaleMetric: concurrency
    containers:
    - name: vllm
      image: registry.example.com/ai/vllm-openai:v0.6.0
      args:
      - --model=/models/llama-7b-sft
      - --tensor-parallel-size=2
      - --max-model-len=4096
      - --gpu-memory-utilization=0.9
      resources:
        requests:
          nvidia.com/gpu: "2"
          cpu: "8"
          memory: "64Gi"
        limits:
          nvidia.com/gpu: "2"
          cpu: "16"
          memory: "128Gi"
      volumeMounts:
      - name: model-storage
        mountPath: /models
    volumes:
    - name: model-storage
      persistentVolumeClaim:
        claimName: llm-7b-model-pvc
---
# 模型版本管理（Canary 部署）
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llm-7b-inference
  namespace: ai-serving
spec:
  predictor:
    canaryTrafficPercent: 10  # 新版本先接 10% 流量
    containers:
    - name: vllm
      image: registry.example.com/ai/vllm-openai:v0.6.1
      args:
      - --model=/models/llama-7b-sft-v2
```

### MLOps 流水线（Argo Workflows）

```yaml
# 🟡 中风险：MLOps 自动化流水线
apiVersion: argoproj.io/v1alpha1
kind: WorkflowTemplate
metadata:
  name: model-training-pipeline
  namespace: ai-platform
spec:
  entrypoint: pipeline
  arguments:
    parameters:
    - name: model-name
    - name: dataset-version
    - name: gpu-count
      value: "8"
  templates:
  - name: pipeline
    steps:
    - - name: validate-data
        template: data-validation
    - - name: train-model
        template: distributed-training
    - - name: evaluate-model
        template: model-evaluation
    - - name: register-model
        template: model-registry
        when: "{{steps.evaluate-model.outputs.parameters.passed}} == true"
    - - name: deploy-staging
        template: deploy-inference
        when: "{{steps.register-model.outputs.parameters.registered}} == true"
  - name: data-validation
    container:
      image: registry.example.com/ai/data-validator:v1
      command: ["python", "validate.py"]
      args: ["--dataset={{workflow.parameters.dataset-version}}"]
    resources:
      requests:
        cpu: "4"
        memory: "16Gi"
  - name: model-evaluation
    container:
      image: registry.example.com/ai/model-evaluator:v1
      command: ["python", "evaluate.py"]
      args: ["--model={{workflow.parameters.model-name}}", "--threshold=0.85"]
    resources:
      requests:
        nvidia.com/gpu: "1"
        cpu: "4"
        memory: "32Gi"
    outputs:
      parameters:
      - name: passed
        valueFrom:
          path: /tmp/eval_passed
```

## 运维操作

### 平台健康检查

```bash
# 🟢 低风险：AI 平台健康检查
# 检查 JupyterHub 状态
kubectl get pods -n ai-platform -l app=jupyterhub
kubectl get pods -n ai-workspace -l app=jupyter-notebook

# 检查训练任务状态
kubectl get pytorchjobs -A
kubectl get mpijobs -A

# 检查推理服务状态
kubectl get inferenceservices -A
kubectl get pods -n ai-serving -l serving.kserve.io/inferenceservice

# 检查 GPU 资源使用
kubectl top nodes -l nvidia.com/gpu.present=true
kubectl get pods -A -o json | jq '[.items[] | select(.spec.containers[].resources.limits["nvidia.com/gpu"] != null)] | group_by(.metadata.namespace) | map({namespace: .[0].metadata.namespace, gpu_pods: length})'

# 检查 MLOps 流水线
kubectl get workflows -n ai-platform --sort-by=.metadata.creationTimestamp | tail -10
```

### 模型版本管理

```bash
# 🟢 低风险：模型版本操作
# 查看已注册模型
kubectl get models -n ai-serving -o custom-columns=NAME:.metadata.name,VERSION:.spec.version,STATUS:.status.state

# 查看推理服务流量分配
kubectl get inferenceservice llm-7b-inference -n ai-serving -o jsonpath='{.status.traffic}'

# 回滚模型版本
# 🟡 中风险：回滚会切换推理服务流量
kubectl patch inferenceservice llm-7b-inference -n ai-serving \
  --type merge -p '{"spec":{"predictor":{"containers":[{"name":"vllm","image":"registry.example.com/ai/vllm-openai:v0.5.0"}]}}}'
```

### GPU 资源调度监控

```bash
# 🟢 低风险：GPU 调度监控
# 查看 GPU 队列等待情况（Volcano）
kubectl get queues -A
kubectl get podgroups -A -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,GPU:.spec.minResources.nvidia\\.com/gpu

# 查看 GPU 节点拓扑
kubectl get nodes -l nvidia.com/gpu.present=true -o custom-columns=\
NAME:.metadata.name,GPU-COUNT:.status.allocatable.nvidia\\.com/gpu,GPU-TYPE:.metadata.labels.nvidia\\.com/gpu\\.product

# 检查训练任务 Gang Scheduling 状态
kubectl get podgroups -n team-nlp -o yaml | grep -A5 "status:"
```

## 故障排查

### 训练任务失败

```bash
# 🟢 低风险：训练任务诊断
# 检查 PyTorchJob 状态
kubectl describe pytorchjob llm-finetune-7b -n team-nlp

# 查看训练日志
kubectl logs -n team-nlp -l job-role=master -c pytorch --tail=100
kubectl logs -n team-nlp -l job-role=worker -c pytorch --tail=100

# 常见错误：NCCL 通信超时
# 错误：NCCL WARN Cuda failure: peer access is not supported between these two devices
# 检查 GPU 拓扑
kubectl exec -n team-nlp llm-finetune-7b-master-0 -- nvidia-smi topo -m

# 常见错误：OOM
# 检查显存使用
kubectl exec -n team-nlp llm-finetune-7b-master-0 -- nvidia-smi

# 常见错误：数据加载慢
# 检查存储 IOPS
kubectl exec -n team-nlp llm-finetune-7b-master-0 -- iostat -x 1 5
```

### 推理服务异常

```bash
# 🟢 低风险：推理服务诊断
# 检查 InferenceService 状态
kubectl describe inferenceservice llm-7b-inference -n ai-serving

# 检查推理 Pod 日志
kubectl logs -n ai-serving -l serving.kserve.io/inferenceservice=llm-7b-inference --tail=50

# 测试推理延迟
curl -s -w "\n%{time_total}s\n" http://llm-7b-inference.ai-serving/v1/completions \
  -H "Content-Type: application/json" \
  -d '{"model":"llama-7b","prompt":"Hello","max_tokens":50}'

# 检查模型加载状态
kubectl exec -n ai-serving -it deploy/llm-7b-inference -- curl -s localhost:8000/health
```

## 最佳实践

### 平台设计原则

1. **Golden Path 优先**：为 80% 的常见场景提供一键式体验，20% 的高级场景提供 escape hatch
2. **GPU 资源池化**：训练和推理共享 GPU 节点池，通过优先级和抢占实现资源复用
3. **模型即代码**：模型版本、配置、评估指标全部 Git 管理，支持审计和回滚
4. **成本透明**：每个实验/训练/推理的 GPU 费用实时可见，接入 [[平台工程/治理/09-cost-optimization-finops|FinOps]] 体系
5. **安全默认**：Notebook 环境默认无网络访问（除白名单），训练容器只读挂载数据集

### 开发者体验

- 统一 CLI：`ai-platform train submit`、`ai-platform model deploy`、`ai-platform notebook start`
- 集成到 [[平台工程/构建/03-backstage-deployment|Backstage]] 开发者门户
- 训练任务提交后自动通知（Slack/飞书），完成后自动注册模型
- 推理服务部署后自动生成 API 文档和监控看板

### 与现有平台集成

- GPU 调度参考 [[综合/gpu-scheduling-cost|GPU 调度与成本优化]]
- 多租户隔离参考 [[平台工程/治理/17-multi-tenant-management|多租户管理]]
- 存储方案参考 [[存储/分布式存储|分布式存储]]
- 监控告警参考 [[可观测性/prometheus|Prometheus 监控]]

## Related

- [[平台工程/构建/01-platform-engineering-overview|平台工程概述]]
- [[平台工程/治理/18-gpu-cluster-governance-ai-platform|GPU 集群治理]]
- [[综合/gpu-scheduling-cost|GPU 调度与成本优化]]
- [[综合/training-inference-data-lifecycle|训练推理数据生命周期]]
- [[AI基础设施/K8s-AI基础设施|K8s AI 基础设施]]
- [[平台工程/构建/03-backstage-deployment|Backstage 部署]]
