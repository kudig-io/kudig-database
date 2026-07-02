---
title: PV 快照：云盘快照、CSI 快照、恢复演练
description: 面向阿里云专有云 K8s 运维工单智能体的 PV 快照实战手册，覆盖阿里云云盘快照、CSI VolumeSnapshot、快照类配置、恢复演练及常见问题排查。
summary: 面向阿里云专有云 K8s 运维工单智能体的 PV 快照实战手册，覆盖阿里云云盘快照、CSI VolumeSnapshot、快照类配置、恢复演练及常见问题排查。
category: reliability-engineering
tags:
- pv
- pvc
- snapshot
- csi
- alicloud-disk
- volume-snapshot
- backup-restore
- ack
- aso
- storage
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: advanced
audience:
- SRE
- 运维工程师
- 存储管理员
- 平台工程师
estimated_read_time: 15min
intent_queries:
- 如何为 PV 创建快照
- CSI VolumeSnapshot 使用步骤
- 阿里云云盘快照与 CSI 快照区别
- PV 快照恢复演练
- 专有云 PV 备份策略
trigger_keywords:
- PV snapshot
- CSI snapshot
- 云盘快照
- volume snapshot
- 存储备份
prerequisites:
- csi-basics
- pv-pvc-basics
- kubectl-basics
- storageclass-basics
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



# PV 快照：云盘快照、CSI 快照、恢复演练

> **适用范围**: Kubernetes v1.28-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐
> **适用场景**: 阿里云 ACK/专有云环境下有状态应用 PV 的快照备份、快速恢复与灾备演练。

## 目录

- [1. 概述](#1-概述)
- [2. PV 备份技术选型](#2-pv-备份技术选型)
- [3. 阿里云云盘快照](#3-阿里云云盘快照)
- [4. CSI VolumeSnapshot](#4-csi-volumesnapshot)
  - [4.5 快照一致性保障：fsfreeze 与数据库 Hook](#45-快照一致性保障fsfreeze-与数据库-hook)
- [5. 从快照恢复 PVC/PV](#5-从快照恢复-pvcpv)
  - [5.3 PVC 扩容与快照恢复后的容量管理](#53-pvc-扩容与快照恢复后的容量管理)
- [6. 恢复演练](#6-恢复演练)
- [7. 跨可用区与跨区域复制](#7-跨可用区与跨区域复制)
- [8. 监控与告警](#8-监控与告警)
- [9. 常见问题与故障排查](#9-常见问题与故障排查)
- [10. 检查清单](#10-检查清单)
- [11. Related](#11-Related)

## 1. 概述

PersistentVolume（PV）是 Kubernetes 有状态应用的核心数据载体。与 etcd 备份不同，PV 备份需要关注块存储或文件存储的一致性、快照原子性及恢复后的挂载可用性。阿里云 ACK 与专有云环境提供两层快照能力：

1. **阿里云云盘快照**：底层块存储快照，由云盘服务提供，一致性高、恢复快。
2. **CSI VolumeSnapshot**：Kubernetes 原生标准接口，通过 `VolumeSnapshotClass` 和 `VolumeSnapshot` CR 管理底层快照，具备可移植性。

本文档面向运维工单智能体，提供从快照技术选型、CSI 配置、恢复到演练的完整操作路径。

## 2. PV 备份技术选型

| 技术 | 一致性 | 恢复速度 | 依赖 | 适用场景 |
|---|---|---|---|---|
| 阿里云云盘快照 | 高（崩溃一致） | 快（分钟级） | 云盘类型、地域 | ACK 公有云/金融云 |
| CSI VolumeSnapshot | 高 | 快 | CSI 驱动支持 | 标准 K8s、多云 |
| Restic/Kopia 文件级 | 中（需应用配合冻结） | 慢 | Velero + DaemonSet | 无 CSI 快照支持的环境 |
| 应用级逻辑备份 | 高（mysqldump/pg_dump） | 慢 | 应用内工具 | 数据库跨版本迁移 |

> **选型建议**：ACK 环境优先使用 CSI VolumeSnapshot 调用云盘快照；专有云若 CSI Snapshot 未就绪，可先用 Restic 过渡，但应尽快补齐 CSI 能力。

## 3. 阿里云云盘快照

### 3.1 通过阿里云控制台创建快照

对于单个关键 PV，可通过 ACK 控制台或 ECS 控制台快速创建快照。操作前需确认云盘类型支持快照（ESSD、SSD、高效云盘均支持）：

```bash
# 通过 aliyun CLI 创建云盘快照
aliyun ecs CreateSnapshot --DiskId d-bp1b7f3z8d4z8z8z8z8z \
  --SnapshotName k8s-pv-prod-mysql-20260629 \
  --Description "生产 MySQL PV 例行快照"

# 查询快照状态
aliyun ecs DescribeSnapshots --DiskId d-bp1b7f3z8d4z8z8z8z8z \
  --RegionId cn-hangzhou
```

### 3.2 自动快照策略

在阿里云控制台为云盘开启自动快照策略，可实现每日/每周定时快照。建议为生产环境关键云盘绑定策略，并设置保留天数：

```bash
# 创建自动快照策略
aliyun ecs CreateAutoSnapshotPolicy \
  --regionId cn-hangzhou \
  --timePoints '["02"]' \
  --repeatWeekdays '["1","3","5"]' \
  --retentionDays 30 \
  --autoSnapshotPolicyName k8s-prod-daily

# 将策略应用到指定云盘
aliyun ecs ApplyAutoSnapshotPolicy \
  --autoSnapshotPolicyId sp-bp1z8d4z8z8z8z8z \
  --diskIds '["d-bp1b7f3z8d4z8z8z8z8z"]'
```

## 4. CSI VolumeSnapshot

### 4.1 确认 CSI 驱动支持快照

在创建 VolumeSnapshot 前，需确认集群已部署支持快照的 CSI 驱动，并已启用 `VolumeSnapshotDataSource` 特性（v1.28+ 默认启用）：

```bash
# 检查 CSI 驱动与 Snapshotter
kubectl get csidriver
kubectl get pods -n kube-system | grep csi

# 检查 VolumeSnapshot CRD
kubectl get crd | grep snapshot
# 预期输出包含 volumesnapshotclasses.snapshot.storage.k8s.io
```

### 4.2 创建 VolumeSnapshotClass

`VolumeSnapshotClass` 定义了快照的底层实现与参数。以下示例使用阿里云磁盘 CSI 驱动的 `alibabacloud-disk`：

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: alicloud-disk-snapshot
  annotations:
    snapshot.storage.kubernetes.io/is-default-class: "true"
driver: diskplugin.csi.alibabacloud.com
parameters:
  forceDelete: "false"
  # 是否立即删除底层快照（生产环境建议 false）
deletionPolicy: Retain
```

### 4.3 为 PVC 创建 VolumeSnapshot

以下 YAML 为 `production` Namespace 中名为 `mysql-data` 的 PVC 创建快照。快照名称包含时间戳，便于版本管理：

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-data-snapshot-20260629
  namespace: production
spec:
  volumeSnapshotClassName: alicloud-disk-snapshot
  source:
    persistentVolumeClaimName: mysql-data
```

创建后可通过以下命令查看快照进度与状态：

```bash
# 查看 VolumeSnapshot 状态
kubectl get volumesnapshot -n production
kubectl describe volumesnapshot mysql-data-snapshot-20260629 -n production

# 查看底层 VolumeSnapshotContent
kubectl get volumesnapshotcontent
```

### 4.4 快照类参数对比

| 参数 | 取值 | 含义 |
|---|---|---|
| `deletionPolicy` | Delete / Retain | 删除 VolumeSnapshot 时是否级联删除底层快照 |
| `forceDelete` | true / false | 是否强制删除正在创建或已完成的快照 |
| `instantSnapshot` | true / false | 是否创建即时快照（部分云盘类型支持） |

### 4.5 快照一致性保障：fsfreeze 与数据库 Hook

云盘快照提供崩溃一致性，但无法保证文件系统级或应用级一致性。对于数据库等关键应用，建议在创建快照前冻结文件系统或执行数据库一致性命令：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 在数据库 Pod 内冻结文件系统（需特权容器）
kubectl exec -n production mysql-0 -- fsfreeze -f /var/lib/mysql

# 创建 VolumeSnapshot（此时建议通过脚本自动化）
kubectl apply -f volumesnapshot-mysql.yaml

# 等待快照 readyToUse 后解冻
kubectl wait --for=jsonpath='{.status.readyToUse}'=true volumesnapshot/mysql-data-snapshot -n production --timeout=600s
kubectl exec -n production mysql-0 -- fsfreeze -u /var/lib/mysql
```

对于 MySQL，更推荐结合 Velero 备份钩子执行 `FLUSH TABLES WITH READ LOCK`；对于 PostgreSQL，可使用 `pg_start_backup` / `pg_stop_backup`。文件系统冻结适用于无数据库 hook 能力的通用有状态应用。

## 5. 从快照恢复 PVC/PV

### 5.1 基于 VolumeSnapshot 恢复 PVC

恢复时通过 `dataSource` 指定 VolumeSnapshot 名称，CSI 驱动会自动从快照创建新 PV 并绑定到 PVC：

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data-restore
  namespace: production-drill
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 100Gi
  dataSource:
    name: mysql-data-snapshot-20260629
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

创建 PVC 后，CSI 驱动会自动生成 PV 并完成绑定。可通过以下命令验证：

```bash
# 验证 PVC 已绑定
kubectl get pvc mysql-data-restore -n production-drill

# 验证 Pod 可使用恢复后的 PVC
kubectl get pods -n production-drill -l app=mysql
```

### 5.2 恢复到原 PVC 的注意事项

如果需要覆盖原 PVC（例如回滚场景），必须先删除原 PVC 与 PV，并确保应用已停止写入，否则会造成数据不一致：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 1. 缩容应用，停止写入
kubectl scale statefulset/mysql --replicas=0 -n production

# 2. 删除原 PVC（保留 PV 或根据策略删除）
kubectl delete pvc mysql-data -n production

# 3. 从快照创建同名 PVC
kubectl apply -f pvc-restore-same-name.yaml

# 4. 扩容应用
kubectl scale statefulset/mysql --replicas=1 -n production
```

### 5.3 PVC 扩容与快照恢复后的容量管理

从快照恢复时，新 PVC 的容量必须大于或等于快照源 PVC 的容量。若需扩容，可在恢复后执行 PVC resize，但需确认 StorageClass 的 `allowVolumeExpansion: true`：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 检查 StorageClass 是否支持扩容
kubectl get storageclass alicloud-disk-ssd -o jsonpath='{.allowVolumeExpansion}'

# 从快照恢复并请求更大容量
kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data-restore-expanded
  namespace: production
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 200Gi
  dataSource:
    name: mysql-data-snapshot-20260629
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
EOF

# 等待文件系统自动扩容
kubectl wait --for=jsonpath='{.status.capacity.storage}'=200Gi pvc/mysql-data-restore-expanded -n production --timeout=300s
```

## 6. 恢复演练

### 6.1 演练场景设计

| 场景 | 目标 | 预期结果 |
|---|---|---|
| 单 PVC 误删恢复 | 从 VolumeSnapshot 恢复应用到测试环境 | 数据完整，应用可启动 |
| 云盘快照跨 AZ 恢复 | 验证快照可在其他可用区创建磁盘 | RTO ≤ 30 分钟 |
| 数据库一致性恢复 | 结合应用 hook 验证数据一致性 | 无表损坏或事务丢失 |
| 全 Namespace 灾难恢复 | 配合 Velero 恢复资源对象 + PV 快照 | 业务功能完整 |

### 6.2 演练执行脚本

以下脚本在隔离 Namespace 中执行快照恢复演练，确保不影响生产：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
set -euo pipefail

SOURCE_NS=production
DRILL_NS=production-drill
SNAP_NAME=mysql-data-snapshot-$(date +%Y%m%d)
PVC_NAME=mysql-data

# 1. 创建快照
cat <<EOF | kubectl apply -f -
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: ${SNAP_NAME}
  namespace: ${SOURCE_NS}
spec:
  volumeSnapshotClassName: alicloud-disk-snapshot
  source:
    persistentVolumeClaimName: ${PVC_NAME}
EOF

# 2. 等待快照就绪
kubectl wait --for=jsonpath='{.status.readyToUse}'=true volumesnapshot/${SNAP_NAME} -n ${SOURCE_NS} --timeout=600s

# 3. 创建演练 Namespace
kubectl create namespace ${DRILL_NS} --dry-run=client -o yaml | kubectl apply -f -

# 4. 从快照恢复 PVC
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: ${PVC_NAME}
  namespace: ${DRILL_NS}
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 100Gi
  dataSource:
    name: ${SNAP_NAME}
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
EOF

# 5. 部署测试应用并验证
kubectl apply -n ${DRILL_NS} -f manifests/mysql-drill.yaml
kubectl wait --for=condition=ready pod -l app=mysql -n ${DRILL_NS} --timeout=300s
kubectl exec -n ${DRILL_NS} deploy/mysql -- mysql -uroot -p${MYSQL_ROOT_PASSWORD} -e "SHOW DATABASES;"
```

## 7. 跨可用区与跨区域复制

### 7.1 跨可用区恢复

阿里云云盘快照默认可在同一地域的不同可用区创建磁盘。恢复时只需指定目标可用区即可：

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data-azb
  namespace: production
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 100Gi
  dataSource:
    name: mysql-data-snapshot-20260629
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

> **注意**：具体可用区由 StorageClass 的 `zoneId` 参数或拓扑约束决定，需在 PVC 中通过 `nodeAffinity` 或 `allowedTopologies` 指定目标 AZ。

### 7.2 跨区域复制

CSI 原生不支持跨区域快照复制。对于跨区域灾备，建议结合以下方案：

| 方案 | 实现方式 | 适用场景 |
|---|---|---|
| OSS 镜像复制 | 对文件类数据使用 OSS 跨区域复制 | 非结构化数据 |
| 数据库级同步 | MySQL 主从、PostgreSQL 流复制 | 结构化数据 |
| 快照导出为镜像 | 通过 ECS 快照创建自定义镜像并复制 | 整机级灾备 |

### 7.3 跨地域复制最佳实践

对于金融级跨地域灾备，建议将 CSI 快照与数据库级同步结合使用：

- **RPO ≤ 15 分钟**：使用数据库原生同步（如 MySQL semi-sync）。
- **RPO ≤ 1 小时**：使用 CSI 快照 + 跨地域镜像复制。
- **RTO ≤ 30 分钟**：在灾备集群预置等量资源并定期演练切换。

## 8. 监控与告警

### 8.1 关键监控指标

| 指标 | 采集方式 | 告警阈值 |
|---|---|---|
| 快照创建耗时 | CSI Snapshotter metrics | > 15 分钟 |
| 快照 readyToUse 状态 | kube_state_metrics | readyToUse=false 持续 > 10 分钟 |
| 快照数量/容量 | OSS / 云盘 API | 超过配额 80% |
| 恢复 PVC 绑定耗时 | kubelet + CSI metrics | > 5 分钟 |

### 8.2 PrometheusRule 示例

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: csi-snapshot-alerts
  namespace: monitoring
spec:
  groups:
  - name: csi-snapshot
    rules:
    - alert: VolumeSnapshotNotReady
      expr: |
        kube_volumesnapshot_status_ready_to_use != 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "VolumeSnapshot 长时间未就绪"
        description: "快照 {{ $labels.name }} 在 Namespace {{ $labels.namespace }} 中超过 10 分钟未 ready"
```

## 9. 常见问题与故障排查

| 现象 | 根因 | 处理方案 |
|---|---|---|
| VolumeSnapshot 状态为 `Pending` | CSI Snapshotter 未启动或 CRD 缺失 | 检查 `snapshot-controller` Pod 与 CRD |
| 快照创建超时 | 云盘 I/O 高或快照配额不足 | 错峰备份或申请配额 |
| 恢复后文件系统损坏 | 快照时应用仍在写 | 配置应用级冻结或一致性 hook |
| PVC 处于 Pending | StorageClass 与快照类型不匹配 | 确认原 PV 与目标 SC 使用相同云盘类型 |
| 跨区域无法恢复 | CSI 不支持跨 Region 快照 | 使用数据库同步或镜像复制方案 |
| 快照容量大于源 PVC | 快照包含已删除但未释放的数据块 | 执行文件系统 trim 或重新创建快照 |
| 扩容后文件系统未扩展 | 未启用 allowVolumeExpansion | 修改 StorageClass 或手动扩展文件系统 |

## 10. 检查清单

- [ ] 已确认 CSI 驱动支持 VolumeSnapshot
- [ ] 已创建 VolumeSnapshotClass 并设置合适的 deletionPolicy
- [ ] 已为关键 PVC 创建初始快照并验证 readyToUse
- [ ] 已配置定时快照策略（CSI Schedule 或云盘自动快照）
- [ ] 已为数据库等应用配置一致性 hook
- [ ] 已验证从快照恢复 PVC 的流程
- [ ] 已执行跨 AZ 恢复演练
- [ ] 已配置快照监控与告警
- [ ] 已制定快照保留与清理策略
- [ ] 已记录快照与原始 PV 的映射关系

## 11. Related

- [[domain-04-storage-data/01-k8s-storage/10-storage-backup-disaster-recovery.md|存储备份与灾难恢复]]
- [[domain-04-storage-data/01-k8s-storage/15-storage-disaster-recovery.md|存储灾难恢复]]
- [[domain-04-storage-data/01-k8s-storage/05-csi-drivers-integration.md|CSI 驱动集成]]
- [[domain-04-storage-data/01-k8s-storage/03-pvc-patterns-practices.md|PVC 模式与最佳实践]]
- [[domain-09-reliability-engineering/01-backup-recovery/16-enterprise-backup-strategy.md|企业级备份策略]]
- [[domain-09-reliability-engineering/01-backup-recovery/02-namespace-backup-restore.md|Namespace 级别备份恢复：Velero]]
- [[domain-09-reliability-engineering/02-disaster-recovery/99-velero-backup-recovery-guide.md|Velero 备份恢复指南]]
