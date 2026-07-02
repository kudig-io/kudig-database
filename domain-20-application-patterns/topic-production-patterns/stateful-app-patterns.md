---
title: Stateful 应用生产模式
description: 生产级状态应用：StatefulSet 拓扑、PVC 快照备份恢复、Headless Service 与有序升级实践
summary: 生产级状态应用：StatefulSet 拓扑、PVC 快照备份恢复、Headless Service 与有序升级实践，含数据一致性与灾备清单。
category: application-patterns
tags:
- statefulset
- pvc
- backup
- restore
- headless-service
- production
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 数据库工程师
estimated_read_time: 18min
intent_queries:
- StatefulSet 生产模式是什么
- 如何做 PVC 备份恢复
trigger_keywords:
- StatefulSet
- PVC
- 快照
- 备份恢复
- Headless
prerequisites:
- kubectl-basics
- statefulset-basics
- storage-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含涉及数据持久化的运维操作。执行前务必确认备份可用。命令风险等级标注：🔴 高风险（可能造成数据丢失）、🟡 中风险、🟢 低风险。

# Stateful 应用生产模式

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

StatefulSet 管理 有状态应用（数据库、消息队列、分布式存储）。与 Deployment 不同，StatefulSet 保证 Pod 的稳定网络标识、有序启停和持久化存储绑定。错误的状态应用运维操作（如强制删除 Pod、跳过备份直接升级）是数据丢失的高频根因。本文涵盖 StatefulSet 生产拓扑、PVC 备份恢复、有序升级和数据一致性保障。

---

## 1. StatefulSet 生产拓扑

### 1.1 核心特性

| 特性 | 含义 | 生产影响 |
|---|---|---|
| 稳定网络标识 | Pod 名: `<sts>-0`, `<sts>-1`... | 客户端可直连特定实例（主从选举） |
| 稳定存储 | PVC 绑定: `<sts>-<pod>-<pvc>`，Pod 重建后重连同一 PVC | 数据不随 Pod 重启丢失 |
| 有序启停 | 创建 0→N，删除 N→0 | 滚动升级逐个进行，主节点最后升级 |
| 滚动升级策略 | `OnDelete` / `RollingUpdate` | `RollingUpdate` + `partition` 可金丝雀升级 |

### 1.2 生产 StatefulSet 骨架

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: database
spec:
  serviceName: database-headless    # 必须关联 Headless Service
  replicas: 3
  podManagementPolicy: OrderedReady # 或 Parallel（无主从依赖时用）
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      partition: 0                  # >0 时仅升级 ordinal < partition 的 Pod（金丝雀）
  template:
    spec:
      terminationGracePeriodSeconds: 180   # 数据库优雅关闭需更长时间
      containers:
        - name: db
          readinessProbe:           # 状态应用就绪检测更严格
            ...
          lifecycle:
            preStop:
              exec:
                command: ["/bin/sh", "-c", "pg_ctl stop -m fast"]   # 优雅关闭
  volumeClaimTemplates:             # 每个 Pod 独立 PVC
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 100Gi
```

> ⚠️ **生产红线**: StatefulSet 的 `terminationGracePeriodSeconds` 必须 ≥ 数据库优雅关闭时间。默认 30s 对数据库远远不够，强杀可能导致数据损坏。生产建议 120-300s。

### 1.3 podManagementPolicy 决策

| 策略 | 行为 | 适用场景 |
|---|---|---|
| `OrderedReady` (默认) | 严格顺序：0 启动成功后 1 才启动 | 主从复制（需先选主） |
| `Parallel` | 并行启动所有 Pod | 对等集群（如 Elasticsearch、无主依赖） |

---

## 2. PVC 备份与恢复

### 2.1 三层备份策略

| 层级 | 工具 | 频率 | RPO | 适用 |
|---|---|---|---|---|
| **应用级备份** | `pg_dump` / `mysqldump` / 复制 | 每日 | 24h | 跨平台恢复、逻辑迁移 |
| **PVC 快照** | VolumeSnapshot (CSI) | 每小时 | 1h | 快速回滚到时间点 |
| **集群级备份** | Velero | 每日 | 24h | 灾难恢复、跨集群迁移 |

### 2.2 VolumeSnapshot 生产实践

```yaml
# 1. 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: db-snap-20260702
spec:
  volumeSnapshotClassName: csi-snap-class
  source:
    persistentVolumeClaimName: data-database-0
---
# 2. 从快照恢复（创建新 PVC）
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-database-0-restored
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: fast-ssd
  resources:
    requests:
      storage: 100Gi
  dataSource:                     # 从快照创建
    name: db-snap-20260702
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

🟢 低风险。验证快照:
```bash
kubectl get volumesnapshot db-snap-20260702 -o jsonpath='{.status.readyToUse}'
# 应为 true
```

> ⚠️ **前置条件**: CSI 驱动必须支持快照（`VolumeSnapshotClass` 已配置）。并非所有存储后端都支持一致性快照——数据库需先 `fsync`/checkpoint 再快照，否则快照可能不一致。

### 2.3 Velero 状态应用备份

```bash
# 🟡 中风险：备份包含 PV 数据，注意存储成本
# 备份单个 StatefulSet（含 PV，需 restic 或 CSI snapshot）
velero backup create db-backup-20260702 \
  --include-namespaces production \
  --include-resources statefulsets,pods,persistentvolumeclaims \
  --selector app=database \
  --snapshot-volumes=true

# 🔴 高风险：恢复前确认目标 namespace 已隔离
velero restore create --from-backup db-backup-20260702 \
  --namespace-mappings production:production-restored
```

---

## 3. 有序升级与金丝雀

### 3.1 partition 金丝雀升级

通过 `partition` 字段控制仅升级部分 Pod，实现状态应用的金丝雀发布：

```bash
# 🟡 中风险：仅升级 ordinal ≥ partition 的 Pod
# 设置 partition=2 → 仅 Pod-2 升级（Pod-0/1 保持旧版）
kubectl patch statefulset database --type='json' \
  -p='[{"op":"replace","path":"/spec/updateStrategy/rollingUpdate/partition","value":2}]'

# 观察 Pod-2 升级后的数据一致性、性能指标
kubectl rollout status statefulset/database --watch

# 满意后逐步推进: partition=1 → partition=0
```

### 3.2 主从架构升级顺序

对于主从复制数据库，升级顺序至关重要：

```
升级顺序（partition 递减，StatefulSet 从高 ordinal 开始升级）:
  Pod-2 (Replica) → Pod-1 (Replica) → Pod-0 (Primary，最后)

⚠️ Pod-0 是 Primary 时，升级前需先手动 switchover:
  1. 在应用层触发主从切换，将 Primary 移到 Pod-1/2
  2. 确认 Pod-0 降级为 Replica 且数据同步完成
  3. 升级 Pod-0
```

> 🔴 高风险。状态应用升级**必须**有回滚方案: 数据库二进制降级通常不被支持（如 PG major version downgrade）。升级前**必须**有可用快照，且需在非生产验证数据迁移脚本。

---

## 4. 数据一致性保障

### 4.1 Pod 删除与 PVC 残留

StatefulSet Pod 被删除后，PVC **不会自动删除**（这是设计行为，防止数据丢失）。但这可能导致：

- 缩容后旧 PVC 残留占用存储成本
- 新 Pod 误绑定旧 PVC 的脏数据

```bash
# 🟢 检查残留 PVC
kubectl get pvc | grep data-database

# 🟡 中风险：清理缩容后的残留 PVC（确认不再需要）
kubectl delete pvc data-database-2   # 仅当 StatefulSet 已永久缩容
```

### 4.2 `persistentVolumeClaimRetentionPolicy`

v1.27+ 支持自动清理 PVC（beta v1.32）：

```yaml
spec:
  persistentVolumeClaimRetentionPolicy:
    whenDeleted: Retain              # StatefulSet 删除时保留 PVC（安全默认）
    whenScaled: Delete               # 缩容时删除对应 PVC（谨慎：数据丢失）
```

> ⚠️ `whenScaled: Delete` 会导致缩容时数据永久删除。仅在确信缩容是永久性的（如临时扩容的只读副本）时使用。

---

## 5. 生产检查清单

| # | 检查项 | 验证命令 | 合格标准 |
|---|---|---|---|
| 1 | terminationGracePeriodSeconds ≥ 关闭时间 | `kubectl get sts -o yaml \| grep terminationGrace` | 数据库 ≥ 120s |
| 2 | volumeClaimTemplates 使用 SSD 存储类 | 检查 storageClassName | 数据库用 fast-ssd，非 HDD |
| 3 | VolumeSnapshotClass 已配置 | `kubectl get volumesnapshotclass` | CSI 快照可用 |
| 4 | 定期快照 CronJob 运行 | `kubectl get cronjob \| grep snapshot` | 每小时快照 + 每日全量 |
| 5 | 备份恢复演练已执行 | 查阅演练记录 | 季度演练，RTO 达标 |
| 6 | persistentVolumeClaimRetentionPolicy 已配置 | 检查 sts spec | 显式声明 Retain/Delete 策略 |
| 7 | 升级前有可用快照 | `kubectl get volumesnapshot` | 升级窗口前 30min 内快照 |
| 8 | PDB 配置(防止同时驱逐) | `kubectl get pdb` | minAvailable ≥ replicas - 1 |

---

## 6. 排障速查

| 症状 | 可能根因 | 诊断命令 | 修复 |
|---|---|---|---|
| StatefulSet 卡在某 Pod 不滚动 | 该 Pod readinessProbe 失败 / partition 阻塞 | `kubectl describe pod <sts>-N` | 修复探针或调 partition |
| Pod 启动后立即崩溃 | PVC 数据损坏 / 版本不兼容 | `kubectl logs <sts>-N` + 检查 PV | 从快照恢复或修复数据 |
| PVC 挂载失败 | StorageClass 不可用 / 配额耗尽 | `kubectl describe pod` 看 Events | 检查 CSI 驱动 + ResourceQuota |
| 升级后主从不一致 | 升级顺序错误 / 跳过 switchover | 检查复制延迟指标 | 回滚 + 正确执行 switchover |
| 缩容后存储成本未降 | 残留 PVC 未清理 | `kubectl get pvc` | 清理残留 PVC |

---

## 7. 跨域协作

- **Pod 可用性与 PDB**: 见 [[topic-production-patterns/pod-availability-lifecycle|Pod 可用性生产模式]]
- **存储备份恢复深入**: 见 `domain-04-storage-data/01-k8s-storage/15-storage-disaster-recovery.md`
- **数据库专项运维**: 见 `domain-16-database-middleware/01-databases/` (PostgreSQL/MySQL/Redis/Etcd 生产指南)
- **灾备 Runbook**: 见 `domain-09-reliability-engineering/09-disaster-recovery-playbooks/03-disaster-recovery-bc-runbook.md`


<!-- risk-assessed -->
