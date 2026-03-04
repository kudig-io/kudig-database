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
