# Karmada

> **成熟度**: Incubating | **加入时间**: 2022-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://karmada.io |
| **GitHub** | https://github.com/karmada-io/karmada |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Orchestration & Multi-cluster |

---

## 项目概述

Karmada（Kubernetes Armada）是开放的多云多集群 Kubernetes 管理系统。它提供统一的 API 来管理跨多个 Kubernetes 集群的工作负载，支持跨集群调度、故障转移和策略驱动的资源分发。

## 核心特性

- **多集群管理**: 统一管理多个 Kubernetes 集群
- **跨集群调度**: 基于策略的工作负载分发
- **故障转移**: 自动检测集群故障并迁移工作负载
- **Kubernetes 原生**: 完全兼容 Kubernetes API
- **集群联邦**: 统一的资源视图和管理
- **多云支持**: 支持公有云、私有云、边缘集群

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Karmada Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   Karmada Control Plane                    │ │
│  │                                                            │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │  karmada-   │  │  karmada-   │  │   karmada-      │   │ │
│  │  │  apiserver  │  │  controller │  │   scheduler     │   │ │
│  │  │             │  │  manager    │  │                 │   │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │ │
│  │                                                            │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │   etcd      │  │  karmada-   │  │   karmada-      │   │ │
│  │  │             │  │  webhook    │  │   aggregated-   │   │ │
│  │  │             │  │             │  │   apiserver     │   │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                    Push/Pull Resources                           │
│                              │                                   │
│  ┌───────────────────────────┼───────────────────────────────┐  │
│  │                           ▼                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │  │
│  │  │  Cluster 1  │  │  Cluster 2  │  │   Cluster N     │  │  │
│  │  │  (AWS EKS)  │  │  (Azure AKS)│  │   (On-Premise)  │  │  │
│  │  │             │  │             │  │                 │  │  │
│  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │  ┌─────────┐   │  │  │
│  │  │ │karmada- │ │  │ │karmada- │ │  │  │karmada- │   │  │  │
│  │  │ │ agent   │ │  │ │ agent   │ │  │  │ agent   │   │  │  │
│  │  │ └─────────┘ │  │ └─────────┘ │  │  └─────────┘   │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │  │
│  │                   Member Clusters                         │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心概念

| 概念 | 说明 |
|------|------|
| PropagationPolicy | 定义资源如何分发到成员集群 |
| OverridePolicy | 定义资源在不同集群的差异化配置 |
| ResourceBinding | 资源与集群的绑定关系 |
| Work | 下发到成员集群的实际工作负载 |
| Cluster | 成员集群的表示 |

---

## 快速开始

### 安装 Karmada 控制面

```bash
# 使用 kubectl karmada 插件
kubectl krew install karmada

# 初始化 Karmada（在 host 集群）
kubectl karmada init

# 或使用 Helm
helm repo add karmada-charts https://karmada-io.github.io/karmada-helm-charts
helm install karmada karmada-charts/karmada \
  --namespace karmada-system \
  --create-namespace
```

### 注册成员集群

```bash
# Push 模式（控制面推送到成员集群）
kubectl karmada join member1 \
  --kubeconfig=/path/to/member1-kubeconfig \
  --cluster-kubeconfig=/path/to/member1-kubeconfig

# Pull 模式（成员集群主动拉取）
# 1. 在成员集群部署 agent
kubectl apply -f https://raw.githubusercontent.com/karmada-io/karmada/master/artifacts/agent/karmada-agent.yaml

# 查看成员集群
kubectl get clusters
```

### 分发工作负载

```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
  namespace: default
spec:
  replicas: 3
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
        image: nginx:latest
---
# propagation-policy.yaml
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: nginx-propagation
  namespace: default
spec:
  resourceSelectors:
    - apiVersion: apps/v1
      kind: Deployment
      name: nginx
  placement:
    clusterAffinity:
      clusterNames:
        - member1
        - member2
    replicaScheduling:
      replicaDivisionPreference: Weighted
      replicaSchedulingType: Divided
      weightPreference:
        staticWeightList:
          - targetCluster:
              clusterNames:
                - member1
            weight: 2
          - targetCluster:
              clusterNames:
                - member2
            weight: 1
```

---

## 调度策略

### 副本调度

```yaml
# 平均分配
placement:
  replicaScheduling:
    replicaSchedulingType: Divided
    replicaDivisionPreference: Aggregated

# 权重分配
placement:
  replicaScheduling:
    replicaSchedulingType: Divided
    replicaDivisionPreference: Weighted
    weightPreference:
      staticWeightList:
        - targetCluster:
            clusterNames: [cluster1]
          weight: 2
        - targetCluster:
            clusterNames: [cluster2]
          weight: 1

# 复制模式（每个集群都部署完整副本）
placement:
  replicaScheduling:
    replicaSchedulingType: Duplicated
```

### 集群亲和性

```yaml
placement:
  clusterAffinity:
    # 指定集群
    clusterNames:
      - cluster1
      - cluster2
    # 标签选择
    labelSelector:
      matchLabels:
        region: us-west
    # 字段选择
    fieldSelector:
      matchExpressions:
        - key: provider
          operator: In
          values:
            - aws
            - azure
```

### 故障转移

```yaml
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: nginx-propagation
spec:
  resourceSelectors:
    - apiVersion: apps/v1
      kind: Deployment
      name: nginx
  placement:
    clusterAffinity:
      clusterNames:
        - member1
        - member2
    spreadConstraints:
      - maxGroups: 2
        minGroups: 1
  failover:
    application:
      decisionConditions:
        tolerationSeconds: 300
      purgeMode: Gracefully
```

---

## 差异化配置

### OverridePolicy

```yaml
apiVersion: policy.karmada.io/v1alpha1
kind: OverridePolicy
metadata:
  name: nginx-override
  namespace: default
spec:
  resourceSelectors:
    - apiVersion: apps/v1
      kind: Deployment
      name: nginx
  overrideRules:
    - targetCluster:
        clusterNames:
          - member1
      overriders:
        plaintext:
          - path: "/spec/replicas"
            operator: replace
            value: 5
        imageOverrider:
          - component: Tag
            operator: replace
            value: "1.21"
    - targetCluster:
        clusterNames:
          - member2
      overriders:
        plaintext:
          - path: "/spec/replicas"
            operator: replace
            value: 3
```

---

## 多集群服务发现

```yaml
# 启用 MultiClusterService
apiVersion: networking.karmada.io/v1alpha1
kind: MultiClusterService
metadata:
  name: nginx-mcs
  namespace: default
spec:
  types:
    - CrossCluster
  range:
    clusterNames:
      - member1
      - member2
```

---

## 监控与状态

```bash
# 查看资源分发状态
kubectl get resourcebinding -A

# 查看工作状态
kubectl get work -A

# 查看集群状态
kubectl get clusters -o wide

# 查看调度结果
kubectl describe propagationpolicy nginx-propagation
```

---

## 最佳实践

1. **集群分组**: 使用标签对集群分组（区域、环境、云厂商）
2. **渐进式迁移**: 从非关键工作负载开始逐步迁移
3. **故障转移测试**: 定期验证故障转移能力
4. **资源配额**: 在控制面配置跨集群资源配额
5. **网络规划**: 确保集群间网络连通性

---

## 参考资源

- [官方文档](https://karmada.io/docs)
- [GitHub Repo](https://github.com/karmada-io/karmada)
- [用户案例](https://karmada.io/docs/casestudies/)
- [API 参考](https://karmada.io/docs/reference/api/)

---

**维护者**: Kudig Team | **许可证**: MIT
