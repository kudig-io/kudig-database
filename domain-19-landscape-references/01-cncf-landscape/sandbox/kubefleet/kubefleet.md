---
title: KubeFleet
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- crd
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KubeFleet 是什么
- 如何 KubeFleet
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KubeFleet
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

title: KubeFleet
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
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
- KubeFleet 是什么
- 如何 KubeFleet
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KubeFleet
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
# KubeFleet

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kubefleet.io/ |
| **GitHub** | https://github.com/kubefleet/kubefleet |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KubeFleet 是一个多集群资源编排平台，提供跨 Kubernetes 集群的工作负载分发、配置管理和策略驱动的资源放置能力。它通过 Hub-Member 架构和声明式 Placement 策略，实现将 Kubernetes 资源（Deployment、Service、ConfigMap 等）自动分发到多个成员集群，并支持基于集群属性、资源可用性和自定义策略的智能调度。

### 核心特性

- **声明式 Placement**: 使用 ClusterResourcePlacement CRD 定义跨集群资源分发策略
- **多种调度策略**: 支持 PickAll、PickN、PickFixed 等多种集群选择策略
- **配置覆盖**: 通过 ClusterResourceOverride 实现按集群差异化配置
- **集群属性调度**: 基于集群标签、资源容量、拓扑等属性进行智能放置
- **渐进式发布**: 支持滚动更新策略，逐步将变更应用到多个集群
- **状态聚合**: 统一查看跨集群资源的部署状态和健康状况

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                   Hub Cluster                         │
│                                                       │
│  ┌──────────────────┐  ┌──────────────────────────┐  │
│  │ Placement         │  │  Resource Change          │  │
│  │ Controller        │  │  Controller               │  │
│  │ (调度策略评估)    │  │  (资源变更检测/分发)      │  │
│  └────────┬─────────┘  └────────────┬─────────────┘  │
│           │                          │                │
│  ┌────────▼──────────────────────────▼─────────────┐ │
│  │         ClusterResourcePlacement CRDs            │ │
│  │         ClusterResourceOverride CRDs             │ │
│  │         MemberCluster CRDs                       │ │
│  └────────┬──────────────────────────┬─────────────┘ │
└───────────┼──────────────────────────┼───────────────┘
            │                          │
   ┌────────▼────────┐       ┌────────▼────────┐
   │  Member Cluster 1│       │  Member Cluster 2│
   │  ┌─────────────┐│       │  ┌─────────────┐│
   │  │ Fleet Agent  ││       │  │ Fleet Agent  ││
   │  │ (同步资源)   ││       │  │ (同步资源)   ││
   │  └──────┬──────┘│       │  └──────┬──────┘│
   │  ┌──────▼──────┐│       │  ┌──────▼──────┐│
   │  │ Deployments  ││       │  │ Deployments  ││
   │  │ Services     ││       │  │ Services     ││
   │  │ ConfigMaps   ││       │  │ ConfigMaps   ││
   │  └─────────────┘│       │  └─────────────┘│
   └─────────────────┘       └─────────────────┘
```

---

## 快速开始

### 安装 Hub Cluster

```bash
# 使用 Helm 安装 KubeFleet Hub
helm repo add kubefleet https://kubefleet.io/charts
helm install kubefleet-hub kubefleet/hub \
  --namespace kubefleet-system \
  --create-namespace
```

### 注册成员集群

```bash
# 在成员集群上安装 Fleet Agent
helm install kubefleet-agent kubefleet/agent \
  --namespace kubefleet-system \
  --create-namespace \
  --set config.hubURL=https://hub-api-server:443 \
  --set config.clusterName=member-cluster-1 \
  --set config.token=${JOIN_TOKEN}
```

### 创建资源分发策略

```yaml
# placement.yaml - 将 Deployment 分发到所有成员集群
apiVersion: placement.kubefleet.io/v1
kind: ClusterResourcePlacement
metadata:
  name: app-distribution
spec:
  resourceSelectors:
    - group: apps
      version: v1
      kind: Deployment
      name: my-app
    - group: ""
      version: v1
      kind: Service
      name: my-app-svc
    - group: ""
      version: v1
      kind: ConfigMap
      name: my-app-config
  policy:
    placementType: PickAll  # 分发到所有成员集群
```

```bash
# 在 Hub 集群上应用
kubectl apply -f placement.yaml

# 查看分发状态
kubectl get clusterresourceplacement app-distribution -o yaml
```

---

## 高级功能

### PickN 策略 - 选择 N 个集群

```yaml
apiVersion: placement.kubefleet.io/v1
kind: ClusterResourcePlacement
metadata:
  name: regional-deploy
spec:
  resourceSelectors:
    - group: apps
      version: v1
      kind: Deployment
      name: web-frontend
  policy:
    placementType: PickN
    numberOfClusters: 3
    affinity:
      clusterAffinity:
        requiredDuringSchedulingIgnoredDuringExecution:
          clusterSelectorTerms:
            - labelSelector:
                matchLabels:
                  env: production
        preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 60
            preference:
              labelSelector:
                matchLabels:
                  region: us-west
    topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: region
        whenUnsatisfiable: DoNotSchedule
```

### 配置覆盖

```yaml
# 为不同集群应用差异化配置
apiVersion: placement.kubefleet.io/v1alpha1
kind: ClusterResourceOverride
metadata:
  name: staging-override
spec:
  clusterResourcePlacementName: app-distribution
  clusterSelector:
    matchLabels:
      env: staging
  overrides:
    - apiVersion: apps/v1
      kind: Deployment
      name: my-app
      jsonPatch:
        - op: replace
          path: /spec/replicas
          value: 1
        - op: replace
          path: /spec/template/spec/containers/0/resources/requests/cpu
          value: "100m"
---
apiVersion: placement.kubefleet.io/v1alpha1
kind: ClusterResourceOverride
metadata:
  name: production-override
spec:
  clusterResourcePlacementName: app-distribution
  clusterSelector:
    matchLabels:
      env: production
  overrides:
    - apiVersion: apps/v1
      kind: Deployment
      name: my-app
      jsonPatch:
        - op: replace
          path: /spec/replicas
          value: 5
```

### 渐进式发布

```yaml
apiVersion: placement.kubefleet.io/v1
kind: ClusterResourcePlacement
metadata:
  name: gradual-rollout
spec:
  resourceSelectors:
    - group: apps
      version: v1
      kind: Deployment
      name: critical-service
  policy:
    placementType: PickAll
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      unavailablePeriodSeconds: 300  # 每个集群间隔 5 分钟
```

---

## 与其他方案对比

| 特性 | KubeFleet | Karmada | OCM | Clusternet |
|:---|:---|:---|:---|:---|
| 架构 | Hub-Member | Push/Pull | Hub-Agent | Hub-Agent |
| 调度策略 | PickAll/PickN/PickFixed | Replica/Divided | Placement | Replication |
| 配置覆盖 | JSON Patch | Override Policy | ManifestWork | Localization |
| 渐进式发布 | 内置 | 需扩展 | 需扩展 | 不支持 |
| 拓扑调度 | TopologySpreadConstraints | SpreadConstraints | 有限 | 不支持 |
| 状态聚合 | 内置 | 内置 | 内置 | 内置 |

---

## 最佳实践

1. **Hub 高可用**: Hub 集群使用多副本部署，确保控制面高可用
2. **标签规范**: 统一集群标签体系（region、env、tier），便于调度策略编写
3. **渐进式发布**: 关键服务使用 RollingUpdate 策略，避免同时更新所有集群
4. **资源选择器**: 精确定义 resourceSelectors，避免意外分发不需要的资源
5. **监控告警**: 监控 ClusterResourcePlacement 的 status conditions，及时发现分发异常

---

## 参考资源

- [KubeFleet 官方文档](https://kubefleet.io/docs/)
- [KubeFleet GitHub](https://github.com/kubefleet/kubefleet)
- [Fleet Networking](https://github.com/kubefleet/fleet-networking)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking.md|networking]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
