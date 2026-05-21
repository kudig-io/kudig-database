---
title: 备份与灾难恢复（Backup & Disaster Recovery）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- helm
- flux
- harbor
- mysql
- postgresql
- job
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 备份与灾难恢复（Backup & Disaster Recovery） 是什么
- 如何 备份与灾难恢复（Backup & Disaster Recovery）
trigger_keywords:
- 备份与灾难恢复
- Backup
- Disaster
- Recovery
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- etcd-basics
- mysql-basics
- backup-basics
---

# 备份与灾难恢复（Backup & Disaster Recovery）

## 概述

在 Kubernetes 生产环境中，**备份与灾难恢复（BDR）** 是保障业务连续性的最后防线。2026 年的最佳实践要求企业不仅备份应用数据，还要备份**etcd 集群状态、Kubernetes 资源定义、Secrets 以及容器镜像**。一套完整的 BDR 策略应涵盖 **恢复时间目标（RTO）** 和 **恢复点目标（RPO）**，并通过定期的灾难恢复演练验证其有效性。

## 核心概念/原理

### 1. 备份范围

Kubernetes 环境中的备份对象包括四个层次：

| 层次 | 备份内容 | 工具示例 |
|------|----------|----------|
| **集群状态** | etcd 数据、所有 K8s 资源（Deployment、Service、ConfigMap、Secret 等） | etcd snapshot、Velero |
| **应用配置** | YAML 清单、Helm Chart、Kustomize 配置、Git 仓库 | Git、[[domain-19-landscape-references/01-cncf-landscape/graduated/flux/flux|Flux]]、Argo CD |
| **持久化数据** | PVC 中的业务数据、数据库、对象存储 | Velero、Kasten、数据库原生备份 |
| **镜像与 Artifact** | 容器镜像、Helm Chart 包、SBOM | Harbor、Registry replication |

### 2. etcd 备份

etcd 是 Kubernetes 的"大脑"，存储了所有集群状态和配置：
- **内置快照**：`etcdctl snapshot save /backup/etcd-$(date +%Y%m%d).db`
- **定时备份**：通过 CronJob 每小时自动执行快照并上传至对象存储
- **加密传输**：备份文件应加密存储，防止 etcd 数据泄露
- **恢复验证**：每季度至少进行一次 etcd 恢复演练，验证 RTO

```bash
# etcd 备份命令
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-snapshot.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```

### 3. Velero

**Velero** 是 VMware 开源的 Kubernetes 备份与迁移工具，支持：
- **整集群/Namespace 级别备份**：备份所有 K8s 资源和关联的 PVC 数据
- **定时备份（Schedule）**：通过 Cron 表达式自动执行备份任务
- **灾难恢复**：将备份恢复到同一集群或全新的目标集群
- **云存储集成**：支持 S3、Azure Blob、GCS 等后端存储
- **快照集成**：与 AWS EBS、GCP PD、Azure Disk CSI 集成实现卷快照备份

```yaml
# Velero 定时备份示例
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-backup
spec:
  schedule: 0 2 * * *
  template:
    includedNamespaces:
      - production
    snapshotVolumes: true
    ttl: 720h0m0s
```

### 4. 3-2-1 备份原则

行业通用的备份黄金法则：
- **3 份数据副本**：1 份生产数据 + 2 份备份
- **2 种不同存储介质**：如本地 SSD + 对象存储
- **1 份异地备份**：备份数据应存放在与生产环境不同的地理区域

对于 Kubernetes，建议：
- 本地快速恢复副本（同一区域的对象存储或 NAS）
- 跨区域灾难恢复副本（不同云区域的 S3/GCS）
- 离线/空气间隙副本（Air-gapped，用于勒索软件防护）

## 关键机制或特性

### 应用一致性备份

- **崩溃一致性（Crash-consistent）**：直接备份 PVC，相当于断电瞬间的数据状态
- **应用一致性（Application-consistent）**：通过 pre-hook/post-hook 在备份前冻结数据库写操作（如 `FLUSH TABLES WITH READ LOCK`）
- 对于数据库，建议优先使用数据库原生备份工具（如 MySQL XtraBackup、PostgreSQL pg_dump、MongoDB mongodump），再用 Velero 备份非数据库应用

### 跨集群恢复策略

| 场景 | RTO 目标 | 策略 |
|------|----------|------|
| **Namespace 误删除** | < 1 小时 | Velero 从同一集群的历史备份恢复 |
| **可用区故障** | < 4 小时 | 切换到跨 AZ 的备用 Namespace/集群 |
| **区域级灾难** | < 24 小时 | 在备用区域通过 Cluster API + Velero 重建完整集群 |
| **勒索软件攻击** | < 48 小时 | 从空气间隙离线备份恢复，重建全新集群 |

### 数据库专属高可用

对于 Kubernetes 上的有状态数据库，除了备份还应建立：
- **同步复制**：如 PostgreSQL Patroni、MySQL Group Replication、CockroachDB
- **跨集群副本**：使用数据库 Operator 在异地建立只读副本
- **Point-in-Time Recovery（PITR）**：基于 WAL/事务日志恢复到任意时间点

## 使用场景

1. **Namespace 级误操作恢复**：开发团队误删除了 production Namespace，通过 Velero 在 30 分钟内恢复所有资源和数据
2. **跨云迁移**：企业将 AWS EKS 上的工作负载迁移到阿里云 ACK，使用 Velero 备份并恢复
3. **勒索软件防御**：攻击者加密了集群数据，通过 S3 版本控制和跨区域复制恢复干净副本
4. **数据库 PITR**：业务在凌晨 2 点遭遇数据损坏，利用 PostgreSQL WAL 归档恢复到凌晨 1:55 的状态
5. **集群重建演练**：每季度模拟区域级灾难，使用 Cluster API + Velero 在备用区域 4 小时内重建完整集群

## 最佳实践/注意事项

- **备份不等于恢复**：备份只是第一步，必须定期进行恢复演练并记录 RTO/RPO 实际值
- **Secrets 需要单独保护**：Velero 默认备份 Secret，但这些 Secret 可能包含数据库密码，备份文件必须加密
- **验证备份完整性**：定期检查备份文件是否损坏，避免在真正需要恢复时才发现备份不可用
- **保留策略管理**：设置合理的 TTL（如 30 天每日备份 + 1 年每月备份），避免存储成本失控
- **区分配置与数据**：Git 中的 YAML 是配置的"事实来源"，Velero 主要用于数据和状态的灾难恢复
- **etcd 备份与控制平面高可用**：即使控制平面是多节点的，etcd 快照仍是集群级灾难恢复的基础
- **文档化恢复流程**：将恢复步骤写成 Runbook，确保在高压情况下任何人都能按步骤执行
- **网络隔离测试**：在隔离的测试环境中验证从生产备份恢复的数据，避免污染生产环境

## 故障排查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| Velero 备份失败 | 对象存储凭证过期或桶不存在 | `velero backup describe <name> --details` | 更新 BSL 凭证并验证桶存在 |
| etcd 快照恢复后集群异常 | 快照版本与当前集群不兼容 | `etcdctl snapshot status <file>` | 确认快照来自相同 K8s 版本的集群 |
| PVC 数据恢复为空 | 快照未正确创建或 CSI 驱动不支持 | `velero backup describe <name> \| grep VolumeSnapshot` | 确认 CSI 驱动支持 VolumeSnapshot |
| 恢复后 Secret 无法解密 | 加密密钥不匹配 | `kubectl get secret <name> -o yaml` | 使用原集群的 encryption config 解密 |
| 备份存储空间持续增长 | TTL 过长或过期备份未清理 | `velero backup get` | 设置合理的 `ttl` 并验证 GC 策略 |
| 跨集群恢复 Service IP 冲突 | 目标集群 Service CIDR 不同 | `kubectl get svc -A` | 恢复前调整目标集群 Service CIDR 或使用 restore mapping |

## 生产检查清单

- [ ] etcd 快照 CronJob 每小时执行，上传至对象存储
- [ ] Velero 定时备份已配置（生产 Namespace）
- [ ] 备份遵循 3-2-1 原则（3 副本、2 种介质、1 份异地）
- [ ] 备份文件已加密存储
- [ ] 每季度至少进行一次恢复演练
- [ ] RTO/RPO 目标已定义并经过验证
- [ ] 数据库使用原生备份工具 + PITR
- [ ] 恢复流程已写成 Runbook 并可被任何团队成员执行
- [ ] 空气间隙离线备份已配置（勒索软件防护）

## 命令快速参考

```bash
# etcd 备份
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d%H%M).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# etcd 恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot.db --data-dir=/var/lib/etcd-restored

# Velero: 创建备份
velero backup create prod-backup --include-namespaces production --snapshot-volumes

# Velero: 定时备份
velero schedule create daily-prod --schedule="0 2 * * *" --include-namespaces production --ttl 720h

# Velero: 恢复
velero restore create --from-backup prod-backup --include-namespaces production

# Velero: 查看备份状态
velero backup get && velero restore get
```

## 交叉引用

- [Velero Documentation](https://velero.io/docs/)
- [Kubernetes etcd Backup and Restore](https://kubernetes.io/docs/tasks/administer-cluster/configure-upgrade-etcd/#backing-up-an-etcd-cluster)
- [Kasten K10 Documentation](https://docs.kasten.io/)
- 相关主题：[有状态服务运维](stateful-services-operations.md) · [Persistent Volumes](../storage/persistent-volumes.md) · [Volume Snapshots](../storage/volume-snapshots.md)

## Related

- [[domain-19-landscape-references/topic-index/backup-dr-index|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
