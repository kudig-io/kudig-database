---
title: K8s 多租户架构与隔离研究
summary: 深入研究 Kubernetes 多租户架构的三大模式（命名空间隔离、虚拟集群、独立集群），分析安全隔离、资源隔离和网络隔离的技术方案。
category: research
tags:
- research
- multi-tenancy
- isolation
- security
- vcluster
- resource-quota
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# K8s 多租户架构与隔离研究

## 研究背景

Kubernetes 多租户（Multi-Tenancy）是平台工程中的核心架构决策。从 SaaS 平台到企业内部平台，不同租户（团队/客户/项目）共享同一 Kubernetes 集群时，必须解决：

- **安全隔离**：租户 A 不能访问租户 B 的资源
- **资源隔离**：一个租户不能耗尽集群资源影响其他租户
- **网络隔离**：跨租户网络流量默认不可达
- **性能隔离（Noisy Neighbor）**：一个租户的高负载不影响其他租户
- **管理自治**：租户可以自主管理自己命名空间的资源

## 核心问题

1. 三大多租户模式（Soft/Soft+/Hard）的隔离强度和成本权衡？
2. vCluster（虚拟集群）方案相比命名空间隔离的优势和局限？
3. 资源隔离（ResourceQuota/LimitRange/PriorityClass）的最佳实践组合？
4. 多租户场景下的 RBAC、NetworkPolicy 和 Pod Security 设计？

## 调研发现

### 发现一：多租户模式对比

| 模式 | 隔离强度 | 方案 | 成本 | 适用场景 |
|------|---------|------|------|---------|
| **Soft** | 低 | 命名空间 + RBAC + NetworkPolicy | 1x | 可信租户（内部团队） |
| **Soft+** | 中 | + Pod Security + Resource Quota | 1x | 半可信租户 |
| **Hard** | 高 | 独立集群 / vCluster | 2-3x | 不可信租户（外部客户） |

### 发现二：vCluster 架构

```
宿主集群 (Host Cluster)
┌────────────────────────────────────────┐
│  namespace: tenant-a                    │
│  ┌──────────────────────────────────┐  │
│  │  vCluster Pod                     │  │
│  │  ├── K3s (轻量控制面)              │  │
│  │  ├── etcd (SQLite/etcd)           │  │
│  │  ├── syncer (同步到宿主集群)       │  │
│  │  └── 虚拟 K8s API Server          │  │
│  └──────────────────────────────────┘  │
│  ├── Pod: tenant-a-app-x (真实 Pod)    │
│  └── Pod: tenant-a-app-y (真实 Pod)    │
│                                        │
│  namespace: tenant-b                    │
│  ┌──────────────────────────────────┐  │
│  │  vCluster Pod (完全独立的 K8s)    │  │
│  └──────────────────────────────────┘  │
└────────────────────────────────────────┘

优势:
  → 租户获得完整的 K8s API（可创建 CRD、RBAC）
  → 租户间完全 API 级隔离
  → 成本远低于独立集群
```

### 发现三：资源隔离最佳实践

```yaml
# 命名空间级资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-a-quota
  namespace: tenant-a
spec:
  hard:
    requests.cpu: "100"            # CPU 上限
    requests.memory: 200Gi         # 内存上限
    limits.cpu: "200"              # CPU Limit 上限
    limits.memory: 400Gi
    persistentvolumeclaims: "50"   # PVC 数量上限
    requests.storage: "5Ti"        # 存储上限
    pods: "200"                    # Pod 数量上限
    services.loadbalancers: "5"    # LB 数量上限
---
# LimitRange 约束单 Pod 资源
apiVersion: v1
kind: LimitRange
metadata:
  name: tenant-a-limits
  namespace: tenant-a
spec:
  limits:
  - type: Container
    default:                       # 默认 limit
      cpu: "2"
      memory: 4Gi
    defaultRequest:                # 默认 request
      cpu: "500m"
      memory: 1Gi
    max:                           # 最大 limit
      cpu: "16"
      memory: 64Gi
    min:                           # 最小 request
      cpu: "50m"
      memory: 128Mi
```

### 发现四：隔离成熟度矩阵

| 隔离维度 | Soft 模式 | Soft+ 模式 | Hard 模式 |
|---------|----------|-----------|----------|
| API 隔离 | RBAC | RBAC + Webhook | 独立 API Server |
| 网络隔离 | NetworkPolicy | + 默认拒绝 | 独立 CNI/网络 |
| 资源隔离 | ResourceQuota | + PriorityClass | 独立节点池 |
| 存储隔离 | PVC namespace | + StorageClass | 独立存储后端 |
| Pod 安全 | Pod Security | + Seccomp/AppArmor | 独立运行时 |

## 结论与建议

1. **内部团队用 Soft+ 模式**：命名空间 + RBAC + NetworkPolicy + ResourceQuota 足够。
2. **外部客户用 Hard 模式**：vCluster 或独立集群，强隔离是合规要求。
3. **vCluster 是成本最优的强隔离方案**：比独立集群省 60-80% 成本。
4. **ResourceQuota + LimitRange 是资源隔离的最低要求**：不设配额等于没有隔离。
5. **NetworkPolicy 默认拒绝是多租户的基石**：跨命名空间流量默认不可达。

## 参考资料

- vCluster: https://www.vcluster.com/
- K8s Multi-Tenancy: https://kubernetes.io/docs/concepts/security/multi-tenancy/
- [[安全/index.md|安全目录]]
- [[平台工程/index.md|平台工程目录]]
- [[研究/zero-trust-k8s-security.md|零信任安全架构]]

## Related

- [[综合/rbac-multitenancy.md|RBAC × 多租户]]
- [[研究/platform-engineering-idp.md|平台工程 IDP]]
