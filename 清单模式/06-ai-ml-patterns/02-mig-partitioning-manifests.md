---
title: MIG 分区配置
description: NVIDIA Multi-Instance GPU (MIG) 分区与 Kubernetes 集成
summary: 使用 MIG 将 A100/H100 GPU 分区为多个实例，提高 GPU 利用率并实现细粒度资源分配
category: manifests-patterns
tags:
- k8s
- manifests
- ai-ml-infra
- gpu
- mig
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
estimated_read_time: 12min
intent_queries:
- MIG 分区如何配置
- A100 MIG Kubernetes
- GPU 多实例配置
trigger_keywords:
- mig
- gpu
- partitioning
- a100
- h100
prerequisites:
- gpu-basics
- k8s-pod-basics
authors:
- name: KUDIG Team
  role: contributor
---

# MIG 分区配置

## 1. MIG 概述

Multi-Instance GPU (MIG) 将单块 A100/H100 GPU 分区为最多 7 个独立实例，每个实例有隔离的 GPU 核心、内存和缓存。

| GPU 型号 | 支持分区 | 最大实例 |
|----------|----------|----------|
| A100 40GB | 1g.5gb / 2g.10gb / 3g.20gb / 4g.20gb / 7g.40gb | 7 |
| A100 80GB | 1g.10gb / 2g.20gb / 3g.40gb / 4g.40gb / 7g.80gb | 7 |
| H100 80GB | 1g.10gb / 2g.20gb / 3g.40gb / 4g.40gb / 7g.80gb | 7 |

## 2. MIG 策略模式

```yaml
# NVIDIA GPU Operator ConfigMap 配置 MIG 策略
apiVersion: v1
kind: ConfigMap
metadata:
  name: gpu-operator-mig-config
  namespace: gpu-operator
data:
  config.yaml: |
    version: v1
    flags:
      migStrategy: mixed         # single 或 mixed
      migConfig: all-1g.5gb      # 默认分区配置
```

### 2.1 Single 模式

所有 GPU 使用相同的 MIG 分区配置：

```yaml
# 所有 GPU 分区为 1g.5gb（7 实例/GPU）
apiVersion: nvidia.com/v1
kind: MigConfig
metadata:
  name: all-1g.5gb
spec:
  devices: all
  migConfigs:
    - device: all
      configs:
        - name: 1g.5gb
          count: 7              # 每块 GPU 7 个 1g.5gb 实例
```

### 2.2 Mixed 模式

不同 GPU 使用不同分区配置（更灵活）：

```yaml
apiVersion: nvidia.com/v1
kind: MigConfig
metadata:
  name: mixed-config
spec:
  devices:
    - index: 0                  # GPU 0
      configs:
        - name: 2g.10gb         # 2g.10gb 分区（适合中等模型）
          count: 3
    - index: 1                  # GPU 1
      configs:
        - name: 7g.40gb         # 不分区（整块 GPU）
          count: 1
    - index: 2,3               # GPU 2-3
      configs:
        - name: 1g.5gb          # 小实例（适合轻量推理）
          count: 7
```

## 3. 请求 MIG 实例的 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: mig-inference
spec:
  restartPolicy: Never
  nodeSelector:
    nvidia.com/gpu.product: "NVIDIA-A100-SXM4-40GB"
    nvidia.com/mig.strategy: "single"
  containers:
    - name: inference
      image: registry.example.com/model-server:v1.0.0
      resources:
        limits:
          nvidia.com/mig-1g.5gb: 1   # 请求 1 个 1g.5gb MIG 实例
      command: ["python", "-c"]
      args:
        - "import torch; print(torch.cuda.get_device_name(0)); print(torch.cuda.get_device_properties(0))"
```

## 4. 不同 MIG 配置的 Pod 示例

```yaml
# 3g.20gb 实例（约 20GB 显存，适合中等模型）
apiVersion: v1
kind: Pod
metadata:
  name: medium-model-inference
spec:
  containers:
    - name: model
      image: registry.example.com/llama-server:v1.0.0
      resources:
        limits:
          nvidia.com/mig-3g.20gb: 1
      env:
        - name: MODEL_NAME
          value: "llama-2-13b"
        - name: GPU_MEMORY_FRACTION
          value: "0.9"
---
# 7g.40gb 实例（整块 GPU，适合大模型）
apiVersion: v1
kind: Pod
metadata:
  name: large-model-training
spec:
  containers:
    - name: trainer
      image: registry.example.com/trainer:v1.0.0
      resources:
        limits:
          nvidia.com/mig-7g.40gb: 1
```

## 5. Node 标签与调度

GPU Operator 自动为 MIG 实例添加标签：

```yaml
# 节点自动标签
nvidia.com/gpu.product: "NVIDIA-A100-SXM4-40GB"
nvidia.com/mig.strategy: "single"
nvidia.com/gpu.count: "8"
# 每种 MIG 实例类型的可分配数量
nvidia.com/mig-1g.5gb.count: "56"      # 8 GPU × 7 实例
nvidia.com/mig-2g.10gb.count: "24"     # 8 GPU × 3 实例
nvidia.com/mig-3g.20gb.count: "16"
```

```yaml
# 按可用 MIG 实例选择节点
spec:
  nodeSelector:
    nvidia.com/mig.strategy: "single"
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
          - matchExpressions:
              - key: nvidia.com/mig-1g.5gb.count
                operator: Gt
                values: ["0"]         # 至少有 1 个 1g.5gb 实例可用
```

## 6. MIG 管理 Job

```yaml
# 动态配置 MIG 分区
apiVersion: batch/v1
kind: Job
metadata:
  name: mig-reconfigure
  namespace: gpu-operator
spec:
  template:
    spec:
      restartPolicy: Never
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-40GB"
      containers:
        - name: mig-manager
          image: nvcr.io/nvidia/cloud-native/k8s-mig-manager:v0.3.0-ubuntu20.04
          command: ["/bin/bash", "-c"]
          args:
            - |
              # 启用 MIG 模式
              nvidia-smi -mig 1
              # 创建 1g.5gb 分区
              nvidia-smi mig -cgi 19,19,19,19,19,19,19 -C
              # 验证
              nvidia-smi mig -lgi
          securityContext:
            privileged: true
```

## 7. 生产实践

| 实践 | 说明 |
|------|------|
| 根据模型大小选择分区 | 7B 模型用 2g.10gb，13B 用 3g.20gb |
| Mixed 模式更灵活 | 不同 GPU 不同分区 |
| 监控 MIG 实例利用率 | DCGM Exporter 支持 MIG 级别指标 |
| 合理设置 ResourceQuota | 按 MIG 类型限制 |
| 避免频繁重配分区 | 会影响运行中的工作负载 |

## 8. MIG 配置选择指南

| 模型大小 | 推荐分区 | 显存 | 并发能力 |
|----------|----------|------|----------|
| < 3B 参数 | 1g.5gb | 5GB | 高（7 实例/GPU） |
| 3B-7B 参数 | 2g.10gb | 10GB | 中（3 实例/GPU） |
| 7B-13B 参数 | 3g.20gb | 20GB | 低（2 实例/GPU） |
| 13B-30B 参数 | 4g.20gb | 20GB | 很低（2 实例/GPU） |
| > 30B 参数 | 7g.40gb | 40GB | 独占 |

## Related

- [[清单模式/06-ai-ml-patterns/01-gpu-pod-scheduling|GPU Pod 调度]]
- [[清单模式/06-ai-ml-patterns/08-gpu-sharing-time-slicing|GPU 时间切片]]

## See Also

- [NVIDIA MIG 文档](https://docs.nvidia.com/datacenter/tesla/mig-user-guide/)
- [GPU Operator MIG 配置](https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/mig.html)

<!-- risk-assessed -->
