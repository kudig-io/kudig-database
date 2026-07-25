---
title: PyTorch 分布式训练 Job
description: 使用 Kubernetes Job 和 PyTorchJob 进行分布式模型训练
summary: 使用 PyTorchJob Operator 和原生 Job 实现 PyTorch DDP 分布式训练，包括多节点多 GPU 配置
category: manifests-patterns
tags:
- k8s
- manifests
- ai-ml-infra
- pytorch
- distributed-training
- ddp
- gpu
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
estimated_read_time: 14min
intent_queries:
- PyTorch 分布式训练 Kubernetes
- PyTorchJob 配置
- DDP 多节点训练
trigger_keywords:
- pytorch
- pytorchjob
- ddp
- distributed-training
- nccl
prerequisites:
- gpu-basics
- k8s-job-basics
- pytorch-basics
authors:
- name: KUDIG Team
  role: contributor
---

# PyTorch 分布式训练 Job

## 1. 分布式训练架构

```
┌──────────────────────────────────┐
│         PyTorchJob CR            │
└──────────┬───────────────────────┘
           ↓
    ┌──────┴──────┐
    │  Master Pod  │  ← Rank 0 (协调者)
    └──────┬──────┘
           ↓ NCCL/ gloo
  ┌────────┼────────┐
  ↓        ↓        ↓
Worker1   Worker2  WorkerN   ← Rank 1..N
(GPU)     (GPU)    (GPU)
```

## 2. PyTorchJob CR（推荐方式）

使用 Kubeflow Training Operator：

```yaml
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: distributed-training
  namespace: ai-training
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        metadata:
          annotations:
            sidecar.istio.io/inject: "false"
        spec:
          nodeSelector:
            nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
          tolerations:
            - key: nvidia.com/gpu
              operator: Exists
          containers:
            - name: pytorch
              image: registry.example.com/trainer:v1.0.0
              resources:
                limits:
                  nvidia.com/gpu: 8
                  memory: 512Gi
                  cpu: "64"
              env:
                - name: NCCL_DEBUG
                  value: "WARN"
                - name: NCCL_SOCKET_IFNAME
                  value: "eth0,ens5f0"
                - name: NCCL_IB_HCA
                  value: "mlx5_0,mlx5_1"
                - name: OMP_NUM_THREADS
                  value: "8"
              volumeMounts:
                - name: dshm
                  mountPath: /dev/shm
                - name: training-data
                  mountPath: /data
                - name: checkpoints
                  mountPath: /checkpoints
              command:
                - torchrun
                - --nproc_per_node=8
                - --nnodes=4
                - --node_rank=0
                - --master_addr=$(MASTER_ADDR)
                - --master_port=29500
                - train.py
                - --batch_size=64
                - --learning_rate=0.0003
                - --epochs=100
                - --data_dir=/data
                - --checkpoint_dir=/checkpoints
          volumes:
            - name: dshm
              emptyDir:
                medium: Memory
                sizeLimit: 64Gi
            - name: training-data
              persistentVolumeClaim:
                claimName: training-dataset
            - name: checkpoints
              persistentVolumeClaim:
                claimName: model-checkpoints
    Worker:
      replicas: 3
      restartPolicy: OnFailure
      template:
        spec:
          nodeSelector:
            nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
          tolerations:
            - key: nvidia.com/gpu
              operator: Exists
          containers:
            - name: pytorch
              image: registry.example.com/trainer:v1.0.0
              resources:
                limits:
                  nvidia.com/gpu: 8
                  memory: 512Gi
                  cpu: "64"
              command:
                - torchrun
                - --nproc_per_node=8
                - --nnodes=4
                - --node_rank=1
                - --master_addr=$(MASTER_ADDR)
                - --master_port=29500
                - train.py
```

## 3. 训练脚本要点

```python
# train.py — DDP 分布式训练核心逻辑
import torch
import torch.distributed as dist
from torch.nn.parallel import DistributedDataParallel as DDP
from torch.utils.data import DataLoader, DistributedSampler

def setup():
    dist.init_process_group(backend="nccl")
    local_rank = int(os.environ["LOCAL_RANK"])
    torch.cuda.set_device(local_rank)

def cleanup():
    dist.destroy_process_group()

def main():
    setup()
    local_rank = int(os.environ["LOCAL_RANK"])

    model = MyModel().cuda()
    model = DDP(model, device_ids=[local_rank])

    dataset = MyDataset()
    sampler = DistributedSampler(dataset)
    loader = DataLoader(dataset, batch_size=64, sampler=sampler)

    optimizer = torch.optim.AdamW(model.parameters(), lr=0.0003)

    for epoch in range(100):
        sampler.set_epoch(epoch)  # 关键：确保每个 epoch 数据不同
        for batch in loader:
            loss = model(batch)
            loss.backward()
            optimizer.step()
            optimizer.zero_grad()

        # 只有 Rank 0 保存 checkpoint
        if dist.get_rank() == 0:
            torch.save(model.state_dict(), f"/checkpoints/model_epoch_{epoch}.pt")

    cleanup()
```

## 4. 数据集 PVC

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: training-dataset
  namespace: ai-training
spec:
  accessModes:
    - ReadWriteMany              # 多 Pod 并行读取
  resources:
    requests:
      storage: 5Ti
  storageClassName: nfs-client
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: model-checkpoints
  namespace: ai-training
spec:
  accessModes:
    - ReadWriteMany
  resources:
    requests:
      storage: 500Gi
  storageClassName: nfs-client
```

## 5. Checkpoint 存储（S3）

```yaml
# 使用 S3 存储 checkpoint（更可靠）
env:
  - name: AWS_S3_ENDPOINT
    value: "s3.us-east-1.amazonaws.com"
  - name: CHECKPOINT_S3_PATH
    value: "s3://my-bucket/checkpoints/run-001/"
  - name: AWS_ACCESS_KEY_ID
    valueFrom:
      secretKeyRef:
        name: aws-credentials
        key: access_key_id
  - name: AWS_SECRET_ACCESS_KEY
    valueFrom:
      secretKeyRef:
        name: aws-credentials
        key: secret_access_key
```

## 6. 使用 Volcano/Shifter 调度

确保所有 Worker Pod 同时启动（gang scheduling）：

```yaml
apiVersion: scheduling.volcano.sh/v1beta1
kind: PodGroup
metadata:
  name: training-job
spec:
  minMember: 4                  # 必须 4 个 Pod 都调度成功
  priorityClassName: high-priority
  queue: training-queue
```

```yaml
# PyTorchJob 中添加调度器名称
spec:
  template:
    spec:
      schedulerName: volcano
      priorityClassName: high-priority
```

## 7. 容错与自动恢复

```yaml
spec:
  pytorchReplicaSpecs:
    Master:
      restartPolicy: OnFailure
      maxRestart: 3              # 最多重启 3 次
    Worker:
      restartPolicy: OnFailure
      maxRestart: 3
```

## 8. 训练监控

```yaml
# 训练 Pod 中暴露 metrics
containers:
  - name: pytorch
    ports:
      - containerPort: 6006
        name: tensorboard
    env:
      - name: WANDB_API_KEY
        valueFrom:
          secretKeyRef:
            name: wandb-credentials
            key: api-key
```

## 9. 生产实践

| 实践 | 说明 |
|------|------|
| 使用 gang scheduling | Volcano/Shifter 确保所有 Worker 同时启动 |
| Checkpoint 定期保存 | 防止长时间训练中途失败 |
| Rank 0 保存 checkpoint | 避免多进程同时写文件冲突 |
| 使用高速网络 | InfiniBand/RDMA 加速 NCCL |
| `/dev/shm` 加大 | NCCL 需要大量共享内存 |
| 监控 GPU 利用率 | 确认多 GPU 训练效率 |
| 数据并行采样 | `DistributedSampler` + `set_epoch` |

## 10. 调试命令

```bash
# 🟢 低风险：只读调试
# 查看 PyTorchJob 状态
kubectl get pytorchjob -n ai-training

# 查看 Pod 状态
kubectl get pods -n ai-training -l job-name=distributed-training

# 查看 Worker 日志
kubectl logs -f distributed-training-worker-0 -n ai-training

# 查看 NCCL 通信日志
kubectl logs distributed-training-master-0 -n ai-training | grep NCCL
```

## Related

- [[03-清单模式/07-AI-ML模式/06-mpi-operator-patterns|MPIJob 分布式训练]]
- [[03-清单模式/07-AI-ML模式/01-gpu-pod-scheduling|GPU Pod 调度]]

## See Also

- [Kubeflow Training Operator](https://github.com/kubeflow/training-operator)
- [PyTorch DDP 教程](https://pytorch.org/tutorials/intermediate/ddp_tutorial.html)
- [Volcano Gang Scheduling](https://volcano.sh/en/docs/)

<!-- risk-assessed -->
