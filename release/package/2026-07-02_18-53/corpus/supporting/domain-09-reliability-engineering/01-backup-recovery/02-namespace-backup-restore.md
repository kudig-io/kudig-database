---
title: Namespace 级别备份恢复：Velero
description: 面向阿里云专有云 K8s 运维工单智能体的 Velero 实战手册，覆盖 Namespace 级备份、跨集群迁移、状态应用备份钩子及恢复演练。
summary: 面向阿里云专有云 K8s 运维工单智能体的 Velero 实战手册，覆盖 Namespace 级备份、跨集群迁移、状态应用备份钩子及恢复演练。
category: reliability-engineering
tags:
- velero
- namespace
- backup-restore
- cross-cluster
- migration
- oss
- ack
- aso
- statefulset
- hooks
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 应用架构师
estimated_read_time: 15min
intent_queries:
- 如何使用 Velero 备份 Namespace
- Velero 跨集群迁移步骤
- 阿里云 OSS Velero 配置
- Namespace 级别灾难恢复
- Velero 备份钩子配置
trigger_keywords:
- velero
- namespace backup
- 跨集群迁移
- 应用备份
- 定时备份
prerequisites:
- velero-basics
- kubectl-basics
- oss-basics
- statefulset-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Namespace 级别备份恢复：Velero

> **适用范围**: Kubernetes v1.28-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐
> **适用场景**: 阿里云 ACK/专有云环境下 Namespace 级资源备份、跨集群迁移、误删恢复。

## 目录

- [1. 概述](#1-概述)
- [2. Velero 架构与阿里云集成](#2-velero-架构与阿里云集成)
- [3. 安装与初始化](#3-安装与初始化)
  - [3.4 BackupStorageLocation 与 VolumeSnapshotLocation 配置](#34-backupstoragelocation-与-volumesnapshotlocation-配置)
  - [3.5 最小权限 RAM Policy](#35-最小权限-ram-policy)
- [4. Namespace 备份策略](#4-Namespace-备份策略)
- [5. 状态应用备份钩子](#5-状态应用备份钩子)
- [6. 恢复流程](#6-恢复流程)
  - [6.3 恢复后的资源 reconcile 与验证](#63-恢复后的资源-reconcile-与验证)
- [7. 跨集群迁移](#7-跨集群迁移)
- [8. 定时备份与保留策略](#8-定时备份与保留策略)
- [9. 验证与演练](#9-验证与演练)
- [10. 常见问题与故障排查](#10-常见问题与故障排查)
- [11. 检查清单](#11-检查清单)
- [12. Related](#12-Related)

## 1. 概述

在阿里云专有云与 ACK 环境中，etcd 备份虽然能保护整个控制面，但无法提供细粒度的应用级恢复。Velero 作为 Kubernetes 生态中最成熟的开源备份恢复工具，能够以 Namespace 为粒度备份资源对象与持久卷数据，并支持跨集群迁移与灾难恢复。

本文档面向运维工单智能体，提供从 Velero 安装、Namespace 备份、状态应用钩子、恢复到跨集群迁移的完整操作路径。

> **核心原则**：按 Namespace 制定备份策略；对状态应用必须配置 pre/post 备份钩子；定期验证备份可恢复性；跨集群恢复时注意 StorageClass 与网络差异。

## 2. Velero 架构与阿里云集成

Velero 由以下核心组件构成：

| 组件 | 作用 | 部署方式 |
|---|---|---|
| velero CLI | 本地操作备份/恢复命令 | 运维工程师本地安装 |
| Velero Server | 监听 Backup/Restore CR，执行备份恢复 | Deployment in velero Namespace |
| BackupStorageLocation (BSL) | 对象存储后端配置（如 OSS） | CR |
| VolumeSnapshotLocation (VSL) | 卷快照后端配置（如云盘快照） | CR |
| Restic/Kopia | 文件级持久卷数据备份（可选） | DaemonSet |

在阿里云环境中，推荐将 OSS 作为 BSL，将云盘快照或 CSI 快照作为 VSL。对于未支持 CSI 快照的场景，可使用 Restic/Kopia 进行文件级备份。

## 3. 安装与初始化

### 3.1 创建 OSS 备份 Bucket

Velero 需要稳定的对象存储作为备份目标。以下命令在阿里云控制台/CLI 中创建专用 Bucket，并启用版本控制与跨地域复制：

```bash
# 创建 OSS Bucket（建议与集群同地域，并开启版本控制）
aliyun oss mb oss://my-k8s-velero-backup --acl private

# 开启版本控制与跨区域复制（通过控制台或 CLI）
aliyun oss versioning --method put oss://my-k8s-velero-backup Enabled
```

### 3.2 准备访问凭证

Velero 访问 OSS 需要 AccessKey 或 RAM Role。推荐使用 Worker RAM Role，避免长期 AccessKey 泄漏风险：

```bash
# 创建 velero 专用 RAM 用户并授予最小权限
aliyun ram CreateUser --UserName velero-backup
aliyun ram AttachPolicyToUser --UserName velero-backup --PolicyName AliyunOSSFullAccess --PolicyType System
aliyun ram CreateAccessKey --UserName velero-backup
```

### 3.3 使用 velero install 部署服务端

以下命令安装 Velero Server，指定 OSS 为 BSL，并启用 Restic 文件级备份。命令执行前请确保本地 velero CLI 已下载并配置正确：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 Velero，使用 OSS 作为 BackupStorageLocation
velero install \
  --provider alibabacloud \
  --plugins registry.cn-hangzhou.aliyuncs.com/acs/velero-plugin-alibabacloud:v1.0 \
  --bucket my-k8s-velero-backup \
  --secret-file ./credentials-velero \
  --use-volume-snapshots=true \
  --use-restic=true \
  --default-volumes-to-restic \
  --backup-location-config region=cn-hangzhou \
  --snapshot-location-config region=cn-hangzhou

# 验证安装
kubectl get pods -n velero
velero backup-location get
velero snapshot-location get
```
### 3.4 BackupStorageLocation 与 VolumeSnapshotLocation 配置

Velero 的存储后端通过 CR 声明，便于多后端切换与多集群共享。以下示例显式声明 OSS BSL 与阿里云云盘 VSL：

```yaml
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: alibabacloud
  objectStorage:
    bucket: my-k8s-velero-backup
    prefix: cluster-prod
  config:
    region: cn-hangzhou
    s3ForcePathStyle: "false"
---
apiVersion: velero.io/v1
kind: VolumeSnapshotLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: alibabacloud
  config:
    region: cn-hangzhou
```

### 3.5 最小权限 RAM Policy

为避免使用 `AliyunOSSFullAccess` 这类过宽权限，建议为 Velero 分配最小化 Policy，仅允许访问指定 Bucket：

```json
{
  "Version": "1",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "oss:PutObject",
        "oss:GetObject",
        "oss:DeleteObject",
        "oss:ListParts",
        "oss:ListObjects"
      ],
      "Resource": [
        "acs:oss:*:*:my-k8s-velero-backup",
        "acs:oss:*:*:my-k8s-velero-backup/*"
      ]
    }
  ]
}
```

## 4. Namespace 备份策略

### 4.1 全量 Namespace 备份

以下命令备份 `production` Namespace 下的所有资源与 PV 数据。Velero 会创建 Backup CR 并将元数据写入 OSS，PV 数据通过 CSI 快照或 Restic 备份：

```bash
# 创建包含指定 Namespace 的备份
velero backup create prod-ns-daily \
  --include-namespaces production \
  --snapshot-volumes \
  --ttl 168h0m0s \
  --labels app=production,env=prod

# 查看备份进度
velero backup describe prod-ns-daily --details
velero backup logs prod-ns-daily
```

### 4.2 排除非必要资源

为减少备份体积与恢复时间，应排除 Events、临时 Pod、控制器 ReplicaSet 等可由控制器自动重建的资源：

```yaml
apiVersion: velero.io/v1
kind: Backup
metadata:
  name: prod-ns-daily
  namespace: velero
spec:
  includedNamespaces:
  - production
  excludedResources:
  - events
  - replicasets
  - pods
  snapshotVolumes: true
  ttl: 168h0m0s
  labelSelector:
    matchExpressions:
    - key: backup-exclude
      operator: DoesNotExist
```

### 4.3 按标签备份关键应用

对于多租户 Namespace，可通过标签进一步细分备份范围，例如仅备份带有 `tier=critical` 标签的工作负载：

```bash
# 按标签选择备份对象
velero backup create critical-apps-backup \
  --selector "tier=critical" \
  --include-namespaces production \
  --snapshot-volumes \
  --ttl 72h
```

## 5. 状态应用备份钩子

MySQL、PostgreSQL、MongoDB 等状态应用在备份前需要冻结文件系统或执行一致性转储，否则恢复后可能出现数据损坏。

### 5.1 MySQL pre-backup hook 示例

以下 Backup CR 在备份 `production` Namespace 中标签为 `app=mysql` 的 Pod 前，执行 `FLUSH TABLES WITH READ LOCK`，确保 MyISAM/InnoDB 数据一致性：

```yaml
apiVersion: velero.io/v1
kind: Backup
metadata:
  name: production-mysql-backup
  namespace: velero
spec:
  includedNamespaces:
  - production
  includedResources:
  - pods
  - persistentvolumeclaims
  - services
  - statefulsets
  labelSelector:
    matchLabels:
      app: mysql
  snapshotVolumes: true
  ttl: 168h
  hooks:
    resources:
    - name: mysql-freeze-hook
      includedNamespaces:
      - production
      includedResources:
      - pods
      labelSelector:
        matchLabels:
          app: mysql
      pre:
      - exec:
          container: mysql
          command:
          - /bin/sh
          - -c
          - "mysql -u root -p$MYSQL_ROOT_PASSWORD -e 'FLUSH TABLES WITH READ LOCK; SELECT SLEEP(30);'"
          onError: Fail
          timeout: 60s
      post:
      - exec:
          container: mysql
          command:
          - /bin/sh
          - -c
          - "mysql -u root -p$MYSQL_ROOT_PASSWORD -e 'UNLOCK TABLES;'"
          onError: Continue
          timeout: 30s
```

### 5.2 PostgreSQL 一致性备份钩子

PostgreSQL 推荐使用 `pg_start_backup` / `pg_stop_backup` 或 `pg_basebackup` 进行一致性备份。以下示例在 Velero 快照前触发 `pg_basebackup`：

```yaml
hooks:
  resources:
  - name: postgres-basebackup-hook
    includedNamespaces:
    - production
    labelSelector:
      matchLabels:
        app: postgres
    pre:
    - exec:
        container: postgres
        command:
        - /bin/sh
        - -c
        - |
          rm -rf /var/lib/postgresql/data/backup_label.old
          pg_basebackup -D /backup/pgbase -Ft -z -P -Xs
        onError: Fail
        timeout: 600s
```

## 6. 恢复流程

### 6.1 同集群 Namespace 恢复

当 Namespace 被误删或部分资源损坏时，可从 Velero 备份恢复。恢复前建议先清理目标 Namespace 中冲突资源，或指定恢复目标为新的 Namespace：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出可用备份
velero backup get

# 恢复到原 Namespace
velero restore create prod-ns-restore \
  --from-backup prod-ns-daily \
  --include-namespaces production \
  --wait

# 恢复到新 Namespace（用于验证或隔离）
velero restore create prod-ns-restore-test \
  --from-backup prod-ns-daily \
  --namespace-mappings production:production-restore-test \
  --wait

# 查看恢复结果
velero restore describe prod-ns-restore --details
kubectl get all -n production
```
### 6.2 恢复冲突处理

| 冲突场景 | 处理方案 |
|---|---|
| 目标 Namespace 已存在同名 Deployment | Velero 默认跳过；使用 `--existing-resource-policy=update` 覆盖 |
| PV 与现有 PVC 绑定冲突 | 先删除旧 PVC 或使用 namespace-mappings 隔离 |
| Service ClusterIP 重复 | 恢复时排除 Service，由业务方重新创建 |
| Secret/ConfigMap 已更新 | 使用 `--include-resources` 精确选择恢复对象 |

### 6.3 恢复后的资源 reconcile 与验证

Velero 恢复仅重建资源对象，部分控制器（如 Deployment、StatefulSet）需要一定时间完成 reconcile。恢复后应重点检查以下状态：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查所有工作负载是否达到期望副本数
kubectl get deployments,statefulsets,daemonsets -n production

# 检查 Pod 状态与事件
kubectl get pods -n production -w
kubectl describe pod -n production -l app=critical

# 验证 Service Endpoint 与网络连通性
kubectl get svc -n production
kubectl get endpoints -n production
kubectl run test --image=registry.cn-hangzhou.aliyuncs.com/acs/netshoot --rm -it -- /bin/bash
```
## 7. 跨集群迁移

### 7.1 迁移前置条件

| 检查项 | 说明 |
|---|---|
| 目标集群可访问源 OSS Bucket | 配置跨集群 RAM/OSS 权限 |
| StorageClass 对应关系 | 源集群 `alicloud-disk-ssd` 需映射到目标集群等价 SC |
| 网络策略 | Service CIDR、Pod CIDR 可能不同，需调整 |
| CRD 兼容性 | 源集群 CRD 版本需在目标集群可用 |

### 7.2 执行跨集群迁移

以下流程将 `production` Namespace 从集群 A 迁移到集群 B：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在源集群 A 创建备份
velero backup create migration-prod \
  --include-namespaces production \
  --snapshot-volumes=false \
  --default-volumes-to-restic \
  --ttl 24h

# 确保目标集群 B 的 Velero 指向同一 OSS Bucket
velero backup-location get

# 在目标集群 B 执行恢复
velero restore create migration-prod-restore \
  --from-backup migration-prod \
  --include-namespaces production

# 验证迁移后应用
kubectl get pods -n production
kubectl get svc -n production
```
## 8. 定时备份与保留策略

### 8.1 Schedule CR 示例

以下 Schedule 每天凌晨 2 点执行全量 Namespace 备份，保留 30 天：

```yaml
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: production-daily
  namespace: velero
spec:
  schedule: "0 2 * * *"
  template:
    includedNamespaces:
    - production
    snapshotVolumes: true
    ttl: 720h0m0s
    labelSelector:
      matchExpressions:
      - key: backup-exclude
        operator: DoesNotExist
```

### 8.2 保留策略矩阵

| 备份类型 | 频率 | TTL | 存储后端 |
|---|---|---|---|
| 关键 Namespace 全量 | 每日 | 30 天 | OSS 标准 + 跨区域复制 |
| 全集群元数据 | 每周 | 90 天 | OSS 归档 |
| 变更前快照 | 变更前手动 | 180 天 | OSS 标准 |
| 临时验证备份 | 按需 | 24 小时 | OSS 标准 |

## 9. 验证与演练

### 9.1 备份可恢复性验证

定期从备份创建隔离恢复环境，验证应用可用性：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建测试恢复（不覆盖生产）
velero restore create validate-$(date +%Y%m%d) \
  --from-backup production-daily-20260629020000 \
  --namespace-mappings production:production-drill \
  --include-resources deployments,services,configmaps,secrets,persistentvolumeclaims

# 验证测试 Namespace
kubectl get pods -n production-drill
kubectl exec -n production-drill deploy/app -c app -- /health-check.sh
```
### 9.2 演练报告模板

| 项目 | 内容 |
|---|---|
| 演练日期 | 2026-06-29 |
| 备份名称 | production-daily-20260629020000 |
| 恢复目标 | 新 Namespace production-drill |
| RTO 实际值 | 8 分钟 |
| 发现问题 | Service ClusterIP 在新集群冲突 |
| 改进项 | 恢复脚本中自动替换 ClusterIP |

## 10. 常见问题与故障排查

| 现象 | 根因 | 处理方案 |
|---|---|---|
| Backup 状态为 `PartiallyFailed` | 部分 Pod hook 超时或 PV 快照失败 | 查看 `velero backup logs` 定位具体资源 |
| Restore 后 Pod 无法启动 | PVC 与旧 PV 绑定或镜像拉取失败 | 检查 StorageClass、镜像仓库访问 |
| OSS 上传失败 | AccessKey 失效或 Bucket 无权限 | 验证 credentials-velero 与 RAM 策略 |
| Restic 备份极慢 | 大文件或未启用增量备份 | 评估切换到 CSI 快照或 Kopia |
| 跨集群恢复后 Service 不通 | CIDR 冲突或 SLB 未重新创建 | 删除并重新创建 LoadBalancer Service |
| 备份包含大量 Pod 但无需备份 | 未正确设置 labelSelector | 使用 `--selector` 或 `excludedResources` 过滤 |
| Velero Pod 启动失败 | 插件镜像拉取失败或 OSS 配置错误 | 检查 imagePullSecrets 与 BSL 配置 |
| 恢复后应用无法连接外部服务 | Secret 中的 Token 或证书已过期 | 重新生成 Secret 并更新应用配置 |

## 11. 检查清单

- [ ] 已在专用 Namespace 中部署 Velero Server
- [ ] 已配置 OSS BackupStorageLocation 并验证访问
- [ ] 已根据应用重要性划分备份策略
- [ ] 已为 MySQL/PostgreSQL 等状态应用配置备份钩子
- [ ] 已排除 Events、ReplicaSets 等可重建资源
- [ ] 已创建 Schedule CR 实现定时备份
- [ ] 已配置 TTL 与 OSS Lifecycle 实现保留策略
- [ ] 已验证跨集群恢复能力
- [ ] 已记录 RTO/RPO 目标并季度演练
- [ ] 已监控 Backup/Restore 状态并配置告警

## 12. Related

- [[domain-09-reliability-engineering/02-disaster-recovery/99-velero-backup-recovery-guide.md|Velero 备份恢复指南]]
- [[domain-09-reliability-engineering/02-disaster-recovery/07-kubernetes-backup-restore-deep-dive.md|Kubernetes 备份恢复深度解析]]
- [[domain-09-reliability-engineering/01-backup-recovery/16-enterprise-backup-strategy.md|企业级备份策略]]
- [[domain-09-reliability-engineering/01-backup-recovery/03-pv-backup-snapshot.md|PV 快照：云盘快照、CSI 快照、恢复演练]]
- [[domain-04-storage-data/01-k8s-storage/10-storage-backup-disaster-recovery.md|存储备份与灾难恢复]]
- [[domain-04-storage-data/01-k8s-storage/15-storage-disaster-recovery.md|存储灾难恢复]]


<!-- risk-assessed -->
