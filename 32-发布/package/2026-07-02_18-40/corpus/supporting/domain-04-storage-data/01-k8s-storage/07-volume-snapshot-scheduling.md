---
title: VolumeSnapshot 定时快照策略
description: 'Kubernetes VolumeSnapshot 定时快照：VolumeSnapshotClass 配置、CronJob 定时策略、跨区域复制、快照清理与 Velero 集成'
summary: 'Kubernetes VolumeSnapshot 定时快照：VolumeSnapshotClass 配置、CronJob 定时策略、跨区域复制、快照清理与 Velero 集成'
category: storage-data
tags:
- storage
- k8s
- volumesnapshot
- cronjob
- velero
- disaster-recovery
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
- VolumeSnapshot 定时快照 是什么
- 如何 VolumeSnapshotClass 配置
- CronJob 定时快照策略
- Velero 集成快照
trigger_keywords:
- VolumeSnapshot
- VolumeSnapshotClass
- CronJob
- 定时快照
- 跨区域复制
- Velero
- 快照清理
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


# VolumeSnapshot 定时快照策略

> **适用版本**: Kubernetes v1.28 - v1.32 | **CSI Snapshot Controller**: v6.x | **最后更新**: 2026-07
> **文档定位**: VolumeSnapshot 是 K8s 原生快照能力。本文覆盖定时快照策略、跨区域复制、快照清理和 Velero 集成。

## 1. 架构概览

### 1.1 VolumeSnapshot 架构

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────┐
│              VolumeSnapshot Architecture                 │
│                                                          │
│  ┌─────────────────────────────────────────────────────┐│
│  │  CSI Snapshot Controller                             ││
│  │  ┌──────────────┐  ┌──────────────────────────────┐ ││
│  │  │ Snapshot     │  │ Snapshot Content             │ ││
│  │  │ Controller   │  │ Controller                   │ ││
│  │  └──────────────┘  └──────────────────────────────┘ ││
│  └─────────────────────────────────────────────────────┘│
│                                                          │
│  ┌─────────────────────────────────────────────────────┐│
│  │  VolumeSnapshotClass                                ││
│  │  ┌────────────────────────────────────────────────┐ ││
│  │  │ driver: ebs.csi.aws.com                        │ ││
│  │  │ deletionPolicy: Retain                         │ ││
│  │  │ parameters: ...                                │ ││
│  │  └────────────────────────────────────────────────┘ ││
│  └─────────────────────────────────────────────────────┘│
│                                                          │
│  ┌─────────────────────────────────────────────────────┐│
│  │  VolumeSnapshot (用户创建)                           ││
│  │  ┌────────────────────────────────────────────────┐ ││
│  │  │ source:                                        │ ││
│  │  │   persistentVolumeClaimName: my-pvc            │ ││
│  │  │ volumeSnapshotClassName: ebs-snapclass         │ ││
│  │  └────────────────────────────────────────────────┘ ││
│  └─────────────────────────────────────────────────────┘│
│                                                          │
│  ┌─────────────────────────────────────────────────────┐│
│  │  VolumeSnapshotContent (自动/手动创建)               ││
│  │  ┌────────────────────────────────────────────────┐ ││
│  │  │ source:                                        │ ││
│  │  │   snapshotHandle: snap-0123456789abcdef0        │ ││
│  │  │ volumeSnapshotRef: ...                         │ ││
│  │  └────────────────────────────────────────────────┘ ││
│  └─────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────┘
```
### 1.2 快照流程

```
用户创建 VolumeSnapshot
        │
        ▼
CSI Snapshot Controller 检测
        │
        ▼
调用 CSI Driver 创建快照
        │
        ▼
创建 VolumeSnapshotContent
        │
        ▼
快照就绪 (ReadyToUse: true)
        │
        ▼
可从快照创建新 PVC
```

## 2. 环境准备

### 2.1 安装 CSI Snapshot Controller

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查是否已安装（部分 K8s 发行版已内置）
kubectl get pods -n kube-system | grep snapshot

# 如果未安装，手动安装
# 1. 下载 snapshot CRDs
kubectl apply -f https://raw.githubusercontent.com/kubernetes-csi/external-snapshotter/master/client/config/crd/snapshot.storage.k8s.io_volumesnapshotclasses.yaml
kubectl apply -f https://raw.githubusercontent.com/kubernetes-csi/external-snapshotter/master/client/config/crd/snapshot.storage.k8s.io_volumesnapshotcontents.yaml
kubectl apply -f https://raw.githubusercontent.com/kubernetes-csi/external-snapshotter/master/client/config/crd/snapshot.storage.k8s.io_volumesnapshots.yaml

# 2. 安装 RBAC
kubectl apply -f https://raw.githubusercontent.com/kubernetes-csi/external-snapshotter/master/deploy/kubernetes/snapshot-controller/rbac-snapshot-controller.yaml

# 3. 安装 Snapshot Controller
kubectl apply -f https://raw.githubusercontent.com/kubernetes-csi/external-snapshotter/master/deploy/kubernetes/snapshot-controller/setup-snapshot-controller.yaml

# 验证安装
kubectl get pods -n kube-system -l app=snapshot-controller
```
### 2.2 VolumeSnapshotClass 配置

```yaml
# AWS EBS VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ebs-snapclass
driver: ebs.csi.aws.com
deletionPolicy: Retain    # 删除快照时不删除底层存储快照
parameters:
  # 标签（用于快照管理）
  tagSpecification: "Name={{ .VolumeSnapshotName }},Environment=production"
---
# Azure Disk VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: azure-disk-snapclass
driver: disk.csi.azure.com
deletionPolicy: Retain
parameters:
  incremental: "true"      # 增量快照（节省存储）
  resourceGroup: "myResourceGroup"
---
# GCE PD VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: gce-pd-snapclass
driver: pd.csi.storage.gke.io
deletionPolicy: Retain
parameters:
  storage-locations: "us-central1"
  # 加密快照
  encryption-key: "projects/my-project/locations/us-central1/keyRings/my-ring/cryptoKeys/my-key"
---
# Ceph RBD VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ceph-rbd-snapclass
driver: rbd.csi.ceph.com
deletionPolicy: Retain
parameters:
  clusterID: "my-ceph-cluster"
  csi.storage.k8s.io/snapshotter-secret-name: csi-rbd-secret
  csi.storage.k8s.io/snapshotter-secret-namespace: ceph-csi
```

## 3. CronJob 定时快照策略

### 3.1 快照创建 CronJob

```yaml
# snapshot-cronjob.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: volume-snapshot-hourly
  namespace: default
spec:
  schedule: "0 * * * *"    # 每小时执行
  concurrencyPolicy: Forbid
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 3
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-creator
          containers:
            - name: snapshot-creator
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  TIMESTAMP=$(date +%Y%m%d-%H%M%S)
                  NAMESPACE="default"
                  PVC_NAME="mysql-data"
                  SNAPSHOT_CLASS="ebs-snapclass"
                  
                  # 创建快照
                  cat <<EOF | kubectl apply -f -
                  apiVersion: snapshot.storage.k8s.io/v1
                  kind: VolumeSnapshot
                  metadata:
                    name: ${PVC_NAME}-snapshot-${TIMESTAMP}
                    namespace: ${NAMESPACE}
                    labels:
                      app: mysql
                      snapshot-type: scheduled
                      schedule: hourly
                  spec:
                    volumeSnapshotClassName: ${SNAPSHOT_CLASS}
                    source:
                      persistentVolumeClaimName: ${PVC_NAME}
                  EOF
                  
                  # 等待快照就绪
                  kubectl wait --for=condition=ReadyToUse \
                    volumesnapshot/${PVC_NAME}-snapshot-${TIMESTAMP} \
                    -n ${NAMESPACE} \
                    --timeout=300s
                  
                  echo "Snapshot ${PVC_NAME}-snapshot-${TIMESTAMP} created successfully"
          restartPolicy: OnFailure
---
# 每日快照
apiVersion: batch/v1
kind: CronJob
metadata:
  name: volume-snapshot-daily
  namespace: default
spec:
  schedule: "0 2 * * *"    # 每天凌晨 2 点
  concurrencyPolicy: Forbid
  successfulJobsHistoryLimit: 7
  failedJobsHistoryLimit: 3
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-creator
          containers:
            - name: snapshot-creator
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  TIMESTAMP=$(date +%Y%m%d)
                  NAMESPACE="default"
                  PVC_NAME="mysql-data"
                  SNAPSHOT_CLASS="ebs-snapclass"
                  
                  cat <<EOF | kubectl apply -f -
                  apiVersion: snapshot.storage.k8s.io/v1
                  kind: VolumeSnapshot
                  metadata:
                    name: ${PVC_NAME}-daily-${TIMESTAMP}
                    namespace: ${NAMESPACE}
                    labels:
                      app: mysql
                      snapshot-type: scheduled
                      schedule: daily
                      retention: 30d
                  spec:
                    volumeSnapshotClassName: ${SNAPSHOT_CLASS}
                    source:
                      persistentVolumeClaimName: ${PVC_NAME}
                  EOF
                  
                  kubectl wait --for=condition=ReadyToUse \
                    volumesnapshot/${PVC_NAME}-daily-${TIMESTAMP} \
                    -n ${NAMESPACE} \
                    --timeout=600s
                  
                  echo "Daily snapshot created: ${PVC_NAME}-daily-${TIMESTAMP}"
          restartPolicy: OnFailure
```

### 3.2 RBAC 配置

```yaml
# snapshot-rbac.yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: snapshot-creator
  namespace: default
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: snapshot-creator-role
  namespace: default
rules:
  - apiGroups: ["snapshot.storage.k8s.io"]
    resources: ["volumesnapshots"]
    verbs: ["create", "get", "list", "watch", "delete"]
  - apiGroups: [""]
    resources: ["persistentvolumeclaims"]
    verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: snapshot-creator-binding
  namespace: default
subjects:
  - kind: ServiceAccount
    name: snapshot-creator
    namespace: default
roleRef:
  kind: Role
  name: snapshot-creator-role
  apiGroup: rbac.authorization.k8s.io
```

### 3.3 多 PVC 批量快照

```yaml
# batch-snapshot-cronjob.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: batch-snapshot-hourly
  namespace: production
spec:
  schedule: "0 * * * *"
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-creator
          containers:
            - name: batch-snapshot
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  TIMESTAMP=$(date +%Y%m%d-%H%M%S)
                  NAMESPACE="production"
                  SNAPSHOT_CLASS="ebs-snapclass"
                  
                  # 获取所有带 backup=true 标签的 PVC
                  PVC_LIST=$(kubectl get pvc -n ${NAMESPACE} \
                    -l backup=true \
                    -o jsonpath='{.items[*].metadata.name}')
                  
                  for PVC in ${PVC_LIST}; do
                    echo "Creating snapshot for PVC: ${PVC}"
                    
                    cat <<EOF | kubectl apply -f -
                    apiVersion: snapshot.storage.k8s.io/v1
                    kind: VolumeSnapshot
                    metadata:
                      name: ${PVC}-snapshot-${TIMESTAMP}
                      namespace: ${NAMESPACE}
                      labels:
                        snapshot-type: scheduled
                        schedule: hourly
                        source-pvc: ${PVC}
                    spec:
                      volumeSnapshotClassName: ${SNAPSHOT_CLASS}
                      source:
                        persistentVolumeClaimName: ${PVC}
                    EOF
                  done
                  
                  echo "Batch snapshot completed for ${TIMESTAMP}"
          restartPolicy: OnFailure
```

## 4. 快照清理策略

### 4.1 基于数量的清理

```yaml
# snapshot-retention-by-count.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: snapshot-cleanup-count
  namespace: default
spec:
  schedule: "0 3 * * *"    # 每天凌晨 3 点
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-cleaner
          containers:
            - name: cleanup
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  NAMESPACE="default"
                  RETENTION_COUNT=24  # 保留最近 24 个快照
                  
                  # 获取按时间排序的快照列表
                  SNAPSHOTS=$(kubectl get volumesnapshot -n ${NAMESPACE} \
                    -l snapshot-type=scheduled,schedule=hourly \
                    --sort-by=.metadata.creationTimestamp \
                    -o jsonpath='{.items[*].metadata.name}')
                  
                  SNAP_COUNT=$(echo ${SNAPSHOTS} | wc -w)
                  
                  if [ ${SNAP_COUNT} -gt ${RETENTION_COUNT} ]; then
                    DELETE_COUNT=$((SNAP_COUNT - RETENTION_COUNT))
                    echo "Deleting ${DELETE_COUNT} old snapshots..."
                    
                    for SNAP in ${SNAPSHOTS}; do
                      if [ ${DELETE_COUNT} -le 0 ]; then
                        break
                      fi
                      echo "Deleting snapshot: ${SNAP}"
                      kubectl delete volumesnapshot ${SNAP} -n ${NAMESPACE}
                      DELETE_COUNT=$((DELETE_COUNT - 1))
                    done
                  else
                    echo "No snapshots to delete. Count: ${SNAP_COUNT}, Retention: ${RETENTION_COUNT}"
                  fi
          restartPolicy: OnFailure
```

### 4.2 基于时间的清理

```yaml
# snapshot-retention-by-time.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: snapshot-cleanup-time
  namespace: default
spec:
  schedule: "0 4 * * *"    # 每天凌晨 4 点
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-cleaner
          containers:
            - name: cleanup
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  NAMESPACE="default"
                  
                  # 计算 7 天前的时间戳
                  CUTOFF_DATE=$(date -d "7 days ago" +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || \
                                date -v-7d +%Y-%m-%dT%H:%M:%SZ)
                  
                  echo "Deleting snapshots older than: ${CUTOFF_DATE}"
                  
                  # 获取所有定时快照
                  kubectl get volumesnapshot -n ${NAMESPACE} \
                    -l snapshot-type=scheduled \
                    -o json | jq -r --arg cutoff "${CUTOFF_DATE}" \
                    '.items[] | select(.metadata.creationTimestamp < $cutoff) | .metadata.name' | \
                  while read SNAP; do
                    echo "Deleting old snapshot: ${SNAP}"
                    kubectl delete volumesnapshot ${SNAP} -n ${NAMESPACE}
                  done
          restartPolicy: OnFailure
```

### 4.3 标签驱动的差异化清理

```yaml
# 不同保留策略通过标签实现
# 短期快照（保留 3 天）
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-hourly-snapshot
  labels:
    snapshot-type: scheduled
    retention: short-term    # 3 天
    schedule: hourly
spec:
  volumeSnapshotClassName: ebs-snapclass
  source:
    persistentVolumeClaimName: mysql-data
---
# 长期快照（保留 30 天）
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-daily-snapshot
  labels:
    snapshot-type: scheduled
    retention: long-term     # 30 天
    schedule: daily
spec:
  volumeSnapshotClassName: ebs-snapclass
  source:
    persistentVolumeClaimName: mysql-data
```

```yaml
# 清理 CronJob 中根据标签选择保留时间
# retention-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: snapshot-retention-config
data:
  retention.json: |
    {
      "short-term": {"days": 3, "label": "retention=short-term"},
      "medium-term": {"days": 7, "label": "retention=medium-term"},
      "long-term": {"days": 30, "label": "retention=long-term"},
      "yearly": {"days": 365, "label": "retention=yearly"}
    }
```

## 5. 跨区域快照复制

### 5.1 AWS EBS 快照跨区域复制

```yaml
# aws-snapshot-copy-cronjob.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: ebs-snapshot-cross-region
  namespace: default
spec:
  schedule: "0 6 * * *"    # 每天凌晨 6 点
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: aws-snapshot-copy
          containers:
            - name: snapshot-copy
              image: amazon/aws-cli:latest
              env:
                - name: SOURCE_REGION
                  value: "us-east-1"
                - name: TARGET_REGION
                  value: "us-west-2"
              command:
                - /bin/sh
                - -c
                - |
                  # 获取源区域的最新快照
                  SNAPSHOTS=$(aws ec2 describe-snapshots \
                    --owner-ids self \
                    --region ${SOURCE_REGION} \
                    --filters "Name=tag:snapshot-type,Values=scheduled" \
                    --query 'Snapshots[*].[SnapshotId,StartTime]' \
                    --output text | sort -k2 -r | head -5)
                  
                  for SNAP_INFO in ${SNAPSHOTS}; do
                    SNAP_ID=$(echo ${SNAP_INFO} | awk '{print $1}')
                    echo "Copying snapshot ${SNAP_ID} to ${TARGET_REGION}"
                    
                    # 复制快照到目标区域
                    aws ec2 copy-snapshot \
                      --source-region ${SOURCE_REGION} \
                      --source-snapshot-id ${SNAP_ID} \
                      --destination-region ${TARGET_REGION} \
                      --description "Cross-region copy of ${SNAP_ID}" \
                      --encrypted \
                      --kms-key-id alias/snapshot-key
                  done
          restartPolicy: OnFailure
```

### 5.2 Azure Disk 快照复制

```yaml
# azure-snapshot-copy-cronjob.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: azure-snapshot-copy
  namespace: default
spec:
  schedule: "0 6 * * *"
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: snapshot-copy
              image: mcr.microsoft.com/azure-cli:latest
              env:
                - name: SOURCE_RG
                  value: "myResourceGroup-east"
                - name: TARGET_RG
                  value: "myResourceGroup-west"
                - name: TARGET_LOCATION
                  value: "westus2"
              command:
                - /bin/sh
                - -c
                - |
                  # 获取源资源组中的快照
                  SNAPSHOTS=$(az snapshot list \
                    --resource-group ${SOURCE_RG} \
                    --query "[?tags.snapshot-type=='scheduled'].name" \
                    -o tsv)
                  
                  for SNAP in ${SNAPSHOTS}; do
                    echo "Copying snapshot ${SNAP} to ${TARGET_RG}"
                    
                    # 获取源快照
                    SOURCE_SNAP=$(az snapshot show \
                      --resource-group ${SOURCE_RG} \
                      --name ${SNAP} \
                      --query 'id' -o tsv)
                    
                    # 创建跨区域副本
                    az snapshot create \
                      --resource-group ${TARGET_RG} \
                      --name "${SNAP}-copy" \
                      --source ${SOURCE_SNAP} \
                      --location ${TARGET_LOCATION} \
                      --sku Standard_LRS \
                      --incremental true
                  done
          restartPolicy: OnFailure
```

## 6. 从快照恢复

### 6.1 从快照创建 PVC

```yaml
# restore-from-snapshot.yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data-restored
  namespace: default
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: ebs-gp3
  resources:
    requests:
      storage: 100Gi    # 必须 >= 原始 PVC 大小
  dataSource:
    name: mysql-data-snapshot-20260702
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
---
# 使用恢复的 PVC 的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: mysql-restored
  namespace: default
spec:
  containers:
    - name: mysql
      image: mysql:8.0
      volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
  volumes:
    - name: data
      persistentVolumeClaim:
        claimName: mysql-data-restored
```

### 6.2 自动化恢复流程

```yaml
# automated-restore-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: restore-from-snapshot
  namespace: default
spec:
  template:
    spec:
      serviceAccountName: snapshot-restorer
      containers:
        - name: restore
          image: bitnami/kubectl:latest
          command:
            - /bin/sh
            - -c
            - |
              SNAPSHOT_NAME=$1
              RESTORE_PVC_NAME=$2
              NAMESPACE=$3
              
              # 检查快照是否就绪
              READY=$(kubectl get volumesnapshot ${SNAPSHOT_NAME} \
                -n ${NAMESPACE} \
                -o jsonpath='{.status.readyToUse}')
              
              if [ "${READY}" != "true" ]; then
                echo "Snapshot ${SNAPSHOT_NAME} is not ready"
                exit 1
              fi
              
              # 获取快照大小
              SIZE=$(kubectl get volumesnapshot ${SNAPSHOT_NAME} \
                -n ${NAMESPACE} \
                -o jsonpath='{.status.restoreSize}')
              
              # 创建恢复 PVC
              cat <<EOF | kubectl apply -f -
              apiVersion: v1
              kind: PersistentVolumeClaim
              metadata:
                name: ${RESTORE_PVC_NAME}
                namespace: ${NAMESPACE}
              spec:
                accessModes:
                  - ReadWriteOnce
                storageClassName: ebs-gp3
                resources:
                  requests:
                    storage: ${SIZE}
                dataSource:
                  name: ${SNAPSHOT_NAME}
                  kind: VolumeSnapshot
                  apiGroup: snapshot.storage.k8s.io
              EOF
              
              # 等待 PVC 就绪
              kubectl wait --for=condition=Bound \
                pvc/${RESTORE_PVC_NAME} \
                -n ${NAMESPACE} \
                --timeout=300s
              
              echo "Restore completed: ${RESTORE_PVC_NAME}"
          args:
            - "mysql-data-snapshot-20260702"
            - "mysql-data-restored"
            - "default"
      restartPolicy: Never
```

## 7. 与 Velero 集成

### 7.1 Velero 安装与配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 Velero（支持快照）
velero install \
  --provider aws \
  --plugins velero/velero-plugin-for-aws:v1.9.0 \
  --bucket my-velero-backup \
  --backup-location-config region=us-east-1 \
  --snapshot-location-config region=us-east-1 \
  --secret-file ./credentials-velero
```
### 7.2 Velero 定时备份（包含快照）

```yaml
# velero-schedule.yaml
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-backup-with-snapshots
  namespace: velero
spec:
  schedule: "0 2 * * *"    # 每天凌晨 2 点
  template:
    includedNamespaces:
      - production
      - database
    includedResources:
      - persistentvolumeclaims
      - persistentvolumes
      - deployments
      - statefulsets
      - services
      - configmaps
      - secrets
    snapshotVolumes: true    # 包含卷快照
    storageLocation: default
    volumeSnapshotLocations:
      - default
    ttl: 720h0m0s           # 保留 30 天
    metadata:
      labels:
        backup-type: daily
---
# 每周完整备份
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: weekly-full-backup
  namespace: velero
spec:
  schedule: "0 3 * * 0"    # 每周日凌晨 3 点
  template:
    includedNamespaces:
      - "*"
    excludedResources:
      - events
      - events.events.k8s.io
    snapshotVolumes: true
    storageLocation: default
    volumeSnapshotLocations:
      - default
    ttl: 2160h0m0s          # 保留 90 天
    metadata:
      labels:
        backup-type: weekly-full
```

### 7.3 Velero 备份验证

```yaml
# backup-verification-cronjob.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-verification
  namespace: velero
spec:
  schedule: "0 8 * * *"    # 每天早上 8 点验证
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: verify
              image: velero/velero:latest
              command:
                - /bin/sh
                - -c
                - |
                  # 检查最近的备份状态
                  velero backup get --output json | \
                    jq '.items | sort_by(.status.completionTimestamp) | last | 
                    {
                      name: .metadata.name,
                      status: .status.phase,
                      completionTime: .status.completionTimestamp,
                      volumeSnapshots: .status.volumeSnapshotsAttempted
                    }'
                  
                  # 验证快照完整性
                  LATEST_BACKUP=$(velero backup get -o json | \
                    jq -r '.items | sort_by(.status.completionTimestamp) | last | .metadata.name')
                  
                  velero backup describe ${LATEST_BACKUP} --details
          restartPolicy: OnFailure
```

## 8. 监控与告警

### 8.1 Prometheus 指标

```yaml
# snapshot-metrics-servicemonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: snapshot-controller-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: snapshot-controller
  namespaceSelector:
    matchNames:
      - kube-system
  endpoints:
    - port: metrics
      interval: 15s
```

### 8.2 告警规则

```yaml
# snapshot-alerting-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: volume-snapshot-alerts
  namespace: monitoring
spec:
  groups:
    - name: volume-snapshot.rules
      rules:
        # 快照创建失败
        - alert: VolumeSnapshotCreationFailed
          expr: |
            increase(kubernetes_volume_snapshot_create_errors_total[1h]) > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "VolumeSnapshot 创建失败"

        # 快照数量过多
        - alert: TooManyVolumeSnapshots
          expr: |
            count by (namespace) (kube_volumesnapshot_info) > 100
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "命名空间 {{ $labels.namespace }} 快照数量超过 100"

        # 快照状态异常
        - alert: VolumeSnapshotNotReady
          expr: |
            kube_volumesnapshot_status_ready_to_use == 0
          for: 30m
          labels:
            severity: warning
          annotations:
            summary: "VolumeSnapshot {{ $labels.volumesnapshot }} 长时间未就绪"
```

## 9. 生产最佳实践

### 9.1 快照策略建议

| 场景 | 频率 | 保留期 | 说明 |
|------|------|--------|------|
| **数据库** | 每小时 | 24 小时 | 高频快照，短期保留 |
| **数据库（日备份）** | 每天 | 30 天 | 日级备份，中期保留 |
| **应用数据** | 每 6 小时 | 7 天 | 中频快照 |
| **配置数据** | 每天 | 90 天 | 低频快照，长期保留 |
| **关键数据** | 每周 | 1 年 | 年度归档 |

### 9.2 容量规划

```yaml
capacity_planning:
  # 快照存储成本
  snapshot_storage:
    # 增量快照（云厂商通常支持）
    incremental: true
    
    # 估算公式
    # 快照存储 = 每日变更量 × 保留天数 × 副本数
    # 示例: 10GB/天 × 30天 × 2区域 = 600GB
  
  # 跨区域复制成本
  cross_region:
    transfer_cost: "按流量计费"
    storage_cost: "目标区域存储价格"
```

### 9.3 安全建议

```yaml
security:
  # 快照加密
  encryption:
    - 使用 KMS 加密快照
    - 定期轮转加密密钥
  
  # 访问控制
  rbac:
    - 限制快照创建/删除权限
    - 使用独立 ServiceAccount
  
  # 审计
  audit:
    - 记录所有快照操作
    - 定期审查快照列表
```

---

## Related

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-04-storage-data/01-k8s-storage/03-storage-backup-disaster-recovery|存储备份与灾难恢复]]

## See Also

- [Kubernetes VolumeSnapshot](https://kubernetes.io/docs/concepts/storage/volume-snapshots/)
- [CSI Snapshot Controller](https://github.com/kubernetes-csi/external-snapshotter)
- [Velero Documentation](https://velero.io/docs/)


<!-- risk-assessed -->
