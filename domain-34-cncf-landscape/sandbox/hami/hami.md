---
title: HAMi (Heterogeneous AI Computing Virtualization Middleware)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- scheduler
- prometheus
- helm
- gpu
- cuda
- nvidia
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- HAMi (Heterogeneous AI Computing Virtualization Middleware) 是什么
- 如何 HAMi (Heterogeneous AI Computing Virtualization Middleware)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- HAMi
- Heterogeneous
- AI
- Computing
- Virtualization
- Middleware
- cncf
- landscape
---

# HAMi (Heterogeneous AI Computing Virtualization Middleware)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://project-hami.io/ |
| **GitHub** | https://github.com/Project-HAMi/HAMi |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, C |
| **CNCF 状态** | Sandbox |

---

## 项目概述

HAMi（原 vGPU_4k8s）是一个异构计算设备虚拟化中间件，为 Kubernetes 提供 GPU、NPU 等加速器的共享和虚拟化能力。它允许多个 Pod 共享同一块物理 GPU，并提供显存和算力的精细化隔离，有效提升 GPU 利用率。HAMi 支持 NVIDIA GPU、AMD GPU、华为 Ascend NPU、寒武纪 MLU 等多种异构设备。

### 核心特性

- **GPU 共享**: 多个 Pod 共享同一块物理 GPU，提升利用率
- **显存隔离**: 为每个 Pod 分配精确的显存限额，防止 OOM
- **算力隔离**: 限制每个 Pod 的 GPU 算力占比
- **多设备支持**: NVIDIA GPU、AMD GPU、华为 Ascend、寒武纪 MLU、海光 DCU
- **设备拓扑感知**: 基于 NVLink/PCIe 拓扑优化 GPU 分配
- **无侵入式**: 无需修改应用代码，对用户透明

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│            Kubernetes API Server             │
└──────────────────┬──────────────────────────┘
                   │
    ┌──────────────▼──────────────┐
    │      HAMi Scheduler         │
    │  (扩展调度器 / 设备分配)     │
    │  - 显存感知调度              │
    │  - 算力感知调度              │
    │  - 拓扑感知调度              │
    └──────────────┬──────────────┘
                   │
    ┌──────────────▼──────────────────────────┐
    │              Node                        │
    │                                          │
    │  ┌──────────────────────────────┐       │
    │  │     HAMi Device Plugin       │       │
    │  │  (设备注册 / 健康检查)        │       │
    │  └──────────────┬───────────────┘       │
    │                 │                        │
    │  ┌──────────────▼───────────────┐       │
    │  │    HAMi Container Runtime     │       │
    │  │  (libvgpu.so 注入)            │       │
    │  │  - 显存限制                    │       │
    │  │  - 算力限制                    │       │
    │  │  - API 拦截                    │       │
    │  └──────────────┬───────────────┘       │
    │                 │                        │
    │  ┌──────────────▼───────────────┐       │
    │  │   Physical GPU / NPU         │       │
    │  │   NVIDIA / AMD / Ascend      │       │
    │  └──────────────────────────────┘       │
    └──────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 添加 Helm 仓库
helm repo add hami https://project-hami.github.io/HAMi/
helm repo update

# 安装 HAMi
helm install hami hami/hami \
  --namespace kube-system \
  --set scheduler.kubeScheduler.imageTag=v1.28.0
```

### 使用 GPU 共享

```yaml
# Pod 申请部分 GPU 资源
apiVersion: v1
kind: Pod
metadata:
  name: gpu-task-1
spec:
  containers:
    - name: cuda-app
      image: nvidia/cuda:12.0-runtime
      resources:
        limits:
          nvidia.com/gpu: 1               # 使用 1 个虚拟 GPU
          nvidia.com/gpumem: 4096         # 限制 4GB 显存
          nvidia.com/gpucores: 30         # 限制 30% 算力
      command: ["python", "train.py"]
```

### 多 GPU 共享示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: inference-service
spec:
  replicas: 4
  selector:
    matchLabels:
      app: inference
  template:
    metadata:
      labels:
        app: inference
    spec:
      containers:
        - name: model-server
          image: myorg/model-server:latest
          resources:
            limits:
              nvidia.com/gpu: 1
              nvidia.com/gpumem: 2048     # 每个副本 2GB 显存
              nvidia.com/gpucores: 25     # 每个副本 25% 算力
          # 4 个副本共享 1 块 8GB GPU
```

---

## 高级配置

### 节点级 GPU 配置

```yaml
# 通过 ConfigMap 配置节点策略
apiVersion: v1
kind: ConfigMap
metadata:
  name: hami-device-config
  namespace: kube-system
data:
  config.yaml: |
    nvidia:
      resourceCountName: nvidia.com/gpu
      resourceMemoryName: nvidia.com/gpumem
      resourceCoreName: nvidia.com/gpucores
      defaultMemory: 0                    # 0 表示不限制
      defaultCores: 0                     # 0 表示不限制
      deviceSplitCount: 10               # 每块 GPU 最多虚拟化为 10 份
      deviceMemoryScaling: 1.0           # 显存超分比例
```

### 华为 Ascend NPU 支持

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ascend-task
spec:
  containers:
    - name: mindspore-app
      image: ascend-app:latest
      resources:
        limits:
          huawei.com/Ascend910: 1
          huawei.com/Ascend910-memory: 16384  # 16GB HBM
```

---

## 监控

HAMi 暴露 Prometheus 指标用于 GPU 使用监控：

```yaml
# ServiceMonitor 配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: hami-monitor
spec:
  selector:
    matchLabels:
      app: hami-device-plugin
  endpoints:
    - port: metrics
      interval: 15s
```

关键指标：
- `hami_container_gpu_memory_usage_bytes` - 容器 GPU 显存使用
- `hami_container_gpu_utilization` - 容器 GPU 利用率
- `hami_node_gpu_memory_total_bytes` - 节点 GPU 显存总量
- `hami_node_gpu_count` - 节点 GPU 数量

---

## 与其他方案对比

| 特性 | HAMi | NVIDIA MPS | NVIDIA MIG | 时间片共享 |
|:---|:---|:---|:---|:---|
| 显存隔离 | 软隔离 | 无 | 硬隔离 | 无 |
| 算力隔离 | 软隔离 | 部分 | 硬隔离 | 时间片 |
| 最小粒度 | 1MB 显存 | 线程级 | 1/7 GPU | 整卡 |
| 多设备支持 | NVIDIA/AMD/NPU | 仅 NVIDIA | 仅 A100+ | 仅 NVIDIA |
| 故障隔离 | 进程级 | 进程级 | 硬件级 | 无 |
| 适用场景 | 推理/开发/训练 | 推理 | 生产推理 | 开发测试 |

---

## 最佳实践

1. **资源规划**: 根据模型的实际显存和算力需求设置 limits，避免过度超分
2. **监控告警**: 部署 GPU 监控面板，对显存使用率和 OOM 事件设置告警
3. **拓扑感知**: 多 GPU 训练任务启用拓扑感知调度，优先分配 NVLink 连接的 GPU
4. **分级策略**: 推理服务使用 GPU 共享提升利用率，训练任务使用独占模式保证性能
5. **设备分片**: 根据业务需求合理设置 deviceSplitCount，避免过多碎片化

---

## 参考资源

- [HAMi 官方文档](https://project-hami.io/docs/)
- [HAMi GitHub](https://github.com/Project-HAMi/HAMi)
- [HAMi 示例](https://github.com/Project-HAMi/HAMi/tree/master/examples)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
