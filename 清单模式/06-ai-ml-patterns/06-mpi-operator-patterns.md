---
title: MPIJob 分布式训练模式
description: 使用 MPI Operator 进行 MPI 分布式训练（Horovod/DeepSpeed）
summary: MPIJob 配置实现 Horovod/DeepSpeed 分布式训练，包括 launcher-worker 架构与 NCCL 优化
category: manifests-patterns
tags:
- k8s
- manifests
- ai-ml-infra
- mpi
- mpijob
- horovod
- distributed-training
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
estimated_read_time: 12min
intent_queries:
- MPIJob 如何配置
- MPI Operator Kubernetes
- Horovod 分布式训练
trigger_keywords:
- mpijob
- mpi-operator
- horovod
- mpirun
- deepspeed
prerequisites:
- gpu-basics
- distributed-training-basics
authors:
- name: KUDIG Team
  role: contributor
---

# MPIJob 分布式训练模式

## 1. MPIJob 架构

```
┌────────────────────────────┐
│      Launcher Pod          │
│  (执行 mpirun / deepspeed) │
└──────────┬─────────────────┘
           │ SSH / k8s exec
    ┌──────┼──────┐
    ↓      ↓      ↓
 Worker0 Worker1 WorkerN
 (GPU)   (GPU)   (GPU)
```

Launcher 是唯一运行 `mpirun` 的 Pod，Worker Pods 通过 SSH 执行训练进程。

## 2. MPIJob CR（Horovod）

```yaml
apiVersion: kubeflow.org/v2beta1
kind: MPIJob
metadata:
  name: horovod-training
  namespace: ai-training
spec:
  slotsPerWorker: 8               # 每个 Worker 的 GPU 数
  runPolicy:
    cleanPodPolicy: Running       # 完成后保留 Running Pod
    backoffLimit: 3               # 失败重试次数
  sshAuthMountPath: /home/mpiuser/.ssh
  mpiReplicaSpecs:
    Launcher:
      replicas: 1
      template:
        spec:
          containers:
            - name: mpi-launcher
              image: registry.example.com/horovod-trainer:v1.0.0
              command:
                - mpirun
                - --allow-run-as-root
                - -np
                - "32"            # 总进程数 (4 workers × 8 GPU)
                - -bind-to
                - none
                - -map-by
                - slot
                - -x
                - NCCL_DEBUG=INFO
                - -x
                - NCCL_SOCKET_IFNAME=eth0
                - -x
                - NCCL_IB_HCA=mlx5
                - python
                - /train.py
                - --batch_size=32
                - --epochs=100
              resources:
                limits:
                  cpu: "4"
                  memory: 8Gi
    Worker:
      replicas: 4
      template:
        spec:
          nodeSelector:
            nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
          tolerations:
            - key: nvidia.com/gpu
              operator: Exists
          containers:
            - name: mpi-worker
              image: registry.example.com/horovod-trainer:v1.0.0
              resources:
                limits:
                  nvidia.com/gpu: 8
                  memory: 512Gi
                  cpu: "64"
              volumeMounts:
                - name: dshm
                  mountPath: /dev/shm
                - name: data
                  mountPath: /data
          volumes:
            - name: dshm
              emptyDir:
                medium: Memory
                sizeLimit: 64Gi
            - name: data
              persistentVolumeClaim:
                claimName: training-data
```

## 3. Horovod 训练脚本要点

```python
# train.py — Horovod 分布式训练
import horovod.torch as hvd
import torch

# 初始化 Horovod
hvd.init()
torch.cuda.set_device(hvd.local_rank())

# 包装 DatasetSampler
train_sampler = torch.utils.data.distributed.DistributedSampler(
    dataset, num_replicas=hvd.size(), rank=hvd.rank()
)

model = MyModel().cuda()

# Horovod: 广播初始参数
hvd.broadcast_parameters(model.state_dict(), root_rank=0)
hvd.broadcast_optimizer_state(optimizer, root_rank=0)

# Horovod: 分布式优化器
optimizer = hvd.DistributedOptimizer(
    optimizer, named_parameters=model.named_parameters()
)

for epoch in range(100):
    train_sampler.set_epoch(epoch)
    for batch in train_loader:
        loss = model(batch)
        loss.backward()
        optimizer.step()
        optimizer.zero_grad()

    # 只有 Rank 0 保存
    if hvd.rank() == 0:
        torch.save(model.state_dict(), "/checkpoints/model.pt")
```

## 4. DeepSpeed 集成

```yaml
apiVersion: kubeflow.org/v2beta1
kind: MPIJob
metadata:
  name: deepspeed-training
  namespace: ai-training
spec:
  slotsPerWorker: 8
  mpiReplicaSpecs:
    Launcher:
      replicas: 1
      template:
        spec:
          containers:
            - name: launcher
              image: registry.example.com/deepspeed:v1.0.0
              command:
                - deepspeed
                - --hostfile
                - /etc/mpi/hostfile
                - --num_gpus
                - "32"
                - train.py
                - --deepspeed_config=ds_config.json
    Worker:
      replicas: 4
      template:
        spec:
          containers:
            - name: worker
              image: registry.example.com/deepspeed:v1.0.0
              resources:
                limits:
                  nvidia.com/gpu: 8
              volumeMounts:
                - name: dshm
                  mountPath: /dev/shm
```

## 5. DeepSpeed 配置

```json
{
  "train_micro_batch_size_per_gpu": 4,
  "gradient_accumulation_steps": 4,
  "steps_per_print": 100,
  "zero_optimization": {
    "stage": 3,
    "overlap_comm": true,
    "contiguous_gradients": true,
    "reduce_bucket_size": 50000000
  },
  "fp16": {
    "enabled": true,
    "loss_scale": 0,
    "initial_scale_power": 16
  },
  "optimizer": {
    "type": "AdamW",
    "params": {
      "lr": 0.0003,
      "weight_decay": 0.01
    }
  },
  "activation_checkpointing": {
    "partition_activations": true,
    "cpu_checkpointing": true
  }
}
```

## 6. NCCL 网络优化

```yaml
# Worker 环境变量
env:
  - name: NCCL_DEBUG
    value: "WARN"
  - name: NCCL_SOCKET_IFNAME
    value: "eth0"               # 网络接口
  - name: NCCL_IB_HCA
    value: "mlx5_0,mlx5_1"      # InfiniBand 设备
  - name: NCCL_IB_DISABLE
    value: "0"
  - name: NCCL_NET_GDR_LEVEL
    value: "PHB"                # GPU Direct RDMA
  - name: NCCL_P2P_LEVEL
    value: "NVL"                # NVLink 拓扑
  - name: NCCL_BUFFSIZE
    value: "4194304"            # 4MB 缓冲区
```

## 7. 容错与检查点

```yaml
# 定期 checkpoint 以支持故障恢复
containers:
  - name: worker
    command:
      - mpirun
      - --allow-run-as-root
      - -np
      - "32"
      - python
      - train.py
      - --checkpoint_freq=500   # 每 500 步保存
      - --checkpoint_dir=/checkpoints
      - --resume_from_checkpoint=true
```

## 8. 生产实践

| 实践 | 说明 |
|------|------|
| 使用 InfiniBand | NCCL 性能提升 10 倍 |
| 调整 `slotsPerWorker` | 等于每节点 GPU 数 |
| `/dev/shm` 加大 | NCCL 通信需要 |
| 设置 `cleanPodPolicy` | `Running` 保留日志，`All` 清理 |
| 使用 DeepSpeed ZeRO-3 | 训练超大模型 |
| 启用 `activation_checkpointing` | 用计算换显存 |
| Gang Scheduling | 确保所有 Worker 同时启动 |

## 9. 调试

```bash
# 🟢 低风险：状态检查
# 查看 MPIJob 状态
kubectl get mpijob -n ai-training

# 查看 Launcher 日志
kubectl logs horovod-training-launcher -n ai-training

# 查看 Worker 日志
kubectl logs horovod-training-worker-0 -n ai-training

# 检查 NCCL 连接
kubectl exec horovod-training-worker-0 -n ai-training -- \
  nvidia-smi topo -m
```

## Related

- [[清单模式/06-ai-ml-patterns/05-training-job-pytorch|PyTorch 分布式训练]]
- [[清单模式/06-ai-ml-patterns/01-gpu-pod-scheduling|GPU Pod 调度]]

## See Also

- [MPI Operator](https://github.com/kubeflow/mpi-operator)
- [Horovod 文档](https://horovod.readthedocs.io/)
- [DeepSpeed](https://www.deepspeed.ai/)

<!-- risk-assessed -->
