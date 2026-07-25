---
title: AI 语料库配置：RAG 分块策略、场景化 Profile 与向量库构建
description: '# AI 语料库配置'
summary: '# AI 语料库配置'
category: reference
tags:
- k8s
- rag
- chunking
- vector-database
- profile
- corpus
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI 语料库配置：RAG 分块策略、场景化 Profile 与向量库构建 是什么
- 如何 AI 语料库配置：RAG 分块策略、场景化 Profile 与向量库构建
trigger_keywords:
- AI
- 语料库配置：RAG
- 分块策略
- 场景化
- Profile
- 与向量库构建
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# AI 语料库配置

> **CNCF 状态**: 参考文档 | **类别**: AI Infrastructure | **主要语言**: YAML, Python

## 概述

Kubernetes AI 语料库配置是一份涵盖在 K8s 上部署和管理大规模 AI/ML 训练语料的技术配置参考文档。它整合了 GPU 调度、分布式存储、数据流水线、训练框架部署等多个维度的配置最佳实践。该文档覆盖了从数据准备（数据清洗、格式化、分布式存储）、到模型训练（PyTorch DDP、DeepSpeed、Megatron）、再到推理服务（vLLM、TGI、TensorRT-LLM）的全链路 K8s 配置。

## Key Features（核心能力）

- **GPU 调度配置**：NVIDIA GPU Operator、MIG 切分、GPU 共享的 K8s 配置
- **分布式训练**：PyTorchJob、MPIJob CRD 和 DeepSpeed 配置
- **数据流水线**：使用 Ray Data、Apache Arrow 进行分布式数据处理
- **存储配置**：利用 Alluxio、JuiceFS 加速训练数据读取
- **推理服务**：KServe + vLLM/TGI 的大模型推理部署
- **可观测性**：GPU 利用率监控、训练指标收集

## 架构与工作原理

AI 语料库配置分为三层：基础设施层（GPU Operator 管理 NVIDIA 驱动和设备插件；分布式存储提供训练数据访问）；训练层（通过 Volcano/Kubeflow Training Operator 管理分布式训练任务；Ray 集群处理数据流水线）；推理层（KServe 部署模型推理服务，GPU Autoscaler 根据请求量弹性扩缩）。每层都有对应的 K8s CRD 和配置模板。

## K8s 集成

GPU 通过 NVIDIA GPU Operator 以 Device Plugin 方式暴露给 K8s。训练任务通过 PyTorchJob/MPIJob CRD 定义，由 Training Operator 调度到 GPU 节点。RDMA/InfiniBand 通过 SR-IOV Network Device Plugin 配置。训练数据通过 CSI 驱动（如 JuiceFS、Lustre）挂载。推理服务通过 KServe InferenceService CRD 定义，配合 GPU Autoscaler 自动伸缩。

## 生产用例

- **大语言模型训练**：LLM 预训练和微调的 K8s 集群配置
- **GPU 集群管理**：大规模 GPU 集群的调度和利用率优化
- **模型推理服务**：大模型的在线推理部署和弹性伸缩
- **MLOps 流水线**：从数据处理到模型部署的端到端自动化

## 安装与配置

### GPU Operator 部署

```bash
# 🟢 安装 NVIDIA GPU Operator
helm repo add nvidia https://nvidia.github.io/gpu-operator
helm repo update
helm install gpu-operator nvidia/gpu-operator \
  -n gpu-operator --create-namespace \
  --set driver.version=550.90.07 \
  --set toolkit.enabled=true \
  --set devicePlugin.enabled=true \
  --set mig.strategy=mixed

# 🟢 验证 GPU 可用
kubectl get nodes -o custom-columns=NAME:.metadata.name,GPU:.status.allocatable.'nvidia\.com/gpu'
kubectl run gpu-test --rm -it --image=nvidia/cuda:12.4.0-base-ubuntu22.04 -- nvidia-smi
```

### Training Operator 部署

```bash
# 🟢 安装 Kubeflow Training Operator
kubectl apply -k "github.com/kubeflow/training-operator/manifests/overlays/standalone?ref=v1.8.0"

# 🟢 验证 CRD
kubectl get crd | grep kubeflow
```

### PyTorchJob 分布式训练配置

```yaml
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llm-finetune-7b
spec:
  nprocPerNode: "8"  # 每节点 GPU 数
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: registry.example.com/llm-trainer:v2.1
            command:
            - torchrun
            - --nproc_per_node=8
            - --nnodes=4
            - train.py
            - --model=llama-7b
            - --data-path=/data/corpus
            - --batch-size=4
            - --gradient-accumulation-steps=8
            resources:
              limits:
                nvidia.com/gpu: 8
                memory: "256Gi"
              requests:
                cpu: "32"
                memory: "128Gi"
            volumeMounts:
            - name: training-data
              mountPath: /data
            - name: shm
              mountPath: /dev/shm
          volumes:
          - name: training-data
            persistentVolumeClaim:
              claimName: corpus-pvc
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: "64Gi"
          nodeSelector:
            nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
          tolerations:
          - key: nvidia.com/gpu
            operator: Exists
            effect: NoSchedule
    Worker:
      replicas: 3
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: registry.example.com/llm-trainer:v2.1
            resources:
              limits:
                nvidia.com/gpu: 8
```

### KServe 推理服务配置

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llm-inference
spec:
  predictor:
    minReplicas: 1
    maxReplicas: 4
    containers:
    - name: kserve-container
      image: vllm/vllm-openai:latest
      args:
      - --model=/models/llama-7b-chat
      - --tensor-parallel-size=2
      - --max-model-len=4096
      - --gpu-memory-utilization=0.90
      resources:
        limits:
          nvidia.com/gpu: 2
          memory: "64Gi"
        requests:
          cpu: "8"
          memory: "32Gi"
    scaleTarget: 10  # 每实例并发数
    scaleMetric: concurrency
```

### 数据加速存储 (JuiceFS)

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: corpus-pvc
spec:
  accessModes: [ReadWriteMany]
  storageClassName: juicefs-sc
  resources:
    requests:
      storage: 10Ti
---
# JuiceFS StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: juicefs-sc
provisioner: csi.juicefs.com
parameters:
  csi.storage.k8s.io/provisioner-secret-name: juicefs-secret
  csi.storage.k8s.io/provisioner-secret-namespace: kube-system
mountOptions:
- cache-dir=/data/jfscache
- cache-size=102400  # 100GB 本地缓存
- prefetch=3
```

## 运维操作

```bash
# 🟢 检查 GPU 节点状态
kubectl get nodes -l nvidia.com/gpu.present=true
kubectl describe node <gpu-node> | grep -A5 "Allocatable" | grep nvidia

# 🟢 查看训练任务状态
kubectl get pytorchjob -A
kubectl describe pytorchjob <job-name>
kubectl logs <job-name>-master-0 -f

# 🟢 检查推理服务
kubectl get inferenceservice -A
kubectl get pods -l serving.kserve.io/inferenceservice=llm-inference
curl -s http://<ingress>/v1/models

# 🟡 删除失败训练任务
kubectl delete pytorchjob <job-name> --force --grace-period=0

# 🟢 GPU 利用率监控
kubectl top pods -n training --sort-by=cpu
dcgm-exporter metrics: kubectl port-forward svc/dcgm-exporter 9400:9400 -n gpu-operator
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| GPU Pod Pending | GPU 资源不足/节点污点 | `kubectl describe pod` | 扩容 GPU 节点/检查 toleration |
| NCCL 通信超时 | RDMA/网络配置错误 | `kubectl logs <pod>` 查看 NCCL 日志 | 检查 SR-IOV/InfiniBand 配置 |
| OOM Killed | 显存/内存不足 | `nvidia-smi`; `kubectl describe pod` | 减小 batch-size/启用 gradient checkpointing |
| 训练速度异常慢 | IO 瓶颈/GPU 降频 | `iostat -x 1`; `nvidia-smi -q -d CLOCK` | 使用 JuiceFS 缓存/检查散热 |
| 推理服务 503 | 模型加载失败/显存不足 | `kubectl logs <isvc-pod>` | 检查模型路径/调整 gpu-memory-utilization |

### 排查流程

```
AI 训练/推理异常
├── Pod 无法调度？
│   ├── 检查 GPU 资源: kubectl describe node | grep nvidia
│   ├── 检查污点: kubectl get node -o jsonpath='{.spec.taints}'
│   └── 检查 PVC: kubectl get pvc (Bound?)
├── 训练任务失败？
│   ├── NCCL 错误 → 检查节点间网络/RDMA
│   ├── CUDA OOM → 调整 batch/gradient accumulation
│   └── 数据读取超时 → 检查存储后端/缓存
└── 推理服务异常？
    ├── 503 → 检查模型加载日志
    ├── 延迟高 → 检查 GPU 利用率/并发数
    └── 自动伸缩不触发 → 检查 KPA 配置
```

## 生产案例

### 案例1：大规模 LLM 训练 GPU 利用率低

- **场景**：32 卡 A100 训练 LLaMA-13B，GPU 利用率仅 40%
- **排查**：`nvidia-smi dmon` 显示 GPU 频繁等待数据；`iostat` 显示 NFS 读取延迟 200ms+
- **方案**：部署 JuiceFS 替代 NFS，启用 100GB 本地 SSD 缓存 + 预取；数据预处理改为 WebDataset 格式
- **效果**：GPU 利用率提升至 92%，训练时间缩短 55%

### 案例2：推理服务显存溢出导致反复重启

- **场景**：vLLM 推理服务处理长文本时 OOM，Pod 反复 CrashLoop
- **排查**：日志显示 KV Cache 分配失败；默认 max-model-len=8192 占用过多显存
- **方案**：设置 `--max-model-len=4096` + `--gpu-memory-utilization=0.85` + 添加 readinessProbe 检测显存
- **效果**：服务稳定运行，通过 HPA 水平扩展处理峰值流量

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| K8s + GPU Operator | 灵活、开源、资源利用率高 | 运维复杂度高 | 自建 GPU 集群 |
| AWS SageMaker | 全托管、开箱即用 | 成本高、厂商锁定 | 快速上线/小团队 |
| Slurm + K8s混合 | HPC 传统生态兼容 | 调度器冲突、维护复杂 | 已有 Slurm 环境 |
| Ray on K8s (KubeRay) | 分布式计算原生、弹性好 | 生态较新、GPU调度较弱 | 数据处理/强化学习 |

## 检查清单

- [ ] GPU Operator 已安装且所有节点 GPU 可识别
- [ ] Training Operator CRD 已注册
- [ ] 分布式存储已配置且 PVC Bound
- [ ] RDMA/InfiniBand 网络已验证（多节点训练）
- [ ] 训练任务配置了资源限制和容错策略
- [ ] 推理服务配置了健康检查和自动伸缩
- [ ] GPU 监控（DCGM Exporter）已部署
- [ ] 训练数据备份和检查点存储已配置

## Related

- [[23-实体/15-参考与索引/kudig-rag-chunking-strategy.md|kudig-rag-chunking-strategy]] — RAG 分块策略指南与 Manpage 安装指南
- [[23-实体/15-参考与索引/k8s-ai-agent-engineering.md|k8s-ai-agent-engineering]] — AI Agent 工程：RAG、多 Agent 编排、安全护栏与生产部署


<!-- risk-assessed -->
