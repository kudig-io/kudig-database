---
title: Kubernetes Multi-Tenancy Patterns — Isolation, Governance, and Resource Management
description: K8s 多租户模式 — 命名空间隔离、资源配额、网络策略、RBAC、vCluster、租户治理
summary: Kubernetes 多租户架构设计，涵盖软隔离到硬隔离的完整方案与治理实践
category: practice
tags:
- multi-tenancy
- isolation
- resource-quota
- rbac
- governance
- vcluster
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: special-topics
---
# Kubernetes 多租户模式

> 从命名空间隔离到虚拟集群的多租户架构设计。

## 多租户隔离级别

```
┌─────────────────────────────────────────────────────────────┐
│  隔离强度（从弱到强）                                        │
│                                                             │
│  L1: 命名空间 + RBAC + ResourceQuota                        │
│      └── 逻辑隔离，共享控制平面和节点                        │
│                                                             │
│  L2: L1 + NetworkPolicy + PodSecurity                      │
│      └── 网络隔离 + 安全约束                                │
│                                                             │
│  L3: 专用节点池 + Taint/Toleration                         │
│      └── 计算资源物理隔离                                    │
│                                                             │
│  L4: vCluster / 虚拟集群                                    │
│      └── 独立控制平面，共享基础设施                          │
│                                                             │
│  L5: 独立集群（最强隔离）                                    │
│      └── 完全独立，成本最高                                  │
└─────────────────────────────────────────────────────────────┘
```

## L1-L2: 命名空间级隔离

### 完整租户配置

```yaml
# 1. 命名空间 + 标签
apiVersion: v1
kind: Namespace
metadata:
  name: team-alpha
  labels:
    tenant: team-alpha
    environment: production
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
---
# 2. 资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-alpha-quota
  namespace: team-alpha
spec:
  hard:
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    persistentvolumeclaims: "10"
    services: "20"
    secrets: "50"
    configmaps: "50"
    pods: "100"
    services.loadbalancers: "2"
    requests.storage: 500Gi
---
# 3. LimitRange（默认值）
apiVersion: v1
kind: LimitRange
metadata:
  name: team-alpha-limits
  namespace: team-alpha
spec:
  limits:
    - type: Container
      default:
        cpu: "1"
        memory: 512Mi
      defaultRequest:
        cpu: 100m
        memory: 128Mi
      max:
        cpu: "8"
        memory: 16Gi
      min:
        cpu: 50m
        memory: 64Mi
    - type: Pod
      max:
        cpu: "16"
        memory: 32Gi
---
# 4. 网络策略（默认拒绝 + 白名单）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: team-alpha
spec:
  podSelector: {}
  policyTypes: ["Ingress", "Egress"]
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-internal
  namespace: team-alpha
spec:
  podSelector: {}
  policyTypes: ["Ingress", "Egress"]
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              tenant: team-alpha
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              tenant: team-alpha
    - to:  # 允许 DNS
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - port: 53
          protocol: UDP
    - to:  # 允许外部 HTTPS
        - ipBlock:
            cidr: 0.0.0.0/0
      ports:
        - port: 443
          protocol: TCP
---
# 5. RBAC（租户管理员）
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: team-alpha-admin
  namespace: team-alpha
rules:
  - apiGroups: ["", "apps", "batch", "networking.k8s.io"]
    resources: ["*"]
    verbs: ["*"]
  - apiGroups: [""]
    resources: ["secrets"]
    verbs: ["get", "list", "create", "update", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-alpha-admin-binding
  namespace: team-alpha
subjects:
  - kind: Group
    name: team-alpha-admins
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: team-alpha-admin
  apiGroup: rbac.authorization.k8s.io
```

## L3: 专用节点池

```yaml
# 节点污点（专用节点）
# kubectl taint nodes node-1 tenant=team-alpha:NoSchedule

# 租户工作负载 Toleration + NodeSelector
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
  namespace: team-alpha
spec:
  template:
    spec:
      tolerations:
        - key: tenant
          value: team-alpha
          effect: NoSchedule
      nodeSelector:
        tenant: team-alpha
      # 或使用 NodeAffinity
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: tenant
                    operator: In
                    values: ["team-alpha"]
```

## L4: vCluster（虚拟集群）

```yaml
# vCluster 配置（独立控制平面）
apiVersion: v1
kind: ConfigMap
metadata:
  name: vcluster-team-alpha
  namespace: team-alpha
data:
  values.yaml: |
    sync:
      ingresses:
        enabled: true
      persistentvolumes:
        enabled: true
      nodes:
        enabled: false
    isolation:
      enabled: true
      podSecurityStandard: restricted
      resourceQuota:
        enabled: true
        quota:
          requests.cpu: "20"
          requests.memory: 40Gi
          limits.cpu: "40"
          limits.memory: 80Gi
          pods: "100"
      limitRange:
        enabled: true
      networkPolicy:
        enabled: true
    controlPlane:
      statefulSet:
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
---
# 安装 vCluster
# vcluster create team-alpha -n team-alpha --values values.yaml
# 连接:
# vcluster connect team-alpha -n team-alpha
# kubectl get pods  # 在虚拟集群中操作
```

## 租户治理

### 自动化租户配置（Kyverno Generate）

```yaml
# 新命名空间自动生成租户配置
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: tenant-bootstrap
spec:
  rules:
    - name: generate-resource-quota
      match:
        resources:
          kinds: ["Namespace"]
          selector:
            matchLabels:
              tenant: "?*"
      generate:
        synchronize: true
        apiVersion: v1
        kind: ResourceQuota
        name: default-quota
        namespace: "{{request.object.metadata.name}}"
        data:
          spec:
            hard:
              requests.cpu: "10"
              requests.memory: 20Gi
              limits.cpu: "20"
              limits.memory: 40Gi
              pods: "50"
    - name: generate-network-policy
      match:
        resources:
          kinds: ["Namespace"]
          selector:
            matchLabels:
              tenant: "?*"
      generate:
        synchronize: true
        apiVersion: networking.k8s.io/v1
        kind: NetworkPolicy
        name: default-deny
        namespace: "{{request.object.metadata.name}}"
        data:
          spec:
            podSelector: {}
            policyTypes: ["Ingress", "Egress"]
```

### 成本分摊

```yaml
# Kubecost 租户成本分配
# 按命名空间标签分配
# 标签: tenant=team-alpha

# 成本报告查询
# kubecost API:
# /allocation?aggregate=namespace&window=7d&filterNamespaces=team-alpha

# 或按标签
# /allocation?aggregate=label:tenant&window=30d
```

## 多租户方案选型

| 方案 | 隔离级别 | 成本 | 复杂度 | 适用 |
|------|----------|------|--------|------|
| Namespace + Quota | 逻辑 | 低 | 低 | 内部团队 |
| + NetworkPolicy | 网络 | 低 | 中 | 安全要求 |
| 专用节点池 | 计算 | 中 | 中 | 性能隔离 |
| vCluster | 控制平面 | 中 | 中 | 多团队自治 |
| 独立集群 | 完全 | 高 | 高 | 合规/外部客户 |

## 最佳实践

| 实践 | 说明 |
|------|------|
| 默认拒绝 | 新租户默认无权限 |
| 自动化配置 | 策略自动生成 Quota/NetPol |
| 标签规范 | 统一 tenant 标签体系 |
| 成本可见 | 每租户成本报告 |
| 自助服务 | 开发者门户申请租户 |
| 审计日志 | 记录所有租户操作 |
| 定期审查 | 清理不活跃租户资源 |
| 升级隔离 | 按需升级隔离级别 |

## Related

- [[专项技术/index.md|专项技术]]
- [[安全/策略治理/index.md|策略治理]]
- [[生产运维/集群治理/index.md|集群治理]]
- [[生产运维/成本治理/index.md|成本治理]]
