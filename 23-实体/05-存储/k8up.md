---
title: K8up (entities)
description: '## 概述'
summary: 'K8up 是一个 Kubernetes 备份 Operator，基于 Restic 实现 PersistentVolume 的自动化备份。它通过 CRD 声明式管理备份、恢复、归档和清理策略，支持将备份存储到 S3、GCS、Azure Blob 等对象存储后端。'
category: entities
tags:
- k8s
- cncf
- storage
- k8up
- prometheus
- grafana
- ingress
- crd
- operator
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
- K8up 是什么
- 如何 K8up
trigger_keywords:
- K8up
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8up

> **CNCF 状态**: Sandbox | **类别**: Storage | **主要语言**: Go

## 概述

K8up 是由 Appuio 开发的 Kubernetes 备份 Operator，基于 Restic 实现 PersistentVolume 的自动化备份。2020 年加入 CNCF Sandbox。K8up 通过 CRD 声明式管理备份（Backup）、恢复（Restore）、归档（Archive）和清理（Prune）策略，支持将备份存储到 S3、GCS、Azure Blob、MinIO 等对象存储后端。它专注于 Kubernetes 原生的备份体验，让备份策略像应用配置一样通过 GitOps 管理。

## 核心特性

- **Restic 引擎**: 基于成熟工具 Restic 实现增量、加密、去重备份
- **声明式 CRD**: 通过 Backup/Restore/Archive/Prune/Schedule CRD 管理
- **多存储后端**: S3、GCS、Azure Blob、MinIO、SFTP 等
- **数据库钩子**: 支持 preBackupCommands 注解执行数据库一致性转储
- **Prometheus 指标**: 暴露备份成功/失败指标，支持告警集成
- **Namespace 隔离**: 每个命名空间可独立配置备份策略

## 架构

K8up 由 Operator 和 Executor 组成。Operator（Deployment）监听 K8up CRD 变更，为每个备份任务创建 Executor Job。Executor Pod 挂载目标 PVC，使用 Restic 将数据备份到配置的对象存储。Schedule CRD 定义定时备份计划，Operator 根据计划自动创建 Backup 任务。备份仓库密码存储在 Kubernetes Secret 中，Restic 仓库使用 AES-256 加密。Prune 任务根据保留策略（如 keep-daily 7、keep-weekly 4）清理旧备份。

## Kubernetes 集成

K8up 完全基于 Kubernetes 原生 API。通过 CRD 定义备份策略，通过 Operator 管理执行。Backup CRD 可注解到 PVC 或 Pod 上实现自动备份。preBackupCommands 注解允许在备份前执行命令（如 `pg_dump`）。通过 ServiceAccount 和 RBAC 控制备份范围。Prometheus ServiceMonitor 集成实现监控告警。

## 生产使用场景

1. **PVC 定期备份**: 为有状态应用（数据库、文件存储）的 PV 配置每日增量备份
2. **数据库一致性备份**: 通过 preBackupCommands 执行 `pg_dump` 或 `mongodump`
3. **跨集群备份**: 将备份推送到 S3，在另一个集群恢复
4. **合规归档**: 将备份归档到冷存储满足合规要求

## 安装与配置

```bash
helm repo add appuio https://charts.appuio.ch
helm install k8up appuio/k8up -n k8up-system --create-namespace
# 验证部署
kubectl get pods -n k8up-system
kubectl get crd | grep k8up
```

```yaml
# 备份仓库 Secret
apiVersion: v1
kind: Secret
metadata:
  name: backup-credentials
  namespace: default
type: Opaque
stringData:
  password: "restic-repo-password"
  aws-access-key-id: "AKIA..."
  aws-secret-access-key: "xxx"
---
# 定时备份计划
apiVersion: k8up.io/v1
kind: Schedule
metadata:
  name: daily-backup
  namespace: default
spec:
  backend:
    repoPasswordSecretRef:
      name: backup-credentials
      key: password
    s3:
      endpoint: https://s3.amazonaws.com
      bucket: my-k8s-backups
      accessKeyIDSecretRef:
        name: backup-credentials
        key: aws-access-key-id
      secretAccessKeySecretRef:
        name: backup-credentials
        key: aws-secret-access-key
  backup:
    schedule: '0 2 * * *'
  prune:
    schedule: '0 4 * * *'
    retention:
      keepDaily: 7
      keepWeekly: 4
      keepMonthly: 6
---
# 数据库一致性备份（Pod 注解）
# metadata.annotations:
#   k8up.io/backupcommand: 'pg_dump -U postgres mydb'
#   k8up.io/file-extension: '.sql'
```

```bash
# 手动触发备份
kubectl apply -f - <<EOF
apiVersion: k8up.io/v1
kind: Backup
metadata:
  name: manual-backup-$(date +%Y%m%d)
spec:
  backend:
    repoPasswordSecretRef:
      name: backup-credentials
      key: password
    s3:
      endpoint: https://s3.amazonaws.com
      bucket: my-k8s-backups
EOF
```

## 运维操作

```bash
# 🟢 查看备份状态
kubectl get backups -A
kubectl get schedules -A
kubectl get restores -A

# 🟢 查看备份 Job 日志
kubectl get jobs -n default -l k8up.io/owned-by
kubectl logs job/<backup-job-name>

# 🟢 检查 Prometheus 指标
curl -s http://k8up-operator:8080/metrics | grep k8up

# 🟡 手动触发恢复
kubectl apply -f restore.yaml

# 🟡 查看 Restic 仓库状态
kubectl exec -it <backup-pod> -- restic -r s3:https://s3.amazonaws.com/my-k8s-backups snapshots

# 🔴 删除备份数据（不可恢复）
kubectl delete backup <name>
# 注意：仅删除 CRD，Restic 仓库数据需通过 Prune 清理
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Backup Job 失败 | S3 凭据错误/网络不通 | `kubectl logs job/<name>` | 检查 Secret 和网络连接 |
| PVC 未被备份 | PVC 缺少注解/命名空间不匹配 | `kubectl get pvc -o yaml \| grep k8up` | 添加 k8up.io/backup 注解 |
| 数据库备份不一致 | preBackupCommand 未执行 | 检查 Pod 注解 | 添加 k8up.io/backupcommand 注解 |
| Restore 失败 | 仓库密码错误/数据损坏 | `kubectl logs job/<restore-job>` | 核对密码或从其他快照恢复 |
| Prune 未清理旧备份 | 保留策略配置错误 | `kubectl get schedule <name> -o yaml` | 调整 retention 配置 |

```
排查流程：
├─ 备份失败
│  ├─ kubectl get backups 查看状态
│  ├─ kubectl logs job/<name> 查看错误
│  ├─ 检查 S3/GCS 凭据和网络
│  └─ 检查 PVC 注解是否正确
├─ 恢复失败
│  ├─ 确认 Restic 仓库密码正确
│  ├─ 检查目标 PVC 是否存在
│  └─ 检查快照列表是否完整
└─ 调度问题
   ├─ 检查 Schedule CRD 配置
   └─ 检查 Operator 日志
```

## 生产案例

### 案例 1：PostgreSQL 数据库每日备份

- **场景**: 生产 PostgreSQL 需要每日一致性备份，RPO < 24h
- **排查**: 直接备份 PV 文件无法保证数据库一致性
- **方案**: K8up + preBackupCommand 执行 `pg_dump`，备份到 S3，保留 7 天
- **效果**: 每日自动备份，恢复测试 RTO < 10min

### 案例 2：多集群备份归档

- **场景**: 3 个集群的有状态应用需要统一备份和合规归档
- **排查**: 各集群独立备份，无统一管理视图
- **方案**: K8up 统一备份到 S3，Archive CRD 将旧备份归档到 Glacier
- **效果**: 统一备份管理，满足 7 年合规保留要求

## 替代方案对比

| 维度 | K8up | Velero | Kasten K10 | Longhorn Backup |
|------|------|--------|-----------|----------------|
| 备份范围 | PV 级别 | 资源+PV | 应用感知 | 仅 Longhorn |
| 引擎 | Restic | Restic/Kopia | 自研 | 自研 |
| 复杂度 | 低 | 中 | 高 | 低 |
| 开源 | ✅ | ✅ | ❌ 商业 | ✅ |
| 适用场景 | 轻量 PV 备份 | 全面备份 | 企业级 | Longhorn 用户 |

## 架构定位

在 CNCF 生态中，K8up 属于 **Storage / Backup** 类别，专注于轻量级的 PV 级别备份。它与 Velero 互补，适合需要简洁声明式备份的场景。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[backstage]] — Backstage
- [[23-实体/04-网络/emissary-ingress.md|ingress]]]] — Emissary-Ingress
- [[kubevela]] — KubeVela
- [[piraeus-datastore]] — Piraeus Datastore
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- k8up
- [[23-实体/15-参考与索引/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/storage-index.md|Storage 存储知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
