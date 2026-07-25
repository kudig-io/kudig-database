---
title: Kubernetes 数据保护策略
description: → 配置备份 (GitOps)
summary: → 配置备份 (GitOps)
category: synthesis
tags:
- data-protection
- backup
- disaster-recovery
- k8s
- velero
- csi
- etcd
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 数据保护策略 是什么
- 如何 Kubernetes 数据保护策略
trigger_keywords:
- Kubernetes
- 数据保护策略
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
relationships:
- target: '[[23-实体/02-K8s核心组件/kubernetes.md]]'
  type: uses
- target: '[[17-系统基础/05-速查卡/k8s.md]]'
  type: related_to
- target: '[[20-最佳实践/01-best-practices/infrastructure/storage.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]] 数据保护策略

## 概述

Kubernetes 数据保护是保障业务连续性的核心能力。与虚拟机时代不同，K8s 的声明式架构使得"配置即代码"成为可能——工作负载定义可通过 Git 仓库恢复，但持久化数据、Secret、集群状态等仍需要专门的备份与恢复策略。本页系统化介绍 K8s 环境下的多层数据保护体系。

## 分层保护体系

### 三层保护架构

```
应用层:
  → 配置备份 (GitOps — 仓库即备份)
  → Secret 加密备份 (Sealed Secrets / SOPS)
  → 应用元数据导出 (CRD、ConfigMap)

数据层:
  → CSI 快照 (VolumeSnapshot — 块存储级)
  → 卷备份 (Velero — PV 数据 + K8s 资源)
  → 数据库逻辑备份 (pg_dump / mysqldump — 跨平台)
  → 对象存储版本控制 (S3 versioning — 不可变备份)

集群层:
  → etcd 备份 (集群状态快照)
  → 集群状态导出 (kubectl dump / Velero)
  → 控制平面配置备份 (PKI、kubeconfig)
```

## Velero + CSI 快照

### Velero 备份机制

Velero 是 K8s 数据保护的事实标准，支持两种备份模式：

| 模式 | 机制 | 适用场景 |
|------|------|---------|
| **VolumeSnapshot** | CSI 驱动创建存储级快照 | 块存储（EBS/PD/云盘），恢复快 |
| **Restic/Filesystem** | Pod 内文件级备份 | 不支持 CSI 的存储，NFS 等 |

### 生产备份示例

```bash
# 🟢 低风险：备份操作（不影响运行中的服务）
# 按命名空间备份（包含 K8s 资源 + PV 快照）
velero backup create prod-daily-backup \
  --include-namespaces production \
  --snapshot-volumes=true \
  --volume-snapshot-locations default \
  --include-cluster-resources=false \
  --label-selector backup-tier=critical

# 验证备份完成
velero backup describe prod-daily-backup --details
velero backup logs prod-daily-backup

# 🟡 中风险：恢复操作
# 从备份恢复
velero restore create --from-backup prod-daily-backup \
  --namespace-mappings production:production-restored

# 恢复到新命名空间进行验证
velero restore create --from-backup prod-daily-backup \
  --namespace-mappings production:staging-restore \
  --wait
```

### VolumeSnapshot 定时备份

```yaml
# VolumeSnapshotClass 定义
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-snapshot-class
driver: disk.csi.alibabacloud.com
deletionPolicy: Retain                # 备份不会被自动删除

---
# VolumeSnapshotSchedule（通过 Velero 或 external-snapshotter）
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-production-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"                # 每天凌晨 2 点
  template:
    includedNamespaces:
      - production
    snapshotVolumes: true
    ttl: 720h                          # 保留 30 天
```

## 3-2-1 原则在 [[17-系统基础/05-速查卡/k8s.md|K8s]] 中的实践

传统备份的 3-2-1 原则在云原生环境下的映射：

```
3 份数据:
  - 生产数据（etcd + PV + 对象存储）
  - 本地备份（Velero 快照 + CSI 快照）
  - 异地备份（跨区域对象存储复制）

2 种介质:
  - 块存储快照（CSI VolumeSnapshot — 快速恢复）
  - 对象存储备份（S3/OSS — 异地容灾）

1 份异地:
  - 跨区域对象存储复制（如 S3 Cross-Region Replication）
  - 跨云备份（Velero 备份文件存储到另一个云）
```

## etcd 备份与恢复

### 定期快照

```bash
# 🟢 低风险：备份操作
# 创建 etcd 快照
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/etcd/pki/ca.crt \
  --cert=/etc/etcd/pki/server.crt \
  --key=/etc/etcd/pki/server.key

# 验证快照完整性
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-20260711.db -w table

# 🔴 高风险：灾难恢复（需要停止 etcd 集群）
# 从快照恢复到新集群
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-20260711.db \
  --data-dir=/var/lib/etcd-restored \
  --initial-cluster=etcd-0=https://etcd-0:2380 \
  --initial-cluster-token=new-cluster \
  --initial-advertise-peer-urls=https://etcd-0:2380
```

## 灾难恢复 RTO/RPO 目标

| 保护层级 | RPO（数据丢失） | RTO（恢复时间） | 适用场景 |
|----------|-----------------|-----------------|---------|
| GitOps | 0（Git 仓库实时同步） | < 5min | 无状态服务配置恢复 |
| Velero + CSI 快照 | < 24h（每日备份） | < 30min | 命名空间级恢复 |
| etcd 快照 | < 6h（每 6h 快照） | < 60min | 全集群恢复 |
| 数据库 PITR | ~ 0（持续复制） | < 15min | 关键数据库 |
| 跨区域容灾 | ~ 0（实时复制） | < 5min | Region 级故障 |

## 最佳实践

- **多层级备份组合**：不要依赖单一备份手段——GitOps（配置）+ Velero（K8s 资源）+ CSI 快照（PV）+ 数据库 PITR 形成多层防护
- **定期恢复演练**：备份不验证等于没有备份——至少每季度执行一次完整恢复演练，验证 RTO/RPO 目标
- **备份加密**：Secret 和敏感数据备份必须加密（Sealed Secrets / Velero encryption），备份文件存储到加密对象存储
- **监控备份成功率**：配置告警监控 Velero backup 的 CompletionTimestamp 和 Errors 字段，备份失败必须告警
- **实施备份保留策略**：定义合理的备份保留周期（如每日备份保留 30 天，每周备份保留 12 周），平衡存储成本和恢复灵活性

## 常见陷阱

- **备份了但从未恢复测试**：Velero 备份可能因 CSI 驱动版本不兼容而恢复失败——必须定期验证恢复流程
- **etcd 快照恢复导致数据不一致**：etcd 快照包含全集群状态，恢复到运行中的集群会导致状态冲突——需要在新集群或清空 etcd 后恢复
- **CSI 快照与 PV 存储类不匹配**：恢复时如果目标集群的 StorageClass 与源集群不同，PV 绑定会失败——需要在恢复前确保 StorageClass 映射正确

## 相关 Domain

- 可靠性/01-backup-recovery/01-backup-strategies
- domain-04-[[20-最佳实践/01-best-practices/infrastructure/storage.md|storage]]-data/03-csi/01-csi-snapshot

## 相关页面

- [[22-概念/08-可靠性与运维/chaos-drill-integration.md|混沌工程与灾备演练]] — 恢复演练实践
- [[22-概念/04-存储/persistent-volume-claim.md|PVC]] — 持久卷声明与快照
- [[22-概念/08-可靠性与运维/cross-cloud-migration-playbook.md|跨云迁移手册]] — 跨云数据迁移

## Related

- [[01-集群基础/01-架构总览/01-kubernetes-architecture-overview.md|Kubernetes 架构全景图 (Architecture Overview)]]
- [[21-生态参考/02-论文/01-kubernetes-production-readiness-assessment.md|Kubernetes 生产就绪性评估框架 (Production Readiness Assessment Framework)]]


<!-- risk-assessed -->
