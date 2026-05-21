---
title: Karmada
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- etcd
- apiserver
- scheduler
- controller-manager
- helm
- opa
- rbac
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Karmada 是什么
- 如何 Karmada
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Karmada
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- etcd-basics
- policy-basics
---

title: Karmada
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- apiserver
- scheduler
- helm
- opa
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Karmada 是什么
- 如何 Karmada
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Karmada
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

## 生产实战与调优

### 典型生产场景

1. **多云/混合云统一管理** — 将 AWS EKS、阿里云 ACK、自建 IDC K8s 通过 Karmada 统一编排，使用 PropagationPolicy 将应用按地域/成本策略分发到不同云。
2. **跨集群故障转移 (Failover)** — 配合 `FailoverBehavior`，当某集群不可用时自动将工作负载迁移到备用集群，实现跨区域 HA。
3. **灰度发布与流量调配** — 使用 OverridePolicy 对不同集群设置不同的镜像版本或副本数，实现跨集群灰度。
4. **多租户隔离管理** — 不同业务团队通过 Karmada 的 namespace 级别 PropagationPolicy 管理各自的应用部署范围。
5. **边缘计算分发** — 将边缘节点注册为独立的 Karmada member cluster，通过 Karmada 统一分发配置和工作负载。

### 配置调优参数

```yaml
# PropagationPolicy - 跨集群调度核心配置
apiVersion: policy.karmada.io/v1alpha1
kind: PropagationPolicy
metadata:
  name: multi-cluster-app
spec:
  resourceSelectors:
    - apiVersion: apps/v1
      kind: Deployment
      name: my-app
  placement:
    clusterAffinity:
      clusterNames:
        - cluster-beijing
        - cluster-shanghai
        - cluster-guangzhou
    replicaScheduling:
      replicaDivisionPreference: Weighted
      replicaSchedulingPreference: Aggregated  # 或 Weighted
      weightPreference:
        staticWeightList:
          - targetCluster:
              clusterNames: [cluster-beijing]
            weight: 50
          - targetCluster:
              clusterNames: [cluster-shanghai]
            weight: 30
          - targetCluster:
              clusterNames: [cluster-guangzhou]
            weight: 20
    failover:
      application:
        decisionConditions:
          tolerationSeconds: 300   # 集群不可用 300s 后触发迁移
        gracePeriodSeconds: 600    # 优雅迁移等待时间
        purgeMode: Graciously      # 优雅删除旧副本

# Karmada 控制面资源限制
# karmada-controller-manager
#   --concurrent-work-syncs=5        # 默认 5，大集群可增至 10-20
#   --concurrent-resource-syncs=5    # 资源同步并发数
#   --concurrent-cluster-syncs=2     # 集群状态同步并发数
```

关键调优点：
- `concurrent-work-syncs`：Work 对象同步并发数，集群数量 > 50 时建议提高到 10-20
- `tolerationSeconds`：故障转移容忍时间，太短会因网络抖动误触发，建议 300-600s
- `resync` 间隔：资源同步默认 0（event-driven），大规模场景可设置定时 resync 作为兜底

### 性能基准数据（参考值）

| 管理集群规模 | 工作负载数量 | Propagation 延迟 | 控制面 CPU | 控制面内存 |
|-------------|------------|-----------------|-----------|-----------|
| 5 集群 | 1000 Deployment | < 2s | 2 core | 4Gi |
| 20 集群 | 5000 Deployment | < 5s | 4 core | 8Gi |
| 50 集群 | 10000 Deployment | < 10s | 8 core | 16Gi |
| 100 集群 | 20000 Deployment | < 30s | 16 core | 32Gi |

> 注：Propagation 延迟指从 PropagationPolicy 创建到目标集群上资源就绪的时间，包含 API 调用和网络延迟。

### 常见坑和注意事项

1. **RBAC 配置复杂** — Karmada 需要通过 ServiceAccount 或 Token 访问每个 member cluster，生产环境建议使用 ServiceAccount Token（非 kubeconfig 文件），并定期轮转。
2. **资源冲突** — 如果 member cluster 上已有同名资源（手动创建的），Propagation 会失败。建议通过 Karmada 统一管理，避免手动干预 member cluster。
3. **etcd 压力集中** — Karmada 控制面的 etcd 存储所有集群的资源模板，当管理集群数 > 50 时 etcd 需要专用 NVMe SSD 和充足的内存。
4. **网络分区处理** — Karmada 依赖 kube-apiserver 的健康检查判断集群可用性，跨云场景下网络抖动可能导致误判。建议配置合理的 `Cluster.HealthyThreshold`。
5. **OverridePolicy 优先级** — 多个 OverridePolicy 可能冲突，按创建时间排序，后创建的覆盖先创建的。建议使用 `ResourceSelector` 精确匹配，避免全量 Override。
6. **控制面单点** — Karmada 控制面自身需要 HA 部署（多副本 + etcd 集群），否则控制面故障会导致全局调度失灵。

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/armada.md|armada]]
- [[synthesis/etcd x 高可用模式|etcd × 高可用模式]] — Cross-reference
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
