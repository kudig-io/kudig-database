---
title: AI Infrastructure on Kubernetes Domain Guide
description: AI Infrastructure on Kubernetes Domain Guide — Kubernetes 生产运维知识库
summary: AI Infrastructure on Kubernetes Domain Guide — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- ai
- ml
- gpu
- llm
- AI基础设施
- reference
- prometheus
- job
- gateway
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI Infrastructure on Kubernetes Domain Guide 是什么
- 如何 AI Infrastructure on Kubernetes Domain Guide
trigger_keywords:
- AI
- Infrastructure
- 'on'
- Kubernetes
- Domain
- Guide
prerequisites:
- kubectl-basics
- prometheus-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# AI Infrastructure on Kubernetes Domain Guide

## Source

Distilled from domain-11-ai-infra (37 documents, Kubernetes v1.28-v1.32).

## GPU Management

- **Device Plugin**: Exposes GPUs as schedulable resources (`nvidia.com/gpu`)
- **GPU sharing**: Time-slicing (multiple workloads share one GPU) or MIG (Multi-Instance GPU for A100+)
- **Scheduling**: Extended resources with node selectors for GPU type
- **Monitoring**: DCGM (Data Center GPU Manager) for GPU metrics (utilization, memory, temperature, power)

## Distributed Training

- **Frameworks**: PyTorch distributed, TensorFlow, Horovod, Megatron-LM
- **Communication**: NCCL for GPU-to-GPU, RDMA for cross-node
- **Operators**: Kubeflow Training Operator, Volcano for batch scheduling

## LLM Inference Serving

- **Serving frameworks**: vLLM, TensorRT-LLM, TGI (Text Generation Inference)
- **Optimization**: Quantization (INT8/FP8), speculative decoding, PagedAttention
- **Serving patterns**: API gateway with routing, model parallelism, multi-model serving

## MLOps Pipeline

| Stage | Tools |
|-------|-------|
| Data pipeline | Kubeflow Pipelines, Argo Workflows |
| Training | Kubeflow Training Operator, Ray |
| Experiment tracking | MLflow, Weights & Biases |
| Model registry | MLflow Model Registry |
| Deployment | KServe, Seldon Core |
| Monitoring | Prometheus + custom metrics, LLM observability tools |

## Cost Optimization

- Spot instances for fault-tolerant training jobs
- GPU autoscaling with Karpenter
- Model quantization to reduce inference GPU requirements
- Cost monitoring with Kubecost

## 运维操作

```bash
# 🟢 检查 GPU 节点状态
kubectl get nodes -o custom-columns=NAME:.metadata.name,GPU:.status.allocatable.'nvidia\.com/gpu'
kubectl describe node <gpu-node> | grep -A5 "Allocatable" | grep nvidia

# 🟢 检查 GPU Operator 状态
kubectl get pods -n gpu-operator
kubectl get clusterpolicy  # NVIDIA ClusterPolicy

# 🟢 查看训练任务
kubectl get pytorchjob,mpijob -A
kubectl describe pytorchjob <name> -n <ns>
kubectl logs <job>-master-0 -f

# 🟢 检查推理服务
kubectl get inferenceservice -A
kubectl get pods -l serving.kserve.io/inferenceservice=<name>
curl -s http://<ingress>/v1/models

# 🟢 GPU 利用率监控
kubectl exec -it <gpu-pod> -- nvidia-smi
kubectl exec -it <gpu-pod> -- nvidia-smi dmon -s u -d 1  # 实时监控

# 🟡 删除失败训练任务
kubectl delete pytorchjob <name> -n <ns> --force --grace-period=0

# 🟢 检查 RDMA/InfiniBand
kubectl get sriovnetworknodestate -A
ibstat  # 在节点上检查 IB 状态
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| GPU Pod Pending | GPU 资源不足/节点污点 | `kubectl describe pod` | 扩容 GPU 节点/检查 toleration |
| NCCL 通信超时 | RDMA/网络配置错误 | 查看训练日志 NCCL 部分 | 检查 SR-IOV/IB 配置 |
| CUDA OOM | 显存不足 | `nvidia-smi`; Pod 日志 | 减小 batch/启用 gradient checkpoint |
| 训练速度异常慢 | IO 瓶颈/GPU 降频 | `iostat -x 1`; `nvidia-smi -q -d CLOCK` | 使用缓存存储/检查散热 |
| 推理 503 | 模型加载失败 | `kubectl logs <isvc-pod>` | 检查模型路径/显存配置 |
| GPU 不可见 | Device Plugin 异常 | `kubectl get pods -n gpu-operator` | 重启 Device Plugin Pod |

### 排查流程

```
AI 工作负载异常
├── GPU 不可用？
│   ├── nvidia-smi 在节点上工作？
│   ├── Device Plugin Pod 运行？
│   └── 节点标签/污点正确？
├── 训练失败？
│   ├── NCCL 错误 → 检查节点间网络/RDMA
│   ├── OOM → 调整 batch size/gradient accumulation
│   └── 数据读取超时 → 检查存储后端
└── 推理异常？
    ├── 503 → 检查模型加载日志
    ├── 延迟高 → 检查 GPU 利用率/并发
    └── 自动伸缩不触发 → 检查 KPA 配置
```

## 生产案例

### 案例1：GPU 利用率低（数据 IO 瓶颈）

- **场景**：32 卡 A100 训练 LLaMA-13B，GPU 利用率仅 40%
- **排查**：`nvidia-smi dmon` 显示 GPU 频繁等待数据；NFS 读取延迟 200ms+
- **方案**：部署 JuiceFS 替代 NFS + 100GB SSD 缓存；数据预处理为 WebDataset 格式
- **效果**：GPU 利用率 92%，训练时间缩短 55%

### 案例2：推理服务显存溢出

- **场景**：vLLM 处理长文本时 OOM，Pod CrashLoop
- **排查**：KV Cache 分配失败；max-model-len=8192 占用过多显存
- **方案**：`--max-model-len=4096` + `--gpu-memory-utilization=0.85` + readinessProbe
- **效果**：服务稳定，HPA 水平扩展处理峰值

## 检查清单

- [ ] GPU Operator 已安装且所有节点 GPU 可识别
- [ ] Training Operator CRD 已注册
- [ ] 分布式存储已配置且 PVC Bound
- [ ] RDMA/InfiniBand 已验证（多节点训练）
- [ ] 训练任务配置了资源限制和容错
- [ ] 推理服务配置了健康检查和自动伸缩
- [ ] GPU 监控 (DCGM Exporter) 已部署
- [ ] 检查点存储已配置

## Related

- [[reference|#reference Hub]] — tag hub

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows
- [[概念/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[概念/scheduling-algorithm.md|scheduling-algorithm]] — Scheduling Algorithm
- [[概念/autoscaling-strategies.md|autoscaling-strategies]] — Autoscaling Strategies
- [[概念/autoscaling-strategies.md|Autoscaling Strategies]]
- [[概念/scheduling-algorithm.md|Scheduling Algorithm]]
- [[概念/resource-management.md|Resource Management]]


<!-- risk-assessed -->
