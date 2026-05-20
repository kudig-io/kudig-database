---
title: Open Cluster Management (OCM)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- apiserver
- prometheus
- operator
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
- Open Cluster Management (OCM) 是什么
- 如何 Open Cluster Management (OCM)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Open
- Cluster
- Management
- OCM
- cncf
- landscape
---


# Open Cluster Management (OCM)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://open-cluster-management.io/ |
| **GitHub** | https://github.com/open-cluster-management-io |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Open Cluster Management (OCM) 是一个社区驱动的多集群管理平台，提供 Kubernetes 多集群编排的核心能力。OCM 采用 Hub-Spoke 架构，通过轻量级的代理模型实现集群注册、工作负载分发、策略治理和应用生命周期管理。

### 核心特性

- **集群注册与管理**: 基于 CSR 的集群注册机制，安全地纳管 Spoke 集群
- **工作负载分发**: ManifestWork API 实现跨集群的资源分发
- **Placement 调度**: 基于标签、集群属性和自定义策略的智能调度
- **策略治理 (Governance)**: 跨集群的合规性检查和策略执行
- **Addon 框架**: 可插拔的扩展机制，轻松扩展多集群能力
- **ManagedServiceAccount**: 跨集群的服务账户管理
- **Pull 模型**: Spoke 集群主动拉取配置，无需 Hub 直连 Spoke

---

## 架构设计

```
┌────────────────────────────────────────────────────┐
│                    Hub Cluster                       │
│                                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────┐ │
│  │ Registration  │  │ Placement    │  │ Addon     │ │
│  │ Controller    │  │ Controller   │  │ Manager   │ │
│  └──────┬───────┘  └──────┬───────┘  └─────┬─────┘ │
│         │                 │                 │        │
│  ┌──────┴─────────────────┴─────────────────┴────┐  │
│  │              Hub API Server                    │  │
│  │                                                │  │
│  │  ManagedCluster  │ ManifestWork │ Placement   │  │
│  │  ClusterSet      │ Policy       │ AddOnConfig │  │
│  └────────┬───────────────┬───────────────┬──────┘  │
└───────────┼───────────────┼───────────────┼─────────┘
            │               │               │
            ▼               ▼               ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────┐
│  Spoke Cluster 1 │ │  Spoke Cluster 2 │ │  Spoke ...   │
│                  │ │                  │ │              │
│ ┌──────────────┐│ │ ┌──────────────┐│ │              │
│ │ Klusterlet   ││ │ │ Klusterlet   ││ │              │
│ │ (Agent)      ││ │ │ (Agent)      ││ │              │
│ │              ││ │ │              ││ │              │
│ │ Registration ││ │ │ Registration ││ │              │
│ │ Work Agent   ││ │ │ Work Agent   ││ │              │
│ │ Addon Agents ││ │ │ Addon Agents ││ │              │
│ └──────────────┘│ │ └──────────────┘│ │              │
└─────────────────┘ └─────────────────┘ └─────────────┘
```

### 核心组件

| 组件 | 位置 | 说明 |
|:---|:---|:---|
| **Registration Controller** | Hub | 处理集群注册请求和证书管理 |
| **Placement Controller** | Hub | 根据策略将工作负载调度到目标集群 |
| **Work Controller** | Hub | 管理 ManifestWork 资源的分发 |
| **Addon Manager** | Hub | 管理多集群扩展插件的生命周期 |
| **Klusterlet** | Spoke | 集群代理，包含 Registration Agent 和 Work Agent |
| **Policy Framework** | Hub+Spoke | 策略定义、传播和合规性检查 |

---

## 快速开始

### 使用 clusteradm 安装

```bash
# 安装 clusteradm CLI
curl -L https://raw.githubusercontent.com/open-cluster-management-io/clusteradm/main/install.sh | bash

# 在 Hub 集群初始化
clusteradm init --wait

# 获取加入命令（包含 token）
clusteradm get token

# 在 Spoke 集群加入 Hub
clusteradm join --hub-token <token> \
  --hub-apiserver https://hub-api:6443 \
  --cluster-name spoke-cluster-1 --wait

# 在 Hub 集群接受 Spoke 注册
clusteradm accept --clusters spoke-cluster-1
```

### 验证集群注册

```bash
# 查看已注册的集群
kubectl get managedcluster

# 查看集群详情
kubectl get managedcluster spoke-cluster-1 -o yaml
```

---

## 配置详解

### ManagedCluster 资源

```yaml
apiVersion: cluster.open-cluster-management.io/v1
kind: ManagedCluster
metadata:
  name: spoke-cluster-1
  labels:
    cloud: aws
    region: us-east-1
    environment: production
    vendor: OpenShift
spec:
  hubAcceptsClient: true
  leaseDurationSeconds: 60
```

### ManifestWork - 跨集群资源分发

```yaml
apiVersion: work.open-cluster-management.io/v1
kind: ManifestWork
metadata:
  name: deploy-nginx
  namespace: spoke-cluster-1  # 目标集群同名 namespace
spec:
  workload:
    manifests:
      - apiVersion: apps/v1
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
                  image: nginx:1.25
                  ports:
                    - containerPort: 80
      - apiVersion: v1
        kind: Service
        metadata:
          name: nginx-svc
          namespace: default
        spec:
          selector:
            app: nginx
          ports:
            - port: 80
              targetPort: 80
  manifestConfigs:
    - resourceIdentifier:
        group: apps
        resource: deployments
        name: nginx
        namespace: default
      feedbackRules:
        - type: JSONPaths
          jsonPaths:
            - name: available-replicas
              path: ".status.availableReplicas"
```

### Placement - 集群调度

```yaml
apiVersion: cluster.open-cluster-management.io/v1beta1
kind: Placement
metadata:
  name: production-clusters
  namespace: default
spec:
  numberOfClusters: 3  # 选择 3 个集群
  clusterSets:
    - production-set
  predicates:
    - requiredClusterSelector:
        labelSelector:
          matchLabels:
            environment: production
        claimSelector:
          matchExpressions:
            - key: platform.open-cluster-management.io
              operator: In
              values:
                - AWS
                - Azure
  prioritizerPolicy:
    mode: Exact
    configurations:
      - scoreCoordinate:
          type: BuiltIn
          builtIn: Steady  # 保持稳定调度
        weight: 1
      - scoreCoordinate:
          type: BuiltIn
          builtIn: ResourceAllocatableCPU
        weight: 2
---
apiVersion: cluster.open-cluster-management.io/v1beta2
kind: ManagedClusterSet
metadata:
  name: production-set
spec:
  clusterSelector:
    selectorType: LabelSelector
    labelSelector:
      matchLabels:
        environment: production
```

---

## 高级功能

### 策略治理 (Governance)

```yaml
# 定义策略
apiVersion: policy.open-cluster-management.io/v1
kind: Policy
metadata:
  name: require-resource-limits
  namespace: default
spec:
  remediationAction: enforce  # inform 或 enforce
  disabled: false
  policy-templates:
    - objectDefinition:
        apiVersion: policy.open-cluster-management.io/v1
        kind: ConfigurationPolicy
        metadata:
          name: resource-limits-policy
        spec:
          remediationAction: enforce
          severity: high
          object-templates:
            - complianceType: musthave
              objectDefinition:
                apiVersion: v1
                kind: LimitRange
                metadata:
                  name: default-limits
                  namespace: default
                spec:
                  limits:
                    - type: Container
                      default:
                        cpu: "500m"
                        memory: "512Mi"
                      defaultRequest:
                        cpu: "100m"
                        memory: "128Mi"
---
# 将策略绑定到集群
apiVersion: policy.open-cluster-management.io/v1
kind: PlacementBinding
metadata:
  name: bind-resource-limits
  namespace: default
spec:
  placementRef:
    name: production-clusters
    kind: Placement
    apiGroup: cluster.open-cluster-management.io
  subjects:
    - name: require-resource-limits
      kind: Policy
      apiGroup: policy.open-cluster-management.io
```

### Addon 开发框架

```yaml
# 部署自定义 Addon
apiVersion: addon.open-cluster-management.io/v1alpha1
kind: ManagedClusterAddOn
metadata:
  name: my-monitoring-addon
  namespace: spoke-cluster-1
spec:
  installNamespace: open-cluster-management-agent-addon
---
apiVersion: addon.open-cluster-management.io/v1alpha1
kind: ClusterManagementAddOn
metadata:
  name: my-monitoring-addon
  annotations:
    addon.open-cluster-management.io/lifecycle: "addon-manager"
spec:
  supportedConfigs:
    - group: addon.open-cluster-management.io
      resource: addondeploymentconfigs
  installStrategy:
    type: Placements
    placements:
      - name: production-clusters
        namespace: default
```

### ManifestWorkReplicaSet - 批量分发

```yaml
apiVersion: work.open-cluster-management.io/v1alpha1
kind: ManifestWorkReplicaSet
metadata:
  name: deploy-monitoring-stack
  namespace: default
spec:
  placementRefs:
    - name: production-clusters
  manifestWorkTemplate:
    workload:
      manifests:
        - apiVersion: v1
          kind: Namespace
          metadata:
            name: monitoring
        - apiVersion: apps/v1
          kind: Deployment
          metadata:
            name: prometheus
            namespace: monitoring
          spec:
            replicas: 1
            selector:
              matchLabels:
                app: prometheus
            template:
              metadata:
                labels:
                  app: prometheus
              spec:
                containers:
                  - name: prometheus
                    image: prom/prometheus:v2.50.0
```

---

## 监控与运维

### 集群状态检查

```bash
# 查看所有托管集群状态
kubectl get managedcluster -o wide

# 查看集群条件
kubectl get managedcluster spoke-cluster-1 -o jsonpath='{.status.conditions}' | jq .

# 查看 ManifestWork 状态
kubectl get manifestwork -n spoke-cluster-1

# 查看策略合规状态
kubectl get policy -A -o wide

# 查看 Placement 决策
kubectl get placementdecision -A
```

### Prometheus 指标

| 指标 | 说明 |
|:---|:---|
| `ocm_managed_cluster_count` | 托管集群总数 |
| `ocm_manifestwork_status` | ManifestWork 应用状态 |
| `ocm_policy_compliance` | 策略合规状态 |
| `ocm_placement_decision_count` | Placement 调度决策数 |
| `ocm_addon_status` | Addon 运行状态 |

---

## 最佳实践

1. **集群组织**: 使用 ManagedClusterSet 按环境、区域或团队组织集群
2. **渐进式部署**: 先使用 `inform` 模式验证策略影响，再切换为 `enforce`
3. **Placement 策略**: 利用 Spread 和 Steady 策略平衡负载和稳定性
4. **状态反馈**: 在 ManifestWork 中配置 feedbackRules 获取资源状态
5. **Addon 管理**: 使用 Addon 框架扩展能力，避免直接在 Spoke 集群操作
6. **安全模型**: 遵循最小权限原则，Klusterlet 仅需访问其对应 namespace

---

## 参考资源

- [OCM 官方文档](https://open-cluster-management.io/)
- [OCM GitHub 组织](https://github.com/open-cluster-management-io)
- [clusteradm CLI](https://github.com/open-cluster-management-io/clusteradm)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
