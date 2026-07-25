---
title: AI 基础设施：GPU 调度、分布式训练、LLM 推理与成本优化
description: '## GPU 调度'
summary: '关键 K8s 资源：PyTorchJob、TFJob、MPIJob（KubeFlow Operator）。'
category: reference
tags:
- k8s
- ai-infra
- gpu
- distributed-training
- llm
- cost-optimization
- job
- operator
- nvidia
- kubeflow
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI 基础设施：GPU 调度、分布式训练、LLM 推理与成本优化 是什么
- 如何 AI 基础设施：GPU 调度、分布式训练、LLM 推理与成本优化
trigger_keywords:
- AI
- 基础设施：GPU
- 调度
- 分布式训练
- LLM
- 推理与成本优化
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# AI 基础设施

## 概述

Kubernetes 已成为 AI/ML 基础设施的标准编排平台。本页概述在 K8s 上运行 AI 工作负载的四大核心领域：**GPU 资源调度与管理**、**分布式训练编排**、**LLM 推理服务部署**和**成本优化**。这些能力依赖于 KubeFlow Training Operator、NVIDIA GPU Operator、Volcano/Kueue 批调度器、KServe 推理框架等 CNCF 和生态系统项目。

## GPU 调度

K8s GPU 资源管理方式：

- **Device Plugin**：NVIDIA Device Plugin 暴露 `nvidia.com/gpu` 资源，kubelet 自动分配
- **MIG（Multi-Instance GPU）**：A100/H100 GPU 物理分区为 7 个独立实例，每个实例隔离运行
- **MPS（Multi-Process Service）**：多个进程共享同一 GPU 的计算资源，提升利用率
- **GPU Operator**：NVIDIA GPU Operator 自动化管理 GPU 驱动安装、容器运行时配置、DCGM 监控
- **vGPU/GPU 共享**：通过时间分片或内存分区实现 GPU 共享（如 HAMI、GPU Manager）

```yaml
# 请求 GPU 的 Pod 示例
spec:
  containers:
  - name: training
    image: pytorch/pytorch:latest
    resources:
      limits:
        nvidia.com/gpu: 1
```

## 分布式训练

| 框架 | 特点 | 适用场景 |
|------|------|----------|
| PyTorch DDP | 数据并行 | 中小模型 |
| DeepSpeed | ZeRO 优化 | 大模型训练 |
| Megatron-LM | 模型并行/Pipeline 并行 | 超大模型 |
| KubeFlow Training | K8s 原生 | 通用 |

关键 K8s 资源：`PyTorchJob`、`TFJob`、`MPIJob`（通过 Kubeflow Training Operator）。这些 CRD 管理分布式训练的 Worker/PodLauncher 生命周期，自动处理节点亲和性、GPU 分配和故障恢复。配合 Kueue 或 Volcano 实现训练任务的队列化调度和公平资源分配。

## LLM 推理优化

- **vLLM**：PagedAttention 技术，连续批处理（Continuous Batching），显存利用率高，吞吐量大
- **TensorRT-LLM**：NVIDIA 优化推理引擎，支持 INT8/FP8 量化，延迟最优
- **Triton Inference Server**：多模型多框架服务，支持动态 batching
- **量化**：GPTQ/AWQ/GGUF 降低显存需求（FP16→INT4 可减少 75% 显存）
- **推理框架**：KServe（标准推理服务）、Text Generation Inference (TGI)

## 成本优化

- **Spot/抢占式实例 + 检查点恢复**：训练任务使用 Spot 实例降低 70% 成本，配合定期 Checkpoint 实现容错
- **资源请求精确化**：根据实际利用率调整 GPU 请求量，避免过度请求
- **模型蒸馏/量化**：减少推理计算需求，INT4 量化可使吞吐提升 2-3 倍
- **推理自动缩容**：基于请求量自动扩缩，空闲时缩容到零（配合 KServe）
- **GPU 共享**：多个低 QPS 推理服务共享同一 GPU（通过 MPS 或时间分片）

## 生产部署要点

- **GPU 监控**：通过 DCGM Exporter 暴露 GPU 利用率、显存、温度指标到 Prometheus
- **训练容错**：定期 Checkpoint 到对象存储（S3/OSS），节点故障时从最近 Checkpoint 恢复
- **数据本地化**：使用节点本地存储（如 HwameiStor）缓存训练数据，减少网络 I/O
- **队列管理**：使用 Kueue 管理训练任务队列，避免资源争抢
- **弹性训练**：结合 Volcano 弹性训练能力，节点扩缩容时自动调整 Worker 数量

## 安装与配置

```bash
# 安装 NVIDIA GPU Operator
helm repo add nvidia https://helm.ngc.nvidia.com/nvidia
helm install gpu-operator nvidia/gpu-operator \
  --namespace gpu-operator --create-namespace \
  --set driver.enabled=true \
  --set toolkit.enabled=true \
  --set dcgmExporter.enabled=true

# 验证 GPU 节点
kubectl get nodes -o custom-columns=NAME:.metadata.name,GPU:.status.allocatable.'nvidia\.com/gpu'
kubectl describe node <gpu-node> | grep -A5 "Allocatable" | grep nvidia

# 安装 KubeFlow Training Operator
kubectl apply -k "github.com/kubeflow/training-operator/manifests/overlays/standalone?ref=v1.7.0"

# 安装 KServe
kubectl apply -f https://github.com/kserve/kserve/releases/download/v0.12.0/kserve.yaml

# 安装 Kueue (批调度)
kubectl apply -f https://github.com/kubernetes-sigs/kueue/releases/download/v0.6.0/manifests.yaml
```

```yaml
# PyTorchJob 分布式训练示例
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llm-finetune
  namespace: training
spec:
  nprocPerNode: "8"  # 每节点 GPU 数
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        spec:
          containers:
            - name: pytorch
              image: myorg/llm-trainer:v1.0
              resources:
                limits:
                  nvidia.com/gpu: 8
              volumeMounts:
                - name: checkpoint
                  mountPath: /checkpoints
                - name: data
                  mountPath: /data
          volumes:
            - name: checkpoint
              persistentVolumeClaim:
                claimName: training-checkpoint
            - name: data
              persistentVolumeClaim:
                claimName: training-data
    Worker:
      replicas: 3
      template:
        spec:
          containers:
            - name: pytorch
              image: myorg/llm-trainer:v1.0
              resources:
                limits:
                  nvidia.com/gpu: 8
---
# KServe InferenceService 示例
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llm-serve
  namespace: inference
spec:
  predictor:
    model:
      modelFormat:
        name: pytorch
      storageUri: "s3://models/llama-7b-int4"
      resources:
        limits:
          nvidia.com/gpu: 1
    minReplicas: 1
    maxReplicas: 10
    scaleTarget: 10  # 每实例并发数
    scaleMetric: concurrency
```

## 运维操作

```bash
# 🟢 检查 GPU 节点状态
kubectl get nodes -l nvidia.com/gpu.present=true
kubectl describe node <gpu-node> | grep -A10 "Allocated resources"

# 🟢 检查 GPU Operator 状态
kubectl get pods -n gpu-operator
kubectl get clusterpolicy  # NVIDIA ClusterPolicy

# 🟢 查看训练任务
kubectl get pytorchjob,mpijob,tfjob -A
kubectl describe pytorchjob <name> -n <ns>
kubectl logs <job>-master-0 -f

# 🟢 检查推理服务
kubectl get inferenceservice -A
kubectl get pods -n inference
kubectl logs <isvc-pod> -c kserve-container

# 🟢 GPU 监控指标
kubectl exec -n gpu-operator <dcgm-pod> -- dcgmi discovery -l
kubectl exec -n gpu-operator <dcgm-pod> -- dcgmi health -c -g 0

# 🟡 查看 Kueue 队列状态
kubectl get clusterqueue
kubectl get localqueue -A
kubectl get workload -A

# 🟢 检查 GPU 显存使用
kubectl exec <gpu-pod> -- nvidia-smi
kubectl exec <gpu-pod> -- nvidia-smi --query-gpu=memory.used,memory.total --format=csv
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| GPU 不可调度 | Device Plugin 未运行 | `kubectl get pods -n gpu-operator` | 重启 GPU Operator |
| Pod Pending (GPU) | GPU 资源不足 | `kubectl describe pod` | 扩容 GPU 节点/释放资源 |
| 训练任务失败 | NCCL 通信错误 | 检查 Job 日志 | 检查节点间网络/RDMA |
| 推理服务 OOM | 模型超出显存 | `nvidia-smi` | 使用量化/减小 batch size |
| GPU 温度过高 | 散热不足/负载过高 | DCGM 指标 | 降频/增加散热 |
| Checkpoint 失败 | 存储 I/O 不足 | 检查 PVC 状态 | 使用高性能存储 (NVMe) |

### 排查流程

```
AI 工作负载异常
├── GPU 资源问题
│   ├── nvidia-smi → 检查 GPU 状态
│   ├── kubectl describe node → 检查可分配 GPU
│   ├── kubectl get pods -n gpu-operator → 检查 Operator
│   └── dmesg | grep -i nvidia → 检查驱动错误
├── 训练任务失败
│   ├── kubectl logs <job-pod> → 查看错误日志
│   ├── 检查 NCCL 环境变量配置
│   ├── 检查节点间网络连通性 (RDMA/TCP)
│   └── 检查 Checkpoint 存储可访问性
└── 推理服务异常
    ├── kubectl get inferenceservice → 检查状态
    ├── kubectl logs → 查看模型加载错误
    ├── nvidia-smi → 检查显存使用
    └── 检查模型文件完整性
```

## 生产案例

### 案例 1: 大模型训练 GPU 利用率优化

- **场景**: 70B 参数模型训练，GPU 利用率仅 40%，训练时间过长
- **排查**: 数据加载成为瓶颈；GPU 等待数据；单节点 I/O 带宽不足
- **方案**: 使用 JuiceFS 分布式文件系统缓存训练数据；增加 DataLoader workers；使用 NVMe 本地盘做预取
- **效果**: GPU 利用率从 40% 提升至 85%；训练时间缩短 50%

### 案例 2: LLM 推理服务成本优化

- **场景**: 7B 模型推理服务 24/7 运行，但夜间 QPS 仅为白天的 10%
- **排查**: 固定 4 副本运行，夜间 GPU 利用率 <10%
- **方案**: KServe 自动缩容 (scale-to-zero)；INT4 量化减少显存；夜间缩容到 1 副本
- **效果**: GPU 成本降低 60%；P99 延迟增加 <50ms (可接受)

## 检查清单

- [ ] GPU Operator 所有组件 Running
- [ ] GPU 节点正确报告 nvidia.com/gpu 资源
- [ ] DCGM Exporter 指标已接入 Prometheus
- [ ] 训练任务 Checkpoint 存储已配置
- [ ] Kueue/Volcano 队列已配置
- [ ] 推理服务自动缩容已配置
- [ ] GPU 温度/显存告警已配置
- [ ] 节点间 RDMA/高速网络已验证
- [ ] 模型文件存储可访问 (S3/OSS/PVC)
- [ ] 成本监控 (Kubecost) 已部署

---

> 来源：.zread/wiki/drafts/17-ai-ji-chu-she-shi-*.md

## Related

- [[23-实体/15-参考与索引/k8s-ai-infra-domain-guide.md|k8s-ai-infra-domain-guide]] — AI Infrastructure on Kubernetes Domain Guide
- [[kubeflow]] — Kubeflow
- [[23-实体/15-参考与索引/k8s-ai-corpus-configuration.md|k8s-ai-corpus-configuration]] — AI 语料配置

<!-- risk-assessed -->
