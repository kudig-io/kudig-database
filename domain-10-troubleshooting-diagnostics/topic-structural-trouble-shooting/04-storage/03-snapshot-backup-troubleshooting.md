---
title: CSI 快照与卷备份故障排查指南 [topic-structural-trouble-shooting]
description: 'title: CSI 快照与卷备份故障排查指南'
summary: 'title: CSI 快照与卷备份故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- backup-restore
- prometheus
- docker
- postgresql
- job
- cronjob
- crd
- webhook
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- CSI 快照与卷备份故障排查指南 是什么
- 如何 CSI 快照与卷备份故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- CSI 快照与卷备份故障排查指南 故障排查
- CSI 快照与卷备份故障排查指南 排障步骤
trigger_keywords:
- CSI
- 快照与卷备份故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: CSI 快照与卷备份故障排查指南
description: '# CSI 快照与卷备份故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[Prometheus|prometheus]]
- postgresql
- job
- [[CronJob|cronjob]]
- crd
- webhook
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- CSI 快照与卷备份故障排查指南 是什么
- 如何 CSI 快照与卷备份故障排查指南
- CSI 快照与卷备份故障排查指南 故障排查
- CSI 快照与卷备份故障排查指南 排障步骤
trigger_keywords:
- CSI
- 快照与卷备份故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# CSI 快照与卷备份故障排查指南

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25 - v1.32 | **最后更新**: 2026-04 | **难度**: 高级

---

## 0. 10 分钟快速诊断

1. **Snapshot 组件存活**：`kubectl get pods -n kube-system | grep snapshot`，确认 snapshot-controller 和 snapshot-validation-webhook 运行正常。
2. **CRD 检查**：`kubectl get crd volumesnapshots.snapshot.storage.k8s.io volumesnapshotcontents.snapshot.storage.k8s.io volumesnapshotclasses.snapshot.storage.k8s.io`，确认 CSI Snapshot CRD 已安装。
3. **SnapshotClass 配置**：`kubectl get volumesnapshotclass`，确认存在可用的 SnapshotClass 且 `driver` 字段与 CSI 驱动匹配。
4. **快照状态检查**：`kubectl get volumesnapshot -A`，观察 `READYTOUSE` 列状态。
5. **Sidecar 日志**：查看 CSI 驱动的 snapshotter sidecar 容器日志，定位 `CreateSnapshot`/`DeleteSnapshot` 错误。
6. **快速缓解**：
   - 快照创建卡住：检查 CSI 驱动后端存储配额和快照数量限制。
   - 恢复失败：确认源 PVC 已删除（如需从快照创建新 PVC）或快照 `READYTOUSE=true`。
7. **证据留存**：保存 VolumeSnapshot、VolumeSnapshotContent 的 YAML 状态、CSI 驱动日志、后端存储快照列表。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 快照创建失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 快照长期 Pending | `SnapshotContent is still not created` | VolumeSnapshot 状态 | `kubectl get volumesnapshot` |
| CSI 驱动报错 | `CreateSnapshot failed: snapshot already exists` | CSI snapshotter 日志 | `kubectl logs <csi-driver-pod> -c csi-snapshotter` |
| 存储后端拒绝 | `Backend snapshot limit exceeded` | CSI 驱动日志 | CSI 驱动 Pod 日志 |
| SnapshotClass 缺失 | `failed to get snapshot class: volumesnapshotclass.snapshot.storage.k8s.io "xxx" not found` | snapshot-controller | `kubectl get events` |
| 驱动不匹配 | `snapshot controller failed to update ... on API server: cannot find CSI driver` | snapshot-controller | snapshot-controller 日志 |

#### 1.1.2 快照恢复失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 从快照创建 PVC 失败 | `Failed to create volume: ... snapshot not found` | CSI provisioner | provisioner 日志 |
| 恢复后数据不一致 | 应用报数据损坏或校验失败 | 应用日志 | 应用 Pod 日志 |
| PVC 处于 Pending | `waiting for snapshot ... to be ready` | CSI provisioner | `kubectl describe pvc` |
| 快照内容丢失 | `snapshot content does not exist` | VolumeSnapshotContent | `kubectl get volumesnapshotcontent` |

#### 1.1.3 快照清理与生命周期问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 删除 VolumeSnapshot 后后端快照残留 | VolumeSnapshot 已删除但存储后端快照仍存在 | 后端存储控制台 | 云厂商控制台/存储 CLI |
| VolumeSnapshotContent 残留 | `the object has been modified; please apply your changes to the latest version` | snapshot-controller | controller 日志 |
| Finalizer 阻塞删除 | `volume snapshot content is being used by ...` | 删除事件 | `kubectl describe volumesnapshotcontent` |

#### 1.1.4 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **定时备份任务失败** | CronJob 创建的 VolumeSnapshot 持续失败 | SnapshotClass 配置错误或存储配额耗尽 | 检查 SnapshotClass 和存储配额 |
| **灾备演练恢复失败** | 从异地快照恢复 PVC 后应用无法启动 | 快照一致性（crash consistency vs application consistency） | 使用 pre/post snapshot hooks |
| **集群升级后快照失效** | 升级 K8s/CSI 驱动后现有快照无法使用 | CSI 驱动版本与 snapshot sidecar 不兼容 | 检查 CSI 兼容性矩阵 |
| **快照链过长导致性能下降** | 创建/删除快照耗时急剧增加 | 存储后端快照链深度超限 | 限制快照保留数量，定期合并 |

### 1.2 报错查看方式汇总

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 VolumeSnapshot 状态
kubectl get volumesnapshot -A -o wide

# 查看 VolumeSnapshotContent 状态
kubectl get volumesnapshotcontent -A

# 查看 VolumeSnapshotClass
kubectl get volumesnapshotclass -o yaml

# 查看 snapshot-controller 日志
kubectl logs -n kube-system deployment/snapshot-controller --tail=200

# 查看 CSI 驱动的 snapshotter sidecar 日志
kubectl logs -n kube-system <csi-driver-pod> -c csi-snapshotter --tail=200

# 查看具体 VolumeSnapshot 的详细状态和事件
kubectl describe volumesnapshot <snapshot-name> -n <namespace>

# 查看 VolumeSnapshotContent 的详细状态
kubectl describe volumesnapshotcontent <content-name>
```
---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

CSI 快照的工作流程涉及多个组件的协同：

```
用户创建 VolumeSnapshot
        │
        ▼
┌─────────────────────┐
│ snapshot-controller │  ──► 监听 VolumeSnapshot，创建 VolumeSnapshotContent
│ (external-snapshotter)│
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ CSI Snapshot Sidecar │ ──► 调用 CSI 驱动的 CreateSnapshot RPC
│ (csi-snapshotter)    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   CSI Driver         │ ──► 与存储后端交互，实际创建快照
│ (vendor specific)    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   存储后端           │ ──► 物理快照（LVM/COW/ROW/copy-on-write）
│ (cloud/block/nfs)    │
└─────────────────────┘
```

**关键概念**：
- **Crash Consistency**：快照捕获的是文件系统在某一刻的状态，类似于断电时的状态。对于数据库等有状态应用，可能需要额外操作确保一致性。
- **Application Consistency**：通过 pre-snapshot hook（如 `fsfreeze`、`pg_dump`、`LOCK TABLES`）确保应用在快照时处于静默状态。
- **VolumeSnapshotContent**：快照的实际后端资源映射，由 snapshot-controller 动态创建。

### 2.2 排查逻辑决策树

```
快照相关问题
    ├── 快照创建失败
    │   ├── VolumeSnapshot 状态为 Pending
    │   │   ├── snapshot-controller 未运行？──► 部署/修复 controller
    │   │   ├── VolumeSnapshotClass 不存在？──► 创建 SnapshotClass
    │   │   ├── CSI driver 不支持快照？──► 升级 driver 或更换存储类
    │   │   └── 存储后端配额/限制？──► 清理旧快照或申请扩容
    │   └── CSI 驱动报错
    │       ├── 快照已存在（name conflict）──► 删除冲突快照或更换名称
    │       ├── 存储空间不足 ──► 扩容存储池
    │       └── 驱动内部错误 ──► 查看 driver 日志并联系厂商
    ├── 快照恢复失败
    │   ├── PVC 创建 Pending
    │   │   ├── 快照未 READYTOUSE ──► 等待快照完成或重新创建
    │   │   ├── CSI provisioner 报错 ──► 检查 provisioner 日志
    │   │   └── 存储类参数不匹配 ──► 使用与源 PVC 相同的 StorageClass
    │   └── 恢复后数据损坏
    │       ├── 未使用一致性快照 ──► 配置 Velero hooks 或手动静默应用
    │       └── 快照在创建过程中损坏 ──► 验证快照完整性并重新创建
    └── 快照清理异常
        ├── VolumeSnapshot 删除后后端残留
        │   ├── Finalizer 未移除 ──► 手动移除 finalizer（谨慎）
        │   └── CSI DeleteSnapshot 失败 ──► 查看 snapshotter 日志
        └── VolumeSnapshotContent 残留
            ├── Retain 策略导致 ──► 手动删除后端快照后清理资源
            └── Controller 无法连接 API ──► 检查 controller 状态
```

### 2.3 详细诊断命令

#### Snapshot 组件状态诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# CSI Snapshot 组件状态诊断

echo "=== CSI Snapshot 组件状态诊断 ==="

# 1. 检查 snapshot CRD
echo "1. Snapshot CRD 检查:"
for crd in volumesnapshots.snapshot.storage.k8s.io \
           volumesnapshotcontents.snapshot.storage.k8s.io \
           volumesnapshotclasses.snapshot.storage.k8s.io; do
  if kubectl get crd $crd &>/dev/null; then
    echo "  ✓ CRD $crd 已安装"
  else
    echo "  ✗ CRD $crd 缺失，需要安装 snapshot CRD"
  fi
done

# 2. 检查 snapshot-controller
echo ""
echo "2. Snapshot Controller 检查:"
kubectl get deployment snapshot-controller -n kube-system -o wide 2>/dev/null || \
  echo "  ✗ snapshot-controller 未在 kube-system 命名空间中找到"

# 检查 snapshot-validation-webhook（可选组件）
echo ""
echo "3. Snapshot Validation Webhook 检查:"
kubectl get deployment snapshot-validation-deployment -n kube-system -o wide 2>/dev/null || \
  echo "  ⚠ snapshot-validation-deployment 未部署（可选组件）"

# 4. 检查 CSI 驱动的 snapshotter sidecar
echo ""
echo "4. CSI 驱动 Snapshot Sidecar 检查:"
for pod in $(kubectl get pods -n kube-system -o name | grep -E "csi|driver"); do
  if kubectl get $pod -n kube-system -o jsonpath='{.spec.containers[*].name}' | grep -q snapshotter; then
    echo "  ✓ $pod 包含 snapshotter sidecar"
    SNAPSHOTTER_IMAGE=$(kubectl get $pod -n kube-system -o jsonpath='{.spec.containers[?(@.name=="csi-snapshotter")].image}')
    echo "    镜像: $SNAPSHOTTER_IMAGE"
  fi
done

# 5. 列出所有 VolumeSnapshotClass
echo ""
echo "5. VolumeSnapshotClass 列表:"
kubectl get volumesnapshotclass -o json | jq -r '.items[] | "  \(.metadata.name): driver=\(.driver), deletionPolicy=\(.deletionPolicy)"'
```
#### 快照创建故障诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 快照创建故障诊断脚本
# 用法: ./diagnose-snapshot-creation.sh <volumesnapshot-name> <namespace>

SNAPSHOT_NAME=${1:-""}
NAMESPACE=${2:-"default"}

if [ -z "$SNAPSHOT_NAME" ]; then
  echo "用法: $0 <volumesnapshot-name> <namespace>"
  exit 1
fi

echo "=== VolumeSnapshot $NAMESPACE/$SNAPSHOT_NAME 创建故障诊断 ==="

# 1. 检查 VolumeSnapshot 对象状态
echo "1. VolumeSnapshot 状态:"
kubectl get volumesnapshot $SNAPSHOT_NAME -n $NAMESPACE -o json | jq -r '
  {
    "status": .status.readyToUse,
    "error": .status.error.message,
    "snapshotContentName": .status.boundVolumeSnapshotContentName,
    "sourcePVC": .spec.source.persistentVolumeClaimName
  }'

# 2. 检查关联的 VolumeSnapshotContent
echo ""
echo "2. 关联 VolumeSnapshotContent 状态:"
CONTENT_NAME=$(kubectl get volumesnapshot $SNAPSHOT_NAME -n $NAMESPACE -o jsonpath='{.status.boundVolumeSnapshotContentName}')
if [ -n "$CONTENT_NAME" ]; then
  kubectl get volumesnapshotcontent $CONTENT_NAME -o json | jq -r '
    {
      "status": .status.readyToUse,
      "error": .status.error.message,
      "snapshotHandle": .status.snapshotHandle,
      "volumeSnapshotRef": .spec.volumeSnapshotRef.name
    }'
else
  echo "  ✗ VolumeSnapshot 尚未绑定到 VolumeSnapshotContent"
fi

# 3. 检查源 PVC 状态
echo ""
echo "3. 源 PVC 状态:"
SOURCE_PVC=$(kubectl get volumesnapshot $SNAPSHOT_NAME -n $NAMESPACE -o jsonpath='{.spec.source.persistentVolumeClaimName}')
if [ -n "$SOURCE_PVC" ]; then
  kubectl get pvc $SOURCE_PVC -n $NAMESPACE -o json | jq -r '
    {
      "phase": .status.phase,
      "storageClass": .spec.storageClassName,
      "volumeName": .spec.volumeName
    }'
else
  echo "  ✗ VolumeSnapshot 未指定源 PVC"
fi

# 4. 检查 snapshot-controller 事件
echo ""
echo "4. 相关 Events:"
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$SNAPSHOT_NAME --sort-by='.lastTimestamp' | tail -10

# 5. 检查 CSI snapshotter 日志
echo ""
echo "5. CSI Snapshotter 日志 (最近 50 条错误):"
SNAPSHOTTER_POD=$(kubectl get pods -n kube-system -o name | grep -E "csi|driver" | head -1)
if [ -n "$SNAPSHOTTER_POD" ]; then
  kubectl logs -n kube-system $SNAPSHOTTER_POD -c csi-snapshotter --tail=200 2>/dev/null | \
    grep -iE "error|fail|$SNAPSHOT_NAME" | tail -20
else
  echo "  ⚠ 未找到 CSI driver pod"
fi
```
#### 快照恢复故障诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 快照恢复故障诊断脚本
# 用法: ./diagnose-snapshot-restore.sh <pvc-name> <namespace>

PVC_NAME=${1:-""}
NAMESPACE=${2:-"default"}

if [ -z "$PVC_NAME" ]; then
  echo "用法: $0 <pvc-name> <namespace>"
  exit 1
fi

echo "=== PVC $NAMESPACE/$PVC_NAME（来自快照）恢复故障诊断 ==="

# 1. 检查 PVC 状态和事件
echo "1. PVC 状态:"
kubectl get pvc $PVC_NAME -n $NAMESPACE -o json | jq -r '
  {
    "phase": .status.phase,
    "storageClass": .spec.storageClassName,
    "dataSource": .spec.dataSource,
    "dataSourceRef": .spec.dataSourceRef
  }'

# 2. 检查数据源快照状态
echo ""
echo "2. 数据源快照状态:"
DATA_SOURCE_NAME=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.dataSource.name}')
DATA_SOURCE_KIND=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.dataSource.kind}')
if [ "$DATA_SOURCE_KIND" = "VolumeSnapshot" ] && [ -n "$DATA_SOURCE_NAME" ]; then
  kubectl get volumesnapshot $DATA_SOURCE_NAME -n $NAMESPACE -o json 2>/dev/null | jq -r '
    {
      "readyToUse": .status.readyToUse,
      "restoreSize": .status.restoreSize,
      "error": .status.error.message
    }'
else
  echo "  ⚠ PVC 未使用 VolumeSnapshot 作为数据源"
fi

# 3. 检查 CSI provisioner 日志
echo ""
echo "3. CSI Provisioner 日志:"
PROVISIONER_POD=$(kubectl get pods -n kube-system -o name | grep provisioner | head -1)
if [ -n "$PROVISIONER_POD" ]; then
  kubectl logs -n kube-system $PROVISIONER_POD -c csi-provisioner --tail=200 2>/dev/null | \
    grep -iE "error|fail|$PVC_NAME" | tail -20
else
  echo "  ⚠ 未找到 CSI provisioner pod"
fi

# 4. 检查 StorageClass 是否支持从快照创建
echo ""
echo "4. StorageClass 快照支持检查:"
SC_NAME=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.storageClassName}')
if [ -n "$SC_NAME" ]; then
  kubectl get storageclass $SC_NAME -o json | jq -r '
    {
      "provisioner": .provisioner,
      "allowVolumeExpansion": .allowVolumeExpansion,
      "parameters": .parameters
    }'
else
  echo "  ✗ PVC 未指定 StorageClass"
fi
```
---

## 3. 解决方案与风险控制

### 3.1 快照创建失败解决方案

#### 方案一：修复 VolumeSnapshotClass 配置

```yaml
# 标准 VolumeSnapshotClass 配置示例
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-snapclass
  # 设置为默认 SnapshotClass（可选）
  annotations:
    snapshot.storage.kubernetes.io/is-default-class: "true"
driver: ebs.csi.aws.com  # 替换为实际的 CSI 驱动名称
# 删除策略：Delete 表示删除 VolumeSnapshot 时同时删除后端快照
# Retain 表示保留后端快照（手动清理）
deletionPolicy: Delete
parameters:
  # AWS EBS 示例参数
  type: snap
  # 加密相关（可选）
  # encrypted: "true"
  # kmsKeyId: "arn:aws:kms:us-east-1:xxx:key/xxx"
```

**关键检查点**：
- `driver` 必须与 StorageClass 的 `provisioner` 匹配
- `deletionPolicy` 根据合规需求选择 `Delete` 或 `Retain`
- 云厂商特定参数需参考 CSI 驱动文档

#### 方案二：创建带应用一致性的快照（Velero 集成）

```yaml
# Velero Backup 配置示例，包含 pre/post hooks 确保应用一致性
apiVersion: velero.io/v1
kind: Backup
metadata:
  name: app-consistent-backup
  namespace: velero
spec:
  includedNamespaces:
  - my-app
  snapshotVolumes: true
  storageLocation: default
  volumeSnapshotLocations:
  - default
  # 应用一致性 Hook 配置
  hooks:
    resources:
    - name: database-hook
      includedNamespaces:
      - my-app
      labelSelector:
        matchLabels:
          app: postgres
      pre:
      - exec:
          container: postgres
          command: ["/bin/sh", "-c", "pg_dumpall > /tmp/pre-backup.sql"]
          onError: Fail
          timeout: 2m
      post:
      - exec:
          container: postgres
          command: ["/bin/sh", "-c", "rm /tmp/pre-backup.sql"]
          onError: Continue
          timeout: 1m
```

#### 方案三：手动快照创建脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 手动创建带验证的快照
# 用法: ./create-verified-snapshot.sh <pvc-name> <namespace>

PVC_NAME=$1
NAMESPACE=${2:-default}
SNAPSHOT_NAME="${PVC_NAME}-snap-$(date +%Y%m%d-%H%M%S)"
SNAPSHOTCLASS_NAME=$(kubectl get volumesnapshotclass -o jsonpath='{.items[0].metadata.name}')

if [ -z "$PVC_NAME" ]; then
  echo "用法: $0 <pvc-name> [namespace]"
  exit 1
fi

echo "=== 创建 VolumeSnapshot ==="
echo "源 PVC: $NAMESPACE/$PVC_NAME"
echo "SnapshotClass: $SNAPSHOTCLASS_NAME"
echo "快照名称: $SNAPSHOT_NAME"

# 创建 VolumeSnapshot
cat << EOF | kubectl apply -f -
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: $SNAPSHOT_NAME
  namespace: $NAMESPACE
spec:
  volumeSnapshotClassName: $SNAPSHOTCLASS_NAME
  source:
    persistentVolumeClaimName: $PVC_NAME
EOF

# 等待快照就绪（最多 10 分钟）
echo ""
echo "等待快照就绪 (最多 10 分钟)..."
kubectl wait --for=jsonpath='{.status.readyToUse}=true' volumesnapshot/$SNAPSHOT_NAME -n $NAMESPACE --timeout=600s

if [ $? -eq 0 ]; then
  echo "✓ 快照创建成功"
  kubectl get volumesnapshot $SNAPSHOT_NAME -n $NAMESPACE -o json | jq -r '
    {
      "creationTime": .status.creationTime,
      "restoreSize": .status.restoreSize,
      "snapshotHandle": .status.boundVolumeSnapshotContentName
    }'
else
  echo "✗ 快照创建失败或超时"
  kubectl describe volumesnapshot $SNAPSHOT_NAME -n $NAMESPACE
  exit 1
fi
```
### 3.2 快照恢复解决方案

#### 方案一：从快照恢复 PVC

```yaml
# 从 VolumeSnapshot 恢复 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: restored-pvc
  namespace: default
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: fast-ssd  # 建议与源 PVC 使用相同的 StorageClass
  resources:
    requests:
      storage: 10Gi  # 必须 >= 快照的 restoreSize
  dataSource:
    name: my-snapshot  # VolumeSnapshot 名称
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

**重要约束**：
- `spec.resources.requests.storage` 必须大于或等于快照的 `restoreSize`
- `storageClassName` 建议使用与源 PVC 相同的类，不同类可能导致性能或功能差异
- 恢复操作必须在快照 `status.readyToUse=true` 后进行

#### 方案二：跨区域快照复制与恢复（云厂商场景）

```yaml
# AWS EBS 跨区域快照复制示例
# 注：此配置需要特定 CSI 驱动支持
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ebs-snapclass-cross-region
driver: ebs.csi.aws.com
deletionPolicy: Retain
parameters:
  # 创建快照后复制到目标区域
  tagSpecification_1: "CopyToRegion=us-west-2"
```

### 3.3 快照清理与 Finalizer 处理

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 强制清理卡住的 VolumeSnapshot/VolumeSnapshotContent
# ⚠️ 警告：此脚本仅应在确认后端快照已安全删除后使用

RESOURCE_TYPE=$1  # volumesnapshot 或 volumesnapshotcontent
RESOURCE_NAME=$2
NAMESPACE=$3

if [ -z "$RESOURCE_TYPE" ] || [ -z "$RESOURCE_NAME" ]; then
  echo "用法: $0 <volumesnapshot|volumesnapshotcontent> <name> [namespace]"
  echo "⚠️ 警告：此操作会移除 finalizer，可能导致后端资源泄漏"
  exit 1
fi

echo "=== 强制清理 $RESOURCE_TYPE/$RESOURCE_NAME ==="

if [ "$RESOURCE_TYPE" = "volumesnapshot" ]; then
  if [ -z "$NAMESPACE" ]; then
    echo "✗ 清理 VolumeSnapshot 需要指定 namespace"
    exit 1
  fi
  # 移除 finalizer
  kubectl patch volumesnapshot $RESOURCE_NAME -n $NAMESPACE --type merge -p '{"metadata":{"finalizers":[]}}'
  # 删除资源
  kubectl delete volumesnapshot $RESOURCE_NAME -n $NAMESPACE --wait=false
else
  # VolumeSnapshotContent 是集群级资源
  kubectl patch volumesnapshotcontent $RESOURCE_NAME --type merge -p '{"metadata":{"finalizers":[]}}'
  kubectl delete volumesnapshotcontent $RESOURCE_NAME --wait=false
fi

echo "✓ 强制清理命令已执行"
echo "请通过后端存储控制台确认物理快照已删除"
```
### 3.4 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 删除 VolumeSnapshot（Delete 策略） | ⭐⭐ 中 | 后端快照被删除，无法恢复 | 使用 Retain 策略先备份 |
| 从快照恢复 PVC | ⭐ 低 | 新 PVC 创建，不影响源数据 | 删除恢复的 PVC |
| 强制移除 Finalizer | ⭐⭐⭐ 高 | 可能导致后端快照泄漏 | 手动清理后端快照 |
| 修改 VolumeSnapshotClass | ⭐⭐ 中 | 影响新快照行为，不影响已有快照 | 恢复原始配置 |
| 跨集群/跨区域恢复 | ⭐⭐ 中 | 快照句柄可能在目标区域无效 | 确保快照已复制到目标区域 |

### 3.5 验证与监控

#### 快照健康检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 快照健康检查脚本

echo "=== VolumeSnapshot 健康检查 ==="

# 1. 检查所有 Pending 的快照
echo "1. 未就绪的 VolumeSnapshot:"
kubectl get volumesnapshot -A -o json | jq -r '
  .items[] | select(.status.readyToUse != true) |
  "  \(.metadata.namespace)/\(.metadata.name): readyToUse=\(.status.readyToUse // "Pending")"
'

# 2. 检查孤立的 VolumeSnapshotContent
echo ""
echo "2. 孤立的 VolumeSnapshotContent (无关联 VolumeSnapshot):"
kubectl get volumesnapshotcontent -o json | jq -r '
  .items[] | select(.status.readyToUse == true and .spec.deletionPolicy == "Delete") |
  "  \(.metadata.name): snapshotHandle=\(.status.snapshotHandle // "N/A")"
'

# 3. 按命名空间统计快照数量
echo ""
echo "3. 各命名空间快照统计:"
kubectl get volumesnapshot -A -o json | jq -r '
  [ .items[] | {namespace: .metadata.namespace, name: .metadata.name} ] |
  group_by(.namespace)[] | "  \(.[0].namespace): \(length) 个快照"
'
```
#### Prometheus 监控告警规则

```yaml
# Prometheus 快照监控告警
groups:
- name: csi-snapshot
  rules:
  - alert: VolumeSnapshotNotReady
    expr: |
      time() - kube_volumesnapshot_created{job="kube-state-metrics"} > 600
      and kube_volumesnapshot_status_ready{job="kube-state-metrics"} != 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "VolumeSnapshot 长时间未就绪"
      description: "命名空间 {{ $labels.namespace }} 中的快照 {{ $labels.volumesnapshot }} 创建超过 10 分钟仍未就绪"

  - alert: VolumeSnapshotContentOrphaned
    expr: |
      kube_volumesnapshotcontent_status_ready{job="kube-state-metrics"} == 1
      unless on(volumesnapshotcontent)
        (kube_volumesnapshot_info{job="kube-state-metrics"} * 0 + 1)
    for: 1h
    labels:
      severity: info
    annotations:
      summary: "VolumeSnapshotContent 可能已孤立"
      description: "VolumeSnapshotContent {{ $labels.volumesnapshotcontent }} 没有关联的 VolumeSnapshot"

  - alert: SnapshotControllerDown
    expr: up{job="snapshot-controller"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Snapshot Controller 不可用"
      description: "snapshot-controller 已宕机超过 5 分钟，快照创建/删除操作将失败"
```

### 3.6 最佳实践

1. **快照命名规范**：使用 `<pvc-name>-snap-<timestamp>` 格式，便于追踪来源和创建时间
2. **定期清理策略**：为 VolumeSnapshotClass 配置合理的 `deletionPolicy`，配合 CronJob 定期清理旧快照
3. **应用一致性**：对数据库等关键应用，使用 Velero hooks 或手动执行静默操作后再创建快照
4. **跨区域备份**：对关键数据，配置云厂商的跨区域快照复制策略
5. **监控覆盖**：为 snapshot-controller 和 CSI snapshotter 配置日志告警，及时发现创建/删除失败
6. **容量规划**：监控存储后端的快照数量上限和存储池容量，避免因配额耗尽导致快照失败

### 典型问题案例

#### 案例一：数据库快照恢复后数据损坏

**问题描述**：从 VolumeSnapshot 恢复的 PostgreSQL 数据库启动时报 "could not read block" 错误。

**根本原因**：快照创建时数据库处于写入活跃状态，导致文件系统级别的不一致（仅 crash consistent）。

**解决方案**：
1. 使用 Velero pre-hook 在快照前执行 `pg_dumpall` 或 `pg_start_backup()`
2. 或者使用数据库原生的备份工具（如 `pg_basebackup`）替代 CSI 快照
3. 对于必须使用的场景，先执行 `fsfreeze -f <mountpoint>` 再创建快照

#### 案例二：集群迁移后快照无法恢复

**问题描述**：从集群 A 导出的 VolumeSnapshot YAML 在集群 B 应用后无法恢复 PVC。

**根本原因**：VolumeSnapshotContent 包含后端特定的 `snapshotHandle`，该句柄在集群 B 的存储后端不存在。

**解决方案**：
1. 跨区域迁移时，使用存储厂商的跨区域复制功能复制快照数据
2. 或者使用 Velero 等备份工具进行应用级迁移
3. 避免直接导出/导入 VolumeSnapshotContent 到不同集群

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[hot|hot]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-19-landscape-references/topic-index/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index.md|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/01-pv-pvc-troubleshooting.md|01-pv-pvc-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/02-csi-troubleshooting.md|02-csi-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/04-storage-performance-troubleshooting.md|04-storage-performance-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/04-storage/05-storageclass-troubleshooting.md|05-storageclass-troubleshooting]]


<!-- risk-assessed -->
