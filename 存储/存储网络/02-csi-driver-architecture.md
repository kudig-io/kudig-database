---
title: Kubernetes CSI Driver Architecture & Implementation
description: CSI 驱动架构与实现 — CSI 规范、驱动开发、卷生命周期、快照/克隆/扩容、生产运维
summary: Container Storage Interface 深度解析，涵盖架构设计、驱动开发、生产运维最佳实践
category: practice
tags:
- csi
- storage
- persistent-volume
- snapshot
- volume-lifecycle
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: storage
---
# Kubernetes CSI 驱动架构与实现

> Container Storage Interface — K8s 存储扩展的标准接口。

## CSI 架构

```
┌─────────────────────────────────────────────────────────┐
│                  Kubernetes Control Plane                 │
│  ┌──────────────────┐  ┌──────────────────────────────┐  │
│  │  kube-controller  │  │  external-provisioner        │  │
│  │  (PV Controller)  │  │  external-attacher           │  │
│  │                   │  │  external-resizer            │  │
│  │                   │  │  external-snapshotter        │  │
│  └────────┬──────────┘  └──────────────┬───────────────┘  │
│           │                            │                   │
│  ┌────────▼────────────────────────────▼───────────────┐  │
│  │              kubelet (VolumeManager)                  │  │
│  └────────────────────────┬────────────────────────────┘  │
└───────────────────────────┼───────────────────────────────┘
                            │ gRPC (Unix Socket)
┌───────────────────────────▼───────────────────────────────┐
│                    CSI Driver (DaemonSet)                   │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────┐   │
│  │  Identity    │  │  Controller  │  │  Node          │   │
│  │  Service     │  │  Service     │  │  Service       │   │
│  │  (GetInfo,   │  │  (Create,    │  │  (NodeStage,   │   │
│  │   Probe)     │  │   Delete,    │  │   NodePublish, │   │
│  │              │  │   Expand)    │  │   GetStats)    │   │
│  └──────────────┘  └──────────────┘  └────────────────┘   │
└───────────────────────────────────────────────────────────┘
                            │
                            ▼
┌───────────────────────────────────────────────────────────┐
│              Storage Backend (NFS/iSCSI/Cloud API)         │
└───────────────────────────────────────────────────────────┘
```

## 卷生命周期

```
PVC 创建 → Provision (CreateVolume) → PV 绑定
    → Attach (ControllerPublishVolume) → 挂载到节点
    → NodeStage (格式化/mkfs) → NodePublish (mount 到 Pod)
    → Pod 运行中...
    → Pod 删除 → NodeUnpublish (umount) → NodeUnstage
    → Detach (ControllerUnpublishVolume)
    → PVC 删除 → Delete (DeleteVolume) 或 Retain
```

## StorageClass 配置

```yaml
# 高性能 SSD StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
  annotations:
    storageclass.kubernetes.io/is-default-class: "false"
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
  kmsKeyId: "arn:aws:kms:region:account:key/id"
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定（拓扑感知）
mountOptions:
  - noatime
  - nodiratime
---
# 共享文件存储
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: shared-nfs
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs.internal
  share: /exports/k8s
reclaimPolicy: Retain
mountOptions:
  - nfsvers=4.1
  - rsize=1048576
  - wsize=1048576
  - hard
  - timeo=600
  - retrans=2
```

## 卷快照与克隆

```yaml
# VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ebs-snapshot
driver: ebs.csi.aws.com
deletionPolicy: Delete
parameters:
  tagSpecification_1: "owner=kubernetes"
---
# 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: db-snapshot-20260721
  namespace: production
spec:
  volumeSnapshotClassName: ebs-snapshot
  source:
    persistentVolumeClaimName: postgres-data
---
# 从快照恢复
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data-restored
  namespace: production
spec:
  storageClassName: fast-ssd
  dataSource:
    name: db-snapshot-20260721
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
---
# 卷克隆（从现有 PVC）
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-clone
spec:
  storageClassName: fast-ssd
  dataSource:
    name: postgres-data
    kind: PersistentVolumeClaim
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
```

## 在线扩容

```yaml
# 扩容 PVC（无需重启 Pod）
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data
spec:
  resources:
    requests:
      storage: 200Gi  # 从 100Gi 扩到 200Gi
```

```bash
# 验证扩容
kubectl get pvc postgres-data -o jsonpath='{.status.capacity.storage}'
# 检查文件系统
kubectl exec -it postgres -- df -h /data
```

## CSI 驱动开发（Go SDK）

```go
// 使用 container-storage-interface/spec
package driver

import (
    "github.com/container-storage-interface/spec/lib/go/csi"
    "google.golang.org/grpc/codes"
    "google.golang.org/grpc/status"
)

type MyCSIDriver struct {
    csi.UnimplementedControllerServer
    csi.UnimplementedNodeServer
    csi.UnimplementedIdentityServer
}

// CreateVolume — 动态供给
func (d *MyCSIDriver) CreateVolume(ctx context.Context, req *csi.CreateVolumeRequest) (*csi.CreateVolumeResponse, error) {
    name := req.GetName()
    size := req.GetCapacityRange().GetRequiredBytes()
    params := req.GetParameters()

    // 调用存储后端 API 创建卷
    volumeID, err := d.backend.CreateVolume(name, size, params)
    if err != nil {
        return nil, status.Errorf(codes.Internal, "create volume: %v", err)
    }

    return &csi.CreateVolumeResponse{
        Volume: &csi.Volume{
            VolumeId:      volumeID,
            CapacityBytes: size,
            VolumeContext: params,
        },
    }, nil
}

// NodePublishVolume — 挂载到 Pod
func (d *MyCSIDriver) NodePublishVolume(ctx context.Context, req *csi.NodePublishVolumeRequest) (*csi.NodePublishVolumeResponse, error) {
    targetPath := req.GetTargetPath()
    volumeID := req.GetVolumeId()

    // 执行 mount
    if err := d.mounter.Mount(volumeID, targetPath, req.GetVolumeCapability()); err != nil {
        return nil, status.Errorf(codes.Internal, "mount: %v", err)
    }

    return &csi.NodePublishVolumeResponse{}, nil
}
```

## 生产运维

### 监控指标

```promql
# CSI 操作延迟
csi_operations_seconds_bucket{driver_name="ebs.csi.aws.com"}

# 卷操作失败
csi_operations_seconds_count{driver_name="ebs.csi.aws.com", grpc_status_code!="OK"}

# PVC 绑定等待
kubelet_volume_stats_available_bytes / kubelet_volume_stats_capacity_bytes < 0.1
```

### 故障排查

| 问题 | 排查命令 | 常见原因 |
|------|----------|----------|
| PVC Pending | `kubectl describe pvc` | StorageClass 不存在/配额不足 |
| Attach 失败 | `kubectl get volumeattachment` | 节点/AZ 不匹配 |
| Mount 失败 | `kubectl describe pod` + 节点日志 | 权限/文件系统损坏 |
| 扩容失败 | `kubectl describe pvc` | 驱动不支持/后端限制 |
| 快照失败 | `kubectl describe volumesnapshot` | SnapshotClass 配置错误 |

### 备份策略

```bash
# 定期快照（CronJob）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: db-snapshot
spec:
  schedule: "0 2 * * *"
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: snapshot
              image: bitnami/kubectl
              command:
                - /bin/sh
                - -c
                - |
                  kubectl create -f - <<EOF
                  apiVersion: snapshot.storage.k8s.io/v1
                  kind: VolumeSnapshot
                  metadata:
                    name: db-snap-$(date +%Y%m%d)
                    namespace: production
                  spec:
                    volumeSnapshotClassName: ebs-snapshot
                    source:
                      persistentVolumeClaimName: postgres-data
                  EOF
          restartPolicy: OnFailure
```

## 最佳实践

1. 使用 `WaitForFirstConsumer` 绑定模式（拓扑感知）
2. 生产数据使用 `reclaimPolicy: Retain`
3. 启用卷加密（静态加密）
4. 设置合理的 IOPS/吞吐参数
5. 监控卷使用率（< 80% 告警）
6. 定期快照 + 异地备份
7. 测试扩容流程（非生产环境验证）
8. 使用 VolumeSnapshot 进行数据库备份

## CSI Sidecar 容器详解

### Sidecar 架构

```
CSI Controller Pod (Deployment/StatefulSet):
┌───────────────────────────────────────────┐
│  csi-provisioner    → CreateVolume/DeleteVolume  │
│  csi-attacher       → ControllerPublish/Unpublish │
│  csi-resizer        → ControllerExpandVolume      │
│  csi-snapshotter    → CreateSnapshot/DeleteSnap   │
│  csi-driver         → 实际存储后端交互          │
└───────────────────────────────────────────┘

CSI Node Pod (DaemonSet):
┌───────────────────────────────────────────┐
│  csi-node-driver-registrar → 注册驱动到 kubelet  │
│  csi-driver                → NodeStage/NodePublish │
└───────────────────────────────────────────┘
```

### Sidecar 版本兼容性

| Sidecar | 最低 K8s 版本 | 功能 |
|---------|-------------|------|
| external-provisioner v4.0+ | 1.28+ | 动态供给 |
| external-attacher v4.5+ | 1.28+ | 卷挂载/卸载 |
| external-resizer v1.10+ | 1.28+ | 卷扩容 |
| external-snapshotter v7.0+ | 1.28+ | 快照管理 |
| node-driver-registrar v2.10+ | 1.28+ | 节点注册 |

## 拓扑感知供给

### WaitForFirstConsumer 工作原理

```yaml
# 拓扑感知 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: topology-aware-ssd
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer  # 关键配置
allowedTopologies:
  - matchLabelExpressions:
      - key: topology.kubernetes.io/zone
        values:
          - ap-southeast-1a
          - ap-southeast-1b
parameters:
  type: gp3
```

### 拓扑约束流程

```
1. Pod 创建 → Scheduler 选择节点 (考虑拓扑约束)
2. PVC 保持 Pending (WaitForFirstConsumer)
3. Scheduler 确定节点后 → 触发 Provision
4. CSI 驱动在目标 AZ 创建卷
5. PV 绑定 → Pod 调度到同一 AZ 节点
```

## 卷健康监控

### VolumeHealth CSI 功能

```yaml
# 启用卷健康监控 (CSI Driver 必须支持)
apiVersion: storage.k8s.io/v1
kind: CSIDriver
metadata:
  name: ebs.csi.aws.com
spec:
  volumeLifecycleModes:
    - Persistent
  podInfoOnMount: true
  # 卷健康监控通过 VolumeAttributes 暴露
```

### 卷健康检查 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: volume-health-checker
  namespace: platform
spec:
  schedule: "*/30 * * * *"
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: checker
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== 卷健康检查 $(date) ==="
                  
                  # 检查异常 PVC
                  echo "[1] 异常 PVC:"
                  kubectl get pvc -A --no-headers | grep -v Bound
                  
                  # 检查容量告警
                  echo "[2] 容量 < 10% 的 PVC:"
                  kubectl get pvc -A -o json | jq -r '.items[] |
                    select(.status.capacity.storage != null) |
                    "\(.metadata.namespace)/\(.metadata.name)"' | \
                  while read PVC; do
                    NS=${PVC%%/*}; NAME=${PVC##*/}
                    AVAIL=$(kubectl exec -n $NS $(kubectl get pods -n $NS -o name 2>/dev/null | head -1) -- \
                      df -h /data 2>/dev/null | tail -1 | awk '{print $5}')
                    if [ -n "$AVAIL" ] && [ "${AVAIL%%%*}" -gt 90 ]; then
                      echo "  ⚠️  $PVC: 使用率 $AVAIL"
                    fi
                  done
                  
                  # 检查 VolumeAttachment 异常
                  echo "[3] 异常 VolumeAttachment:"
                  kubectl get volumeattachment --no-headers | grep -v "true"
                  
                  echo "=== 检查完成 ==="
          restartPolicy: OnFailure
```

## CSI 驱动升级策略

### 升级检查单

```bash
#!/bin/bash
# 🟢 只读：CSI 驱动升级前检查
echo "=== CSI 驱动升级前检查 ==="

# 1. 当前版本
echo "[1/5] 当前 CSI 驱动版本:"
kubectl get ds -n kube-system -l app=csi-nodeplugin -o jsonpath='{.items[0].spec.template.spec.containers[*].image}'
echo ""

# 2. 活跃卷数量
echo "[2/5] 活跃 PV 数量:"
kubectl get pv --no-headers | wc -l

# 3. 进行中的操作
echo "[3/5] 进行中的卷操作:"
kubectl get volumeattachment --no-headers | grep -c "false" || echo "0"

# 4. 快照状态
echo "[4/5] 进行中的快照:"
kubectl get volumesnapshot -A --no-headers 2>/dev/null | grep -v "true" | wc -l

# 5. 驱动健康
echo "[5/5] CSI 驱动 Pod 状态:"
kubectl get pods -n kube-system -l app=csi-nodeplugin --no-headers | grep -v Running
kubectl get pods -n kube-system -l app=csi-provisioner --no-headers | grep -v Running

echo "=== 检查完成 ==="
```

### 升级最佳实践

| 步骤 | 操作 | 风险 |
|------|------|------|
| 1 | 在非生产集群验证新版本 | 无 |
| 2 | 确认无进行中的卷操作 | 低 |
| 3 | 先升级 Controller (Deployment) | 低 |
| 4 | 滚动升级 Node Plugin (DaemonSet) | 中 |
| 5 | 验证已挂载卷不受影响 | 低 |
| 6 | 监控 48h 异常指标 | 无 |

> **重要**: CSI Node Plugin 升级不会卸载已挂载的卷，但新挂载操作会短暂失败。建议在低峰期执行。

## 多集群存储管理

### 跨集群快照复制

```yaml
# 快照导出/导入流程
# 1. 源集群创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: cross-cluster-snap
  namespace: production
  labels:
    replicate-to: cluster-b
spec:
  volumeSnapshotClassName: ebs-snapshot
  source:
    persistentVolumeClaimName: critical-data
---
# 2. 快照控制器自动复制到目标区域 (存储后端功能)
# 3. 目标集群从复制的快照创建 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: critical-data-dr
  namespace: production
spec:
  storageClassName: fast-ssd
  dataSource:
    name: cross-cluster-snap-replica
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
```

## Related

- [[存储/存储网络/index.md|存储网络]]
- [[存储/云存储对比/index.md|云存储对比]]
- [[可靠性/备份恢复/index.md|备份恢复]]
