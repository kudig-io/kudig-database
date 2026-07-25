---
title: High Availability Patterns
description: High Availability Patterns — Kubernetes 生产运维知识库
summary: High Availability Patterns — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- ha
- leader-election
- etcd
- anti-affinity
- pod-disruption-budget
- scheduler
- controller-manager
- pdb
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- High Availability Patterns 是什么
- 如何 High Availability Patterns
trigger_keywords:
- High
- Availability
- Patterns
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# High Availability Patterns

## Control Plane HA

| Component | HA Mechanism | Minimum Replicas |
|-----------|-------------|------------------|
| **API Server** | Stateless, behind Load Balancer | 2 (recommended 3) |
| **etcd** | Raft consensus, odd nodes (2f+1) | 3 (tolerates 1 failure) |
| **Scheduler** | Leader election via Lease | 2 (recommended 3) |
| **Controller Manager** | Leader election via Lease | 2 (recommended 3) |

## etcd Cluster Sizing

| Nodes | Fault Tolerance | Use Case |
|-------|----------------|----------|
| 1 | 0 | Development |
| 3 | 1 | Small production |
| 5 | 2 | Large production |

Adding nodes beyond 5 degrades write performance due to Raft replication overhead.

## Workload HA Patterns

- **PodAntiAffinity**: Spread replicas across nodes or failure domains (topologyKey)
- **PodDisruptionBudget (PDB)**: Limit simultaneous voluntary [[Disruptions|disruptions]] during node drains or cluster upgrades
- **Topology Spread Constraints**: Built-in scheduler feature for even [[Distribution|distribution]] across failure domains (zones, nodes, hostnames)

## Leader Election

Stateful control plane components (scheduler, controller-manager) use [[Kubernetes|Kubernetes]] Lease objects for leader election:
- `leaseDuration`: How long a leader holds lock (default 15s)
- `renewDeadline`: How long leader has to renew (default 10s)
- `retryPeriod`: How often to retry (default 2s)

## Backup and Recovery

- etcd: Regular snapshots with `etcdctl snapshot save`
- Certificates: Backup `/etc/kubernetes/pki` after every change
- Manifests: Store in Git for reproducibility
- Application data: Velero for PV and resource backup to object storage

## 实践示例

### 工作负载高可用配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web
spec:
  replicas: 3
  template:
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels: {app: web}
              topologyKey: kubernetes.io/hostname
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels: {app: web}
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels: {app: web}
```

### etcd 备份脚本

```bash
#!/bin/bash
# 🟡 etcd 快照备份
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d-%H%M).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 验证快照
etcdctl snapshot status /backup/etcd-*.db --write-out=table
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 3 副本就是高可用 | 还需跨节点/可用区分布 + PDB |
| etcd 节点越多越好 | 超过 5 节点写性能下降 |
| PDB 阻止所有驱逐 | PDB 仅限制自愿驱逐，不影响节点故障 |
| 备份了就安全 | 需定期验证恢复流程 |
| Leader Election 无风险 | 脑裂时可能短暂双主 |

## 面试要点

1. **Kubernetes 控制面如何实现高可用？**
   - API Server: 无状态，LB 后多副本
   - etcd: Raft 共识，奇数节点 (2f+1)
   - Scheduler/Controller: Leader Election via Lease

2. **PodDisruptionBudget 的作用？**
   - 限制自愿驱逐时的最大不可用数
   - 保护滚动更新/节点维护时的可用性
   - minAvailable 或 maxUnavailable

3. **etcd 集群为什么推荐 3 或 5 节点？**
   - 3 节点: 容忍 1 故障，适合小型生产
   - 5 节点: 容忍 2 故障，适合大型生产
   - Raft 写性能随节点数增加而下降

## 源码实现分析

### Leader Election 实现

```go
// k8s.io/client-go/tools/leaderelection/leaderelection.go
// K8s 控制平面组件通过 Lease 对象实现 Leader Election
func (le *LeaderElector) acquire(ctx context.Context) {
    for {
        // 1. 尝试创建/更新 Lease 对象
        succeeded := le.tryAcquireOrRenew(ctx)
        if succeeded {
            // 2. 成为 Leader，开始执行控制器逻辑
            le.onStartedLeading(ctx)
            return
        }
        // 3. 未获取到，等待后重试
        time.Sleep(le.config.RetryPeriod)  // 默认 2s
    }
}

// Lease 对象示例（kube-system 命名空间）
// apiVersion: coordination.k8s.io/v1
// kind: Lease
// metadata:
//   name: kube-scheduler
// spec:
//   holderIdentity: kube-master-1    ← 当前 Leader
//   leaseDurationSeconds: 15
//   renewTime: "2026-07-11T10:00:00Z"  ← 最后续约时间
```

### HA 架构模式

```
┌───────────────────────────────────────────────────────────┐
│          K8s 高可用架构模式                          │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  控制平面 HA:                                            │
│  ─────────                                              │
│  API Server: 无状态，LB 后 3+ 副本                   │
│  etcd:       Raft 共识，3/5 节点，跨 AZ              │
│  Scheduler:  Leader Election (Lease)，主备模式       │
│  Controller: Leader Election (Lease)，主备模式       │
│                                                           │
│  工作负载 HA:                                            │
│  ─────────                                              │
│  多副本:     replicas ≥ 3，跨 AZ 分布                │
│  PDB:        minAvailable 保护最小可用数             │
│  反亲和:     podAntiAffinity 跨节点/AZ              │
│  探针:       liveness + readiness + startup          │
│                                                           │
│  基础设施 HA:                                          │
│  ─────────                                              │
│  多 AZ:      控制平面 + 工作节点跨 AZ              │
│  多集群:     异地多活 / 主备切换                   │
│  备份:       etcd 快照 + Velero + 恢复演练         │
│                                                           │
│  关键指标:                                               │
│  • 控制平面可用性 > 99.99%                          │
│  • 工作负载可用性 > 99.9%                           │
│  • RPO < 1min, RTO < 5min                            │
└───────────────────────────────────────────────────────────┘
```

### 生产 HA 配置示例（🟡 部署到集群）

```yaml
# PDB 保护最小可用副本
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-app-pdb
spec:
  minAvailable: 2  # 至少 2 个 Pod 可用
  selector:
    matchLabels:
      app: web-app
---
# 跨 AZ 分布 + 反亲和
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 3
  template:
    spec:
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: web-app
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: web-app
              topologyKey: kubernetes.io/hostname
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 多副本就是 HA | 还需跨 AZ/节点分布 + PDB + 探针 |
| etcd 节点越多越好 | 5 节点已足够，更多会降低写性能 |
| Leader Election 无开销 | 切换期间有短暂不可用（~15s） |
| 备份了就安全 | 必须定期恢复演练，否则备份无意义 |
| 单 AZ 也可以 HA | AZ 故障会导致全部不可用，必须跨 AZ |
| PDB 可以阻止所有驱逐 | PDB 只保护自愿驱逐，不保护节点故障 |

## Related
- [[22-概念/11-交叉分析/etcd × Operator 模式.md|etcd × Operator 模式]] — 综合

- [[22-概念/01-核心架构/eventual-consistency.md|eventual-consistency]] — Eventual Consistency in Kubernetes
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[26-技能/02-控制面/etcd/backup-restore-etcd.md|backup-restore-etcd]] — Backup and Restore etcd
- [[etcd]] — etcd
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[22-概念/01-核心架构/eventual-consistency.md|Eventual Consistency]]
- [[22-概念/05-安全/security-defense-depth.md|Defense-in-Depth Security]]
- [[26-技能/02-控制面/etcd/backup-restore-etcd.md|Backup and Restore etcd]]

- 08-high-availability-patterns

<!-- risk-assessed -->
