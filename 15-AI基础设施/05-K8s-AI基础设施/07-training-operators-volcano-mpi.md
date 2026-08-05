---
title: "K8s 训练 Operator 与批量调度生产实践"
description: "Kubeflow Training Operator、Volcano 批量调度器与 MPI Operator 的生产部署、训练任务管理与故障排查"
summary: "覆盖 Kubeflow Training Operator（PyTorchJob/TFJob/MPIJob/XGBoostJob）、Volcano Gang Scheduling 与 Fair-share Queue、MPI 分布式训练、Checkpoint 断点续训、GPU 配额优先级管理及 NCCL 超时/Pod 挂起等故障排查"
category: AI基础设施
tags:
- training-operator
- volcano
- mpi-operator
- pytorchjob
- gang-scheduling
- distributed-training
- checkpoint
- gpu-quota
- nccl
- fair-share
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
- "PyTorchJob 如何在 K8s 上运行分布式训练"
- "Volcano Gang Scheduling 怎么配置"
- "训练任务 NCCL 超时如何排查"
trigger_keywords:
- training-operator
- volcano
- pytorchjob
- mpijob
- gang-scheduling
- checkpoint
- nccl
- distributed-training
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

# K8s 训练 Operator 与批量调度生产实践

## 概述

大规模分布式训练是 AI 基础设施中资源消耗最大、运维复杂度最高的工作负载类型。一个典型的百亿参数模型训练任务可能需要 64-512 块 GPU 协同工作数周，任何单点故障都可能导致整个任务中断。Kubernetes 原生的调度器面向无状态微服务设计，无法满足训练任务"所有 Pod 必须同时就绪"（Gang Scheduling）、"按队列公平分配资源"（Fair-share）、"任务级生命周期管理"等需求。

本文覆盖三大核心组件：Kubeflow Training Operator 提供训练任务的 CRD 抽象和生命周期管理；Volcano 提供 Gang Scheduling、Queue 管理和 Fair-share 调度；MPI Operator 提供基于 MPI 协议的分布式训练编排。关于分布式训练框架的对比，参见 [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks]]；GPU 调度基础参见 [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads]]。

## 架构与核心概念

### 训练平台整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     训练平台架构                                  │
├─────────────────────────────────────────────────────────────────┤
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  调度层：Volcano Scheduler                                  │  │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐ │  │
│  │  │ Gang        │ │ Queue       │ │ Fair-share /        │ │  │
│  │  │ Scheduling  │ │ Management  │ │ Priority            │ │  │
│  │  └─────────────┘ └─────────────┘ └─────────────────────┘ │  │
│  └───────────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  Operator 层：Training Operator + MPI Operator             │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────────┐ │  │
│  │  │PyTorchJob│ │ TFJob    │ │ MPIJob   │ │ XGBoostJob  │ │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └─────────────┘ │  │
│  └───────────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  执行层：Training Pods                                      │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐        │  │
│  │  │Master/  │ │Worker-0 │ │Worker-1 │ │Worker-N │        │  │
│  │  │Rank-0   │ │(GPU x8) │ │(GPU x8) │ │(GPU x8) │        │  │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘        │  │
│  │       ◄──────── NCCL / MPI 通信 ────────►                 │  │
│  └───────────────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │  存储层：Checkpoint / 数据集                                │  │
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────────┐  │  │
│  │  │ 共享存储     │ │ 对象存储     │ │ 高速缓存         │  │  │
│  │  │ (NFS/CephFS)│ │ (S3/MinIO)  │ │ (Alluxio/JuiceFS)│  │  │
│  │  └──────────────┘ └──────────────┘ └──────────────────┘  │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件对比

| 组件 | 职责 | 核心能力 | 适用场景 |
|------|------|---------|---------|
| Training Operator | 训练任务 CRD 管理 | PyTorchJob/TFJob 生命周期、重启策略 | 所有分布式训练 |
| Volcano | 批量调度器 | Gang Scheduling、Queue、Preemption | 多租户 GPU 集群 |
| MPI Operator | MPI 任务编排 | mpirun 启动、Hostfile 管理 | Horovod/DeepSpeed MPI 模式 |
| Kueue | 作业队列管理 | 资源配额、准入控制、优先级 | 多团队资源共享 |

### Gang Scheduling 原理

Gang Scheduling（也称 All-or-Nothing Scheduling）确保一个训练任务的所有 Pod 要么全部调度成功，要么全部不调度。这避免了"部分 Pod 占用 GPU 但其他 Pod 无法调度"导致的资源死锁。Volcano 通过 PodGroup 和 MinAvailable 机制实现 Gang Scheduling。参见 [[22-概念/07-调度与资源/gang-scheduling]] 了解详细原理。

## 生产部署

### Training Operator 安装

🟡 中风险：安装集群级 Operator 和 CRD。

```bash
# 安装 Kubeflow Training Operator
kubectl apply --server-side -k "github.com/kubeflow/training-operator/manifests/overlays/standalone?ref=v1.8.1"

# 验证安装
kubectl get pods -n kubeflow -l control-plane=kubeflow-training-operator
kubectl get crd | grep kubeflow.org
# 应看到：pytorchjobs, tfjobs, mxjobs, xgboostjobs, mpijobs

# 安装 MPI Operator（独立组件）
kubectl apply -f https://raw.githubusercontent.com/kubeflow/mpi-operator/v0.4.0/deploy/v2beta1/mpi-operator.yaml
```

### Volcano 安装与配置

🔴 高风险：安装替代调度器，配置不当可能影响集群所有工作负载的调度。

```bash
# 安装 Volcano（Helm 方式）
helm repo add volcano-sh https://volcano-sh.github.io/helm-charts
helm repo update

helm install volcano volcano-sh/volcano \
  --namespace volcano-system --create-namespace \
  --set scheduler.number=2 \
  --set scheduler.resources.requests.cpu=2 \
  --set scheduler.resources.requests.memory=4Gi \
  --set controller.resources.requests.cpu=1 \
  --set controller.resources.requests.memory=2Gi \
  --set admission.resources.requests.cpu=500m \
  --set admission.resources.requests.memory=1Gi \
  --version 1.9.0

# 验证安装
kubectl get pods -n volcano-system
kubectl get crd | grep volcano.sh
# 应看到：jobs.batch.volcano.sh, queues.scheduling.volcano.sh, podgroups.scheduling.volcano.sh
```

### Volcano Queue 配置（多租户资源管理）

🟡 中风险：创建资源队列，影响资源分配策略。

```yaml
# 生产环境 Queue 配置
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: team-llm-training
spec:
  weight: 3  # 权重越高，空闲资源分配越多
  capability:
    nvidia.com/gpu: 128  # 最大可用 GPU 上限
    cpu: "512"
    memory: "4096Gi"
  reclaimable: true  # 允许被高优先级队列回收
  guarantee:
    resource:
      nvidia.com/gpu: 32  # 保底 GPU 数量
---
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: team-inference
spec:
  weight: 2
  capability:
    nvidia.com/gpu: 64
  reclaimable: true
  guarantee:
    resource:
      nvidia.com/gpu: 16
---
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: team-research
spec:
  weight: 1
  capability:
    nvidia.com/gpu: 32
  reclaimable: true
  guarantee:
    resource:
      nvidia.com/gpu: 8
```

### PyTorchJob 分布式训练（生产配置）

🟡 中风险：提交大规模训练任务，消耗大量 GPU 资源。

```yaml
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llama3-finetune-64gpu
  namespace: ai-training
  labels:
    team: llm
    project: llama3-sft
spec:
  nprocPerNode: "8"  # 每节点 8 GPU
  elasticPolicy:
    rdzvBackend: c10d
    minReplicas: 4
    maxReplicas: 8
    maxRestarts: 3
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        metadata:
          labels:
            volcano.sh/queue-name: team-llm-training
          annotations:
            scheduling.k8s.io/group-name: llama3-finetune-64gpu
        spec:
          schedulerName: volcano  # 使用 Volcano 调度器
          containers:
          - name: pytorch
            image: registry.internal/ai/pytorch-training:2.4.0-cuda12.4
            command:
            - torchrun
            - --nproc_per_node=8
            - --nnodes=8
            - --node_rank=$(RANK)
            - --master_addr=$(MASTER_ADDR)
            - --master_port=$(MASTER_PORT)
            - /app/train.py
            - --model=llama-3-70b
            - --data-path=/data/sft-dataset/
            - --output-dir=/checkpoints/llama3-sft/
            - --micro-batch-size=2
            - --global-batch-size=128
            - --gradient-accumulation-steps=8
            - --max-steps=10000
            - --save-interval=500
            - --resume-from-checkpoint=auto
            resources:
              limits:
                nvidia.com/gpu: 8
                memory: "256Gi"
              requests:
                nvidia.com/gpu: 8
                memory: "128Gi"
                cpu: "32"
            env:
            - name: NCCL_DEBUG
              value: "WARN"
            - name: NCCL_IB_DISABLE
              value: "0"
            - name: NCCL_SOCKET_IFNAME
              value: "eth0"
            - name: NCCL_TIMEOUT
              value: "1800"
            - name: TORCH_NCCL_ASYNC_ERROR_HANDLING
              value: "1"
            volumeMounts:
            - name: checkpoint-storage
              mountPath: /checkpoints
            - name: data-storage
              mountPath: /data
            - name: shm
              mountPath: /dev/shm
          volumes:
          - name: checkpoint-storage
            persistentVolumeClaim:
              claimName: checkpoint-pvc-llama3
          - name: data-storage
            persistentVolumeClaim:
              claimName: dataset-pvc-sft
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: "32Gi"
          tolerations:
          - key: nvidia.com/gpu
            operator: Exists
            effect: NoSchedule
    Worker:
      replicas: 7
      restartPolicy: OnFailure
      template:
        metadata:
          labels:
            volcano.sh/queue-name: team-llm-training
          annotations:
            scheduling.k8s.io/group-name: llama3-finetune-64gpu
        spec:
          schedulerName: volcano
          containers:
          - name: pytorch
            image: registry.internal/ai/pytorch-training:2.4.0-cuda12.4
            command:
            - torchrun
            - --nproc_per_node=8
            - --nnodes=8
            - --node_rank=$(RANK)
            - --master_addr=$(MASTER_ADDR)
            - --master_port=$(MASTER_PORT)
            - /app/train.py
            - --model=llama-3-70b
            - --data-path=/data/sft-dataset/
            - --output-dir=/checkpoints/llama3-sft/
            - --micro-batch-size=2
            - --global-batch-size=128
            - --gradient-accumulation-steps=8
            - --max-steps=10000
            - --save-interval=500
            - --resume-from-checkpoint=auto
            resources:
              limits:
                nvidia.com/gpu: 8
                memory: "256Gi"
              requests:
                nvidia.com/gpu: 8
                memory: "128Gi"
                cpu: "32"
            env:
            - name: NCCL_DEBUG
              value: "WARN"
            - name: NCCL_IB_DISABLE
              value: "0"
            - name: NCCL_SOCKET_IFNAME
              value: "eth0"
            - name: NCCL_TIMEOUT
              value: "1800"
            - name: TORCH_NCCL_ASYNC_ERROR_HANDLING
              value: "1"
            volumeMounts:
            - name: checkpoint-storage
              mountPath: /checkpoints
            - name: data-storage
              mountPath: /data
            - name: shm
              mountPath: /dev/shm
          volumes:
          - name: checkpoint-storage
            persistentVolumeClaim:
              claimName: checkpoint-pvc-llama3
          - name: data-storage
            persistentVolumeClaim:
              claimName: dataset-pvc-sft
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: "32Gi"
          tolerations:
          - key: nvidia.com/gpu
            operator: Exists
            effect: NoSchedule
```

### MPIJob 分布式训练

```yaml
apiVersion: kubeflow.org/v2beta1
kind: MPIJob
metadata:
  name: deepspeed-mpi-training
  namespace: ai-training
spec:
  slotsPerWorker: 8  # 每 Worker 8 GPU
  runPolicy:
    cleanPodPolicy: Running
    backoffLimit: 3
    activeDeadlineSeconds: 604800  # 7 天超时
  mpiReplicaSpecs:
    Launcher:
      replicas: 1
      template:
        spec:
          schedulerName: volcano
          containers:
          - name: mpi-launcher
            image: registry.internal/ai/deepspeed-training:0.14.4
            command:
            - mpirun
            - --allow-run-as-root
            - -np
            - "32"
            - --bind-to
            - none
            - --map-by
            - slot
            - -x
            - NCCL_DEBUG=WARN
            - -x
            - NCCL_SOCKET_IFNAME=eth0
            - -x
            - LD_LIBRARY_PATH
            - python
            - /app/train_deepspeed.py
            - --deepspeed_config=/app/ds_config.json
            resources:
              limits:
                cpu: "4"
                memory: "8Gi"
    Worker:
      replicas: 4
      template:
        metadata:
          labels:
            volcano.sh/queue-name: team-llm-training
        spec:
          schedulerName: volcano
          containers:
          - name: mpi-worker
            image: registry.internal/ai/deepspeed-training:0.14.4
            resources:
              limits:
                nvidia.com/gpu: 8
                memory: "256Gi"
              requests:
                nvidia.com/gpu: 8
                memory: "128Gi"
            volumeMounts:
            - name: shm
              mountPath: /dev/shm
            - name: checkpoint
              mountPath: /checkpoints
          volumes:
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: "32Gi"
          - name: checkpoint
            persistentVolumeClaim:
              claimName: checkpoint-pvc-deepspeed
```

## 运维操作

### 训练任务生命周期管理

🟢 低风险/只读。

```bash
# 查看训练任务状态
kubectl get pytorchjob -n ai-training
# NAME                    STATE     AGE
# llama3-finetune-64gpu   Running   2d

# 查看任务详细状态
kubectl get pytorchjob llama3-finetune-64gpu -n ai-training -o yaml | grep -A 20 "status:"

# 查看训练日志（Master/Rank-0）
kubectl logs -n ai-training -l training.kubeflow.org/job-name=llama3-finetune-64gpu,training.kubeflow.org/replica-type=master --tail=50 -f

# 查看 Volcano Job 状态
kubectl get vcjob -n ai-training
kubectl get podgroup -n ai-training

# 查看 Queue 资源使用
kubectl get queue -o yaml | grep -A 10 "status:"
```

### Checkpoint 与断点续训

🟢 低风险/只读（查看 Checkpoint）。

```bash
# 查看 Checkpoint 文件
kubectl exec -it <master-pod> -n ai-training -- ls -la /checkpoints/llama3-sft/
# checkpoint-500/
# checkpoint-1000/
# checkpoint-1500/
# latest -> checkpoint-1500

# 验证 Checkpoint 完整性
kubectl exec -it <master-pod> -n ai-training -- \
  python -c "import torch; ckpt=torch.load('/checkpoints/llama3-sft/latest/pytorch_model.bin', map_location='cpu'); print(ckpt.keys())"
```

🟡 中风险（手动触发断点续训）。

```bash
# 断点续训：Training Operator 的 restartPolicy: OnFailure 会自动从最新 Checkpoint 恢复
# 手动重启训练任务（从最新 Checkpoint 继续）
kubectl delete pytorchjob llama3-finetune-64gpu -n ai-training
# 重新 apply 相同的 PyTorchJob YAML（--resume-from-checkpoint=auto 会自动找到最新 ckpt）
kubectl apply -f llama3-finetune-job.yaml
```

### GPU 资源配额与优先级

🟡 中风险：修改资源配额和优先级配置。

```yaml
# Volcano PodGroup 优先级配置
apiVersion: scheduling.volcano.sh/v1beta1
kind: PodGroup
metadata:
  name: llama3-finetune-64gpu
  namespace: ai-training
spec:
  minMember: 8  # Gang Scheduling: 至少 8 个 Pod 同时就绪
  minResources:
    nvidia.com/gpu: 64
    cpu: "256"
    memory: "1024Gi"
  priorityClassName: high-priority
  queue: team-llm-training
---
# PriorityClass 定义
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: training-critical
value: 1000000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "关键训练任务，可抢占低优先级任务"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: training-normal
value: 100000
globalDefault: false
description: "普通训练任务"
```

```bash
# 🟢 查看当前优先级和抢占状态
kubectl get priorityclass
kubectl get events -n ai-training --field-selector reason=Preempted
```

## 故障排查

### Pod 挂起（Pending）

```bash
# 🟢 Step 1: 检查 Pod 事件
kubectl describe pod <pending-pod> -n ai-training | grep -A 15 "Events"

# 🟢 Step 2: 检查 PodGroup 状态（Gang Scheduling 是否满足）
kubectl get podgroup -n ai-training -o yaml | grep -A 10 "status"
# 如果 phase: Inqueue 或 Pending → 资源不满足 minMember

# 🟢 Step 3: 检查 Queue 配额
kubectl get queue team-llm-training -o yaml
# 检查 allocated 是否已达 capability 上限

# 常见原因：
# 1. GPU 资源不足 → 等待其他任务完成或扩容节点
# 2. Gang Scheduling 未满足 → 部分 Pod 调度成功但总数不够 minMember
# 3. Queue 配额用尽 → 联系管理员调整 Queue capability
# 4. PVC 未绑定 → 检查 StorageClass 和 PV 可用性
# 5. 节点 Taint/Toleration 不匹配 → 检查 GPU 节点 Taint

# 🟡 临时方案：降低 minMember（允许部分调度，但可能导致训练失败）
kubectl patch podgroup <name> -n ai-training --type='merge' \
  -p '{"spec":{"minMember":4}}'
```

### NCCL 超时

```bash
# 🟢 Step 1: 确认 NCCL 超时错误
kubectl logs <pod> -n ai-training | grep -i "nccl\|timeout\|watchdog"
# 典型错误：
# "NCCL WARN Watchdog caught collective operation timeout"
# "RuntimeError: NCCL communicator was aborted on rank X"

# 🟢 Step 2: 检查网络连通性
kubectl exec -it <pod> -n ai-training -- \
  python -c "import torch; print(torch.cuda.nccl.version())"

# 🟢 Step 3: 检查 GPU 间通信拓扑
kubectl exec -it <pod> -n ai-training -- nvidia-smi topo -m

# 🟢 Step 4: 检查 InfiniBand/RoCE 网络状态
kubectl exec -it <pod> -n ai-training -- ibstat
kubectl exec -it <pod> -n ai-training -- ibping -S  # Server 端
kubectl exec -it <pod> -n ai-training -- ibping -L <remote-lid>  # Client 端

# 常见原因及修复：
# 1. 网络接口配置错误 → 设置 NCCL_SOCKET_IFNAME 为正确网卡
# 2. InfiniBand 链路故障 → 检查 ibstat 端口状态
# 3. 防火墙阻断 NCCL 端口 → 开放 Pod 间高端口范围
# 4. GPU 硬件故障（XID 错误）→ 检查 dmesg 和 DCGM 告警
# 5. 单节点慢拖慢整体 → 检查 GPU 利用率和 PCIe 带宽

# 🟡 增大 NCCL 超时时间（临时缓解）
# env: NCCL_TIMEOUT=3600 (秒)
# env: TORCH_NCCL_HEARTBEAT_TIMEOUT_SEC=3600
```

### Checkpoint 损坏

```bash
# 🟢 检查 Checkpoint 文件完整性
kubectl exec -it <pod> -n ai-training -- \
  ls -la /checkpoints/llama3-sft/checkpoint-1500/
# 正常应包含：pytorch_model.bin, optimizer.pt, scheduler.pt, trainer_state.json

# 🟢 尝试加载验证
kubectl exec -it <pod> -n ai-training -- \
  python -c "
import torch
try:
    ckpt = torch.load('/checkpoints/llama3-sft/checkpoint-1500/pytorch_model.bin', map_location='cpu')
    print('Checkpoint OK, keys:', list(ckpt.keys())[:5])
except Exception as e:
    print(f'Checkpoint CORRUPTED: {e}')
"

# 修复方案：
# 1. 回退到上一个有效 Checkpoint
#    修改 --resume-from-checkpoint=/checkpoints/llama3-sft/checkpoint-1000
# 2. 如果所有 Checkpoint 损坏 → 从头开始训练
# 3. 预防措施：使用异步 Checkpoint（torch.distributed.checkpoint）
#    避免写入过程中断导致文件不完整
```

### 训练速度异常下降

```bash
# 🟢 检查 GPU 利用率
kubectl exec -it <pod> -n ai-training -- nvidia-smi dmon -s u -d 5

# 🟢 检查是否有 GPU 降频（热节流）
kubectl exec -it <pod> -n ai-training -- nvidia-smi -q -d CLOCK

# 🟢 检查存储 I/O（数据加载瓶颈）
kubectl exec -it <pod> -n ai-training -- iostat -x 5 3

# 🟢 检查 NCCL 通信时间占比
# 在训练日志中查看 step time 分解：
# forward: Xms, backward: Yms, allreduce: Zms, data_loading: Wms

# 常见原因：
# 1. 数据加载瓶颈 → 增加 DataLoader num_workers，使用高速缓存（JuiceFS/Alluxio）
# 2. GPU 热节流 → 检查机房温度和散热
# 3. 网络带宽下降 → 检查 IB 链路错误计数
# 4. Checkpoint 保存阻塞 → 使用异步保存
```

## 最佳实践

1. **Gang Scheduling 必须启用**：分布式训练绝不允许部分调度。设置 `schedulerName: volcano` 并配置 PodGroup 的 `minMember` 等于总 Pod 数。避免资源死锁。

2. **Checkpoint 策略**：
   - 保存频率：每 500-1000 步保存一次（平衡恢复粒度和存储开销）
   - 保留策略：保留最近 3-5 个 Checkpoint（`--save-total-limit=5`）
   - 异步保存：使用 `torch.distributed.checkpoint` 避免阻塞训练
   - 存储后端：使用高吞吐共享存储（CephFS/Lustre）或对象存储 + 本地缓存

3. **NCCL 调优**：
   - 生产环境设置 `NCCL_DEBUG=WARN`（非 INFO，避免日志爆炸）
   - 多机训练必须配置 `NCCL_SOCKET_IFNAME` 指定正确网卡
   - 启用 `TORCH_NCCL_ASYNC_ERROR_HANDLING=1` 实现快速故障检测
   - 设置合理的 `NCCL_TIMEOUT`（建议 1800s，避免网络抖动误判）

4. **Queue 与 Fair-share**：
   - 按团队/项目划分 Queue，设置 `guarantee`（保底）和 `capability`（上限）
   - 启用 `reclaimable: true` 允许空闲资源被其他 Queue 借用
   - 关键任务使用高 PriorityClass，允许抢占低优先级任务

5. **共享内存配置**：分布式训练的 NCCL 通信和 DataLoader 多进程都依赖 `/dev/shm`。必须挂载 `emptyDir.medium: Memory`，大小建议为节点内存的 25-50%。

6. **网络隔离**：训练任务的 NCCL 通信应使用独立网络平面（RDMA/RoCE），与 K8s 管理网络和存储网络分离，避免流量竞争。参见 [[05-网络/01-K8s网络核心/index.md|01-K8s网络核心]] 中的网络架构设计。

7. **弹性训练**：使用 PyTorch Elastic（torchrun + c10d rendezvous）实现训练过程中的节点动态加入/退出，提高大规模训练的容错能力。

8. **资源规划**：大规模训练前使用 [[12-可靠性/03-容量规划/index.md|03-容量规划]] 的方法论评估所需 GPU 数量、训练时长和存储容量。

## Related

- [[22-概念/07-调度与资源/gang-scheduling]]
- [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads]]
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks]]
- [[05-网络/01-K8s网络核心/index.md|01-K8s网络核心]]
- [[12-可靠性/03-容量规划/index.md|03-容量规划]]
