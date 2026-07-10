---
title: 08 - 多租户架构设计 (Multi-Tenancy Architecture)
description: '# 08 - 多租户架构设计 (Multi-Tenancy Architecture)'
summary: 'pod-security.kubernetes.io/enforce: restricted'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- prometheus
- istio
- cilium
- calico
- helm
- job
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- 多租户架构设计 (Multi-Tenancy Architecture) 是什么
- 如何 多租户架构设计 (Multi-Tenancy Architecture)
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- 多租户架构设计
- Multi-Tenancy
- Architecture
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- helm-basics
- service-mesh-basics
- prometheus-basics
- cilium-basics
- cni-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-13-container-runtime/
  label: '相关知识域: domain-13-container-runtime'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 08 - 多租户架构设计 (Multi-Tenancy Architecture)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[entities/kubernetes.md|kubernetes]].io/docs/concepts/security/multi-tenancy](https://kubernetes.io/docs/concepts/security/multi-tenancy/)

<!-- chunk: 多租户隔离级别 -->
## 多租户隔离级别

| 隔离级别 | 描述 | 隔离边界 | 适用场景 | 复杂度 |
|---------|------|---------|---------|-------|
| **软隔离** | 逻辑隔离，共享集群 | Namespace | 内部团队 | 低 |
| **硬隔离** | 物理/逻辑隔离 | 节点池/网络 | 安全敏感租户 | 中 |
| **完全隔离** | 独立集群 | 集群 | 外部客户 | 高 |

<!-- chunk: Namespace隔离 -->
## Namespace隔离

| 功能 | 资源类型 | 用途 | 版本支持 |
|-----|---------|------|---------|
| **命名空间** | Namespace | 资源分组和隔离 | 稳定 |
| **资源配额** | ResourceQuota | 限制资源使用 | 稳定 |
| **限制范围** | LimitRange | 默认和限制 | 稳定 |
| **网络策略** | [[NetworkPolicy|NetworkPolicy]] | 网络隔离 | 稳定 |
| **RBAC** | Role/RoleBinding | 权限隔离 | 稳定 |
| **Pod安全** | NS标签 | 安全策略 | v1.25+ |

```yaml
# 多租户Namespace模板
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a
  labels:
    tenant: a
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
---
# 资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-quota
  namespace: tenant-a
spec:
  hard:
    requests.cpu: "10"
    requests.memory: 20Gi
    limits.cpu: "20"
    limits.memory: 40Gi
    pods: "50"
    services: "20"
    secrets: "50"
    configmaps: "50"
    persistentvolumeclaims: "20"
---
# 限制范围
apiVersion: v1
kind: LimitRange
metadata:
  name: tenant-limits
  namespace: tenant-a
spec:
  limits:
  - type: Container
    default:
      cpu: "500m"
      memory: "512Mi"
    defaultRequest:
      cpu: "100m"
      memory: "128Mi"
    max:
      cpu: "2"
      memory: "4Gi"
---
# 网络策略 - 默认拒绝
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: tenant-a
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
---
# 网络策略 - 允许同NS通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-same-namespace
  namespace: tenant-a
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          tenant: a
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          tenant: a
  - to:  # 允许DNS
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
```

<!-- chunk: RBAC多租户配置 -->
## RBAC多租户配置

```yaml
# 租户管理员角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-admin
  namespace: tenant-a
rules:
- apiGroups: ["", "apps", "batch", "networking.k8s.io"]
  resources: ["*"]
  verbs: ["*"]
- apiGroups: [""]
  resources: ["resourcequotas", "limitranges"]
  verbs: ["get", "list"]  # 只读配额
---
# 租户开发者角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-developer
  namespace: tenant-a
rules:
- apiGroups: ["", "apps", "batch"]
  resources: ["pods", "deployments", "services", "configmaps", "secrets", "jobs"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["pods/log", "pods/exec"]
  verbs: ["get", "create"]
---
# 租户只读角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-viewer
  namespace: tenant-a
rules:
- apiGroups: ["", "apps", "batch", "networking.k8s.io"]
  resources: ["*"]
  verbs: ["get", "list", "watch"]
```

<!-- chunk: 层级命名空间(HNC) -->
## 层级命名空间(HNC)

| 功能 | 说明 | 版本支持 |
|-----|------|---------|
| **父子关系** | NS层级结构 | v1.0+ |
| **策略继承** | 子NS继承父NS策略 | v1.0+ |
| **资源传播** | 自动复制资源到子NS | v1.0+ |
| **配额分配** | 层级配额管理 | v1.0+ |

```yaml
# HNC配置示例
apiVersion: hnc.x-k8s.io/v1alpha2
kind: HierarchyConfiguration
metadata:
  name: hierarchy
  namespace: team-a
spec:
  parent: organization
---
# SubnamespaceAnchor创建子命名空间
apiVersion: hnc.x-k8s.io/v1alpha2
kind: SubnamespaceAnchor
metadata:
  name: project-1
  namespace: team-a
```

<!-- chunk: Virtual Cluster(vCluster) -->
## Virtual Cluster(vCluster)

| 特性 | 说明 | 优势 |
|-----|------|------|
| **虚拟控制平面** | 每租户独立API Server | 完全API隔离 |
| **资源映射** | 虚拟资源映射到宿主 | 资源高效利用 |
| **CRD隔离** | 租户可安装自己的CRD | 不影响其他租户 |
| **版本灵活** | 可运行不同K8S版本 | 升级灵活 |

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装vCluster
helm repo add loft-sh https://charts.loft.sh
helm install vcluster vcluster/vcluster \
  --namespace tenant-a \
  --create-namespace \
  --set syncer.extraArgs[0]="--sync=nodes"

# 连接到vCluster
vcluster connect vcluster -n tenant-a
```
<!-- chunk: 节点隔离 -->
## 节点隔离

| 方法 | 实现方式 | 隔离强度 | 成本 |
|-----|---------|---------|------|
| **NodeSelector** | Pod指定节点标签 | 弱 | 低 |
| **Taint/Toleration** | 节点污点+Pod容忍 | 中 | 中 |
| **节点池** | 专用节点池 | 强 | 高 |
| **RuntimeClass** | 隔离运行时(gVisor/Kata) | 很强 | 中 |

```yaml
# 节点污点配置
kubectl taint nodes node1 tenant=a:NoSchedule

# Pod容忍配置
apiVersion: v1
kind: Pod
metadata:
  name: tenant-a-pod
spec:
  tolerations:
  - key: "tenant"
    operator: "Equal"
    value: "a"
    effect: "NoSchedule"
  nodeSelector:
    tenant: a
```

<!-- chunk: 多租户网络隔离 -->
## 多租户网络隔离

| 隔离方式 | CNI支持 | 隔离级别 |
|---------|--------|---------|
| **NetworkPolicy** | Calico/Cilium | L3/L4 |
| **服务网格mTLS** | [[Istio|Istio]]/Linkerd | L7+加密 |
| **VPC隔离** | Terway | 网络级 |
| **节点网络隔离** | 安全组 | 节点级 |

<!-- chunk: 多租户计费 -->
## 多租户计费

| 计费维度 | 数据来源 | 工具 |
|---------|---------|------|
| **CPU使用** | Prometheus指标 | Kubecost |
| **内存使用** | Prometheus指标 | Kubecost |
| **存储使用** | PVC指标 | Kubecost |
| **网络流量** | CNI指标 | 云监控 |
| **云资源** | 云厂商API | 费用中心 |

<!-- chunk: ACK多租户方案 -->
## ACK多租户方案

| 方案 | 实现方式 | 适用场景 |
|-----|---------|---------|
| **命名空间隔离** | NS+RBAC+NetworkPolicy | 内部团队 |
| **节点池隔离** | 专用节点池 | 资源隔离需求 |
| **ACK集群隔离** | 独立集群 | 强隔离需求 |
| **ACK One** | 多集群管理 | 大规模多租户 |
| **RAM集成** | 阿里云RAM账号 | 统一身份管理 |

```yaml
# ACK RAM用户RBAC绑定
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: tenant-a-admin
  namespace: tenant-a
subjects:
- kind: User
  name: "UID:ram-user-id"  # RAM用户ID
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: tenant-admin
  apiGroup: rbac.authorization.k8s.io
```

<!-- chunk: 多租户最佳实践 -->
## 多租户最佳实践

| 实践 | 说明 | 优先级 |
|-----|------|-------|
| **默认拒绝网络** | 默认NetworkPolicy | P0 |
| **资源配额** | 每NS必须配置 | P0 |
| **RBAC最小权限** | 按需授权 | P0 |
| **Pod安全策略** | restricted级别 | P1 |
| **审计日志** | 按租户过滤 | P1 |
| **监控隔离** | 租户独立Dashboard | P2 |
| **日志隔离** | 租户独立日志存储 | P2 |

<!-- chunk: 多租户成本分摊 -->
## 多租户成本分摊

### Kubecost集成

```yaml
# 安装Kubecost
helm repo add kubecost https://kubecost.github.io/cost-analyzer/
helm install kubecost kubecost/cost-analyzer \
  --namespace kubecost \
  --create-namespace \
  --set kubecostToken="<your-token>"
```

### 成本标签策略

```yaml
# 统一标签规范
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a
  labels:
    tenant: a
    cost-center: "CC-1001"
    department: "engineering"
    environment: "production"
    
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
  namespace: tenant-a
  labels:
    app: myapp
    tenant: a
    cost-center: "CC-1001"
spec:
  template:
    metadata:
      labels:
        app: myapp
        tenant: a
```

### 成本告警

```yaml
# 租户成本超预算告警
- alert: TenantCostOverBudget
  expr: |
    sum(kubecost_pod_cost_hourly{namespace=~"tenant-.*"}) by (namespace) * 24 * 30 > 10000
  labels:
    severity: warning
  annotations:
    summary: "Tenant {{ $labels.namespace }} monthly cost exceeds $10000"
```

<!-- chunk: 多租户自服务平台 -->
## 多租户自服务平台

### Kubernetes Dashboard + RBAC

```yaml
# 租户只能看到自己的命名空间
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: tenant-namespace-viewer
rules:
- apiGroups: [""]
  resources: ["namespaces"]
  verbs: ["get", "list"]
  resourceNames: ["tenant-a"]  # 仅允许访问自己的命名空间

---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: tenant-a-namespace-viewer
subjects:
- kind: User
  name: "tenant-a-admin"
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: tenant-namespace-viewer
  apiGroup: rbac.authorization.k8s.io
```

---

**多租户原则**: 默认隔离，最小权限，资源配额，审计追踪，成本透明

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- Domain-1 架构基础 — 开源项目索引
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)
- 10 - Windows 容器支持与集成指南

## See Also

- 06-cluster-configuration-parameters
- 07-upgrade-paths-strategy
- 09-edge-computing-kubeedge
- 10-windows-containers-support

## Related

- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
