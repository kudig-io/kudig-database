---
title: K8s 灾难恢复与业务连续性研究
summary: 深入研究 Kubernetes 集群的灾难恢复体系，涵盖 RTO/RPO 目标设定、etcd 备份恢复、多活/主备架构、DNS 切换等关键环节。
category: research
tags:
- research
- disaster-recovery
- backup
- etcd
- rto
- rpo
- business-continuity
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# K8s 灾难恢复与业务连续性研究

## 研究背景

Kubernetes 集群的灾难恢复（DR）是生产环境最关键但常被忽视的领域。集群故障场景包括：

- **控制平面故障**：API Server 不可用，etcd 数据损坏
- **可用区故障**：云厂商 AZ 宕机（AWS us-east-1 历史故障）
- **区域故障**：整_region 不可用（自然灾害、云厂商重大故障）
- **数据损坏**：etcd 数据逻辑损坏、PV 数据损坏
- **勒索软件**：集群资源被恶意加密锁定

## 核心问题

1. 如何根据业务需求设定合理的 RTO/RPO 目标并设计对应的 DR 架构？
2. etcd 备份的频率、存储位置和恢复验证流程应该怎样设计？
3. 有状态应用的 PV 备份（Volume Snapshot）如何纳入 DR 流程？
4. 多活/主备/DNS 切换三种 DR 架构的选型标准是什么？

## 调研发现

### 发现一：DR 架构模式对比

| 模式 | RTO | RPO | 成本 | 复杂度 | 适用场景 |
|------|-----|-----|------|--------|---------|
| **主备（Active-Passive）** | 15-60min | 1-15min | 1.5x | 中 | 大多数生产系统 |
| **多活（Active-Active）** | < 1min | ≈0 | 2x+ | 高 | 金融/交易系统 |
| **DNS 切换（Pilot Light）** | 5-30min | 1-6h | 1.2x | 低 | 成本敏感型 |
| **备份恢复（Restore）** | 1-4h | 12-24h | 1x | 低 | 容忍长 RTO |

### 发现二：etcd 备份恢复策略

```bash
# 🔴 高风险：etcd 恢复会覆盖现有数据，必须在空集群执行

# 🟢 备份 etcd（定时执行）
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d-%H%M%S).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/etcd/pki/ca.crt \
  --cert=/etc/etcd/pki/server.crt \
  --key=/etc/etcd/pki/server.key

# 🟢 验证备份完整性
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-20260711.db --write-out=table

# 🟢 从备份恢复（在新集群或清空 etcd 后）
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-20260711.db \
  --data-dir=/var/lib/etcd-restored \
  --initial-cluster=control-1=https://10.0.0.1:2380 \
  --initial-cluster-token=restored-cluster \
  --initial-advertise-peer-urls=https://10.0.0.1:2380
```

**etcd 备份策略矩阵**：

| 维度 | 推荐值 | 说明 |
|------|--------|------|
| 备份频率 | 每 30 分钟 | RPO ≤ 30min |
| 本地保留 | 24 小时 | 快速恢复 |
| 远程存储 | S3/GCS（跨区域） | 防区域故障 |
| 远程保留 | 30 天 | 审计/回滚 |
| 恢复演练 | 每月一次 | 确保 DR 可用 |
| 自动化 | Velero / CronJob | 无人值守 |

### 发现三：Velero 全栈备份方案

Velero 不仅备份 etcd（K8s 资源），还通过 Volume Snapshot 备份 PV 数据：

```yaml
# Velero BackupSchedule — 每日全量备份
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"          # 每天凌晨 2 点
  template:
    includedNamespaces:
    - production
    - monitoring
    snapshotVolumes: true         # 同时备份 PV 快照
    ttl: 720h                     # 保留 30 天
    storageLocation: s3-backup    # 跨区域 S3
    volumeSnapshotLocations:
    - aws-us-east-1
    hooks:
      resources:
      - name: pre-backup-hook
        pre:
        - exec:
            container: app
            command:
            - /bin/sh
            - -c
            - "pg_dump -F c > /tmp/db.dump"  # 应用级一致性快照
```

### 发现四：多集群 DR 架构

**Active-Active 多活架构**：

```
                 ┌─────────────┐
                 │ 全局 DNS/LB  │
                 │ (Route53)    │
                 └──────┬──────┘
              ┌────────┼────────┐
              ↓        ↓        ↓
         ┌────┐   ┌────┐   ┌────┐
         │区域1│   │区域2│   │区域3│
         │EKS  │   │EKS  │   │EKS  │
         │集群  │   │集群  │   │集群  │
         └──┬─┘   └──┬─┘   └──┬─┘
            │        │        │
         ┌──┴────────┴────────┴──┐
         │  全局数据库（多活复制）   │
         │  Aurora Global / Spanner │
         └───────────────────────┘

关键组件:
  → 多集群 GitOps（ArgoCD ApplicationSet）确保配置一致
  → Cilium ClusterMesh 提供跨集群服务发现
  → 数据库全局复制保证数据一致
  → DNS 健康检查自动故障转移
```

### 发现五：DR 演练与混沌验证

| 演练类型 | 频率 | 场景 | 验证目标 |
|---------|------|------|---------|
| **etcd 恢复** | 月度 | 从备份重建 etcd | RTO ≤ 30min |
| **命名空间恢复** | 月度 | 从 Velero 恢复命名空间 | 资源+PV 完整 |
| **AZ 故障** | 季度 | 模拟 AZ 宕机 | 自动重调度+恢复 |
| **区域故障** | 半年 | 模拟整_region 不可用 | DNS 切换+多活 |
| **勒索软件** | 年度 | 模拟数据被加密 | 从异地备份恢复 |

## 结论与建议

1. **RTO/RPO 先行**：根据业务 SLA 确定目标，选择对应的 DR 架构。
2. **主备模式适合大多数企业**：Active-Active 复杂度和成本极高，不应默认选择。
3. **etcd 备份是最关键的 DR 操作**：每 30 分钟一次，跨区域存储，每月恢复演练。
4. **Velero 实现全栈备份**：同时备份 K8s 资源和 PV 数据，是 DR 的核心工具。
5. **DR 演练不是可选项**：未经演练的 DR 方案等于没有 DR。
6. **数据库 DR 需要独立方案**：K8s DR 不覆盖数据库内部一致性，需要数据库级复制。

## 参考资料

- Velero: https://velero.io/
- etcd Disaster Recovery: https://etcd.io/docs/v3.5/op-guide/recovery/
- [[可靠性/灾难恢复/|灾难恢复目录]]
- [[可靠性/备份恢复/|备份恢复目录]]
- [[研究/multi-cluster-management.md|多集群管理研究]]

## Related

- [[综合/kubernetes-etcd.md|Kubernetes × etcd]]
- [[可靠性/index.md|可靠性目录]]
