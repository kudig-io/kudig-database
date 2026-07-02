---
title: AKS 存储与 Managed Disk 深度解析
description: 'Azure Disk CSI、Azure File CSI、BlobFuse2、VolumeSnapshot、Zone-Redundant Storage、NFS v4.1 全面指南'
summary: 'Azure Disk CSI、Azure File CSI、BlobFuse2、VolumeSnapshot、Zone-Redundant Storage、NFS v4.1 全面指南'
category: cloud-providers
tags:
- cloud
- k8s
- aks
- azure
- storage
- csi
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- AKS 存储方案是什么
- 如何配置 Azure Disk CSI
- 如何使用 VolumeSnapshot
trigger_keywords:
- Azure Disk
- Azure File
- BlobFuse2
- VolumeSnapshot
- ZRS
- NFS v4.1
prerequisites:
- kubectl-basics
- cloud-basics
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


# AKS 存储与 Managed Disk 深度解析

## 1. AKS 存储选项概览

| 存储类型 | CSI 驱动 | 访问模式 | 适用场景 |
|---------|---------|---------|---------|
| **Azure Disk** | disk.csi.azure.com | RWO | 数据库、单节点有状态应用 |
| **Azure File** | file.csi.azure.com | RWX | 共享配置、日志、多 Pod 共享 |
| **BlobFuse2** | blob.csi.azure.com | RWX | 大对象存储、AI 训练数据 |
| **NFS v4.1** | 无（原生挂载） | RWX | 高性能共享文件系统 |

## 2. Azure Disk CSI

### 2.1 StorageClass 配置

```yaml
# Premium SSD v2
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: premium-ssd-v2
provisioner: disk.csi.azure.com
parameters:
  skuName: PremiumV2_LRS
  cachingMode: None           # PremiumV2 不支持主机缓存
  diskIOPSReadWrite: "3000"   # 最大 80,000
  diskMBpsReadWrite: "125"    # 最大 1,200 MB/s
  networkAccessPolicy: DenyAll
  publicNetworkAccess: Disabled
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

---
# Ultra Disk
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ultra-disk
provisioner: disk.csi.azure.com
parameters:
  skuName: UltraSSD_LRS
  cachingMode: None
  diskIOPSReadWrite: "16000"  # 最大 160,000
  diskMBpsReadWrite: "2000"   # 最大 4,000 MB/s
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

---
# 标准 SSD（Zone-Redundant）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: standard-ssd-zrs
provisioner: disk.csi.azure.com
parameters:
  skuName: Premium_ZRS        # 跨可用区冗余
  cachingMode: ReadOnly
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 2.2 PVC 使用

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-data
  namespace: database
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: premium-ssd-v2
  resources:
    requests:
      storage: 500Gi

---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
  namespace: database
spec:
  replicas: 3
  selector:
    matchLabels:
      app: postgres
  template:
    spec:
      containers:
      - name: postgres
        image: postgres:16
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
          subPath: pgdata
        resources:
          requests:
            cpu: "4"
            memory: 16Gi
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: premium-ssd-v2
      resources:
        requests:
          storage: 500Gi
```

### 2.3 磁盘性能调优

```
Premium SSD v2 性能规格：
┌──────────┬──────────┬──────────┬──────────┐
│ 磁盘大小  │ 基线 IOPS │ 基线吞吐  │ 最大吞吐  │
├──────────┼──────────┼──────────┼──────────┤
│ 100 GiB  │ 3,000    │ 125 MB/s │ 1,200    │
│ 500 GiB  │ 3,000    │ 125 MB/s │ 1,200    │
│ 1 TiB    │ 3,000    │ 125 MB/s │ 1,200    │
└──────────┴──────────┴──────────┴──────────┘
可独立配置 IOPS 和吞吐，不受磁盘大小限制

Ultra Disk 性能规格：
┌──────────┬──────────┬──────────┐
│ 磁盘大小  │ IOPS 范围 │ 吞吐范围  │
├──────────┼──────────┼──────────┤
│ 100 GiB  │ 160-64K  │ 1-4 GB/s │
│ 500 GiB  │ 160-128K │ 1-4 GB/s │
│ 1 TiB    │ 160-160K │ 1-4 GB/s │
└──────────┴──────────┴──────────┘
```

## 3. Azure File CSI

### 3.1 StorageClass 配置

```yaml
# 标准文件共享
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: azure-file-standard
provisioner: file.csi.azure.com
parameters:
  skuName: Standard_LRS
  protocol: smb              # 或 nfs
  resourceGroup: rg-aks-prod
  storageAccount: saprodfile01
reclaimPolicy: Retain
volumeBindingMode: Immediate
allowVolumeExpansion: true

---
# Premium 文件共享（NFS 协议）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: azure-file-premium-nfs
provisioner: file.csi.azure.com
parameters:
  skuName: Premium_LRS
  protocol: nfs
  shareAccessTier: Premium
reclaimPolicy: Retain
volumeBindingMode: Immediate
allowVolumeExpansion: true
```

### 3.2 多 Pod 共享存储

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-frontend
spec:
  replicas: 5
  template:
    spec:
      containers:
      - name: nginx
        volumeMounts:
        - name: shared-config
          mountPath: /etc/nginx/conf.d
        - name: shared-logs
          mountPath: /var/log/nginx
      volumes:
      - name: shared-config
        persistentVolumeClaim:
          claimName: nginx-config-pvc
      - name: shared-logs
        persistentVolumeClaim:
          claimName: nginx-logs-pvc

---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: nginx-config-pvc
spec:
  accessModes:
  - ReadWriteMany
  storageClassName: azure-file-premium-nfs
  resources:
    requests:
      storage: 10Gi
```

### 3.3 Azure File NFS 限制

```
NFS 文件共享限制：
- 仅支持 Premium 存储账户
- 最大共享大小：100 TiB
- 最大文件大小：4 TiB
- 不支持软配额（需监控）
- 网络要求：需私有终结点或服务终结点
```

## 4. BlobFuse2 CSI

### 4.1 配置

```yaml
# BlobFuse2 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: azure-blob-fuse
provisioner: blob.csi.azure.com
parameters:
  skuName: Standard_LRS
  resourceGroup: rg-aks-prod
  storageAccount: saprodblob01
  containerName: training-data
  protocol: fuse            # blobfuse2
reclaimPolicy: Retain
volumeBindingMode: Immediate

---
# 使用 BlobFuse2 的 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: ai-training-data
spec:
  accessModes:
  - ReadWriteMany
  storageClassName: azure-blob-fuse
  resources:
    requests:
      storage: 5Ti

---
# AI 训练 Job 使用 BlobFuse2
apiVersion: batch/v1
kind: Job
metadata:
  name: model-training
spec:
  template:
    spec:
      containers:
      - name: trainer
        image: training:v1
        volumeMounts:
        - name: data
          mountPath: /data
          mountOptions: "allow-other"
        resources:
          requests:
            nvidia.com/gpu: "2"
      volumes:
      - name: data
        persistentVolumeClaim:
          claimName: ai-training-data
```

### 4.2 BlobFuse2 性能调优

```yaml
# 挂载选项优化
volumeAttributes:
  mountOptions: >-
    --file-cache-timeout-in-seconds=120
    --attr-cache-timeout-in-seconds=120
    --cache-size-mb=8192
    --high-disk-threshold=95
    --low-disk-threshold=80
    --max-concurrency=128
    --block-size-mb=16
```

## 5. VolumeSnapshot

### 5.1 快照类配置

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: disk-snapshot-class
driver: disk.csi.azure.com
deletionPolicy: Retain
parameters:
  incremental: "true"        # 增量快照，节省空间
  resourceGroup: rg-aks-prod
```

### 5.2 创建和恢复快照

```yaml
# 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: postgres-snapshot-20260702
  namespace: database
spec:
  volumeSnapshotClassName: disk-snapshot-class
  source:
    persistentVolumeClaimName: postgres-data

---
# 从快照恢复
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: postgres-restored
  namespace: database
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: premium-ssd-v2
  resources:
    requests:
      storage: 500Gi
  dataSource:
    name: postgres-snapshot-20260702
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

### 5.3 定时快照（Velero）

```bash
# 安装 Velero
velero install \
  --provider azure \
  --plugins velero/velero-plugin-for-microsoft-azure:v1.9.0 \
  --bucket velero-backups \
  --secret-file ./credentials-velero \
  --backup-location-config resourceGroup=rg-aks-prod,storageAccount=saprodblobvelero \
  --snapshot-location-config apiTimeout=300s

# 创建定时快照计划
velero schedule create db-daily-snapshot \
  --schedule="0 2 * * *" \
  --ttl 720h \
  --include-namespaces database \
  --snapshot-volumes=true
```

## 6. Zone-Redundant Storage (ZRS)

```yaml
# ZRS StorageClass（跨可用区冗余）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: premium-ssd-zrs
provisioner: disk.csi.azure.com
parameters:
  skuName: Premium_ZRS
  cachingMode: ReadOnly
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

---
# LRS vs ZRS 选择
# LRS：单数据中心冗余（3 副本）
# ZRS：跨 3 个可用区冗余（推荐生产）
# GRS：跨区域冗余（仅 Azure File 支持）

# 节点调度到可用区时，ZRS 磁盘可自动跟随
# 确保节点池已启用多个可用区
```

## 7. NFS v4.1 挂载

```yaml
# 直接挂载外部 NFS 服务器
apiVersion: v1
kind: PersistentVolume
metadata:
  name: nfs-pv-external
spec:
  capacity:
    storage: 10Ti
  accessModes:
  - ReadWriteMany
  persistentVolumeReclaimPolicy: Retain
  nfs:
    server: 10.0.1.100
    path: /export/data
    readOnly: false
  mountOptions:
  - nfsvers=4.1
  - rsize=1048576
  - wsize=1048576
  - hard
  - timeo=600
  - retrans=3

---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: nfs-pvc-external
spec:
  accessModes:
  - ReadWriteMany
  resources:
    requests:
      storage: 10Ti
  selector:
    matchLabels:
      type: nfs-external
```

## 8. 存储最佳实践

### 8.1 数据库存储

```
数据库存储配置建议：

PostgreSQL:
  存储类型：Premium SSD v2 或 Ultra Disk
  缓存模式：ReadOnly（读多写少）/ None（写密集）
  IOPS：≥ 3000
  吞吐：≥ 125 MB/s
  快照：每日增量快照，保留 30 天

MySQL:
  存储类型：Premium SSD v2
  缓存模式：ReadOnly
  IOPS：≥ 3000
  注意：binlog 和 data 分开存储

Redis (持久化):
  存储类型：Premium SSD v2
  缓存模式：None（避免缓存一致性问题）
  IOPS：≥ 6000
  备份：每小时 RDB + AOF
```

### 8.2 监控与告警

```yaml
# Prometheus 告警规则
groups:
- name: storage-alerts
  rules:
  - alert: PVUsageHigh
    expr: (kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) > 0.85
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "PV {{ $labels.persistentvolumeclaim }} 使用率超过 85%"

  - alert: PVCUnbound
    expr: kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
    for: 5m
    labels:
      severity: critical

  - alert: DiskIOPSHigh
    expr: azure_disk_iops_read_total + azure_disk_iops_write_total > 2500
    for: 10m
```

## Related

- [[02-aks-cluster-lifecycle-upgrades|AKS 集群生命周期与升级]]
- [[05-aks-identity-workload-identity|AKS 身份认证与 Workload Identity]]

## See Also

- Azure Disk CSI 驱动文档
- Azure File CSI 驱动文档
- BlobFuse2 官方文档


<!-- risk-assessed -->
