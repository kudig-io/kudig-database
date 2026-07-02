---
title: CCE存储架构：EVS、SFS与OBS集成
description: 'CCE容器存储深度解析：EVS云盘CSI、SFS Turbo弹性文件、OBS对象存储挂载、存储加密与动态扩容'
summary: 'CCE容器存储深度解析：EVS云盘CSI、SFS Turbo弹性文件、OBS对象存储挂载、存储加密与动态扩容'
category: cloud-providers
tags:
- cloud
- k8s
- huawei-cce
- storage
- evs
- sfs
- obs
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
- CCE存储架构 是什么
- 如何在CCE中使用EVS云盘
- CCE SFS Turbo如何配置
trigger_keywords:
- CCE
- EVS
- SFS Turbo
- OBS
- CSI
- 存储类
- 动态扩容
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

# CCE存储架构：EVS、SFS与OBS集成

## 1. CCE 存储体系概览

CCE 通过 CSI (Container Storage Interface) 插件对接华为云多种存储服务：

| 存储类型 | 访问模式 | 性能等级 | 适用场景 |
|---------|---------|---------|---------|
| EVS 云盘 | RWO (单节点读写) | 极高 | 数据库、高 IOPS 工作负载 |
| SFS Turbo | RWX (多节点读写) | 高 | 共享文件存储、AI 训练 |
| OBS 对象存储 | RWX | 中 | 大数据、归档、静态资源 |
| 极速文件存储 | RWX | 高 | 高性能共享文件 |

**CSI 驱动名称**：
- `csi.huaweicloud.com` — EVS 云盘
- `sfsturbo.csi.huaweicloud.com` — SFS Turbo
- `obs.csi.huaweicloud.com` — OBS 对象存储

## 2. EVS 云盘 (CSI)

### 2.1 StorageClass 定义

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: evs-ssd
provisioner: csi.huaweicloud.com
parameters:
  # 磁盘类型: SSD / SAS / GPSSD (通用型SSD)
  type: SSD
  # 可用区 (必须与节点同可用区)
  availability: cn-north-4a
  # 磁盘加密 (可选)
  encrypted: "true"
  kmsKeyID: "<kms-key-id>"
  # 标签 (可选)
  tagSpecification: "purpose=database"
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: evs-high-throughput
provisioner: csi.huaweicloud.com
parameters:
  type: GPSSD           # 通用型 SSD，性价比高
  availability: cn-north-4a
reclaimPolicy: Retain    # 生产环境推荐 Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

### 2.2 PVC 使用

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-data-pvc
spec:
  accessModes:
    - ReadWriteOnce      # EVS 仅支持单节点读写
  storageClassName: evs-ssd
  resources:
    requests:
      storage: 100Gi
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
spec:
  replicas: 1
  selector:
    matchLabels:
      app: postgres
  template:
    metadata:
      labels:
        app: postgres
    spec:
      containers:
        - name: postgres
          image: postgres:16
          volumeMounts:
            - name: data
              mountPath: /var/lib/postgresql/data
          env:
            - name: POSTGRES_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: postgres-secret
                  key: password
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: evs-ssd
        resources:
          requests:
            storage: 100Gi
```

### 2.3 EVS 性能等级

| 类型 | 最大 IOPS | 最大吞吐 | 适用场景 |
|------|----------|---------|---------|
| SSD | 20,000 | 350 MB/s | 数据库、OLTP |
| GPSSD | 10,000 | 250 MB/s | 通用工作负载 |
| SAS | 2,500 | 150 MB/s | 大容量冷数据 |

性能随容量线性增长：SSD 类型每 GB 约 50 IOPS，最大 20,000 IOPS。

### 2.4 动态扩容

```bash
# 1. 确保 StorageClass 允许扩容
allowVolumeExpansion: true

# 2. 直接修改 PVC (无需重建 Pod)
kubectl patch pvc db-data-pvc -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'

# 3. 等待扩容完成
kubectl get pvc db-data-pvc -w
# 状态变化: Bound → FileSystemResizePending → Bound

# 4. 文件系统在线扩容 (CSI 自动完成，无需重启 Pod)
# 对于 ext4: resize2fs 自动执行
# 对于 xfs: xfs_growfs 自动执行
```

## 3. SFS Turbo 弹性文件存储

### 3.1 StorageClass 定义

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: sfs-turbo
provisioner: sfsturbo.csi.huaweicloud.com
parameters:
  # SFS Turbo ID (已创建的实例)
  csi.storage.k8s.io/provisioner-secret-name: sfsturbo-secret
  csi.storage.k8s.io/provisioner-secret-namespace: kube-system
  # 共享根路径 (可选)
  shareRootPath: "/"
  # 授权 VPC ID
  csi.storage.k8s.io/node-publish-secret-name: sfsturbo-secret
  csi.storage.k8s.io/node-publish-secret-namespace: kube-system
reclaimPolicy: Retain
allowVolumeExpansion: false
volumeBindingMode: Immediate
```

### 3.2 SFS Turbo Secret 配置

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: sfsturbo-secret
  namespace: kube-system
type: Opaque
stringData:
  # SFS Turbo 实例 ID
  shareID: "<sfs-turbo-id>"
  # 授权地址 (VPC CIDR 中的 IP)
  # 可选，不指定则使用节点 IP
```

### 3.3 共享存储使用

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: shared-data-pvc
spec:
  accessModes:
    - ReadWriteMany      # SFS Turbo 支持多节点读写
  storageClassName: sfs-turbo
  resources:
    requests:
      storage: 500Gi
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: shared-worker
spec:
  replicas: 5
  selector:
    matchLabels:
      app: worker
  template:
    metadata:
      labels:
        app: worker
    spec:
      containers:
        - name: worker
          image: busybox
          command: ["sh", "-c", "while true; do echo $(date) >> /data/log.txt; sleep 10; done"]
          volumeMounts:
            - name: shared
              mountPath: /data
      volumes:
        - name: shared
          persistentVolumeClaim:
            claimName: shared-data-pvc
```

### 3.4 SFS Turbo 规格对比

| 规格 | IOPS | 吞吐 | 延迟 | 容量范围 |
|------|------|------|------|---------|
| 标准型 | 15,000 | 250 MB/s | < 1 ms | 500Gi ~ 32Ti |
| 性能型 | 100,000 | 1,000 MB/s | < 0.5 ms | 500Gi ~ 32Ti |

## 4. OBS 对象存储挂载

### 4.1 CSI 驱动配置

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: obs-bucket
provisioner: obs.csi.huaweicloud.com
parameters:
  # OBS 桶名称
  bucket: my-training-data
  # 区域
  region: cn-north-4
  # 子路径 (可选)
  subPath: "datasets/imagenet"
reclaimPolicy: Retain
allowVolumeExpansion: false
volumeBindingMode: Immediate
```

### 4.2 AK/SK 认证 Secret

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: obs-secret
  namespace: default
type: Opaque
stringData:
  access-key: "<AK>"
  secret-key: "<SK>"
  # 或使用临时安全凭证 (STS)
  security-token: "<STOKEN>"
```

### 4.3 PV 静态绑定

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: obs-pv
spec:
  capacity:
    storage: 10Ti     # OBS 容量实际无上限，此处为占位
  accessModes:
    - ReadWriteMany
  persistentVolumeReclaimPolicy: Retain
  csi:
    driver: obs.csi.huaweicloud.com
    volumeHandle: obs-my-bucket
    nodePublishSecretRef:
      name: obs-secret
      namespace: default
    volumeAttributes:
      bucket: my-training-data
      region: cn-north-4
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: obs-pvc
spec:
  accessModes:
    - ReadWriteMany
  resources:
    requests:
      storage: 10Ti
  volumeName: obs-pv
```

### 4.4 AI 训练数据场景

```yaml
apiVersion: apps/v1
kind: Job
metadata:
  name: training-job
spec:
  template:
    spec:
      containers:
        - name: trainer
          image: nvidia/cuda:12.2-devel
          command: ["python", "train.py"]
          volumeMounts:
            - name: dataset
              mountPath: /data/input
              readOnly: true
            - name: output
              mountPath: /data/output
          resources:
            limits:
              nvidia.com/gpu: 4
      volumes:
        - name: dataset
          persistentVolumeClaim:
            claimName: obs-pvc
        - name: output
          persistentVolumeClaim:
            claimName: sfs-turbo-pvc   # 输出用 SFS Turbo，支持多节点写
```

## 5. 存储加密

### 5.1 EVS 服务端加密

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: evs-encrypted
provisioner: csi.huaweicloud.com
parameters:
  type: SSD
  availability: cn-north-4a
  # 启用加密
  encrypted: "true"
  # KMS 密钥 ID (不指定则使用默认密钥)
  kmsKeyID: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
```

### 5.2 KMS 密钥管理

```bash
# 创建 KMS 密钥 (通过控制台或 API)
# 建议为不同环境创建独立密钥:
#   - dev:     默认密钥
#   - staging: 自定义密钥
#   - prod:    自定义密钥 + 自动轮换

# 验证加密状态
kubectl get pv <pv-name> -o jsonpath='{.spec.csi.volumeAttributes}'
```

### 5.3 加密最佳实践

- 生产环境所有 EVS 卷必须启用加密
- KMS 密钥启用自动轮换（推荐 365 天轮换周期）
- 使用独立密钥隔离不同业务的加密域
- 定期审计 KMS 密钥的使用情况

## 6. 快照与备份

### 6.1 VolumeSnapshotClass

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-snapclass
driver: csi.huaweicloud.com
deletionPolicy: Retain
parameters:
  # 快照类型: 手动 / 自动
  type: manual
```

### 6.2 创建快照

```yaml
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: db-snapshot-20260702
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: db-data-pvc
```

### 6.3 从快照恢复

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: db-restore-pvc
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: evs-ssd
  resources:
    requests:
      storage: 100Gi
  dataSource:
    name: db-snapshot-20260702
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
```

### 6.4 自动定时快照

使用 CronJob 定期创建快照（通过 kubectl 脚本）：

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: volume-snapshot-cron
spec:
  schedule: "0 2 * * *"  # 每天凌晨 2 点
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: snapshot-sa
          containers:
            - name: snapshotter
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  SNAPSHOT_NAME="db-snapshot-$(date +%Y%m%d%H%M%S)"
                  cat <<EOF | kubectl apply -f -
                  apiVersion: snapshot.storage.k8s.io/v1
                  kind: VolumeSnapshot
                  metadata:
                    name: ${SNAPSHOT_NAME}
                    namespace: production
                  spec:
                    volumeSnapshotClassName: csi-snapclass
                    source:
                      persistentVolumeClaimName: db-data-pvc
                  EOF
          restartPolicy: OnFailure
```

## 7. 监控与排障

### 7.1 存储监控指标

```bash
# 查看 PV/PVC 状态
kubectl get pv,pvc -A

# 查看 CSI 驱动 Pod 状态
kubectl get pods -n kube-system | grep csi

# 查看挂载信息 (节点上执行)
mount | grep huawei
df -h | grep /var/lib/kubelet/pods
```

### 7.2 常见问题排查

**PVC Pending**：
```bash
kubectl describe pvc <pvc-name>
# 常见原因:
# - StorageClass 不存在
# - 可用区不匹配 (EVS 必须与节点同可用区)
# - 配额不足
```

**挂载失败**：
```bash
kubectl describe pod <pod-name>
# 常见原因:
# - 磁盘已挂载到其他节点 (EVS 不支持多节点)
# - AK/SK 无效 (OBS/SFS)
# - 安全组未放行
```

**性能问题**：
```bash
# 节点上执行 IOPS 测试
fio --name=test --ioengine=libaio --direct=1 --bs=4k \
    --size=1G --numjobs=4 --runtime=60 --rw=randread \
    --filename=/var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~csi/<pv>/globalmount
```

## 8. 最佳实践

1. **StorageClass 命名**：按性能等级命名（如 `evs-ssd`、`evs-gpssd`），便于业务选择
2. **WaitForFirstConsumer**：EVS 使用此模式确保卷与 Pod 同可用区
3. **Retain 策略**：生产环境 PV 使用 Retain，避免误删数据
4. **加密**：所有生产存储启用 KMS 加密
5. **快照策略**：数据库类工作负载每日自动快照，保留 7 天
6. **容量规划**：监控 PV 使用率，在 80% 时触发扩容告警
7. **SFS Turbo**：多读多写场景使用，注意选择合适的性能规格
8. **OBS 只读**：AI 训练数据集建议 OBS 只读挂载 + SFS Turbo 写入

---

*本文档描述 CCE 存储集成的架构、配置与运维。具体参数以华为云官方文档为准。*
