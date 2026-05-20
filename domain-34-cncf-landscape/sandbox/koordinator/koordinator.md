---
title: Koordinator
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- scheduler
- helm
- daemonset
- job
- gpu
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Koordinator 是什么
- 如何 Koordinator
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Koordinator
- cncf
- landscape
---


# Koordinator

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://koordinator.sh/ |
| **GitHub** | https://github.com/koordinator-sh/koordinator |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Koordinator 是一个基于 QoS 的 Kubernetes 混合调度系统，专为提高集群资源利用率而设计。它通过精细化的资源管理和混部（co-location）技术，在保证延迟敏感型（LS）工作负载 SLO 的同时，充分利用空闲资源运行尽力而为型（BE）任务，实现 60%+ 的集群利用率。

### 核心特性

- **混部调度**: 在线服务 (LS) 与离线任务 (BE) 混部，提升资源利用率
- **QoS 保障**: 多级 QoS 分类（LSE/LSR/LS/BE），保证高优先级负载 SLO
- **精细化资源管理**: CPU Burst、内存 QoS、LLC/MBA 缓存隔离
- **Gang 调度**: 一组 Pod 全部调度成功或全部失败，适合大数据任务
- **弹性 Quota**: 跨 Namespace 的弹性资源配额管理
- **设备调度**: GPU 共享调度和拓扑感知调度
- **重调度**: 基于负载均衡的 Pod 迁移和重调度

---

## 架构设计

```
┌────────────────────────────────────────────────────┐
│                 Kubernetes Cluster                   │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │           Koordinator Scheduler               │   │
│  │  (Enhanced kube-scheduler with plugins)       │   │
│  │                                               │   │
│  │  ┌──────────┐ ┌──────────┐ ┌─────────────┐  │   │
│  │  │ Gang     │ │ Elastic  │ │ Device      │  │   │
│  │  │ Schedule │ │ Quota    │ │ Schedule    │  │   │
│  │  └──────────┘ └──────────┘ └─────────────┘  │   │
│  │  ┌──────────┐ ┌──────────┐ ┌─────────────┐  │   │
│  │  │ Coschd   │ │ Load-    │ │ Reservation │  │   │
│  │  │          │ │ Aware    │ │             │  │   │
│  │  └──────────┘ └──────────┘ └─────────────┘  │   │
│  └──────────────────────────────────────────────┘   │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │        Koordinator Manager (koord-manager)    │   │
│  │  (SLO Controller, Quota Controller)           │   │
│  └──────────────────────────────────────────────┘   │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │    Koordlet (DaemonSet - per node agent)      │   │
│  │                                               │   │
│  │  ┌───────────┐ ┌──────────┐ ┌────────────┐  │   │
│  │  │ Resource  │ │ QoS      │ │ Runtime    │  │   │
│  │  │ Collector │ │ Manager  │ │ Hooks      │  │   │
│  │  └───────────┘ └──────────┘ └────────────┘  │   │
│  │  ┌───────────┐ ┌──────────┐                  │   │
│  │  │ CPU       │ │ Memory   │                  │   │
│  │  │ Suppress  │ │ Evict    │                  │   │
│  │  └───────────┘ └──────────┘                  │   │
│  └──────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────┘
```

### QoS 分类体系

| QoS 级别 | 说明 | 典型场景 | 资源保障 |
|:---|:---|:---|:---|
| **LSE** | Latency Sensitive Exclusive | 实时计算 | CPU 绑核，独占资源 |
| **LSR** | Latency Sensitive Reserved | 核心微服务 | CPU 绑核，保留资源 |
| **LS** | Latency Sensitive | 一般在线服务 | CPU 共享，优先调度 |
| **BE** | Best Effort | 离线批处理、AI 训练 | 使用空闲资源，可被压制 |

---

## 快速开始

### 安装 Koordinator

```bash
# 使用 Helm 安装
helm repo add koordinator-sh https://koordinator-sh.github.io/charts/
helm repo update

helm install koordinator koordinator-sh/koordinator \
  --namespace koordinator-system \
  --create-namespace \
  --set scheduler.enabled=true \
  --set manager.enabled=true \
  --set koordlet.enabled=true
```

### 配置混部工作负载

```yaml
# 在线服务 (LS)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-server
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-server
  template:
    metadata:
      labels:
        app: web-server
        koordinator.sh/qosClass: LS
      annotations:
        koordinator.sh/memoryQOS: '{"policy": "default", "wmarkRatio": 95}'
    spec:
      schedulerName: koord-scheduler
      priorityClassName: koord-prod  # 高优先级
      containers:
        - name: web
          image: nginx:latest
          resources:
            requests:
              cpu: "2"
              memory: "4Gi"
            limits:
              cpu: "4"
              memory: "8Gi"
---
# 离线任务 (BE)
apiVersion: batch/v1
kind: Job
metadata:
  name: data-processing
spec:
  template:
    metadata:
      labels:
        koordinator.sh/qosClass: BE
      annotations:
        koordinator.sh/cpuBurst: '{"policy": "cpuBurstOnly"}'
    spec:
      schedulerName: koord-scheduler
      priorityClassName: koord-batch  # 低优先级
      containers:
        - name: processor
          image: spark:latest
          resources:
            requests:
              kubernetes.io/batch-cpu: "4000"    # 4 cores (millicores)
              kubernetes.io/batch-memory: "8Gi"
      restartPolicy: Never
```

---

## 配置详解

### CPU Burst - CPU 突发能力

```yaml
# 允许 LS 工作负载在短时间内使用超过 limit 的 CPU
metadata:
  annotations:
    koordinator.sh/cpuBurst: |
      {
        "policy": "cpuBurstOnly",
        "cpuBurstPercent": 200,
        "cfsQuotaBurstPercent": 300,
        "cfsQuotaBurstPeriodSeconds": 60
      }
```

### Gang 调度

```yaml
apiVersion: scheduling.koordinator.sh/v1alpha1
kind: PodGroup
metadata:
  name: spark-job-group
spec:
  minMember: 4  # 至少 4 个 Pod 都调度成功
  scheduleTimeoutSeconds: 600
---
apiVersion: v1
kind: Pod
metadata:
  name: spark-driver
  labels:
    koordinator.sh/pod-group: spark-job-group
spec:
  schedulerName: koord-scheduler
  containers:
    - name: driver
      image: spark-driver:latest
```

### 弹性 Quota

```yaml
apiVersion: scheduling.koordinator.sh/v1alpha1
kind: ElasticQuota
metadata:
  name: team-a-quota
  namespace: team-a
spec:
  min:
    cpu: "20"
    memory: "40Gi"
  max:
    cpu: "100"
    memory: "200Gi"
---
apiVersion: scheduling.koordinator.sh/v1alpha1
kind: ElasticQuota
metadata:
  name: team-b-quota
  namespace: team-b
spec:
  min:
    cpu: "30"
    memory: "60Gi"
  max:
    cpu: "80"
    memory: "160Gi"
```

### GPU 共享调度

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ai-inference
  labels:
    koordinator.sh/qosClass: LS
spec:
  schedulerName: koord-scheduler
  containers:
    - name: model
      image: inference-server:latest
      resources:
        limits:
          koordinator.sh/gpu-core: "50"     # 50% GPU 算力
          koordinator.sh/gpu-memory: "4Gi"   # 4GB 显存
```

### 重调度策略

```yaml
apiVersion: descheduler/v1alpha2
kind: DeschedulerConfiguration
profiles:
  - name: koord-descheduler
    plugins:
      deschedule:
        enabled:
          - name: LowNodeLoad
      balance:
        enabled:
          - name: LowNodeLoad
    pluginConfig:
      - name: LowNodeLoad
        args:
          evictableNamespaces:
            exclude:
              - kube-system
          nodeMetricExpirationSeconds: 300
          lowThresholds:
            cpu: 30
            memory: 30
          highThresholds:
            cpu: 80
            memory: 85
```

---

## 节点 SLO 配置

```yaml
apiVersion: slo.koordinator.sh/v1alpha1
kind: NodeSLO
metadata:
  name: node-slo-config
spec:
  cpuBurstStrategy:
    policy: cpuBurstOnly
    cpuBurstPercent: 200
  resourceQOSStrategy:
    lsClass:
      cpuQOS:
        groupIdentity: 2
      memoryQOS:
        wmarkRatio: 95
        wmarkScalePermill: 20
        wmarkMinAdj: -25
    beClass:
      cpuQOS:
        groupIdentity: -1
      memoryQOS:
        wmarkRatio: 80
        wmarkScalePermill: 40
        wmarkMinAdj: 50
  resourceUsedThresholdWithBE:
    cpuSuppressThresholdPercent: 65
    memoryEvictThresholdPercent: 70
```

---

## 监控

| 指标 | 说明 |
|:---|:---|
| `koordinator_node_resource_usage` | 节点实际资源利用率 |
| `koordinator_be_resource_allocated` | BE 类型资源分配量 |
| `koordinator_cpu_suppress_ratio` | CPU 压制比例 |
| `koordinator_gang_schedule_duration` | Gang 调度耗时 |

---

## 最佳实践

1. **渐进混部**: 从低资源利用率的集群开始，逐步提高 BE 工作负载比例
2. **QoS 分级**: 严格按业务重要性配置 QoS 级别，确保核心服务 SLO
3. **CPU Burst**: 为突发流量的在线服务启用 CPU Burst，减少延迟抖动
4. **资源画像**: 利用 Koordlet 收集的实际资源使用数据优化资源 request
5. **GPU 共享**: 推理服务使用 GPU 共享调度，提升 GPU 利用率
6. **弹性 Quota**: 跨团队使用弹性 Quota 允许资源借用，提高整体效率

---

## 参考资源

- [Koordinator 官方文档](https://koordinator.sh/docs/)
- [Koordinator GitHub](https://github.com/koordinator-sh/koordinator)
- [混部最佳实践](https://koordinator.sh/docs/best-practices/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## 生产实战与调优

### 典型生产场景

1. **在线/离线混部 (Co-location)** — 在同一集群中运行在线服务（延迟敏感型）和离线任务（吞吐型），通过 Koordinator 的资源画像和干扰检测，将离线任务调度到在线服务的空闲资源上，集群整体利用率从 30% 提升至 60%+。
2. **GPU 共享与拓扑感知调度** — AI 训练/推理场景下，多个小模型推理服务共享同一张 GPU，Koordinator 通过 `GPUShare` 和 NUMA Topology 感知调度，避免 GPU 显存碎片化。
3. **QoS 混合部署** — 将 Pod 分为 LSE (Latency Sensitive Explicit)、LS (Latency Sensitive)、BE (Best Effort) 三级 QoS，在资源紧张时优先驱逐 BE 保障 LS/LSE。
4. **资源画像与动态超卖** — 基于历史实际用量自动调整 Pod 的 request/limit，实现安全超卖。

### 配置调优参数

```yaml
# Koordinator Scheduler 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: koord-scheduler-config
data:
  koord-scheduler.yaml: |
    profiles:
      - schedulerName: koord-scheduler
        plugins:
          coscheduling:
            enabled: true
          loadAwareScheduling:
            enabled: true
            args:
              estimatedScalingFactors:
                cpu: 85        # CPU 实际用量预估系数（%）
                memory: 70     # Memory 实际用量预估系数（%）
              utilizationThresholds:
                cpu: 65        # 节点 CPU 利用率阈值，超过则不调度新 Pod
                memory: 75     # 节点 Memory 利用率阈值

# 资源超卖配置
apiVersion: config.koordinator.sh/v1alpha1
kind: ResourceQOS
metadata:
  name: be-qos
spec:
  resourceQOS:
    - qosClass: BE
      cpuPolicy:
        cpuset: "4-31"         # BE 任务绑定到指定 CPU 核
      memoryPolicy:
        oomPriority: -999      # BE OOM 优先级最低
```

关键调优点：
- `estimatedScalingFactors`：根据实际业务画像调整，CPU 密集型可设 90%，IO 密集型可设 60%
- `utilizationThresholdes`：生产环境 CPU 建议 60-70%，Memory 建议 70-80%
- `ResourceInterpreter`：自定义 CRD 的资源解读器，支持 Gang Scheduling

### 性能基准数据（参考值）

| 场景 | 集群规模 | 混部前利用率 | 混部后利用率 | 在线延迟影响 |
|------|----------|-------------|-------------|-------------|
| 在线+离线 CPU 混部 | 200 节点 | 25-35% | 55-65% | P99 增加 < 5% |
| GPU 共享推理 | 50 GPU | 40% (独占) | 75% (共享) | 吞吐提升 1.5x |
| 大规模混部 | 1000 节点 | 30% | 60% | P99 增加 < 10% |

> 注：实际效果取决于在线服务的延迟敏感程度和资源波动模式，建议先在非核心业务验证。

### 常见坑和注意事项

1. **干扰检测延迟** — Koordinator 的干扰检测默认基于 1 分钟滑动窗口，突发流量场景下反应不够快。建议配合 CPU CFS bandwidth throttling 监控。
2. **GPU 共享需配合 Device Plugin** — 必须安装 Koordinator 提供的 GPU Device Plugin，原生 NVIDIA Device Plugin 不支持显存级别的共享。
3. **BE 任务饥饿** — 如果在线服务长期高负载，BE 任务可能被完全驱逐，导致离线任务永远无法完成。建议设置 BE 最小资源保障。
4. **cgroup v2 兼容性** — Koordinator 依赖 cgroup v2 的 PSI (Pressure Stall Information) 做干扰检测，需确认节点内核版本 >= 5.10 且启用了 cgroup v2。
5. **Gang Scheduling 死锁** — coscheduling 插件配置不当可能导致 Pod 组永远无法全部调度，注意设置合理的 `minMember` 和 `timeout`。
