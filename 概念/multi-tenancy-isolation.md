---
title: Multi-Tenancy Isolation
description: '- [[概念/服务网格 x 零信任安全.md|服务网格 x 零信任安全]] — synthesis'
summary: '- [[概念/服务网格 x 零信任安全.md|服务网格 x 零信任安全]] — synthesis'
category: concepts
tags:
- k8s
- multi-tenancy
- namespace
- rbac
- network-policy
- resource-quota
- opa
- networkpolicy
- rag
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Multi-Tenancy Isolation 是什么
- 如何 Multi-Tenancy Isolation
trigger_keywords:
- Multi-Tenancy
- Isolation
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Multi-Tenancy Isolation

## Soft Isolation (Namespace-based)

Multiple tenants share one cluster, isolated by:

| Mechanism | What it isolates |
|-----------|-----------------|
| **Namespace** | Logical grouping, naming scope |
| **RBAC** | Access control (Role + RoleBinding per namespace) |
| **ResourceQuota** | CPU, memory, storage, PVC count limits |
| **LimitRange** | Default/limits for containers without explicit settings |
| **[[NetworkPolicy|NetworkPolicy]]** | Network traffic between namespaces |
| **Pod Security Standards** | Container security enforcement level |

## Hard Isolation

| Approach | Description | Trade-offs |
|----------|-------------|------------|
| **Separate Clusters** | Each tenant gets own cluster | Highest isolation, highest cost |
| **vCluster** | Virtual Kubernetes API Server per tenant | Good isolation, shared underlying cluster |
| **Kamaji** | Kubernetes control plane as a service | Multi-tenant control planes |

## Tenant Isolation Checklist

1. Namespace per tenant with labels for identification
2. RBAC Role/RoleBinding scoped to namespace
3. ResourceQuota limiting total resource consumption
4. NetworkPolicy denying cross-namespace traffic by default
5. Pod Security Standards enforced at Restricted level
6. LimitRange for default resource bounds
7. Audit logging to track cross-tenant access attempts
8. OPA/Gatekeeper policies to prevent privilege escalation

## 实践示例

### 完整租户隔离配置

```yaml
# 1. 命名空间 + 标签
apiVersion: v1
kind: Namespace
metadata:
  name: team-alpha
  labels:
    tenant: team-alpha
    pod-security.kubernetes.io/enforce: restricted
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
    services.loadbalancers: "2"
---
# 3. 默认资源限制
apiVersion: v1
kind: LimitRange
metadata:
  name: team-alpha-limits
  namespace: team-alpha
spec:
  limits:
  - default:
      cpu: "1"
      memory: 1Gi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    type: Container
---
# 4. 网络隔离
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-cross-namespace
  namespace: team-alpha
spec:
  podSelector: {}
  policyTypes: [Ingress, Egress]
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
    ports:
    - protocol: UDP
      port: 53
---
# 5. RBAC
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-alpha-admin
  namespace: team-alpha
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: admin
subjects:
- kind: Group
  name: team-alpha@company.com
  apiGroup: rbac.authorization.k8s.io
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| Namespace 就是安全边界 | Namespace 仅是逻辑隔离，不是安全边界 |
| RBAC 就够了 | 需要 NetworkPolicy + PSS + Quota 多层防御 |
| 同集群租户可以互信 | 零信任原则，默认拒绝所有跨租户流量 |
| ResourceQuota 防止资源耗尽 | 还需 LimitRange 防止单 Pod 过大 |
| vCluster 等于完全隔离 | vCluster 共享底层节点，内核级隔离需 gVisor/Kata |

## 面试要点

1. **Kubernetes 多租户隔离有哪些层次？**
   - 逻辑隔离: Namespace + RBAC + ResourceQuota
   - 网络隔离: NetworkPolicy + ServiceMesh mTLS
   - 安全隔离: Pod Security Standards + OPA
   - 硬隔离: vCluster / 独立集群 / gVisor

2. **如何实现默认拒绝的网络策略？**
   - 创建 deny-all NetworkPolicy
   - 显式允许同租户流量
   - 允许 DNS 和必要的基础设施访问

3. **软隔离 vs 硬隔离如何选择？**
   - 内部团队: 软隔离 (Namespace) 成本效益高
   - 外部客户/合规要求: 硬隔离 (vCluster/独立集群)
   - 混合: 控制面硬隔离 + 数据面软隔离

## Related

- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[概念/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[技能/audit-rbac-configurations.md|audit-rbac-configurations]] — Audit RBAC Configurations
- [[概念/security-defense-depth.md|Defense-in-Depth Security]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[技能/audit-rbac-configurations.md|Audit RBAC Configurations]]
- [[概念/服务网格 x 零信任安全.md|服务网格 x 零信任安全]] — synthesis
- [[概念/IaC x 多集群管理.md|IaC x 多集群管理]] — synthesis


<!-- risk-assessed -->
