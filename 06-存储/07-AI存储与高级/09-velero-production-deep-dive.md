---
title: "Velero 生产深度指南"
description: "Velero 备份恢复系统生产部署、策略配置、CSI 快照集成与故障排查"
summary: "覆盖 Velero 架构（Server/Restic/Kopia/BSL/VSL）、Helm 生产部署、备份策略、恢复操作、CSI 快照集成、跨集群迁移与性能调优"
category: 存储
tags:
- storage
- velero
- backup
- disaster-recovery
- csi-snapshot
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- AI 工程师
estimated_read_time: 20min
intent_queries:
- "Velero 如何在生产环境部署和配置"
- "Velero 备份恢复失败如何排查"
- "Velero CSI 快照集成如何配置"
trigger_keywords:
- Velero
- 备份
- 恢复
- Restic
- Kopia
- BSL
- 快照
prerequisites:
- kubectl-basics
- storage-basics
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

# Velero 生产深度指南

## 概述

Velero 是 Kubernetes 生态中最成熟的集群级备份恢复工具，支持资源备份、PV 数据备份（通过 Restic/Kopia 文件级备份或 CSI 快照）、定时调度、跨集群迁移和灾难恢复。在生产环境中，Velero 是 RPO（Recovery Point Objective）和 RTO（Recovery Time Objective）保障的关键组件。

本文深入 Velero 的生产部署细节，覆盖从架构原理到日常运维的完整知识体系，特别关注大规模集群（1000+ Namespace、PB 级存储）下的性能调优和故障排查经验。对于 AI 平台，Velero 是保护训练 Checkpoint、模型 artifact 和实验配置的最后防线。

## 架构与核心概念

### Velero 组件架构

```
Velero Architecture:

┌─────────────────────────────────────────────────────┐
│  Velero Server (Deployment)                          │
│  ├── Backup Controller (处理 Backup CR)              │
│  ├── Restore Controller (处理 Restore CR)            │
│  ├── Schedule Controller (处理 Schedule CR)          │
│  ├── DownloadRequest Controller                      │
│  └── Plugin Framework (BSL/VSL/ItemAction)          │
├─────────────────────────────────────────────────────┤
│  Node Agent (DaemonSet) - Restic 或 Kopia            │
│  ├── 文件级 PV 数据备份                               │
│  ├── 增量备份支持                                     │
│  └── 每节点独立执行                                   │
├─────────────────────────────────────────────────────┤
│  Backup Storage Location (BSL)                       │
│  ├── S3 / MinIO / GCS / Azure Blob                  │
│  └── 存储备份元数据 + Restic/Kopia 仓库              │
├─────────────────────────────────────────────────────┤
│  Volume Snapshot Location (VSL)                      │
│  ├── AWS EBS Snapshot / Azure Snapshot / GCP PD     │
│  └── CSI VolumeSnapshot                             │
└─────────────────────────────────────────────────────┘
```

### 备份模式对比

| 备份方式 | 机制 | 优点 | 缺点 | 适用场景 |
|---------|------|------|------|---------|
| CSI Snapshot | VolumeSnapshot API | 快速、一致性好 | 依赖 CSI 驱动支持 | 块存储 PV |
| Restic/Kopia | 文件级备份到对象存储 | 跨平台、增量 | 慢、占带宽 | NFS/共享存储 |
| 原生快照 + 转换 | 云快照 → 对象存储 | 可跨集群恢复 | 流程复杂 | 跨云迁移 |
| 资源清单备份 | 仅备份 YAML | 极快 | 不含 PV 数据 | 配置/元数据 |

### 关键 CRD

- **Backup**：单次备份任务
- **Schedule**：定时备份策略（Cron 表达式）
- **Restore**：恢复任务
- **BackupStorageLocation**：备份存储后端
- **VolumeSnapshotLocation**：卷快照后端
- **DeleteBackupRequest**：备份删除请求

## 生产部署

### Helm 生产部署

🟡 中风险：部署 Velero 会创建集群级 RBAC 和 DaemonSet

```yaml
# velero-values.yaml (生产配置)
configuration:
  backupStorageLocation:
    - name: default
      provider: aws
      bucket: k8s-velero-backups
      config:
        region: us-east-1
        s3Url: https://minio.velero-system.svc:9000
        s3ForcePathStyle: "true"
      credential:
        name: velero-bsl-credentials
        key: cloud
  volumeSnapshotLocation:
    - name: default
      provider: aws
      config:
        region: us-east-1
  features: EnableCSI
  defaultVolumesToFsBackup: false
  logLevel: info
  logFormat: json

# Node Agent 配置（Kopia 替代 Restic）
nodeAgent:
  podVolumePath: /var/lib/kubelet/pods
  uploaderType: kopia
  resources:
    requests:
      cpu: 500m
      memory: 512Mi
    limits:
      cpu: "2"
      memory: 4Gi
  # 限制并发备份数
  extraArgs:
    - --default-repo-maintain-frequency=168h

# Velero Server 资源
resources:
  requests:
    cpu: "1"
    memory: 1Gi
  limits:
    cpu: "2"
    memory: 4Gi

# 并发控制
concurrency:
  backup: 4
  restore: 2

# 指标暴露
metrics:
  enabled: true
  serviceMonitor:
    enabled: true

# 优先级（避免被驱逐）
priorityClassName: system-cluster-critical

initContainers:
  - name: velero-plugin-for-aws
    image: velero/velero-plugin-for-aws:v1.10.0
    volumeMounts:
      - mountPath: /target
        name: plugins
  - name: velero-plugin-for-csi
    image: velero/velero-plugin-for-csi:v0.7.0
    volumeMounts:
      - mountPath: /target
        name: plugins
```

```bash
# 🟡 中风险：安装 Velero
helm repo add vmware-tanzu https://vmware-tanzu.github.io/helm-charts
helm install velero vmware-tanzu/velero \
  --namespace velero-system \
  --create-namespace \
  -f velero-values.yaml
```

### 备份策略配置

🟡 中风险：Schedule 创建后会按计划自动执行备份

```yaml
# AI 平台全量备份（每天凌晨）
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: ai-platform-daily
  namespace: velero-system
spec:
  schedule: "0 2 * * *"
  template:
    ttl: 168h  # 保留 7 天
    includedNamespaces:
      - ai-training
      - ai-inference
      - ai-platform
    excludedResources:
      - events
      - events.events.k8s.io
    storageLocation: default
    volumeSnapshotLocations:
      - default
    # CSI 快照备份 PV
    snapshotVolumes: true
    snapshotMoveData: false
    # 包含集群级资源
    includeClusterResources: true
    orLabelSelectors:
      - matchLabels:
          backup: "ai-platform"
---
# Checkpoint 高频备份（每 6 小时）
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: checkpoint-backup-6h
  namespace: velero-system
spec:
  schedule: "0 */6 * * *"
  template:
    ttl: 72h  # 保留 3 天
    includedNamespaces:
      - ai-training
    labelSelector:
      matchLabels:
        data-type: checkpoint
    storageLocation: default
    snapshotVolumes: true
    defaultVolumesToFsBackup: false
---
# 集群元数据备份（每小时）
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: cluster-metadata-hourly
  namespace: velero-system
spec:
  schedule: "0 * * * *"
  template:
    ttl: 48h
    includedResources:
      - namespaces
      - configmaps
      - secrets
      - serviceaccounts
      - roles
      - rolebindings
      - clusterroles
      - clusterrolebindings
    snapshotVolumes: false
    storageLocation: default
```

### CSI 快照集成

🟡 中风险：CSI 快照需要 VolumeSnapshotClass 和驱动支持

```yaml
# 确保 VolumeSnapshotClass 配置正确
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: velero-csi-snapshot
  labels:
    velero.io/csi-volumesnapshot-class: "true"
driver: ebs.csi.aws.com
deletionPolicy: Retain
parameters:
  tagSpecification_1: "velero-backup=true"
---
# 手动触发带 CSI 快照的备份
apiVersion: velero.io/v1
kind: Backup
metadata:
  name: manual-backup-20260719
  namespace: velero-system
spec:
  includedNamespaces:
    - ai-training
  snapshotVolumes: true
  snapshotMoveData: true  # 将快照数据移动到对象存储（跨集群恢复）
  storageLocation: default
  volumeSnapshotLocations:
    - default
  ttl: 720h
```

## 运维操作

### 恢复操作

🔴 高风险：恢复操作会覆盖现有资源

```bash
# 🟢 低风险/只读：查看可用备份
velero backup get -n velero-system

# 🟢 低风险/只读：查看备份详情
velero backup describe ai-platform-daily-20260719020000 -n velero-system --details

# 🔴 高风险：全 Namespace 恢复
velero restore create restore-ai-training-20260719 \
  --from-backup ai-platform-daily-20260719020000 \
  --include-namespaces ai-training \
  --restore-volumes=true \
  --namespace-mappings ai-training:ai-training-restored \
  -n velero-system

# 🔴 高风险：单资源恢复
velero restore create restore-single-configmap \
  --from-backup cluster-metadata-hourly-20260719120000 \
  --include-resources configmaps \
  --include-namespaces ai-platform \
  --selector app=training-config \
  -n velero-system

# 🟢 低风险/只读：监控恢复进度
velero restore describe restore-ai-training-20260719 -n velero-system
velero restore logs restore-ai-training-20260719 -n velero-system
```

### 跨集群迁移

🔴 高风险：跨集群恢复涉及数据一致性和资源冲突

```bash
# 步骤 1: 在源集群创建备份（含数据移动）
velero backup create migration-backup \
  --include-namespaces ai-training \
  --snapshot-move-data=true \
  --storage-location default \
  -n velero-system

# 步骤 2: 等待备份完成（数据上传到对象存储）
velero backup describe migration-backup -n velero-system -w

# 步骤 3: 在目标集群配置相同的 BSL
# （目标集群 Velero 指向同一对象存储桶）

# 步骤 4: 在目标集群执行恢复
velero restore create migration-restore \
  --from-backup migration-backup \
  --include-namespaces ai-training \
  --restore-volumes=true \
  -n velero-system
```

### 性能调优

🟡 中风险：调整并发参数可能增加节点负载

```bash
# 调整 Velero 并发备份数
kubectl patch deployment velero -n velero-system --type='json' -p='[
  {"op": "add", "path": "/spec/template/spec/containers/0/args/-", "value": "--client-burst=100"},
  {"op": "add", "path": "/spec/template/spec/containers/0/args/-", "value": "--client-qps=50"}
]'

# 调整 Node Agent (Kopia) 并发
kubectl patch daemonset node-agent -n velero-system --type='json' -p='[
  {"op": "add", "path": "/spec/template/spec/containers/0/env/-", "value": {"name": "KUIA_UPLOAD_CONCURRENCY", "value": "4"}},
  {"op": "add", "path": "/spec/template/spec/containers/0/env/-", "value": {"name": "KUIA_DOWNLOAD_CONCURRENCY", "value": "4"}}
]'
```

## 故障排查

### 备份超时

🟢 低风险/只读：诊断备份失败原因

```bash
# 查看备份状态和错误
velero backup describe <backup-name> -n velero-system --details

# 查看 Velero Server 日志
kubectl logs -n velero-system deployment/velero --tail=100 | grep -i "error\|timeout\|fail"

# 查看 Node Agent 日志（Restic/Kopia 备份失败）
kubectl logs -n velero-system daemonset/node-agent --tail=50

# 检查 BSL 连通性
kubectl exec -n velero-system deployment/velero -- \
  velero backup-location get -n velero-system
```

### 常见故障速查

| 故障现象 | 可能原因 | 排查方法 | 修复措施 |
|---------|---------|---------|---------|
| Backup PartiallyFailed | 部分资源备份失败 | `velero backup describe --details` | 检查资源权限/excluded |
| Restic 锁冲突 | 上次备份异常终止 | 查看 Node Agent 日志 | `velero repo unlock` |
| CSI 快照失败 | VolumeSnapshotClass 缺失 | 检查 VSC 标签 | 添加 velero.io 标签 |
| 恢复后 PVC Pending | 快照不存在/跨 AZ | `kubectl describe pvc` | 确认快照可用性 |
| BSL 连接失败 | 凭证过期/网络不通 | `velero backup-location get` | 更新 Secret/检查网络 |
| 备份速度极慢 | 大量小文件/带宽限制 | 检查 Node Agent 资源 | 增加并发/带宽 |

### Restic/Kopia 锁处理

🔴 高风险：强制解锁可能导致数据不一致

```bash
# 🟢 低风险/只读：查看仓库锁状态
velero repo get -n velero-system

# 🟡 中风险：解锁仓库（确认无活跃备份后）
velero repo unlock --repo-name <repo-name> -n velero-system

# 检查是否有残留的备份 Pod
kubectl get pods -n velero-system -l velero.io/backup-name
```

## 最佳实践

1. **3-2-1 备份策略**：3 份副本、2 种介质、1 份异地，BSL 配置跨区域对象存储
2. **定期恢复演练**：每月至少执行一次恢复演练，验证备份有效性，参考 [[12-可靠性/02-灾难恢复/18-disaster-recovery-drills.md|灾备演练]]
3. **CSI 快照优先**：块存储 PV 使用 CSI 快照备份（速度快），NFS/共享存储使用 Kopia 文件级备份
4. **TTL 管理**：按数据重要性设置不同 TTL，避免对象存储成本失控
5. **监控告警**：监控备份成功率、持续时间、BSL 容量，参考 [[06-存储/01-K8s存储/13-storage-monitoring-alerting.md|存储监控告警]]
6. **AI Checkpoint 保护**：为训练 Checkpoint PVC 添加 `backup=ai-platform` 标签，纳入高频备份策略
7. **跨集群 DR**：配置 `snapshotMoveData=true` 实现跨集群/跨云恢复能力，参考 [[12-可靠性/02-灾难恢复/01-multi-region-dr-architecture.md|多区域灾备架构]]
8. **版本升级**：Velero 升级前备份 BSL 配置，遵循 N-1 兼容原则
9. **RBAC 最小化**：Velero ServiceAccount 仅授予备份所需的最小权限，参考 [[06-存储/07-AI存储与高级/08-storage-multitenant-isolation.md|存储多租户隔离]]

## Related

- [[12-可靠性/01-备份恢复/03-pv-backup-snapshot.md|PV 备份与快照]]
- [[12-可靠性/02-灾难恢复/26-velero-backup-recovery-guide.md|Velero 备份恢复指南]]
- [[06-存储/03-分布式存储/01-velero-backup-recovery.md|Velero 备份恢复]]
- [[06-存储/01-K8s存储/11-storage-backup-disaster-recovery.md|存储备份与灾备]]
- [[12-可靠性/02-灾难恢复/01-multi-region-dr-architecture.md|多区域灾备架构]]
