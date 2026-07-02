---
title: TKE 存储深度解析：CBS、CFS 与 TurboFS
description: 'CBS 云盘 CSI、CFS 文件存储、TurboFS 高性能共享、Chromium 加速、数据卷最佳实践全面指南'
summary: 'CBS 云盘 CSI、CFS 文件存储、TurboFS 高性能共享、Chromium 加速、数据卷最佳实践全面指南'
category: cloud-providers
tags:
- cloud
- k8s
- tke
- tencent
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
- TKE 存储方案是什么
- 如何配置 CBS 云盘 CSI
- CFS 和 TurboFS 区别
trigger_keywords:
- CBS
- CFS
- TurboFS
- ChromFS
- 数据卷
- CSI
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


# TKE 存储深度解析：CBS、CFS 与 TurboFS

## 1. TKE 存储选项概览

| 存储类型 | CSI 驱动 | 访问模式 | 性能等级 | 适用场景 |
|---------|---------|---------|---------|---------|
| **CBS 云盘** | com.tencent.cloud.csi.cbs | RWO | 高 | 数据库、单节点有状态应用 |
| **CFS 文件存储** | com.tencent.cloud.csi.cfs | RWX | 中 | 共享配置、日志、多 Pod 共享 |
| **TurboFS** | com.tencent.cloud.csi.turbo | RWX | 极高 | AI 训练、大数据、高性能计算 |
| **ChromFS** | com.tencent.cloud.csi.chrom | RWX | 高 | 海量小文件、容器镜像加速 |

## 2. CBS 云盘 CSI

### 2.1 StorageClass 配置

```yaml
# 高性能 SSD（CLOUD_HSSD）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cbs-hssd
provisioner: com.tencent.cloud.csi.cbs
parameters:
  type: CLOUD_HSSD
  encrypt: "false"
  throughput: "0"
  iops: "0"
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

---
# 超高 IO SSD（CLOUD_TSSD）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cbs-tssd
provisioner: com.tencent.cloud.csi.cbs
parameters:
  type: CLOUD_TSSD
  encrypt: "false"
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

---
# 增强型 SSD（CLOUD_ESSD）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cbs-essd-pl1
provisioner: com.tencent.cloud.csi.cbs
parameters:
  type: CLOUD_ESSD
  performanceLevel: PL1      # PL1/PL2/PL3
  encrypt: "true"
  kmsKeyId: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### 2.2 CBS 性能规格

```
CBS 云盘性能对比：
┌──────────────┬──────────┬──────────┬──────────┬──────────┐
│ 类型          │ 最大容量  │ IOPS     │ 吞吐     │ 延迟     │
├──────────────┼──────────┼──────────┼──────────┼──────────┤
│ CLOUD_SSD    │ 16 TiB   │ 26,000   │ 260 MB/s │ 0.5-3ms  │
│ CLOUD_HSSD   │ 32 TiB   │ 50,000   │ 500 MB/s │ 0.3-1ms  │
│ CLOUD_TSSD   │ 32 TiB   │ 100,000  │ 1,000    │ 0.1-0.5  │
│ CLOUD_ESSD   │ 32 TiB   │ 根据 PL  │ 根据 PL  │ 0.1-0.3  │
│   PL1        │          │ 50,000   │ 350 MB/s │          │
│   PL2        │          │ 100,000  │ 750 MB/s │          │
│   PL3        │          │ 320,000  │ 2,000    │          │
└──────────────┴──────────┴──────────┴──────────┴──────────┘
```

### 2.3 StatefulSet 使用 CBS

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: database
spec:
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        - name: binlog
          mountPath: /var/log/mysql
        resources:
          requests:
            cpu: "4"
            memory: 16Gi
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: cbs-essd-pl1
      resources:
        requests:
          storage: 500Gi
  - metadata:
      name: binlog
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: cbs-hssd
      resources:
        requests:
          storage: 100Gi
```

### 2.4 磁盘加密

```yaml
# 使用 KMS 加密的 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cbs-encrypted
provisioner: com.tencent.cloud.csi.cbs
parameters:
  type: CLOUD_HSSD
  encrypt: "true"
  kmsKeyId: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
reclaimPolicy: Retain
volumeBindingMode: WaitForFirstConsumer
```

## 3. CFS 文件存储

### 3.1 StorageClass 配置

```yaml
# 标准型 CFS
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cfs-standard
provisioner: com.tencent.cloud.csi.cfs
parameters:
  host: "10.0.1.100"           # CFS 挂载地址
  path: "/"                     # CFS 子目录
  vers: "4"                     # NFS 版本
  options: "nolock,hard,timeo=600"
reclaimPolicy: Retain
volumeBindingMode: Immediate
allowVolumeExpansion: false

---
# 性能型 CFS
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cfs-performance
provisioner: com.tencent.cloud.csi.cfs
parameters:
  storageType: "performance"    # 标准型/性能型
  host: "10.0.2.100"
  path: "/"
reclaimPolicy: Retain
volumeBindingMode: Immediate
```

### 3.2 多 Pod 共享存储

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-logs
  namespace: production
spec:
  accessModes:
  - ReadWriteMany
  storageClassName: cfs-standard
  resources:
    requests:
      storage: 100Gi

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: log-collector
spec:
  replicas: 5
  template:
    spec:
      containers:
      - name: collector
        volumeMounts:
        - name: logs
          mountPath: /var/log/collected
      volumes:
      - name: logs
        persistentVolumeClaim:
          claimName: shared-logs
```

## 4. TurboFS 高性能共享

### 4.1 概述

TurboFS 是腾讯云的高性能并行文件系统，基于 Lustre/DAOS 架构，专为 AI 训练和大数据场景设计。

```
TurboFS 性能指标：
  吞吐：最高 100 GB/s
  IOPS：数百万级
  延迟：< 1ms
  容量：最高 100 TiB
  协议：POSIX 兼容
```

### 4.2 StorageClass 配置

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: turbofs-ai
provisioner: com.tencent.cloud.csi.turbo
parameters:
  resourceId: "turbofs-xxxxxxxx"
  subnetId: "subnet-xxxxxxxx"
  vpcId: "vpc-xxxxxxxx"
  protocol: "daos"             # daos 或 lustre
  accessMode: "ReadWriteMany"
reclaimPolicy: Retain
volumeBindingMode: Immediate

---
# PVC 使用
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: ai-training-data
spec:
  accessModes:
  - ReadWriteMany
  storageClassName: turbofs-ai
  resources:
    requests:
      storage: 10Ti
```

### 4.3 AI 训练场景

```yaml
# PyTorch 分布式训练 Job
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llm-training
  namespace: ai-training
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        spec:
          containers:
          - name: pytorch
            image: training:v1
            volumeMounts:
            - name: training-data
              mountPath: /data
            - name: checkpoints
              mountPath: /checkpoints
            resources:
              requests:
                nvidia.com/gpu: "8"
                cpu: "64"
                memory: 512Gi
          volumes:
          - name: training-data
            persistentVolumeClaim:
              claimName: ai-training-data
          - name: checkpoints
            persistentVolumeClaim:
              claimName: ai-checkpoints
    Worker:
      replicas: 7
      template:
        spec:
          containers:
          - name: pytorch
            image: training:v1
            volumeMounts:
            - name: training-data
              mountPath: /data
            resources:
              requests:
                nvidia.com/gpu: "8"
          volumes:
          - name: training-data
            persistentVolumeClaim:
              claimName: ai-training-data
```

## 5. ChromFS（容器镜像加速）

### 5.1 概述

ChromFS 是腾讯云提供的容器镜像加速服务，通过预热和本地缓存加速镜像拉取。

```yaml
# 启用 ChromFS 的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: large-image-app
  annotations:
    # 启用 ChromFS 加速
    chromfs.cloud.tencent.com/enabled: "true"
spec:
  containers:
  - name: app
    image: very-large-image:latest   # 10GB+ 镜像
```

### 5.2 镜像预热

```bash
# 通过 API 预热镜像到 ChromFS
# 预热后，节点拉取镜像从本地缓存获取，速度提升 10x+

# 预热命令（通过腾讯云 API）
curl -X POST "https://chromfs.tencentcloudapi.com" \
  -d "Action=PreheatImage" \
  -d "ImageName=registry.example.com/ai-inference:v1" \
  -d "Region=ap-guangzhou"
```

## 6. 数据卷最佳实践

### 6.1 数据库存储

```
数据库存储配置建议：

MySQL:
  数据盘：CLOUD_ESSD PL1+ (500Gi+)
  Binlog：CLOUD_HSSD (100Gi+)
  备份：CFS 标准型（异机备份）

PostgreSQL:
  数据盘：CLOUD_ESSD PL2+ (1Ti+)
  WAL：CLOUD_TSSD (50Gi)
  备份：CBS 快照 + CFS

Redis:
  数据盘：CLOUD_TSSD (50Gi)
  AOF：CLOUD_TSSD (50Gi)
  注意：避免与数据盘同盘

etcd:
  数据盘：CLOUD_TSSD (50Gi)
  独立节点，避免与其他工作负载混部
```

### 6.2 快照与备份

```bash
# 创建 CBS 快照
tccli cbs CreateSnapshots --DiskIds '["disk-xxxxxxxx"]' --SnapshotName "mysql-data-$(date +%Y%m%d)"

# 定时快照策略
tccli cbs CreateAutoSnapshotPolicy \
  --PolicyName "daily-snapshot" \
  --Frequency "DAILY" \
  --RetentionDays 30 \
  --TriggerHour 2

# 关联快照策略到云盘
tccli cbs BindAutoSnapshotPolicy \
  --DiskIds '["disk-xxxxxxxx"]' \
  --AutoSnapshotPolicyId "asp-xxxxxxxx"
```

### 6.3 监控与告警

```yaml
# Prometheus 告警规则
groups:
- name: tke-storage-alerts
  rules:
  - alert: CBSUsageHigh
    expr: (kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes) > 0.85
    for: 10m
    annotations:
      summary: "PVC {{ $labels.persistentvolumeclaim }} 使用率超过 85%"

  - alert: CBSIOPSHigh
    expr: tencentcloud_cbs_iops_read + tencentcloud_cbs_iops_write > 20000
    for: 10m

  - alert: CFSConnectionDrop
    expr: tencentcloud_cfs_connections_dropped > 0
    for: 5m
```

## 7. 故障排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 CSI 驱动状态
kubectl get pods -n kube-system -l app=cbs-csi-controller
kubectl get pods -n kube-system -l app=cfs-csi-node

# 检查 PVC 状态
kubectl get pvc -A
kubectl describe pvc <pvc-name> -n <namespace>

# 检查 PV 绑定
kubectl get pv | grep -v Bound

# CBS 云盘挂载问题
# 检查节点上的设备映射
lsblk
ls -la /dev/disk/by-id/

# CFS 挂载问题
# 检查 NFS 挂载
mount | grep nfs
showmount -e <cfs-ip>

# TurboFS 连通性
# 检查 DAOS/Lustre 客户端状态
daos pool list
lfs df
```
## Related

- [[04-tke-iam-cam-integration|TKE 身份认证与 CAM 集成]]
- [[05-tke-troubleshooting-playbook|TKE 故障排查手册]]

## See Also

- CBS 云盘 CSI 驱动文档
- CFS 文件存储文档
- TurboFS 官方文档


<!-- risk-assessed -->
