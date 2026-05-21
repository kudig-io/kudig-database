---
title: 工作负载控制器选型
description: '## 概述'
category: skills
tags:
- k8s
- deployment
- statefulset
- daemonset
- workload-selection
- controller-comparison
- redis
- mysql
- kafka
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 工作负载控制器选型 是什么
- 如何 工作负载控制器选型
trigger_keywords:
- 工作负载控制器选型
prerequisites:
- kubectl-basics
- kafka-basics
- redis-basics
- mysql-basics
- logging-basics
---

# 工作负载控制器选型

## 概述

Kubernetes 提供多种工作负载控制器，每种适用于不同的业务场景。准确选择控制器类型是设计 Kubernetes 应用架构的基础。

## 三大控制器核心对比

| 维度 | Deployment | StatefulSet | DaemonSet |
|------|-----------|-------------|-----------|
| **Pod 身份** | 随机 hash 后缀 | 稳定有序编号（-0, -1, -2） | 节点绑定（每节点一个） |
| **Pod 名称** | `web-7d9f6c-xk9wl` | `db-0`, `db-1`, `db-2` | `fluentd-node1` |
| **存储** | 共享 PVC 或无状态 | 每 Pod 独立 PVC（VolumeClaimTemplate） | 通常挂载节点本地路径 |
| **网络** | 通过 Service 统一入口 | 每 Pod 独立 DNS（Headless Service） | 每节点独立访问 |
| **启动顺序** | 随机并行 | 严格顺序（0→1→2） | 随节点就绪 |
| **滚动更新** | 自由并行 | 逆序更新（2→1→0） | 节点逐个更新 |
| **扩缩容** | 任意副本数 | 有序扩容/逆序缩容 | 随节点数自动调整 |
| **典型场景** | 无状态微服务 | 数据库、消息队列 | 日志采集、监控代理 |

## 选型决策树

```
我的应用需要什么?
  │
  ├── 每个节点都需要运行?
  │   └── 是 → DaemonSet（日志/监控/网络代理）
  │
  ├── Pod 之间是否可互换?
  │   ├── 是 → 需要持久化存储?
  │   │   ├── 否 → Deployment（Web/API 无状态服务）
  │   │   └── 是但共享存储 → Deployment + PVC（只读共享）
  │   │
  │   └── 否 → Pod 需要稳定身份?
  │       ├── 是 → 数据库/有状态集群?
  │       │   ├── 是 → StatefulSet（MySQL/Redis/Kafka）
  │       │   └── 否，需有序编号 → StatefulSet（主从/分片）
  │       └── 否 → 需要按顺序启动?
  │           ├── 是 → StatefulSet（启动有依赖关系）
  │           └── 否 → Deployment
```

## 配置对比

### Deployment（无状态 Web 服务）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-api
spec:
  replicas: 5
  selector:
    matchLabels:
      app: web-api
  template:
    spec:
      containers:
      - name: web
        image: myapp:v1.0.0
```

### StatefulSet（MySQL 主从集群）

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql-headless  # 必须指定 Headless Service
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Gi
---
# Headless Service：每 Pod 独立 DNS
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
spec:
  clusterIP: None
  selector:
    app: mysql
```

### DaemonSet（节点日志采集）

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluentd
spec:
  selector:
    matchLabels:
      app: fluentd
  template:
    spec:
      tolerations:
      - effect: NoSchedule
        operator: Exists
      containers:
      - name: fluentd
        image: fluent/fluentd:v1.16
        volumeMounts:
        - name: varlog
          mountPath: /var/log
      volumes:
      - name: varlog
        hostPath:
          path: /var/log
```

## 常见误用场景

| 误用 | 正确选择 | 原因 |
|------|---------|------|
| 用 Deployment 部署 Redis 集群 | StatefulSet | Redis Cluster 需要稳定 Pod 名称和独立存储 |
| 用 StatefulSet 部署无状态 API | Deployment | 不需要顺序保证，StatefulSet 更新更慢 |
| 用 Deployment 部署 CNI Agent | DaemonSet | 需要在每个节点运行且随节点变化自动管理 |
| 用 DaemonSet 部署普通 Web 服务 | Deployment | DaemonSet 不支持 replicas 控制 |

## 资源消耗对比

| 特性 | Deployment | StatefulSet | DaemonSet |
|------|-----------|-------------|-----------|
| 控制器开销 | 通过 RS 二层管理 | 直接管理 Pod | 直接管理 Pod |
| 存储开销 | 共享或无 | 每 Pod 独立 PVC | 节点 HostPath |
| 扩缩速度 | 快（并行） | 慢（顺序） | 自动跟随节点 |
| 更新停机时间 | 零（默认） | 零（逆序滚动） | 节点级别 |

## 版本说明

- StatefulSet 自 v1.9 起 GA
- DaemonSet 自 v1.2 起稳定
- 基于 Kubernetes v1.28 – v1.32

## 相关技能

- [[skills/deployment-rolling-update.md|Deployment 滚动更新策略]]
- [[skills/deployment-canary-and-bluegreen.md|金丝雀与蓝绿发布]]
- [[deployment|Deployment]]
- [[entities/statefulset.md|StatefulSet]]

## Related

- [[fluentd]] — Fluentd
- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-07-platform-engineering/topic-code-analysis/deployment-create/10-workload-comparison.md|Deployment vs StatefulSet vs DaemonSet 选型指南]]