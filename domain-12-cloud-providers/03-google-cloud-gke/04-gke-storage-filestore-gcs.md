---
title: GKE 存储方案 — Filestore、GCS FUSE 与 Persistent Disk
description: 'GKE Filestore CSI、GCS FUSE CSI、Persistent Disk 扩容、Regional PD 高可用及存储性能调优'
summary: 'GKE Filestore CSI、GCS FUSE CSI、Persistent Disk 扩容、Regional PD 高可用及存储性能调优'
category: cloud-providers
tags:
- cloud
- k8s
- gcp
- gke
- storage
- filestore
- gcs
- persistent-disk
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
- GKE 存储方案 是什么
- 如何配置 GKE Filestore
trigger_keywords:
- filestore-csi
- gcs-fuse
- persistent-disk
- regional-pd
- storage-class
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


# GKE 存储方案 — Filestore、GCS FUSE 与 Persistent Disk

## 1. Persistent Disk CSI Driver

### 1.1 GKE 内置 CSI Driver

GKE 默认安装 PD CSI Driver，无需手动安装。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 CSI Driver 状态
kubectl get csidriver pd.csi.storage.gke.io
kubectl get pods -n kube-system -l app=gce-pd-csi-driver
```
### 1.2 StorageClass 定义

```yaml
# Standard PD（默认）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: pd-standard
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: pd.csi.storage.gke.io
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: pd-standard
  replication-type: none

---
# SSD PD
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: pd-ssd
provisioner: pd.csi.storage.gke.io
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: pd-ssd
  replication-type: none

---
# Regional PD（高可用，跨两个可用区复制）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: pd-ssd-regional
provisioner: pd.csi.storage.gke.io
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: pd-ssd
  replication-type: regional-pd
  availability-zones: asia-southeast1-a,asia-southeast1-b

---
# Extreme PD（超高性能，仅支持部分区域）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: pd-extreme
provisioner: pd.csi.storage.gke.io
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: pd-extreme
  provisioned-iops: "50000"
  replication-type: none
```

### 1.3 PD 性能规格

| 卷类型 | IOPS (读/写) | 吞吐 (MB/s) | 最大容量 | 适用场景 |
|--------|-------------|------------|---------|---------|
| pd-standard | 0.75/GB, 1.5/GB | 120 | 64 TiB | 日志、大数据 |
| pd-ssd | 30/GB, 30/GB | 120 | 64 TiB | 数据库、应用 |
| pd-extreme | 自定义 10K-120K | 120-2,400 | 64 TiB | SAP HANA、Oracle |
| regional-pd | 同上 | 同上 | 64 TiB | 高可用数据库 |

## 2. Persistent Disk 在线扩容

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# GKE 支持在线扩容（无需重启 Pod）
# 方式一：修改 PVC
kubectl patch pvc my-pvc -n production \
  -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 方式二：kubectl edit
kubectl edit pvc my-pvc -n production
# 修改 spec.resources.requests.storage

# 验证扩容状态
kubectl get pvc my-pvc -n production -o jsonpath='{.status.conditions[*].message}'
```
```yaml
# 确保 StorageClass 允许扩容
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: pd-ssd-expansible
provisioner: pd.csi.storage.gke.io
allowVolumeExpansion: true  # 必须为 true
parameters:
  type: pd-ssd
```

### 2.1 文件系统在线扩容

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# GKE CSI Driver 自动处理 ext4/xfs 文件系统扩容
# 如果需要手动触发：

# 检查文件系统类型
kubectl exec -it my-pod -- df -Th /data

# ext4 扩容（通常自动完成）
kubectl exec -it my-pod -- resize2fs /dev/sdb

# xfs 扩容
kubectl exec -it my-pod -- xfs_growfs /data
```
## 3. Filestore CSI

### 3.1 创建 Filestore 实例

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 Basic HDD Filestore
gcloud filestore instances create nfs-server \
  --zone=asia-southeast1-a \
  --tier=BASIC_HDD \
  --file-share=name="vol1",capacity=1TB \
  --network=name="default",reserved-ip-range="10.0.1.0/29"

# 创建 Basic SSD
gcloud filestore instances create nfs-ssd \
  --zone=asia-southeast1-a \
  --tier=BASIC_SSD \
  --file-share=name="vol1",capacity=2TB \
  --network=name="default",reserved-ip-range="10.0.1.8/29"

# 创建 Enterprise（多可用区高可用）
gcloud filestore instances create nfs-enterprise \
  --zone=asia-southeast1 \
  --tier=ENTERPRISE \
  --file-share=name="vol1",capacity=10TB \
  --network=name="default"
```
### 3.2 Filestore CSI Driver 配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# GKE 1.23+ 自动安装 Filestore CSI Driver
# 验证安装
kubectl get csidriver filestore.csi.storage.gke.io
```
```yaml
# Filestore StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: filestore-basic
provisioner: filestore.csi.storage.gke.io
volumeBindingMode: Immediate
parameters:
  tier: basic_hdd
  network: default
  reserved-ipv4-name: filestore-ip

---
# Filestore Enterprise
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: filestore-enterprise
provisioner: filestore.csi.storage.gke.io
volumeBindingMode: Immediate
parameters:
  tier: enterprise
  network: default

---
# PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-data
  namespace: production
spec:
  accessModes:
    - ReadWriteMany
  storageClassName: filestore-basic
  resources:
    requests:
      storage: 1Ti
```

### 3.3 Filestore 规格对比

| 层级 | 容量范围 | IOPS | 吞吐 | NFS 协议 | 价格 |
|------|---------|------|------|---------|------|
| Basic HDD | 1-63.9 TB | 1,000 | 100 MB/s | NFSv3 | 低 |
| Basic SSD | 2.5-63.9 TB | 60,000 | 1,200 MB/s | NFSv3 | 中 |
| Enterprise | 1-100 TB | 90,000 | 2,400 MB/s | NFSv3/v4.1 | 高 |
| Zonal | 10-100 TB | 120,000 | 2,400 MB/s | NFSv3/v4.1 | 高 |

## 4. GCS FUSE CSI

GCS FUSE 允许将 Google Cloud Storage Bucket 挂载为文件系统。

### 4.1 安装 GCS FUSE CSI Driver

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# GKE 1.27+ 通过 addon 安装
gcloud container clusters update prod-cluster \
  --region=asia-southeast1 \
  --update-addons GcsFuseCsiDriver=ENABLED
```
### 4.2 GCS FUSE 配置

```yaml
# Pod 直接挂载 GCS Bucket
apiVersion: v1
kind: Pod
metadata:
  name: data-processor
  namespace: production
  annotations:
    gke-gcsfuse/volumes: "true"  # 启用 GCS FUSE
spec:
  serviceAccountName: app-sa  # 需要 Workload Identity
  containers:
    - name: processor
      image: gcr.io/my-project/data-processor:latest
      volumeMounts:
        - name: gcs-data
          mountPath: /data/input
          readOnly: true
        - name: gcs-output
          mountPath: /data/output
      resources:
        requests:
          cpu: "500m"
          memory: "1Gi"
  volumes:
    - name: gcs-data
      csi:
        driver: gcsfuse.csi.storage.gke.io
        volumeAttributes:
          bucketName: my-input-bucket
          mountOptions: "implicit-dirs"
    - name: gcs-output
      csi:
        driver: gcsfuse.csi.storage.gke.io
        volumeAttributes:
          bucketName: my-output-bucket
          mountOptions: "implicit-dirs,file-cache:max-size-mb:-1"
```

### 4.3 GCS FUSE StorageClass

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gcs-fuse-sc
provisioner: gcsfuse.csi.storage.gke.io
volumeBindingMode: Immediate
parameters:
  mountOptions: "implicit-dirs,file-cache:max-size-mb:-1"
```

### 4.4 GCS FUSE 性能优化

```yaml
# 挂载选项调优
volumeAttributes:
  bucketName: my-bucket
  mountOptions: |
    implicit-dirs
    file-cache:max-size-mb:4096
    file-cache:cache-file-for-range-read:true
    metadata-cache:ttl-secs:3600
    metadata-cache:stat-cache-max-size-mb:128
```

| 参数 | 说明 | 默认值 | 推荐值 |
|------|------|--------|--------|
| implicit-dirs | 隐式创建目录 | false | true |
| file-cache:max-size-mb | 文件缓存大小 | -1 (无限制) | 根据磁盘设置 |
| file-cache:cache-file-for-range-read | 范围读缓存 | false | true (顺序读) |
| metadata-cache:ttl-secs | 元数据缓存 TTL | 60 | 3600 (低频变更) |

## 5. 存储性能调优

### 5.1 IOPS 与吞吐优化

```yaml
# 高 IOPS 数据库场景
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: pd-extreme-high-iops
provisioner: pd.csi.storage.gke.io
volumeBindingMode: WaitForFirstConsumer
parameters:
  type: pd-extreme
  provisioned-iops: "100000"

---
# PVC 指定 IOPS
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-data
  namespace: database
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: pd-extreme-high-iops
  resources:
    requests:
      storage: 4Ti
      # IOPS 与容量和类型相关
```

### 5.2 多卷策略

```yaml
# 数据库最佳实践：数据和日志分离
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
  namespace: database
spec:
  serviceName: postgres
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
            - name: wal
              mountPath: /var/lib/postgresql/wal
          resources:
            requests:
              cpu: "4"
              memory: "16Gi"
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: pd-ssd
        resources:
          requests:
            storage: 500Gi
    - metadata:
        name: wal
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: pd-extreme
        resources:
          requests:
            storage: 100Gi
```

## 6. VolumeSnapshot

```yaml
# VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: pd-snapshot-class
driver: pd.csi.storage.gke.io
deletionPolicy: Retain

---
# 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: db-snapshot-20260702
  namespace: database
spec:
  volumeSnapshotClassName: pd-snapshot-class
  source:
    persistentVolumeClaimName: db-data

---
# 从快照恢复
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-data-restored
  namespace: database
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: pd-ssd
  resources:
    requests:
      storage: 500Gi
  dataSource:
    name: db-snapshot-20260702
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

## 7. 存储监控

```yaml
# Prometheus 告警规则
groups:
  - name: gke-storage
    rules:
      - alert: PDUsageHigh
        expr: |
          kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.85
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "PVC {{ $labels.persistentvolumeclaim }} usage above 85%"

      - alert: PDUsageCritical
        expr: |
          kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.95
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "PVC {{ $labels.persistentvolumeclaim }} usage above 95%"
```

## Related

- [[02-gke-autopilot-serverless]]
- [[05-gke-workload-identity-security]]

## See Also

- GKE Persistent Disk 文档
- Filestore CSI Driver
- GCS FUSE CSI


<!-- risk-assessed -->
