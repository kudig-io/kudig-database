---
title: "GPU 工作负载管理"
description: "K8s GPU 工作负载管理：GPU Pod 配置、多 GPU 分配、健康检查、节点亲和性、GPU 资源配额与调度"
summary: "面向 AI 平台工程师与 SRE 的 GPU 工作负载完整管理指南，覆盖 NVIDIA Device Plugin、MIG、调度策略、健康检查、配额与故障排查。"
category: 工作负载
tags:
- gpu
- nvidia
- kubernetes
- ai
- ml
- scheduling
- mig
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
- "Kubernetes 如何配置 GPU Pod"
- "多 GPU 任务如何分配与调度"
- "GPU 健康检查与故障节点如何处理"
trigger_keywords:
- gpu
- nvidia
- cuda
- mig
- device plugin
- gpu scheduling
prerequisites:
- kubectl-basics
- pod-lifecycle
- node-affinity
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

# GPU 工作负载管理

> **适用版本**: Kubernetes v1.28+ / NVIDIA GPU Operator 24.x
> **最后更新**: 2026-07

---

## 概述

随着人工智能和机器学习工作负载的爆发式增长，GPU 已经成为 Kubernetes 集群中最昂贵、最稀缺的计算资源。一张 NVIDIA A100 或 H100 的采购价格动辄数十万元，加上配套的供电、散热和高速互联成本，GPU 集群的总体拥有成本极为惊人。在这样的背景下，GPU 利用率每提升 10 个百分点都意味着巨大的成本节约，而每一次因配置错误导致的 GPU 闲置都是真金白银的浪费。

然而，GPU 的管理远比 CPU 复杂得多。GPU 硬件的故障率显著高于 CPU，一张卡在高负载下运行数月后出现 Xid 错误并不罕见；GPU 驱动版本与 CUDA 版本、容器运行时版本之间存在严格的兼容性矩阵，版本漂移会导致难以排查的故障；GPU 显存不可超卖，一个 Pod 申请了整卡显存就意味着其他 Pod 无法共享；GPU 调度还有拓扑约束，多卡训练任务需要 GPU 之间通过 NVLink 或 PCIe 高速互联。

本文系统覆盖 GPU 工作负载的完整生命周期：从 Device Plugin 部署、Pod GPU 申请、多 GPU 与 MIG 切分，到健康检查、节点亲和性、资源配额与故障自愈。AI 推理服务的容器配置可以结合 [[工作负载/多语言运行时/02-python-on-kubernetes-production.md|Python 应用 Kubernetes 生产实践]] 一起阅读。

---

## 核心概念

### 1. GPU 资源模型

Kubernetes 通过 Extended Resource（扩展资源）机制来管理 GPU，NVIDIA GPU 的资源名为 nvidia.com/gpu。与 CPU 和内存这两种原生资源不同，GPU 扩展资源有几个关键特性需要牢记。

首先，GPU 是整数资源，不能请求 0.5 个 GPU。你只能申请 1、2、4 等整数个 GPU，这与 CPU 可以请求 500m（半核）形成鲜明对比。MIG（Multi-Instance GPU）技术是唯一的例外，它可以将一张物理 GPU 切分为多个硬件隔离的切片，每个切片作为独立的扩展资源。

其次，GPU 不可超卖。在 Kubernetes 中，GPU 资源的 limits 必须等于 requests，这意味着节点上的 GPU 总数就是可分配的上限，不存在像 CPU 那样的超卖空间。

最后，GPU 资源不能跨节点聚合。一个 Pod 申请的所有 GPU 必须位于同一个物理节点上，无法将多个节点的 GPU 组合分配给一个 Pod。这对大规模分布式训练任务的调度提出了特殊要求。

### 2. NVIDIA 软件栈

在 Kubernetes 上运行 GPU 工作负载需要一整套软件栈协同工作，理解每个组件的职责是排查问题的基础。

| 组件 | 作用 | 部署方式 |
|------|------|---------|
| GPU Driver | 内核驱动 | DaemonSet（GPU Operator） |
| nvidia-container-toolkit | 容器运行时集成 | 节点安装 |
| NVIDIA Device Plugin | 向 kubelet 注册 GPU 资源 | DaemonSet |
| GPU Operator | 一键部署整套软件栈 | Operator |
| DCGM Exporter | GPU 指标采集 | DaemonSet |
| MIG Manager | 多实例 GPU 切分管理 | DaemonSet |

GPU Operator 是 NVIDIA 官方提供的 Operator，它能在 Kubernetes 集群中自动部署和管理上述所有组件，包括驱动的安装和升级。对于大多数生产环境，我们强烈推荐使用 GPU Operator 而非手动安装各组件，因为它能确保版本兼容性并简化升级流程。

### 3. GPU 共享方案对比

GPU 共享是提升利用率的关键手段，不同方案在隔离性、粒度和适用场景上各有取舍。

| 方案 | 粒度 | 隔离性 | 适用场景 | 复杂度 |
|------|------|--------|---------|--------|
| 整卡独占 | 1 GPU | 强 | 训练、大模型推理 | 低 |
| MIG（A100/H100） | 1/7 卡 | 硬件级 | 多租户推理 | 中 |
| Time-slicing | 时间片 | 弱（共享显存） | 开发测试、小推理 | 低 |
| MPS | 进程级共享 | 中 | 多进程共享 | 中 |
| vGPU（厂商） | 虚拟化 | 强 | VDI、虚拟化 | 高 |

整卡独占是最简单也是性能最好的方案，适用于大模型训练和推理。MIG 是 A100 和 H100 独有的硬件级切分技术，能将一张卡切分为最多 7 个完全隔离的实例，每个实例有独立的显存、缓存和计算单元，适合多租户推理场景。Time-slicing 通过时间片轮转让多个 Pod 共享一张卡，但显存是共享的，没有硬件隔离，仅适合开发测试。

---

## 生产部署/实现

### 1. 部署 NVIDIA GPU Operator 🔴

GPU Operator 的部署涉及在节点上安装内核驱动，这是一个高风险操作，驱动版本不匹配或安装失败可能导致节点不可用。

```bash
# 🔴 高风险：在节点安装内核驱动，错误可能导致节点不可用
helm repo add nvidia https://helm.ngc.nvidia.com/nvidia
helm repo update

helm install gpu-operator nvidia/gpu-operator \
  --namespace gpu-operator --create-namespace \
  --set driver.version=550.90.07 \
  --set toolkit.enabled=true \
  --set devicePlugin.enabled=true \
  --set dcgmExporter.enabled=true \
  --set migManager.enabled=true \
  --wait
```

验证 GPU 节点就绪：

```bash
# 🟢 低风险：只读
kubectl get nodes -l nvidia.com/gpu.present=true
kubectl describe node <gpu-node> | grep -A5 "Allocatable" | grep nvidia.com/gpu
```

部署完成后，每个 GPU 节点的 Allocatable 中应该出现 nvidia.com/gpu 资源，其数值等于该节点的物理 GPU 数量。如果资源未出现，通常是驱动安装失败或 Device Plugin 未正常运行，需要检查 gpu-operator 命名空间下各 DaemonSet 的日志。

### 2. 单 GPU 推理 Pod 🟡

```yaml
# 🟡 中风险：GPU 资源申请影响调度
apiVersion: v1
kind: Pod
metadata:
  name: inference-gpu
  namespace: ai-serving
spec:
  restartPolicy: Always
  nodeSelector:
    nvidia.com/gpu.product: NVIDIA-A100-SXM4-80GB
  tolerations:
  - key: nvidia.com/gpu
    operator: Exists
    effect: NoSchedule
  containers:
  - name: inference
    image: registry.example.com/vllm:v0.4.2
    resources:
      limits:
        nvidia.com/gpu: "1"     # requests 自动等于 limits
        memory: "64Gi"
        cpu: "8"
    env:
    - name: NVIDIA_VISIBLE_DEVICES
      value: "all"
    - name: NVIDIA_DRIVER_CAPABILITIES
      value: "compute,utility"
    volumeMounts:
    - name: shm
      mountPath: /dev/shm
  volumes:
  - name: shm
    emptyDir:
      medium: Memory
      sizeLimit: 16Gi          # PyTorch 多进程需要大 shared memory
```

这个配置有几个容易被忽视的关键点。nodeSelector 通过 GPU 型号标签确保 Pod 调度到正确型号的 GPU 节点，因为不同型号的 GPU 在显存大小和计算能力上差异巨大。tolerations 用于容忍 GPU 节点上的污点，确保只有 GPU 工作负载才会调度到这些昂贵的节点上。最重要的是 /dev/shm 的挂载：PyTorch 的 DataLoader 多进程模式使用共享内存进行进程间通信，默认的 64MB /dev/shm 完全不够用，必须通过 emptyDir Memory 类型挂载一个足够大的共享内存，否则会出现 "bus error" 或训练崩溃。

### 3. 多 GPU 分布式训练 🔴

```yaml
# 🔴 高风险：多 GPU 训练任务，资源占用大，配置错误导致 NCCL 通信失败
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llm-finetune
  namespace: ai-training
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: registry.example.com/train:v1.0
            resources:
              limits:
                nvidia.com/gpu: "8"      # 单机 8 卡
                memory: "512Gi"
            env:
            - name: NCCL_DEBUG
              value: "INFO"
            - name: NCCL_IB_DISABLE
              value: "0"
            volumeMounts:
            - name: shm
              mountPath: /dev/shm
            - name: dataset
              mountPath: /data
          volumes:
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: 64Gi
          - name: dataset
            persistentVolumeClaim:
              claimName: training-data-pvc
```

多 GPU 分布式训练是 GPU 管理中最复杂的场景。单机 8 卡训练通过 NCCL（NVIDIA Collective Communications Library）实现 GPU 间的高速通信，NCCL_DEBUG=INFO 环境变量会在日志中输出详细的通信拓扑信息，是排查通信故障的必备手段。/dev/shm 需要设置得更大（64Gi），因为多进程数据加载和 NCCL 通信都会使用共享内存。对于多机训练，还需要确保节点间有高速网络互联（InfiniBand 或 RoCE），并正确配置 NCCL_IB_DISABLE 等网络相关参数。

### 4. MIG 切分配置 🟡

MIG 允许将一张 A100 或 H100 切分为多个硬件隔离的实例，极大提升多租户推理场景的 GPU 利用率。

```yaml
# 🟡 中风险：MIG 配置改变 GPU 切分方式，需重启 device plugin
apiVersion: v1
kind: ConfigMap
metadata:
  name: mig-parted-config
  namespace: gpu-operator
data:
  config.yaml: |
    version: v1
    mig-configs:
      all-1g.10gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            "1g.10gb": 7      # 切成 7 个 1g.10gb 实例
      mixed:
        - devices: all
          mig-enabled: true
          mig-devices:
            "3g.40gb": 1
            "2g.20gb": 1
            "1g.10gb": 2
```

申请 MIG 实例：

```yaml
resources:
  limits:
    nvidia.com/mig-1g.10gb: "1"   # 申请一个 MIG 切片
```

MIG 配置中的 "1g.10gb" 表示 1 个 GPU 计算切片配 10GB 显存。一张 A100 80GB 最多可以切分为 7 个 1g.10gb 实例，或者采用混合配置如 1 个 3g.40gb 加 1 个 2g.20gb 加 2 个 1g.10gb。MIG 实例之间是硬件级隔离的，一个实例的故障不会影响其他实例，这使其成为多租户推理的理想选择。需要注意的是，修改 MIG 配置需要重启 Device Plugin，且会中断该节点上所有使用 GPU 的 Pod。

---

## 运维操作

### 1. GPU 资源盘点 🟢

```bash
# 🟢 低风险：只读
# 查看集群 GPU 总量与分配
kubectl get nodes -l nvidia.com/gpu.present=true \
  -o custom-columns=NAME:.metadata.name,GPU:.status.allocatable.nvidia\\.com/gpu

# 查看 GPU 利用率（DCGM）
kubectl -n gpu-operator port-forward ds/nvidia-dcgm-exporter 9400:9400
curl -s http://localhost:9400/metrics | grep DCGM_FI_DEV_GPU_UTIL

# 节点上 nvidia-smi
kubectl -n gpu-operator exec -it ds/nvidia-device-plugin-daemonset -- nvidia-smi
```

GPU 资源盘点是容量规划的基础。通过 DCGM Exporter 暴露的指标，可以实时监控每张 GPU 的利用率、显存使用、温度、功耗等关键指标。在我们的实践中，很多团队的 GPU 平均利用率只有 30-40%，这意味着巨大的优化空间——通过 MIG 切分、time-slicing 或更智能的调度策略，可以将利用率提升到 70% 以上。

### 2. GPU 配额管理 🟡

```yaml
# 🟡 中风险：配额限制 namespace GPU 使用
apiVersion: v1
kind: ResourceQuota
metadata:
  name: gpu-quota
  namespace: ai-training
spec:
  hard:
    requests.nvidia.com/gpu: "16"
    limits.nvidia.com/gpu: "16"
```

GPU 配额管理是多团队共享集群的必备手段。没有配额限制，一个团队可能独占所有 GPU 资源，导致其他团队的任务无法运行。通过 ResourceQuota 为每个命名空间设置 GPU 上限，结合优先级和抢占机制，可以实现公平的资源分配。

### 3. 节点亲和性与污点 🟡

```yaml
# 🟡 中风险：调度约束配置
affinity:
  nodeAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
      nodeSelectorTerms:
      - matchExpressions:
        - key: nvidia.com/gpu.product
          operator: In
          values:
          - NVIDIA-H100-80GB-PCIe
        - key: nvidia.com/gpu.memory
          operator: Gt
          values: ["40000"]
```

GPU 工作负载通常需要精确的节点亲和性配置。不同型号的 GPU 在计算能力和显存大小上差异巨大，一个需要 80GB 显存的大模型推理任务如果被调度到只有 40GB 显存的 GPU 节点上，必然会因显存不足而失败。通过 nodeAffinity 按 GPU 型号和显存大小进行精确调度，可以避免这类问题。

---

## 故障排查

### 症状 1：Pod Pending，提示 Insufficient nvidia.com/gpu

```bash
# 🟢 低风险
kubectl describe pod <pod> | grep -A5 Events
kubectl describe nodes | grep -A5 "Allocated resources"
```

这是 GPU 调度最常见的失败原因。根因可能是 GPU 资源确实不足，所有 GPU 都已被其他 Pod 占用；也可能是节点有污点而 Pod 没有配置对应的 tolerations；或者是 nodeSelector/nodeAffinity 条件过于严格，没有符合条件的节点。处置方法包括扩容 GPU 节点、检查并添加 tolerations、放宽调度约束，或者释放被低优先级任务占用的 GPU。

### 症状 2：CUDA error / Xid 错误

```bash
# 🟢 低风险
kubectl -n gpu-operator logs ds/nvidia-dcgm-exporter | grep -i xid
# 节点 dmesg 查 Xid
```

Xid 错误是 NVIDIA GPU 的硬件或驱动级错误代码。不同的 Xid 编号代表不同的故障类型，其中 Xid 79（GPU has fallen off the bus）是最严重的，表示 GPU 已经从 PCIe 总线上脱落，通常是硬件故障的征兆。处置方法是立即隔离故障节点（cordon 加 drain），将上面的工作负载迁移到其他节点，然后报修硬件。对于频繁出现 Xid 错误的节点，应该从集群中永久移除。

### 症状 3：多卡训练 NCCL 超时

NCCL 超时是分布式训练中最令人头疼的问题之一。根因可能是 /dev/shm 太小导致共享内存不足、NVLink 或 InfiniBand 拓扑配置问题、GPU 间 P2P（Peer-to-Peer）通信不通，或者某个 GPU 出现硬件故障拖慢了整个集合通信。处置方法是增大 shm 配置，设置 NCCL_DEBUG=INFO 分析通信拓扑，检查 NCCL_P2P_LEVEL 和 NCCL_IB_DISABLE 等参数，并排查是否有 GPU 出现 Xid 错误。

### 症状 4：GPU 显存 OOM

显存 OOM（Out of Memory）是模型推理和训练中的常见问题。根因是 batch size 过大、模型未量化导致显存占用过高，或者显存碎片化。处置方法包括减小 batch size、启用梯度累积以在显存受限时模拟大 batch、使用混合精度训练（fp16 或 bf16）减半显存占用，以及对推理模型进行 int8 或 int4 量化。

### 排查决策树

```
GPU 异常
├── Pending?        → 资源不足/污点 → 扩容/容忍
├── Xid 错误?       → 硬件故障 → 隔离节点
├── NCCL 超时?      → shm/拓扑 → 调参
├── 显存 OOM?       → batch/量化 → 优化
└── 利用率为 0?     → 数据加载瓶颈 → 查 CPU/IO
```

---

## 最佳实践

第一，使用 GPU Operator 统一管理驱动与插件，避免手动安装导致的版本漂移。第二，GPU 节点打污点 nvidia.com/gpu:NoSchedule，确保只有 AI 负载才会调度到这些昂贵节点。第三，PyTorch 多进程任务必须挂载足够大的 /dev/shm，这是最常被忽视的配置。第四，部署 DCGM Exporter 配合 Node Problem Detector，自动检测 Xid 故障并隔离问题节点。第五，按命名空间设置 GPU ResourceQuota，防止单团队独占资源。第六，小推理任务用 MIG 或 time-slicing 提升利用率，大训练任务用整卡保证性能。第七，用 nodeAffinity 按 GPU 型号和显存精确调度，分布式训练用 gang-scheduling 保证所有 Pod 同时启动。第八，持续监控 DCGM_FI_DEV_GPU_UTIL 指标，对长期低利用率的任务进行降级或下线处理。

```yaml
# 🟢 低风险：DCGM 告警规则示例（PrometheusRule）
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: gpu-alerts
  namespace: monitoring
spec:
  groups:
  - name: gpu
    rules:
    - alert: GPUHighTemperature
      expr: DCGM_FI_DEV_GPU_TEMP > 85
      for: 5m
      labels:
        severity: warning
    - alert: GPUXidError
      expr: DCGM_FI_DEV_XID_ERRORS > 0
      labels:
        severity: critical
```

---

## Related

- [[工作负载/多语言运行时/02-python-on-kubernetes-production.md|Python 应用 Kubernetes 生产实践]]
- [[工作负载/多语言运行时/05-multicluster-workload-distribution.md|多集群工作负载分发]]
- [[工作负载/99-kubernetes-deployment-patterns-architecture.md|Kubernetes 部署模式架构]]
- [[可观测性]]
- [[可靠性/容量规划]]
- [[AI基础设施]]
