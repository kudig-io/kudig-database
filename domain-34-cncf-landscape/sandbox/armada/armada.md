# Armada

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://armadaproject.io/ |
| **GitHub** | https://github.com/armadaproject/armada |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Armada 是一个多集群批处理作业调度系统，专为在多个 Kubernetes 集群上运行大规模批处理工作负载（如 HPC 计算、ML 训练、CI/CD 等）而设计。它提供统一的作业提交入口、跨集群的公平调度、优先级抢占和作业队列管理，能够管理数百万个并发作业在数千个节点上的高效调度。

### 核心特性

- **多集群调度**: 在多个 Kubernetes 集群间分发批处理作业
- **大规模**: 支持数百万并发作业和数千个节点
- **公平调度**: 基于队列的公平份额调度和优先级抢占
- **Gang 调度**: 支持 Gang Scheduling，保证一组 Pod 同时调度
- **作业队列**: 层级化队列系统，支持资源配额和优先级
- **事件驱动**: 完整的作业生命周期事件流和日志

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│                Armada Control Plane                │
│                                                    │
│  ┌──────────┐  ┌──────────────┐  ┌────────────┐  │
│  │ Armada   │  │  Scheduler   │  │  Lookout   │  │
│  │ Server   │  │  (公平调度/  │  │  (UI/监控) │  │
│  │ (API)    │  │   抢占/Gang) │  │            │  │
│  └────┬─────┘  └──────┬───────┘  └────────────┘  │
│       │               │                            │
│  ┌────▼───────────────▼────────────────────┐      │
│  │           Event Bus (Pulsar/NATS)        │      │
│  └────┬────────────────┬───────────────────┘      │
└───────┼────────────────┼──────────────────────────┘
        │                │
   ┌────▼────┐     ┌────▼────┐
   │Executor │     │Executor │
   │Cluster 1│     │Cluster 2│   ...
   │┌───────┐│     │┌───────┐│
   ││K8s Job││     ││K8s Job││
   │└───────┘│     │└───────┘│
   └─────────┘     └─────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 Helm 安装 Armada Server
helm repo add armada https://armadaproject.io/charts
helm install armada-server armada/armada \
  --namespace armada \
  --create-namespace

# 在每个执行集群上安装 Executor
helm install armada-executor armada/armada-executor \
  --namespace armada \
  --set applicationConfig.apiConnection.armadaUrl=armada-server:50051
```

### 安装 CLI

```bash
# 下载 armadactl
curl -LO "https://github.com/armadaproject/armada/releases/latest/download/armadactl-$(uname -s)-$(uname -m)"
chmod +x armadactl-*
sudo mv armadactl-* /usr/local/bin/armadactl
```

### 创建队列

```bash
# 创建作业队列
armadactl create queue ml-training \
  --priority-factor 1.0 \
  --owners group:ml-team \
  --resource-limits cpu=1000,memory=4Ti,nvidia.com/gpu=100

armadactl create queue batch-jobs \
  --priority-factor 0.5 \
  --owners group:data-team \
  --resource-limits cpu=500,memory=2Ti
```

### 提交作业

```yaml
# job.yaml
queue: ml-training
jobSetId: experiment-001
jobs:
  - priority: 50
    podSpec:
      terminationGracePeriodSeconds: 0
      restartPolicy: Never
      containers:
        - name: training
          image: pytorch/pytorch:latest
          command: ["python", "train.py", "--epochs=100"]
          resources:
            requests:
              cpu: 4
              memory: 16Gi
              nvidia.com/gpu: 1
            limits:
              cpu: 4
              memory: 16Gi
              nvidia.com/gpu: 1
```

```bash
# 提交作业
armadactl submit job.yaml

# 查看作业状态
armadactl watch ml-training experiment-001

# 取消作业
armadactl cancel --queue ml-training --job-set experiment-001
```

---

## 高级功能

### Gang Scheduling

```yaml
# 需要 4 个 GPU Pod 同时调度
queue: ml-training
jobSetId: distributed-training
jobs:
  - priority: 100
    annotations:
      armadaproject.io/gangId: "gang-001"
      armadaproject.io/gangCardinality: "4"
    podSpec:
      containers:
        - name: worker
          image: pytorch/pytorch:latest
          command: ["torchrun", "--nproc_per_node=1", "train.py"]
          resources:
            requests:
              nvidia.com/gpu: 1
```

### 优先级抢占

```bash
# 高优先级队列的作业可以抢占低优先级队列的资源
armadactl create queue urgent-jobs \
  --priority-factor 10.0 \
  --preemption-enabled
```

---

## 与其他方案对比

| 特性 | Armada | Volcano | Kueue | YARN |
|:---|:---|:---|:---|:---|
| 多集群 | 原生支持 | 单集群 | 单集群 | 单集群 |
| 作业规模 | 百万级 | 万级 | 万级 | 百万级 |
| Gang 调度 | 支持 | 支持 | 支持 | 支持 |
| 公平调度 | 队列级 | 队列级 | 队列级 | 队列级 |
| 抢占 | 跨集群 | 集群内 | 集群内 | 集群内 |
| 适用场景 | 大规模多集群 | K8s HPC/AI | K8s 批处理 | Hadoop 生态 |

---

## 最佳实践

1. **队列设计**: 按团队或项目划分队列，设置合理的资源配额和优先级
2. **作业分组**: 使用 JobSet 将相关作业分组，便于统一管理和监控
3. **资源估算**: 准确设置作业的 resource requests，避免资源浪费或调度失败
4. **Executor 分布**: 在不同可用区/区域部署 Executor 集群，提高容灾能力
5. **监控 Lookout**: 使用 Lookout UI 监控队列积压和作业完成率

---

## 参考资源

- [Armada 官方文档](https://armadaproject.io/docs/)
- [Armada GitHub](https://github.com/armadaproject/armada)
- [Armada Lookout UI](https://github.com/armadaproject/armada/tree/master/internal/lookout)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
