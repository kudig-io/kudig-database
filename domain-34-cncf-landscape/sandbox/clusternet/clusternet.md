---
title: Clusternet
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- apiserver
- scheduler
- grafana
- helm
- crd
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
- Clusternet 是什么
- 如何 Clusternet
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Clusternet
- cncf
- landscape
---

# Clusternet

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://clusternet.io/ |
| **GitHub** | https://github.com/clusternet/clusternet |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Clusternet 是一个多集群管理和应用分发平台，专为管理跨云、跨区域的 Kubernetes 集群而设计。它采用 Hub-Agent 架构，支持 Pull 和 Push 两种模式进行集群注册，能够将应用资源（Deployment、Service、Helm Release 等）智能分发到多个子集群。Clusternet 特别适合边缘计算和混合云场景，即使子集群位于 NAT 或防火墙后面也能正常管理。

### 核心特性

- **多模式集群注册**: 支持 Push (从 Hub 主动连接) 和 Pull (Agent 主动连接) 两种模式
- **应用多集群分发**: 通过 Subscription CRD 将 Kubernetes 资源分发到多个集群
- **Helm 原生支持**: 直接分发 Helm Chart，支持多集群差异化 values 配置
- **Shadow API**: 在 Hub 集群中暴露子集群 API，实现透明的跨集群资源访问
- **边缘友好**: Agent 主动发起连接，穿透 NAT/防火墙，适合边缘场景
- **集群联邦**: 统一管理异构集群（不同 K8s 版本、不同云厂商）

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  Hub Cluster (Parent)                  │
│                                                       │
│  ┌──────────────┐ ┌─────────────┐ ┌──────────────┐  │
│  │ clusternet-  │ │ clusternet- │ │ clusternet-  │  │
│  │ hub          │ │ scheduler   │ │ controller   │  │
│  │ (API聚合)    │ │ (调度策略)  │ │ (分发控制)   │  │
│  └──────┬───────┘ └──────┬──────┘ └──────┬───────┘  │
│         │                │                │          │
│  ┌──────▼────────────────▼────────────────▼───────┐ │
│  │              CRD Resources                       │ │
│  │  ManagedCluster | Subscription | HelmRelease    │ │
│  │  Localization  | Globalization | Base           │ │
│  └─────────────────────┬───────────────────────────┘ │
└────────────────────────┼─────────────────────────────┘
                         │
          ┌──────────────┼──────────────┐
          │   WebSocket  │   WebSocket  │
          │   (Pull)     │   (Pull)     │
   ┌──────▼──────┐ ┌─────▼───────┐ ┌────▼─────────┐
   │Child Cluster│ │Child Cluster│ │Child Cluster │
   │    (edge)   │ │   (cloud)   │ │   (on-prem)  │
   │┌───────────┐│ │┌───────────┐│ │┌───────────┐ │
   ││clusternet-││ ││clusternet-││ ││clusternet-│ │
   ││agent      ││ ││agent      ││ ││agent      │ │
   │└───────────┘│ │└───────────┘│ │└───────────┘ │
   │┌───────────┐│ │┌───────────┐│ │┌───────────┐ │
   ││Workloads  ││ ││Workloads  ││ ││Workloads  │ │
   │└───────────┘│ │└───────────┘│ │└───────────┘ │
   └─────────────┘ └─────────────┘ └──────────────┘
```

---

## 快速开始

### 安装 Hub 组件

```bash
# 在 Hub 集群安装
helm repo add clusternet https://clusternet.github.io/charts
helm install clusternet-hub clusternet/clusternet-hub \
  --namespace clusternet-system \
  --create-namespace
```

### 注册子集群 (Pull 模式)

```bash
# 在子集群安装 Agent
# 获取 Hub 连接信息
export HUB_APISERVER=https://hub-apiserver:6443
export CLUSTER_NAME=edge-cluster-01
export TOKEN=<bootstrap-token>

helm install clusternet-agent clusternet/clusternet-agent \
  --namespace clusternet-system \
  --create-namespace \
  --set parentURL=$HUB_APISERVER \
  --set registrationToken=$TOKEN \
  --set clusterID=$CLUSTER_NAME
```

### 在 Hub 查看注册的集群

```bash
kubectl get managedclusters
# NAME               STATUS   AGE
# edge-cluster-01    True     5m
# cloud-cluster-02   True     3m
```

### 分发应用到多集群

```yaml
# subscription.yaml - 将 Deployment 分发到所有边缘集群
apiVersion: apps.clusternet.io/v1alpha1
kind: Subscription
metadata:
  name: nginx-app
  namespace: default
spec:
  subscribers:
    - clusterAffinity:
        matchLabels:
          location: edge
  feeds:
    - apiVersion: apps/v1
      kind: Deployment
      name: nginx
      namespace: default
    - apiVersion: v1
      kind: Service
      name: nginx-svc
      namespace: default
---
# 源资源定义
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
  namespace: default
spec:
  replicas: 2
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
        - name: nginx
          image: nginx:1.25
```

---

## 高级功能

### Helm Chart 多集群分发

```yaml
# helm-subscription.yaml
apiVersion: apps.clusternet.io/v1alpha1
kind: Subscription
metadata:
  name: grafana-deployment
  namespace: default
spec:
  subscribers:
    - clusterAffinity:
        matchLabels:
          tier: monitoring
  feeds:
    - apiVersion: apps.clusternet.io/v1alpha1
      kind: HelmChart
      name: grafana
      namespace: default
---
apiVersion: apps.clusternet.io/v1alpha1
kind: HelmChart
metadata:
  name: grafana
  namespace: default
spec:
  repo: https://grafana.github.io/helm-charts
  chartPullSecret:
    name: ""
    namespace: ""
  chart: grafana
  version: 7.0.0
  targetNamespace: monitoring
```

### 差异化配置 (Localization)

```yaml
# 为不同集群应用差异化配置
apiVersion: apps.clusternet.io/v1alpha1
kind: Localization
metadata:
  name: nginx-edge-config
  namespace: default
spec:
  priority: 100
  feed:
    apiVersion: apps/v1
    kind: Deployment
    name: nginx
    namespace: default
  overridePolicy: ApplyLater
  overrides:
    - name: edge-replicas
      type: JSONPatch
      value: |
        [
          {"op": "replace", "path": "/spec/replicas", "value": 1}
        ]
  clusterAffinity:
    matchLabels:
      location: edge
---
apiVersion: apps.clusternet.io/v1alpha1
kind: Localization
metadata:
  name: nginx-cloud-config
  namespace: default
spec:
  priority: 100
  feed:
    apiVersion: apps/v1
    kind: Deployment
    name: nginx
    namespace: default
  overrides:
    - name: cloud-replicas
      type: JSONPatch
      value: |
        [
          {"op": "replace", "path": "/spec/replicas", "value": 5}
        ]
  clusterAffinity:
    matchLabels:
      location: cloud
```

### Shadow API - 透明访问子集群

```bash
# 通过 Hub 直接访问子集群资源
kubectl --context hub get pods \
  --cluster edge-cluster-01 \
  -n default

# 支持所有 kubectl 操作
kubectl --context hub logs pod/nginx-xxx \
  --cluster edge-cluster-01
```

---

## 与其他方案对比

| 特性 | Clusternet | Karmada | OCM | Liqo |
|:---|:---|:---|:---|:---|
| 架构模式 | Hub-Agent | Push/Pull | Hub-Agent | Peer-to-Peer |
| 集群注册 | Push + Pull | Push | Pull | 自动发现 |
| Helm 支持 | 原生 | 需扩展 | 需扩展 | 不支持 |
| Shadow API | 支持 | 不支持 | Work API | 不支持 |
| 边缘场景 | WebSocket 穿透 | 需配置 | 支持 | 支持 |
| 差异化配置 | Localization | Override Policy | ManifestWork | 有限 |

---

## 最佳实践

1. **边缘优先 Pull 模式**: 边缘集群通常位于 NAT 后，使用 Agent 主动连接 Hub
2. **标签规范**: 统一集群标签体系 (location, tier, env)，便于 Subscription 选择
3. **Hub 高可用**: Hub 集群部署多副本，配置持久化存储
4. **渐进式分发**: 先通过标签选择少量集群验证，再扩大分发范围
5. **监控 Agent 状态**: 监控 ManagedCluster 的 conditions，及时发现断连集群

---

## 参考资源

- [Clusternet 官方文档](https://clusternet.io/docs/)
- [Clusternet GitHub](https://github.com/clusternet/clusternet)
- [Clusternet 设计文档](https://github.com/clusternet/clusternet/blob/main/docs/design.md)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
