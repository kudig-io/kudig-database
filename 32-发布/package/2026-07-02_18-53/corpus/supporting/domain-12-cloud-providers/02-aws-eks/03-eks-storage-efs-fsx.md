---
title: EKS 存储方案 — EBS、EFS 与 FSx for Lustre
description: 'EKS 存储 CSI Driver 配置、EFS 共享存储、FSx 高性能存储及 StorageClass 最佳实践'
summary: 'EKS 存储 CSI Driver 配置、EFS 共享存储、FSx 高性能存储及 StorageClass 最佳实践'
category: cloud-providers
tags:
- cloud
- k8s
- aws
- eks
- storage
- ebs
- efs
- fsx
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
- EKS 存储方案 是什么
- 如何配置 EKS EBS CSI Driver
trigger_keywords:
- ebs-csi
- efs-csi
- fsx-lustre
- storageclass
- volume-snapshot
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


# EKS 存储方案 — EBS、EFS 与 FSx for Lustre

## 1. EBS CSI Driver

### 1.1 安装与配置

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 通过 EKS Addon 安装（推荐）
aws eks create-addon \
  --cluster-name prod-cluster \
  --addon-name aws-ebs-csi-driver \
  --addon-version v1.35.0-eksbuild.1 \
  --service-account-role-arn arn:aws:iam::123456789012:role/ebs-csi-role

# 或通过 Helm 安装
helm repo add aws-ebs-csi-driver https://kubernetes-sigs.github.io/aws-ebs-csi-driver
helm install aws-ebs-csi-driver aws-ebs-csi-driver/aws-ebs-csi-driver \
  --namespace kube-system \
  --set controller.serviceAccount.create=true \
  --set controller.serviceAccount.annotations."eks\.amazonaws\.com/role-arn"=arn:aws:iam::123456789012:role/ebs-csi-role
```
### 1.2 StorageClass 定义

```yaml
# gp3（推荐通用存储）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-gp3
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
  kmsKeyId: arn:aws:kms:ap-southeast-1:123456789012:key/12345678-1234-1234-1234-123456789012
reclaimPolicy: Delete

---
# io2（高性能数据库）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-io2
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: io2
  iops: "10000"
  encrypted: "true"
reclaimPolicy: Retain

---
# io2 Block Express（极致性能）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-io2-block-express
provisioner: ebs.csi.aws.com
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
parameters:
  type: io2
  iops: "256000"
  blockExpress: "true"
  encrypted: "true"
reclaimPolicy: Retain
```

### 1.3 EBS 性能规格

| 卷类型 | 最大 IOPS | 最大吞吐 | 最大容量 | 适用场景 |
|--------|----------|---------|---------|---------|
| gp3 | 16,000 | 1,000 MB/s | 64 TiB | 通用工作负载 |
| gp3 高性能 | 16,000 | 1,000 MB/s | 64 TiB | 大多数数据库 |
| io2 | 64,000 | 1,000 MB/s | 64 TiB | 高 IOPS 数据库 |
| io2 Block Express | 256,000 | 4,000 MB/s | 64 TiB | 极端 IOPS 需求 |
| st1 | 500 | 500 MB/s | 16 TiB | 大数据/日志 |
| sc1 | 250 | 250 MB/s | 16 TiB | 冷数据归档 |

### 1.4 VolumeSnapshot 自动化

```yaml
# VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ebs-snapshot-class
driver: ebs.csi.aws.com
deletionPolicy: Retain
parameters:
  encrypted: "true"

---
# 定时快照（通过 CronJob + Velero 或 kubectl）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: db-snapshot
  namespace: database
spec:
  schedule: "0 */6 * * *"  # 每 6 小时
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-sa
          containers:
            - name: snapshotter
              image: bitnami/kubectl:1.31
              command:
                - /bin/sh
                - -c
                - |
                  TIMESTAMP=$(date +%Y%m%d-%H%M%S)
                  cat <<EOF | kubectl apply -f -
                  apiVersion: snapshot.storage.k8s.io/v1
                  kind: VolumeSnapshot
                  metadata:
                    name: db-snapshot-${TIMESTAMP}
                    namespace: database
                  spec:
                    volumeSnapshotClassName: ebs-snapshot-class
                    source:
                      persistentVolumeClaimName: mysql-data
                  EOF
                  # 清理 7 天前的快照
                  kubectl delete volumesnapshot -n database \
                    -l creationTimestamp<$(date -d '7 days ago' +%Y-%m-%dT%H:%M:%SZ)
          restartPolicy: OnFailure
```

## 2. EFS CSI 共享存储

### 2.1 创建 EFS 文件系统

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 EFS
aws efs create-file-system \
  --creation-token eks-shared-efs \
  --performance-mode generalPurpose \
  --throughput-mode bursting \
  --encrypted \
  --tags Key=Name,Value=eks-shared-efs

# 创建 Mount Target（每个 AZ 一个）
aws efs create-mount-target \
  --file-system-id fs-0123456789abcdef0 \
  --subnet-id subnet-aaaa \
  --security-groups sg-efs-mount-target
```
### 2.2 安装 EFS CSI Driver

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 安装
helm repo add aws-efs-csi-driver https://kubernetes-sigs.github.io/aws-efs-csi-driver
helm install aws-efs-csi-driver aws-efs-csi-driver/aws-efs-csi-driver \
  --namespace kube-system \
  --set controller.serviceAccount.create=true \
  --set controller.serviceAccount.annotations."eks\.amazonaws\.com/role-arn"=arn:aws:iam::123456789012:role/efs-csi-role
```
### 2.3 EFS StorageClass 与 PV

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: efs-sc
provisioner: efs.csi.aws.com
parameters:
  provisioningMode: efs-ap
  fileSystemId: fs-0123456789abcdef0
  directoryPerms: "700"
  gidRangeStart: "1000"
  gidRangeEnd: "2000"
  basePath: "/eks-volumes"

---
# PVC 使用
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-config-pvc
  namespace: production
spec:
  accessModes:
    - ReadWriteMany
  storageClassName: efs-sc
  resources:
    requests:
      storage: 10Gi
```

### 2.4 EFS 性能模式

| 模式 | 吞吐 | 延迟 | 适用场景 |
|------|------|------|---------|
| General Purpose | 低延迟 | < 1ms | Web 内容、配置共享 |
| Max I/O | 高吞吐 | 较高 | 大数据分析、媒体处理 |
| Elastic | 自动扩展 | 低 | 通用（推荐） |

## 3. FSx for Lustre 高性能存储

### 3.1 创建 FSx 文件系统

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
aws fsx create-file-system \
  --file-system-type LUSTRE \
  --storage-capacity 1200 \
  --subnet-ids subnet-aaaa \
  --security-group-ids sg-fsx \
  --lustre-configuration \
    DeploymentType=PERSISTENT_2,PerUnitStorageThroughput=250, \
    ImportPath=s3://my-bucket/data,ExportPath=s3://my-bucket/output \
  --tags Key=Name,Value=eks-fsx-lustre
```
### 3.2 FSx CSI Driver 配置

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fsx-lustre-sc
provisioner: fsx.csi.aws.com
parameters:
  subnetId: subnet-aaaa
  securityGroupIds: sg-fsx
  deploymentType: PERSISTENT_2
  perUnitStorageThroughput: "250"
  dataRepositoryPath: s3://my-bucket/data
  autoImportPolicy: NEW_CHANGED
  fileSystemTypeVersion: "2.12"
mountOptions:
  - flock

---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: ml-training-pvc
  namespace: ml-training
spec:
  accessModes:
    - ReadWriteMany
  storageClassName: fsx-lustre-sc
  resources:
    requests:
      storage: 1200Gi
```

### 3.3 FSx 性能规格

| 部署类型 | 吞吐 MB/s/TiB | 最大吞吐 | 适用场景 |
|---------|---------------|---------|---------|
| SCRATCH_2 | 200 | 10 GB/s | 临时计算 |
| PERSISTENT_1 | 50-200 | 10 GB/s | 持久化 HPC |
| PERSISTENT_2 | 125-1000 | 10 GB/s | 生产 ML 训练 |

## 4. StorageClass 最佳实践

### 4.1 命名规范

```
<介质>-<性能级别>-<用途>

示例:
  ebs-gp3-default       — 默认通用存储
  ebs-io2-database      — 数据库高性能存储
  efs-shared-config     — 共享配置存储
  fsx-lustre-ml         — ML 训练高性能存储
```

### 4.2 回收策略选择

| reclaimPolicy | 适用场景 | 风险 |
|---------------|---------|------|
| Delete | 临时数据、开发环境 | PVC 删除后数据不可恢复 |
| Retain | 生产数据库、重要数据 | 需手动清理 PV |

### 4.3 VolumeBindingMode

```yaml
# WaitForFirstConsumer — 推荐，确保 PV 在正确的 AZ 创建
volumeBindingMode: WaitForFirstConsumer

# Immediate — 立即创建，可能导致跨 AZ 挂载
volumeBindingMode: Immediate
```

### 4.4 数据加密

```yaml
# EBS 加密（KMS）
parameters:
  encrypted: "true"
  kmsKeyId: arn:aws:kms:region:account:key/key-id

# EFS 加密（传输 + 静态）
# 创建 EFS 时已启用加密
# Mount Target 通过 TLS 传输
```

## 5. VolumeSnapshot 与数据保护

### 5.1 Velero 备份方案

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 Velero
velero install \
  --provider aws \
  --plugins velero/velero-plugin-for-aws:v1.10.0 \
  --bucket eks-backup-bucket \
  --backup-location-config region=ap-southeast-1 \
  --snapshot-location-config region=ap-southeast-1 \
  --secret-file ./credentials-velero

# 创建定时备份
velero schedule create daily-backup \
  --schedule="0 2 * * *" \
  --include-namespaces=production,database \
  --ttl 720h \
  --snapshot-volumes=true
```
### 5.2 跨区域快照复制

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 复制快照到灾备区域
aws ec2 copy-snapshot \
  --source-region ap-southeast-1 \
  --source-snapshot-id snap-0123456789abcdef0 \
  --destination-region us-west-2 \
  --description "DR copy of db-snapshot-20260702"
```
## 6. 存储监控与告警

```yaml
# CloudWatch 告警 — EBS IOPS 使用率
# 通过 Prometheus + CloudWatch Exporter
- alert: HighEBSIOPSUtilization
  expr: |
    aws_ebs_volume_read_ops_sum + aws_ebs_volume_write_ops_sum
    > 0.8 * aws_ebs_volume_iops_limit
  for: 10m
  labels:
    severity: warning
  annotations:
    summary: "EBS volume {{ $labels.volume_id }} IOPS utilization above 80%"

- alert: EBSVolumeFull
  expr: |
    aws_ebs_volume_used_bytes / aws_ebs_volume_total_bytes > 0.85
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "EBS volume {{ $labels.volume_id }} is 85% full"
```

## Related

- [[01-eks-cluster-lifecycle-management]]
- [[04-eks-iam-irsa-pod-identity]]

## See Also

- AWS EBS CSI Driver
- AWS EFS CSI Driver
- FSx for Lustre CSI Driver


<!-- risk-assessed -->
