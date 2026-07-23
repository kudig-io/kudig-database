---
title: GPU Pod 调度配置
description: GPU 资源请求、节点选择器与拓扑感知调度
summary: NVIDIA GPU 资源请求配置、节点选择器、GPU 类型筛选及拓扑感知调度最佳实践
category: manifests-patterns
tags:
- k8s
- manifests
- ai-ml-infra
- gpu
- nvidia
- scheduling
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- AI 工程师
- 平台工程师
- SRE
estimated_read_time: 10min
intent_queries:
- GPU Pod 如何配置
- Kubernetes GPU 资源请求
- nvidia.com/gpu 使用
trigger_keywords:
- gpu
- nvidia
- scheduling
- resource
- cuda
prerequisites:
- k8s-pod-basics
- gpu-basics
authors:
- name: KUDIG Team
  role: contributor
---

# GPU Pod 调度配置

## 1. GPU 资源声明

NVIDIA GPU 通过 `nvidia.com/gpu` 资源声明，由 NVIDIA Device Plugin 暴露：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gpu-pod
spec:
  restartPolicy: Never
  containers:
    - name: cuda-container
      image: nvcr.io/nvidia/cuda:12.3.1-base-ubuntu22.04
      resources:
        limits:
          nvidia.com/gpu: 1       # 请求 1 块 GPU
      command: ["nvidia-smi"]
```

## 2. 选择 GPU 节点类型

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: inference-server
spec:
  replicas: 2
  selector:
    matchLabels:
      app: inference
  template:
    metadata:
      labels:
        app: inference
    spec:
      nodeSelector:
        # GPU 节点标签
        accelerator: nvidia
        # GPU 型号（A100/H100/T4 等）
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-40GB"
        # GPU 数量
        nvidia.com/gpu.count: "8"
      tolerations:
        - key: nvidia.com/gpu
          operator: Exists
          effect: NoSchedule
      containers:
        - name: inference
          image: registry.example.com/model-server:v1.0.0
          resources:
            limits:
              nvidia.com/gpu: 1
              memory: 32Gi
              cpu: "8"
```

## 3. 多 GPU Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: multi-gpu-training
spec:
  restartPolicy: Never
  nodeSelector:
    nvidia.com/gpu.product: "NVIDIA-H100-80GB-HBM3"
  containers:
    - name: trainer
      image: registry.example.com/pytorch-trainer:v1.0.0
      resources:
        limits:
          nvidia.com/gpu: 8         # 请求 8 块 GPU
          memory: 512Gi
          cpu: "64"
      env:
        - name: CUDA_VISIBLE_DEVICES
          value: "0,1,2,3,4,5,6,7"
        - name: NCCL_DEBUG
          value: "INFO"
      volumeMounts:
        - name: dshm
          mountPath: /dev/shm        # 共享内存（NCCL 需要）
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

## 4. GPU 节点污点容忍

```yaml
# GPU 节点通常有 taint，只有声明 toleration 的 Pod 才能调度
spec:
  tolerations:
    - key: nvidia.com/gpu
      operator: Exists
      effect: NoSchedule
    # 专用 GPU 池污点
    - key: dedicated
      operator: Equal
      value: gpu-training
      effect: NoSchedule
```

## 5. GPU 亲和性（避免跨 NUMA）

```yaml
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
          - matchExpressions:
              - key: nvidia.com/gpu.product
                operator: In
                values:
                  - "NVIDIA-A100-SXM4-40GB"
                  - "NVIDIA-A100-SXM4-80GB"
              - key: topology.kubernetes.io/zone
                operator: In
                values:
                  - us-east-1a
                  - us-east-1b
```

## 6. GPU 资源配额与 LimitRange

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: gpu-quota
  namespace: ai-team
spec:
  hard:
    requests.nvidia.com/gpu: "16"     # 整个 Namespace 最多 16 GPU
    limits.nvidia.com/gpu: "16"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: gpu-limits
  namespace: ai-team
spec:
  limits:
    - type: Container
      max:
        nvidia.com/gpu: "8"           # 单容器最多 8 GPU
      min:
        nvidia.com/gpu: "1"           # 最少 1 GPU
```

## 7. GPU 监控配置

```yaml
# DCGM Exporter DaemonSet（每 GPU 节点部署）
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: dcgm-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: dcgm-exporter
  template:
    metadata:
      labels:
        app: dcgm-exporter
    spec:
      nodeSelector:
        accelerator: nvidia
      tolerations:
        - key: nvidia.com/gpu
          operator: Exists
      containers:
        - name: exporter
          image: nvcr.io/nvidia/dcgm-exporter:3.2.6-ubuntu22.04
          ports:
            - containerPort: 9400
              name: metrics
          securityContext:
            capabilities:
              add: ["SYS_ADMIN"]
          volumeMounts:
            - name: pod-gpu-resources
              mountPath: /var/lib/kubelet/pod-resources
      volumes:
        - name: pod-gpu-resources
          hostPath:
            path: /var/lib/kubelet/pod-resources
```

## 8. 验证 GPU 可用性

```bash
# 🟢 低风险：GPU 验证
# 检查节点 GPU 资源
kubectl describe node <gpu-node> | grep -A 10 "Allocated resources"

# 查看可分配的 GPU
kubectl get nodes -o custom-columns=\
  "NAME:.metadata.name,GPU:.status.allocatable.nvidia\.com/gpu"

# 在 Pod 中验证 GPU
kubectl exec -it gpu-pod -- nvidia-smi
```

## 9. 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| `Insufficient nvidia.com/gpu` | 节点 GPU 已耗尽 | 增加 GPU 节点或使用 GPU 共享 |
| Pod 一直 Pending | 缺少 toleration 或 nodeSelector | 检查 tolerations 和 nodeSelector |
| `nvidia-smi` 无输出 | Device Plugin 未安装 | 检查 NVIDIA Device Plugin |
| GPU 内存泄漏 | 上一个任务未释放 | 确保 Pod 退出时释放资源 |

## 10. 生产实践

| 实践 | 说明 |
|------|------|
| 按 GPU 型号打标签 | `nvidia.com/gpu.product` |
| 设置 ResourceQuota | 防止团队过度占用 GPU |
| 使用专用 NodePool | GPU 节点与 CPU 节点分离 |
| 监控 GPU 利用率 | 使用 DCGM Exporter |
| 使用 GPU 共享 | 小模型推理用时间切片 |

## Related

- [[清单模式/06-ai-ml-patterns/02-mig-partitioning-manifests|MIG 分区配置]]
- [[清单模式/06-ai-ml-patterns/08-gpu-sharing-time-slicing|GPU 时间切片]]

## See Also

- [NVIDIA GPU Device Plugin](https://github.com/NVIDIA/k8s-device-plugin)
- [GPU 调度最佳实践](https://kubernetes.io/docs/tasks/manage-gpus/scheduling-gpus/)

<!-- risk-assessed -->
