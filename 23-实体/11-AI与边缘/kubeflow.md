---
title: Kubeflow [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- ai-ml
- kubeflow
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubeflow 是什么
- 如何 Kubeflow
trigger_keywords:
- Kubeflow
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kubeflow

> **CNCF 状态**: Incubating | **类别**: AI/ML | **主要语言**: Python, Go

## 概述

Kubeflow 是一个 CNCF 孵化项目，最初由 Google 基于 TensorFlow 扩展内部 ML 基础设施开源，现由 Kubeflow 社区联合多家企业（Google、Cisco、IBM、AWS、Microsoft 等）共同维护。它是 Kubernetes 原生的机器学习平台，提供从数据准备、模型训练、超参数调优到模型服务的端到端 MLOps 能力。Kubeflow 将 ML 工作流的各个环节映射为 K8s 资源，让数据科学家能够在云原生环境中使用熟悉的工具（Jupyter、PyTorch、TensorFlow）进行 ML 开发。

## Key Features（核心能力）

- **Jupyter Notebook 服务**：在 K8s 上提供多用户 Jupyter Notebook 即服务
- **分布式训练**：支持 TensorFlow Training (TFJob) 和 PyTorch Training (PyTorchJob) CRD
- **Katib 超参数调优**：自动化的超参数搜索和神经架构搜索
- **KServe 模型服务**：将训练好的模型部署为可弹性伸缩的推理服务
- **Pipelines**：基于 Argo Workflow 的 ML 流水线编排，支持 DAG 依赖
- **多租户支持**：通过 Dex/Istio 实现用户认证和资源隔离

## 架构与工作原理

Kubeflow 由多个松耦合的组件构成：Notebook Controller 管理 Jupyter Notebook Pod；Training Operator 管理分布式训练任务（TFJob、PyTorchJob、MPIJob）；Katib Controller 管理超参数调优实验；KFP（Kubeflow Pipelines）基于 Argo Workflow 实现 ML 流水线；KServe 提供模型推理服务。所有组件通过 Istio Service Mesh 互联，通过 Dex/OIDC 提供认证授权。

## K8s 集成

Kubeflow 完全基于 Kubernetes 原生能力构建：训练任务通过 CRD 定义，由自定义 Controller 调度；通过 Volcano 或 K8s 默认调度器进行 GPU 资源调度；使用 PVC 管理训练数据；通过 Istio Service Mesh 管理组件间通信。安装通常通过 Kubeflow Manifests 或 Operator 进行，依赖 K8s 1.25+。

## 生产用例

- **分布式模型训练**：在 GPU 集群上运行大规模 TensorFlow/PyTorch 分布式训练
- **超参数调优**：使用 Katib 自动搜索最优模型参数
- **ML CI/CD 流水线**：通过 Kubeflow Pipelines 自动化数据准备到模型部署全流程
- **Jupyter 即服务**：为数据科学团队提供按需的 Notebook 环境

## 安装与配置

```bash
# 🟢 使用 Kubeflow manifests 安装
VERSION=v1.9.0
git clone --branch ${VERSION} https://github.com/kubeflow/manifests.git
cd manifests
while ! kustomize build example | kubectl apply -f -; do echo "Retrying..."; sleep 10; done

# 🟢 验证安装
kubectl get pods -n kubeflow
kubectl get crd | grep kubeflow.org

# 🟢 访问 Kubeflow Dashboard
kubectl port-forward svc/istio-ingressgateway 8080:80 -n istio-system
# 浏览器访问 http://localhost:8080

# 🟢 单独安装 Training Operator
kubectl apply -k "github.com/kubeflow/training-operator/manifests/overlays/standalone?ref=v1.7.0"

# 🟢 单独安装 KServe
kubectl apply -f https://github.com/kserve/kserve/releases/download/v0.12.0/kserve.yaml
```

### PyTorchJob 示例

```yaml
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: pytorch-dist-training
  namespace: kubeflow-user
spec:
  nprocPerNode: "4"  # 每节点 GPU 数
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
            command: ["torchrun", "--nproc_per_node=4", "train.py"]
            resources:
              limits:
                nvidia.com/gpu: 4
                memory: 64Gi
            volumeMounts:
            - name: training-data
              mountPath: /data
          volumes:
          - name: training-data
            persistentVolumeClaim:
              claimName: training-data-pvc
    Worker:
      replicas: 3
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
            command: ["torchrun", "--nproc_per_node=4", "train.py"]
            resources:
              limits:
                nvidia.com/gpu: 4
                memory: 64Gi
```

### KServe InferenceService 示例

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: sklearn-model
  namespace: kubeflow-user
spec:
  predictor:
    model:
      modelFormat:
        name: sklearn
      storageUri: "gs://models-bucket/sklearn/mnist"
      resources:
        requests:
          cpu: 1
          memory: 2Gi
        limits:
          cpu: 2
          memory: 4Gi
    minReplicas: 1
    maxReplicas: 10
    scaleTarget: 10  # 每实例并发数
```

## 运维操作

### 常用命令

```bash
# 🟢 查看训练任务
kubectl get pytorchjob,tfjob,mpijob -A
kubectl describe pytorchjob pytorch-dist-training -n kubeflow-user

# 🟢 查看训练日志
kubectl logs -n kubeflow-user pytorch-dist-training-master-0
kubectl logs -n kubeflow-user pytorch-dist-training-worker-0

# 🟢 查看 Notebook
kubectl get notebooks -A
kubectl describe notebook my-notebook -n kubeflow-user

# 🟢 查看推理服务
kubectl get inferenceservice -A
kubectl describe inferenceservice sklearn-model -n kubeflow-user

# 🟢 查看 Pipeline
kubectl get workflow -n kubeflow-user

# 🟡 删除训练任务
kubectl delete pytorchjob pytorch-dist-training -n kubeflow-user

# 🟢 查看组件日志
kubectl logs -n kubeflow -l app=training-operator --tail=50
kubectl logs -n kubeflow -l app=kubeflow-pipelines-api-server --tail=50
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| PyTorchJob Pending | GPU 资源不足 | `kubectl describe pytorchjob <name>` | 检查节点 GPU 可用量 |
| 训练任务失败 | OOM/应用错误 | `kubectl logs <pod> --previous` | 调整内存限制/修复代码 |
| Notebook 无法启动 | PVC 未绑定/资源不足 | `kubectl describe notebook <name>` | 检查 StorageClass 和资源 |
| 推理服务 503 | 模型加载失败 | `kubectl logs <predictor-pod>` | 检查模型路径和格式 |
| Pipeline 失败 | 步骤依赖错误 | `kubectl get workflow -o yaml` | 检查 DAG 依赖和参数 |
| Dashboard 无法访问 | Istio 配置问题 | `kubectl get svc -n istio-system` | 检查 Ingress Gateway |

### 排查流程

```
1. kubectl get pods -n kubeflow → 确认平台组件状态
2. kubectl get pytorchjob/notebook/inferenceservice → 确认资源状态
3. kubectl describe <resource> → 查看 Events
4. kubectl logs <pod> → 查看应用日志
5. 检查 GPU 节点资源: kubectl describe node | grep gpu
```

## 生产案例

### 案例1: 大规模分布式训练平台
- **场景**: AI 团队需要 128 GPU 分布式训练，支持多团队共享
- **方案**: Kubeflow + Volcano Gang Scheduling + 多租户 Namespace 隔离
- **效果**: GPU 利用率从 40% 提升至 75%，训练任务排队时间降低 60%

### 案例2: ML 流水线自动化
- **场景**: 模型从训练到上线需要 2周手动流程
- **方案**: Kubeflow Pipelines 编排数据准备→训练→评估→部署全流程
- **效果**: 模型上线周期从 2周缩短至 2小时

## 对比替代方案

| 维度 | Kubeflow | MLflow | SageMaker | Azure ML |
|------|----------|--------|-----------|----------|
| 部署方式 | 自托管 K8s | 自托管 | 云托管 | 云托管 |
| 分布式训练 | 原生支持 | 有限 | 支持 | 支持 |
| 超参调优 | Katib | 有限 | 支持 | 支持 |
| 模型服务 | KServe | MLflow Serving | 内置 | 内置 |
| 流水线 | KFP (Argo) | MLflow Projects | Step Functions | Designer |
| 成本 | 免费 (基础设施费) | 免费 | 付费 | 付费 |
| 学习曲线 | 陡峭 | 平缓 | 中等 | 中等 |

## 检查清单

- [ ] GPU 节点已安装 NVIDIA Driver + Device Plugin
- [ ] Istio Service Mesh 已部署 (组件间通信)
- [ ] PVC/StorageClass 已配置 (训练数据存储)
- [ ] 多租户认证已配置 (Dex/OIDC)
- [ ] Training Operator 和 KServe 已部署
- [ ] 监控 GPU 利用率和任务状态
- [ ] 定期清理完成的训练任务释放资源

## Related

- [[kubean]] — Kubean
- [[tikv]] — TiKV
- [[k8gb]] — K8GB
- [[lima]] — Lima
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 99-kubeflow-ai-platform-guide
- troubleshooting.md|02-kubeflow-troubleshooting]]
- kubeflow
- [[23-实体/kaito.md|[[23-实体/11-AI与边缘/kaito|KAITO]]]]
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
