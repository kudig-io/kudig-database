---
title: KubeStellar
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KubeStellar 是什么
- 如何 KubeStellar
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KubeStellar
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

title: KubeStellar
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
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
- KubeStellar 是什么
- 如何 KubeStellar
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KubeStellar
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# KubeStellar

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kubestellar.io/ |
| **GitHub** | https://github.com/kubestellar/kubestellar |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KubeStellar 是一个多集群配置管理和工作负载分发平台，专注于将 Kubernetes 资源从中心控制面高效地分发到大量边缘集群。它采用 kcp（Kubernetes-like Control Plane）作为核心，支持管理数千个集群，特别适合边缘计算、零售、IoT 等需要管理大量分布式集群的场景。

### 核心特性

- **大规模边缘**: 设计支持数千个边缘集群的统一管理
- **kcp 核心**: 基于 kcp 提供逻辑多租户和 API 隔离
- **Binding Policy**: 声明式策略定义资源与集群的绑定关系
- **状态汇总**: 从边缘集群汇总资源状态到控制面
- **增量同步**: 仅同步变化的资源，降低网络开销
- **松耦合**: 边缘集群可在断网情况下独立运行

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│               KubeStellar Core (Hub)                  │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │                  kcp                          │    │
│  │  ┌──────────────────────────────────────────┐│    │
│  │  │         Workload Management WS            ││    │
│  │  │  ┌────────────┐  ┌────────────────────┐ ││    │
│  │  │  │ BindingPolicies │ │ Kubernetes Objects │ ││    │
│  │  │  └────────────┘  └────────────────────┘ ││    │
│  │  └──────────────────────────────────────────┘│    │
│  └──────────────────────────────────────────────┘    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │              KubeStellar Controllers          │    │
│  │  ┌──────────┐ ┌─────────────┐ ┌────────────┐│    │
│  │  │Placement │ │ Transport   │ │ Status     ││    │
│  │  │Controller│ │ Controller  │ │ Aggregator ││    │
│  │  └──────────┘ └─────────────┘ └────────────┘│    │
│  └─────────────────────┬────────────────────────┘    │
└────────────────────────┼─────────────────────────────┘
                         │
       ┌─────────────────┼─────────────────┐
       │                 │                 │
 ┌─────▼─────┐    ┌──────▼─────┐    ┌─────▼──────┐
 │ Edge WEC   │    │ Edge WEC   │    │ Edge WEC   │
 │ Cluster 1  │    │ Cluster 2  │    │ Cluster N  │
 │ ┌────────┐ │    │ ┌────────┐ │    │ ┌────────┐ │
 │ │ Agent  │ │    │ │ Agent  │ │    │ │ Agent  │ │
 │ └────┬───┘ │    │ └────┬───┘ │    │ └────┬───┘ │
 │ ┌────▼───┐ │    │ ┌────▼───┐ │    │ ┌────▼───┐ │
 │ │Workloads│ │    │ │Workloads│ │    │ │Workloads│ │
 │ └────────┘ │    │ └────────┘ │    │ └────────┘ │
 └────────────┘    └────────────┘    └────────────┘
```

---

## 快速开始

### 安装 KubeStellar

```bash
# 安装 kcp
curl -fsSL https://github.com/kcp-dev/kcp/releases/download/v0.11.0/kcp_0.11.0_linux_amd64.tar.gz | tar xz
sudo mv kcp /usr/local/bin/

# 启动 kcp
kcp start &

# 安装 KubeStellar
helm repo add kubestellar https://kubestellar.io/charts
helm install kubestellar kubestellar/kubestellar \
  --namespace kubestellar-system \
  --create-namespace
```

### 注册边缘集群

```yaml
# wec-registration.yaml
apiVersion: edge.kubestellar.io/v1alpha1
kind: WorkloadExecutionCluster
metadata:
  name: edge-store-001
  labels:
    location: us-west
    type: retail
spec:
  kubeconfig:
    secretRef:
      name: edge-store-001-kubeconfig
      namespace: kubestellar-system
```

```bash
kubectl apply -f wec-registration.yaml
```

### 创建绑定策略

```yaml
# binding-policy.yaml
apiVersion: edge.kubestellar.io/v1alpha1
kind: BindingPolicy
metadata:
  name: retail-app-binding
spec:
  # 选择要分发的工作负载
  workloadSelectors:
    - labelSelector:
        matchLabels:
          app: retail-pos
  
  # 选择目标集群
  clusterSelectors:
    - labelSelector:
        matchLabels:
          type: retail
  
  # 分发模式
  policy:
    downsync:
      - kinds:
          - group: apps
            version: v1
            kind: Deployment
          - group: ""
            version: v1
            kind: Service
            kind: ConfigMap
```

### 部署工作负载

```yaml
# retail-app.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: retail-pos
  labels:
    app: retail-pos  # 匹配 BindingPolicy
spec:
  replicas: 1
  selector:
    matchLabels:
      app: retail-pos
  template:
    metadata:
      labels:
        app: retail-pos
    spec:
      containers:
        - name: pos
          image: retail/pos-system:v1.0
          ports:
            - containerPort: 8080
```

```bash
kubectl apply -f retail-app.yaml

# 工作负载会自动分发到所有匹配的边缘集群
```

---

## 高级功能

### 状态汇总

```yaml
# 配置从边缘集群汇总状态
apiVersion: edge.kubestellar.io/v1alpha1
kind: BindingPolicy
metadata:
  name: retail-app-binding
spec:
  workloadSelectors:
    - labelSelector:
        matchLabels:
          app: retail-pos
  clusterSelectors:
    - labelSelector:
        matchLabels:
          type: retail
  policy:
    downsync:
      - kinds:
          - group: apps
            version: v1
            kind: Deployment
    # 状态上报配置
    statusCollectors:
      - group: apps
        version: v1
        kind: Deployment
        statusFields:
          - availableReplicas
          - readyReplicas
          - conditions
```

### 差异化配置

```yaml
# 为不同区域的集群应用差异化配置
apiVersion: edge.kubestellar.io/v1alpha1
kind: CustomTransform
metadata:
  name: us-west-transform
spec:
  clusterSelector:
    matchLabels:
      location: us-west
  
  transforms:
    - group: apps
      version: v1
      kind: Deployment
      patch:
        type: JSONPatch
        value: |
          [
            {"op": "replace", "path": "/spec/replicas", "value": 3}
          ]
---
apiVersion: edge.kubestellar.io/v1alpha1
kind: CustomTransform
metadata:
  name: eu-transform
spec:
  clusterSelector:
    matchLabels:
      location: eu
  
  transforms:
    - group: ""
      version: v1
      kind: ConfigMap
      patch:
        type: JSONPatch
        value: |
          [
            {"op": "add", "path": "/data/region", "value": "EU"}
          ]
```

### 大规模集群管理

```bash
# 查看所有边缘集群状态
kubectl get wec
# NAME              READY   AGE
# edge-store-001    True    5d
# edge-store-002    True    5d
# edge-store-003    True    4d
# ...

# 查看工作负载分发状态
kubectl get bindings
# NAME                  WORKLOADS   CLUSTERS   SYNCED
# retail-app-binding    5           1000       1000
```

---

## 与其他方案对比

| 特性 | KubeStellar | OCM | Karmada | Rancher Fleet |
|:---|:---|:---|:---|:---|
| 核心架构 | kcp | Hub-Agent | Push/Pull | GitOps |
| 目标规模 | 数千集群 | 数百集群 | 数百集群 | 数百集群 |
| API 隔离 | kcp 工作空间 | ManagedCluster | 无 | 无 |
| 状态汇总 | 内置 | Work API | 内置 | 有限 |
| 边缘优化 | 增量同步 | 完整同步 | 完整同步 | Git Diff |
| 断网容忍 | 高 | 中 | 中 | 低 |

---

## 最佳实践

1. **集群标签**: 建立统一的边缘集群标签体系（location, type, tier）
2. **渐进分发**: 使用 BindingPolicy 的集群选择器逐步扩大分发范围
3. **状态监控**: 配置状态汇总，在控制面统一监控所有边缘集群状态
4. **断网设计**: 边缘应用设计为可在断网情况下独立运行
5. **版本控制**: 使用 Workspace 隔离不同版本的工作负载配置

---

## 参考资源

- [KubeStellar 官方文档](https://kubestellar.io/docs/)
- [KubeStellar GitHub](https://github.com/kubestellar/kubestellar)
- [kcp 项目](https://kcp.io/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/gitops.md|gitops]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
