# Velero 企业级备份恢复实践指南

> **适用版本**: Velero v1.15.0  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

## 📋 目录

- [一、核心概念](#一核心概念)
- [二、安装部署](#二安装部署)
- [三、备份策略设计](#三备份策略设计)
- [四、定时备份与保留策略](#四定时备份与保留策略)
- [五、灾难恢复演练](#五灾难恢复演练)
- [六、跨集群迁移](#六跨集群迁移)
- [七、与 CSI 快照集成](#七与-csi-快照集成)
- [八、监控与告警](#八监控与告警)

---

## 一、核心概念

```
Velero 架构
├── Velero Server (Deployment)
│   ├── Backup Controller
│   ├── Restore Controller
│   ├── BackupStorageLocation (BSL) ──► S3 / OSS / GCS / Azure Blob
│   └── VolumeSnapshotLocation (VSL) ──► CSI / 云厂商快照
│
├── Velero CLI
│   ├── velero backup create
│   ├── velero restore create
│   ├── velero schedule create
│   └── velero backup-location / snapshot-location
│
└── 插件生态
    ├── 对象存储插件 (AWS/Azure/GCP/Alibaba)
    ├── CSI 插件 (快照支持)
    └── 社区插件 (多种后端)
```

---

## 二、安装部署

### 2.1 前置条件

```bash
# 创建对象存储 bucket (以 AWS S3 为例)
aws s3 mb s3://my-cluster-backups --region us-east-1

# 创建 IAM 用户/角色 (最小权限)
cat > velero-policy.json <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:GetObject",
        "s3:DeleteObject",
        "s3:PutObject",
        "s3:AbortMultipartUpload",
        "s3:ListMultipartUploadParts",
        "s3:ListBucket",
        "s3:GetBucketLocation"
      ],
      "Resource": [
        "arn:aws:s3:::my-cluster-backups/*",
        "arn:aws:s3:::my-cluster-backups"
      ]
    }
  ]
}
EOF
```

### 2.2 CLI 安装与配置

```bash
# 安装 Velero CLI
wget https://github.com/vmware-tanzu/velero/releases/download/v1.15.0/velero-v1.15.0-linux-amd64.tar.gz
tar -xzf velero-v1.15.0-linux-amd64.tar.gz
sudo mv velero /usr/local/bin/

# 安装 Velero Server (AWS 示例)
velero install \
  --provider aws \
  --plugins velero/velero-plugin-for-aws:v1.11.0 \
  --bucket my-cluster-backups \
  --backup-location-config region=us-east-1 \
  --snapshot-location-config region=us-east-1 \
  --secret-file ./credentials-velero \
  --use-node-agent \
  --use-volume-snapshots \
  --default-volumes-to-fs-backup

# 启用 CSI 快照 (推荐)
velero plugin add velero/velero-plugin-for-csi:v0.7.0
```

### 2.3 Helm 安装 (推荐生产环境)

```yaml
# values-velero.yaml
configuration:
  backupStorageLocation:
    - name: default
      provider: aws
      bucket: my-cluster-backups
      config:
        region: us-east-1
        s3ForcePathStyle: false
  volumeSnapshotLocation:
    - name: default
      provider: aws
      config:
        region: us-east-1
  features: EnableCSI

credentials:
  useSecret: true
  secretContents:
    aws: |
      [default]
      aws_access_key_id = xxx
      aws_secret_access_key = yyy

initContainers:
  - name: velero-plugin-for-aws
    image: velero/velero-plugin-for-aws:v1.11.0
    imagePullPolicy: IfNotPresent
    volumeMounts:
      - mountPath: /target
        name: plugins
  - name: velero-plugin-for-csi
    image: velero/velero-plugin-for-csi:v0.7.0
    imagePullPolicy: IfNotPresent
    volumeMounts:
      - mountPath: /target
        name: plugins

deployNodeAgent: true
nodeAgent:
  podVolumePath: /var/lib/kubelet/pods

# 资源限制
resources:
  requests:
    cpu: 500m
    memory: 256Mi
  limits:
    cpu: 2000m
    memory: 1Gi
```

```bash
helm repo add vmware-tanzu https://vmware-tanzu.github.io/helm-charts
helm install velero vmware-tanzu/velero \
  --namespace velero \
  --create-namespace \
  --values values-velero.yaml
```

---

## 三、备份策略设计

### 3.1 备份范围选择

| 备份类型 | 命令 | 适用场景 |
|:---|:---|:---|
| 全集群备份 | `velero backup create full-backup` | 灾难恢复基线 |
| 命名空间备份 | `--include-namespaces production` | 按团队/环境隔离 |
| 标签选择备份 | `--selector app=critical` | 关键应用保护 |
| 排除系统组件 | `--exclude-namespaces kube-system,velero` | 减少备份体积 |
| 仅资源不卷 | `--default-volumes-to-fs-backup=false` | 无状态应用 |

### 3.2 生产级备份命令

```bash
# 生产环境完整备份 (含 PV 快照)
velero backup create production-full-$(date +%Y%m%d) \
  --include-namespaces production,staging \
  --exclude-resources events,podmetrics \
  --snapshot-volumes \
  --default-volumes-to-fs-backup \
  --ttl 720h0m0s \
  --storage-location default \
  --volume-snapshot-locations default \
  --wait

# 备份时执行 Hook (数据库一致性)
velero backup create db-backup \
  --include-namespaces database \
  --ordered-resources 'statefulsets=postgres-primary,postgres-replica' \
  --hooks
```

---

## 四、定时备份与保留策略

### 4.1 Schedule 配置

```yaml
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: production-daily
  namespace: velero
spec:
  schedule: "0 2 * * *"  # 每天凌晨 2 点
  template:
    includedNamespaces:
      - production
    excludedResources:
      - events
      - podmetrics
    snapshotVolumes: true
    ttl: 168h0m0s  # 保留 7 天
    storageLocation: default
    volumeSnapshotLocations:
      - default
    defaultVolumesToFsBackup: true
---
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: production-weekly
  namespace: velero
spec:
  schedule: "0 3 * * 0"  # 每周日凌晨 3 点
  template:
    includedNamespaces:
      - production
    snapshotVolumes: true
    ttl: 2160h0m0s  # 保留 90 天
```

### 4.2 保留策略最佳实践

| 备份频率 | 保留周期 | 用途 |
|:---|:---|:---|
| 每日增量 | 7 天 | 快速回滚近期变更 |
| 每周全量 | 4 周 | 月度恢复点 |
| 每月全量 | 12 月 | 年度审计与长期归档 |
| 升级前 | 永久 (手动删除) | 重大变更前的安全基线 |

---

## 五、灾难恢复演练

### 5.1 单命名空间恢复

```bash
# 1. 列出可用备份
velero backup get

# 2. 恢复特定命名空间到新命名空间 (演练不覆盖生产)
velero restore create restore-test-$(date +%s) \
  --from-backup production-daily-20260424 \
  --include-namespaces production \
  --namespace-mappings production:production-restore-test \
  --wait

# 3. 验证恢复结果
kubectl get all -n production-restore-test
```

### 5.2 完整集群灾难恢复

```bash
# 1. 新集群安装 Velero (相同 BSL 配置)
velero install --provider aws ... (相同配置)

# 2. 同步备份元数据
velero backup-location get
velero backup get

# 3. 全集群恢复
velero restore create full-cluster-restore \
  --from-backup production-weekly-20260420 \
  --exclude-namespaces kube-system,kube-public,kube-node-lease,velero \
  --wait
```

### 5.3 资源过滤恢复

```bash
# 仅恢复特定资源类型
velero restore create partial-restore \
  --from-backup production-daily \
  --include-resources deployments,services,configmaps \
  --include-namespaces production \
  --wait

# 排除特定标签资源
velero restore create selective-restore \
  --from-backup production-daily \
  --selector 'backup-not-required!=true' \
  --wait
```

---

## 六、跨集群迁移

```bash
# 源集群: 创建备份
velero backup create migrate-$(date +%s) \
  --include-namespaces app-namespace \
  --snapshot-volumes \
  --wait

# 目标集群: 安装 Velero 指向同一个 BSL
velero install --provider aws --bucket shared-backups ...

# 目标集群: 等待备份同步
velero backup get

# 目标集群: 执行恢复
velero restore create migrate-restore \
  --from-backup migrate-xxx \
  --namespace-mappings app-namespace:app-namespace-new \
  --wait
```

---

## 七、与 CSI 快照集成

### 7.1 启用 CSI 快照

```yaml
# 确保 StorageClass 支持快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: csi-snapclass
  labels:
    velero.io/csi-volumesnapshot-class: "true"
driver: ebs.csi.aws.com  # 或 your-csi-driver
deletionPolicy: Retain
```

### 7.2 备份时自动使用 CSI 快照

```bash
velero backup create csi-backup \
  --include-namespaces database \
  --snapshot-volumes \
  --csi-snapshot-timeout 30m \
  --wait
```

### 7.3 快照 vs 文件系统备份对比

| 方式 | 速度 | 一致性 | 适用场景 |
|:---|:---|:---|:---|
| CSI 快照 | 快 (秒级) | 崩溃一致 | 大型数据库卷 |
| Restic/FS | 慢 (分钟级) | 应用一致 (需 Hook) | 小型卷、跨云迁移 |

---

## 八、监控与告警

### 8.1 Velero 指标端点

```yaml
# values-velero.yaml (Helm 配置中启用 metrics)
metrics:
  enabled: true
  scrapeInterval: 30s
  serviceMonitor:
    enabled: true
    namespace: monitoring
```

### 8.2 Prometheus 告警规则

```yaml
- alert: VeleroBackupFailed
  expr: velero_backup_failure_total > 0
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "Velero 备份失败"
    description: "备份 {{ $labels.backup }} 失败，请立即检查"

- alert: VeleroBackupTooOld
  expr: time() - velero_backup_last_successful_timestamp > 86400
  for: 1h
  labels:
    severity: warning
  annotations:
    summary: "Velero 备份超过 24 小时未成功"

- alert: VeleroRestoreFailed
  expr: velero_restore_failure_total > 0
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "Velero 恢复失败"
```

---

## 参考链接

- [Velero 官方文档](https://velero.io/docs/)
- [Velero Helm Chart](https://github.com/vmware-tanzu/helm-charts/tree/main/charts/velero)
- [Velero CSI 支持](https://velero.io/docs/main/csi/)
- [K8s 备份最佳实践](https://kubernetes.io/docs/tasks/administer-cluster/backup-restore/)
