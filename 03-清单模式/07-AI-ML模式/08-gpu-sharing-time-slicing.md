---
title: GPU 时间切片共享配置
description: NVIDIA GPU 时间切片共享机制与配置
summary: 通过 NVIDIA Device Plugin 时间切片让多个 Pod 共享单块 GPU，提高小模型推理的 GPU 利用率
category: manifests-patterns
tags:
- k8s
- manifests
- ai-ml-infra
- gpu
- time-slicing
- gpu-sharing
- nvidia
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
- SRE
estimated_read_time: 10min
intent_queries:
- GPU 共享如何配置
- GPU 时间切片
- 多 Pod 共享 GPU
trigger_keywords:
- gpu-sharing
- time-slicing
- nvidia-device-plugin
- mig
- gpu
prerequisites:
- gpu-basics
- k8s-device-plugin
authors:
- name: KUDIG Team
  role: contributor
---

# GPU 时间切片共享配置

## 1. GPU 共享方案对比

| 方案 | 隔离性 | 隔离维度 | 显存限制 | 适用场景 |
|------|--------|----------|----------|----------|
| **时间切片** | 弱（共享） | 时间片 | 无 | 小模型推理 |
| **MIG** | 强（硬件） | 计算核心+显存 | 有 | 中等模型推理 |
| **vGPU** | 中 | 软件 | 有 | 虚拟化环境 |
| **独占** | 最强 | 整块 GPU | 无 | 大模型训练 |

> ⚠️ 时间切片**不提供隔离**：一个 Pod 可能影响另一个 Pod 的性能。

## 2. NVIDIA Device Plugin 时间切片配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: nvidia-device-plugin-config
  namespace: kube-system
data:
  config.yaml: |
    version: v1
    sharing:
      timeSlicing:
        resources:
          - name: nvidia.com/gpu
            replicas: 4           # 每块 GPU 分为 4 个时间片
            devices: all          # 所有 GPU
    flags:
      migStrategy: none           # 时间切片与 MIG 互斥
      failOnInitError: true
      passDeviceSpecs: true
      deviceListStrategy: envvar
```

## 3. 部署 Device Plugin（时间切片模式）

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: nvidia-device-plugin
  namespace: kube-system
spec:
  selector:
    matchLabels:
      name: nvidia-device-plugin-ds
  template:
    metadata:
      labels:
        name: nvidia-device-plugin-ds
    spec:
      nodeSelector:
        accelerator: nvidia
      tolerations:
        - key: nvidia.com/gpu
          operator: Exists
      priorityClassName: system-node-critical
      containers:
        - name: nvidia-device-plugin-ctr
          image: nvcr.io/nvidia/k8s-device-plugin:v0.14.5
          args:
            - --config=/config/config.yaml
          env:
            - name: NVIDIA_VISIBLE_DEVICES
              value: all
          volumeMounts:
            - name: device-plugin
              mountPath: /var/lib/kubelet/device-plugins
            - name: config
              mountPath: /config
      volumes:
        - name: device-plugin
          hostPath:
            path: /var/lib/kubelet/device-plugins
        - name: config
          configMap:
            name: nvidia-device-plugin-config
```

## 4. 请求共享 GPU 的 Pod

```yaml
# 时间切片后，nvidia.com/gpu 可以分配小数/多 Pod 共享
apiVersion: apps/v1
kind: Deployment
metadata:
  name: light-inference
  namespace: ai-inference
spec:
  replicas: 4
  selector:
    matchLabels:
      app: light-inference
  template:
    metadata:
      labels:
        app: light-inference
    spec:
      nodeSelector:
        nvidia.com/gpu.present: "true"
      containers:
        - name: inference
          image: registry.example.com/model-server:v1.0.0
          resources:
            limits:
              nvidia.com/gpu: 1   # 请求 1 个时间片（1/4 GPU）
          env:
            - name: CUDA_VISIBLE_DEVICES
              value: "0"          # 显式指定 GPU
            - name: GPU_MEMORY_FRACTION
              value: "0.2"        # 限制 PyTorch 显存使用
```

## 5. 显存限制（应用层）

由于时间切片不隔离显存，需要在应用层限制：

### 5.1 PyTorch 显存限制

```python
import torch

# 限制 PyTorch 可使用的 GPU 显存比例
torch.cuda.set_per_process_memory_fraction(0.2, device=0)

# 或设置环境变量
# PYTORCH_CUDA_ALLOC_CONF=max_split_size_mb:128
```

### 5.2 vLLM 显存限制

```yaml
args:
  - --gpu-memory-utilization=0.2  # 只使用 20% 显存
  - --max-model-len=2048          # 限制上下文长度
```

### 5.3 Triton 显存限制

```yaml
args:
  - --cuda-memory-pool-byte-size=0:1073741824  # 限制 1GB GPU 内存池
```

## 6. 按模型大小分区

```yaml
# 不同 GPU 使用不同的时间片数量
apiVersion: v1
kind: ConfigMap
metadata:
  name: gpu-sharing-config
  namespace: kube-system
data:
  config.yaml: |
    version: v1
    sharing:
      timeSlicing:
        resources:
          # T4 GPU: 8 个时间片（小模型推理）
          - name: nvidia.com/gpu
            replicas: 8
            devices: all
            nodeSelector:
              nvidia.com/gpu.product: "Tesla-T4"
          # A100 GPU: 4 个时间片（中等模型推理）
          - name: nvidia.com/gpu
            replicas: 4
            devices: all
            nodeSelector:
              nvidia.com/gpu.product: "NVIDIA-A100-SXM4-40GB"
```

## 7. 监控共享 GPU

```yaml
# DCGM Exporter 已支持时间切片监控
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: gpu-shared-monitor
  namespace: monitoring
spec:
  endpoints:
    - port: metrics
      path: /metrics
      interval: 10s
      metricRelabelings:
        - sourceLabels: [__name__]
          regex: 'DCGM_FI_DEV_GPU_UTIL|DCGM_FI_DEV_MEM_COPY_UTIL|DCGM_FI_DEV_FB_USED'
          action: keep
```

关键监控指标：

| 指标 | 说明 |
|------|------|
| `DCGM_FI_DEV_GPU_UTIL` | GPU 计算利用率 |
| `DCGM_FI_DEV_MEM_COPY_UTIL` | 显存带宽利用率 |
| `DCGM_FI_DEV_FB_USED` | 已用显存 |
| `DCGM_FI_PROF_PIPE_TENSOR_ACTIVE` | Tensor Core 使用率 |

## 8. 生产实践

| 实践 | 说明 |
|------|------|
| 仅用于推理，不用于训练 | 训练对延迟敏感 |
| 小模型才用时间切片 | < 3B 参数 |
| 应用层限制显存 | 避免一个 Pod 占满 |
| 监控 GPU 利用率 | 确保利用率在合理范围 |
| 考虑 MIG 替代 | 如果需要更强的隔离 |

## 9. 时间切片 vs MIG 选择

```
需要 GPU 隔离吗？
├── 是 → MIG（A100/H100）
│        └── 需要硬件分区
└── 否 → 时间切片
         ├── 小模型推理 ✅
         ├── CI/CD 测试 ✅
         └── 大模型训练 ❌（性能影响太大）
```

## 10. 验证配置

```bash
# 🟢 低风险：GPU 验证
# 检查时间切片是否生效
kubectl describe node <gpu-node> | grep -A 5 "Allocated resources"
# 应看到 nvidia.com/gpu 可分配数量增加了

# 在 Pod 中确认 GPU 访问
kubectl exec -it <pod-name> -- nvidia-smi
# 会看到同一块 GPU 被多个进程使用

# 检查实际共享情况
kubectl get pods -o wide --all-namespaces \
  | grep nvidia.com/gpu
```

## Related

- [[03-清单模式/07-AI-ML模式/01-gpu-pod-scheduling|GPU Pod 调度]]
- [[03-清单模式/07-AI-ML模式/02-mig-partitioning-manifests|MIG 分区配置]]

## See Also

- [NVIDIA GPU 时间切片](https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/gpu-sharing.html)
- [k8s-device-plugin](https://github.com/NVIDIA/k8s-device-plugin)

<!-- risk-assessed -->
