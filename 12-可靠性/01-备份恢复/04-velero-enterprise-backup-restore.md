---
title: Velero Enterprise Backup & Restore
description: Velero 企业级备份恢复深度实践 — 架构设计、定时备份、跨集群迁移、灾难恢复演练、性能调优
summary: Velero 生产环境完整指南，涵盖备份策略、恢复流程、跨云迁移、RPO/RTO 保障
category: practice
tags:
- velero
- backup
- restore
- disaster-recovery
- migration
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: reliability
---
# Velero 企业级备份恢复

> Kubernetes 集群的备份、恢复与跨集群迁移的企业级实践。

## 架构设计

```
┌─────────────────────────────────────────────────┐
│                 Velero Server                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │
│  │ Backup   │  │ Restore  │  │ Schedule     │  │
│  │Controller│  │Controller│  │ Controller   │  │
│  └────┬─────┘  └────┬─────┘  └──────┬───────┘  │
│       │              │               │           │
│  ┌────▼──────────────▼───────────────▼───────┐  │
│  │           Plugin Framework                  │  │
│  │  (Volume Snapshot / Object Store / CSM)    │  │
│  └────────────────────┬───────────────────────┘  │
└───────────────────────┼──────────────────────────┘
                        │
        ┌───────────────┼───────────────┐
        ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│ Object Store │ │ Volume       │ │ K8s API      │
│ (S3/GCS/     │ │ Snapshots    │ │ Server       │
│  Azure Blob) │ │ (EBS/PD/     │ │              │
│              │ │  CSI)        │ │              │
└──────────────┘ └──────────────┘ └──────────────┘
```

## 生产部署

### Helm 安装（AWS 示例）

```bash
helm repo add vmware-tanzu https://vmware-tanzu.github.io/helm-charts

helm install velero vmware-tanzu/velero \
  --namespace velero \
  --create-namespace \
  --set configuration.backupStorageLocation[0].name=default \
  --set configuration.backupStorageLocation[0].provider=aws \
  --set configuration.backupStorageLocation[0].bucket=k8s-backups \
  --set configuration.backupStorageLocation[0].config.region=ap-southeast-1 \
  --set configuration.volumeSnapshotLocation[0].name=default \
  --set configuration.volumeSnapshotLocation[0].provider=aws \
  --set configuration.volumeSnapshotLocation[0].config.region=ap-southeast-1 \
  --set initContainers[0].name=velero-plugin-for-aws \
  --set initContainers[0].image=velero/velero-plugin-for-aws:v1.9.0 \
  --set initContainers[0].volumeMounts[0].mountPath=/target \
  --set initContainers[0].volumeMounts[0].name=plugins \
  --set credentials.useSecret=true \
  --set credentials.secretContents.cloud="[default]\naws_access_key_id=AKIA...\naws_secret_access_key=..."
```

### 资源配额（生产推荐）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: velero
  namespace: velero
spec:
  template:
    spec:
      containers:
        - name: velero
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: "2"
              memory: 2Gi
          env:
            - name: VELERO_CLIENT_BURST
              value: "100"
            - name: VELERO_CLIENT_QPS
              value: "50"
```

## 备份策略设计

### 定时备份 Schedule

```yaml
# 全量备份：每日凌晨 2 点
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-full-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"
  template:
    ttl: 168h  # 保留 7 天
    includedNamespaces:
      - "*"
    excludedNamespaces:
      - kube-system
      - velero
    storageLocation: default
    volumeSnapshotLocations:
      - default
    snapshotVolumes: true
---
# 关键业务：每 6 小时
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: critical-apps-backup
  namespace: velero
spec:
  schedule: "0 */6 * * *"
  template:
    ttl: 72h
    includedNamespaces:
      - production
      - database
    labelSelector:
      matchLabels:
        backup-tier: critical
    storageLocation: default
    snapshotVolumes: true
---
# 配置备份（不含卷）：每小时
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: config-only-backup
  namespace: velero
spec:
  schedule: "0 * * * *"
  template:
    ttl: 48h
    includedNamespaces:
      - "*"
    snapshotVolumes: false
    includeClusterResources: true
    storageLocation: default
```

### 备份保留策略

| 备份类型 | 频率 | 保留期 | 内容 |
|----------|------|--------|------|
| 全量备份 | 每日 | 7 天 | 所有命名空间 + 卷快照 |
| 关键业务 | 每 6h | 3 天 | 生产命名空间 + 卷 |
| 配置备份 | 每小时 | 2 天 | 仅资源定义（无卷） |
| 周备份 | 每周 | 30 天 | 全量 + 集群资源 |
| 月备份 | 每月 | 90 天 | 全量归档 |

## 恢复操作

### 完整集群恢复

```bash
# 从备份恢复
velero restore create --from-backup daily-full-backup-20260721

# 恢复到新命名空间
velero restore create --from-backup daily-full-backup-20260721 \
  --namespace-mappings production:production-dr

# 选择性恢复
velero restore create --from-backup daily-full-backup-20260721 \
  --include-namespaces production \
  --include-resources deployments,services,configmaps

# 查看恢复状态
velero restore describe <restore-name> --details
velero restore logs <restore-name>
```

### 跨集群迁移

```bash
# 源集群：创建备份
velero backup create migration-backup \
  --include-namespaces my-app \
  --snapshot-volumes \
  --wait

# 目标集群：确保 Velero 指向同一对象存储
# 然后恢复
velero restore create --from-backup migration-backup \
  --include-namespaces my-app
```

## CSI 卷快照集成

```yaml
# 启用 CSI 插件（替代云厂商特定插件）
apiVersion: velero.io/v1
kind: VolumeSnapshotLocation
metadata:
  name: csi-default
  namespace: velero
spec:
  provider: csi
---
# DataUpload/DataDownload 配置（K8s 1.28+）
apiVersion: velero.io/v2alpha1
kind: DataUpload
metadata:
  name: data-upload-example
spec:
  backupStorageLocation: default
  sourcePVC:
    name: data-pvc
    namespace: production
  dataMover: csi
```

## 监控与告警

### Prometheus 指标

```yaml
# ServiceMonitor
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: velero
  namespace: velero
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: velero
  endpoints:
    - port: metrics
      interval: 30s
```

### 关键告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: velero-alerts
  namespace: velero
spec:
  groups:
    - name: velero
      rules:
        - alert: VeleroBackupFailed
          expr: velero_backup_failure_total > 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Velero 备份失败"
            description: "备份 {{ $labels.schedule }} 连续失败"
        - alert: VeleroBackupStale
          expr: time() - velero_backup_last_successful_timestamp > 86400
          labels:
            severity: warning
          annotations:
            summary: "备份超过 24 小时未成功"
        - alert: VeleroRestoreFailed
          expr: velero_restore_failed_total > 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Velero 恢复失败"
```

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 备份超时 | 大量 PV 快照 | 增加 `--snapshot-timeout` |
| 卷快照失败 | CSI 驱动不兼容 | 检查 VolumeSnapshotClass |
| 恢复后 Pod Pending | PV 绑定冲突 | 使用 `--namespace-mappings` |
| 对象存储连接失败 | 凭证过期 | 更新 Secret/IRSA |
| 备份体积过大 | 未排除临时数据 | 配置 `excludedResources` |

### 诊断命令

```bash
# 查看备份详情
velero backup describe <name> --details

# 查看备份日志
velero backup logs <name>

# 查看插件日志
kubectl logs -n velero deploy/velero -c velero

# 检查备份存储位置状态
velero backup-location get

# 检查卷快照位置
velero snapshot-location get
```

## 最佳实践

1. **3-2-1 备份原则**：3 份副本、2 种介质、1 份异地
2. **定期恢复演练**：每月至少一次恢复验证
3. **备份加密**：启用对象存储 SSE 加密
4. **RBAC 最小权限**：Velero ServiceAccount 仅授予必要权限
5. **版本兼容**：Velero 版本与 K8s 版本保持兼容（N-2）
6. **监控覆盖**：备份成功率、恢复时间、存储用量
7. **文档化**：恢复流程 Runbook 定期更新

## 备份验证自动化

### 自动验证 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-validation
  namespace: velero
spec:
  schedule: "0 6 * * *"  # 每天早 6 点
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: velero
          containers:
            - name: validate
              image: velero/velero:v1.13
              command: [sh, -c]
              args:
                - |
                  echo "=== 备份验证 $(date) ==="
                  
                  # 1. 检查最新备份状态
                  LATEST_BACKUP=$(velero backup get -o name | head -1)
                  STATUS=$(velero backup get -o json | jq -r '.items[0].status.phase')
                  
                  if [ "$STATUS" != "Completed" ]; then
                    echo "ERROR: 最新备份状态异常: $STATUS"
                    curl -X POST $ALERT_WEBHOOK -d '{"text":"备份验证失败"}'
                    exit 1
                  fi
                  
                  # 2. 测试恢复到临时命名空间
                  velero restore create validation-$(date +%s) \
                    --from-backup $LATEST_BACKUP \
                    --namespace-mappings production:validation-tmp \
                    --wait --timeout 10m
                  
                  # 3. 验证恢复的资源
                  POD_COUNT=$(kubectl get pods -n validation-tmp --no-headers | wc -l)
                  if [ "$POD_COUNT" -eq 0 ]; then
                    echo "ERROR: 恢复后无 Pod"
                    exit 1
                  fi
                  
                  # 4. 清理
                  kubectl delete ns validation-tmp --wait=false
                  
                  echo "✅ 备份验证成功"
          restartPolicy: OnFailure
```

### 备份完整性检查脚本

```bash
#!/bin/bash
# validate-backup-integrity.sh — 备份完整性检查
set -euo pipefail

BACKUP_NAME=${1:-$(velero backup get -o name | head -1)}

echo "=== 备份完整性检查: $BACKUP_NAME ==="

# 1. 检查备份元数据
velero backup describe $BACKUP_NAME --details

# 2. 检查备份日志
velero backup logs $BACKUP_NAME | tail -50

# 3. 检查卷快照
velero backup describe $BACKUP_NAME --details | grep -A20 "Volume snapshots"

# 4. 验证对象存储文件
BACKUP_BUCKET=$(velero backup-location get -o json | jq -r '.items[0].spec.objectStorage.bucket')
aws s3 ls s3://$BACKUP_BUCKET/backups/$BACKUP_NAME/ --recursive | wc -l

# 5. 检查备份大小
BACKUP_SIZE=$(velero backup describe $BACKUP_NAME -o json | jq -r '.status.volumeSnapshotsAttempted')
echo "卷快照数量: $BACKUP_SIZE"

echo "=== 检查完成 ==="
```

## 成本优化

### 备份存储成本分析

| 存储类型 | 成本 (每月) | 适用场景 |
|---------|------------|----------|
| S3 Standard | ~$0.023/GB | 频繁访问的备份 |
| S3 IA | ~$0.0125/GB | 30 天以上备份 |
| S3 Glacier | ~$0.004/GB | 90 天以上归档 |
| EBS 快照 | ~$0.05/GB | 卷快照 |

### 生命周期策略

```yaml
# S3 生命周期策略示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: backup-lifecycle-policy
  namespace: velero
data:
  lifecycle.json: |
    {
      "Rules": [
        {
          "ID": "BackupLifecycle",
          "Status": "Enabled",
          "Transitions": [
            {
              "Days": 30,
              "StorageClass": "STANDARD_IA"
            },
            {
              "Days": 90,
              "StorageClass": "GLACIER"
            }
          ],
          "Expiration": {
            "Days": 365
          }
        }
      ]
    }
```

### 备份去重与压缩

```bash
# 启用备份压缩
velero backup create compressed-backup \
  --include-namespaces production \
  --snapshot-volumes \
  --default-volumes-to-fs-backup

# 使用 Restic/Kopia 进行文件级备份（支持去重）
velero backup create fs-backup \
  --include-namespaces production \
  --default-volumes-to-fs-backup
```

## 多集群备份管理

### 集中式备份存储

```yaml
# 多集群共享备份存储
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: central-backup
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: central-k8s-backups
    prefix: cluster-prod-1
  config:
    region: ap-southeast-1
---
# 跨集群恢复
apiVersion: velero.io/v1
kind: Restore
metadata:
  name: cross-cluster-restore
  namespace: velero
spec:
  backupName: prod-1-backup-20260721
  includedNamespaces:
    - production
  namespaceMapping:
    production: production-restored
```

### 备份联邦管理

```bash
#!/bin/bash
# multi-cluster-backup.sh — 多集群备份管理
set -euo pipefail

CLUSTERS=("prod-1" "prod-2" "staging")

for cluster in "${CLUSTERS[@]}"; do
  echo "=== 备份集群: $cluster ==="
  
  # 切换到目标集群
  kubectl config use-context $cluster
  
  # 创建备份
  velero backup create ${cluster}-backup-$(date +%Y%m%d) \
    --include-namespaces production,database \
    --snapshot-volumes \
    --wait
  
  # 验证备份
  velero backup get | head -5
done

echo "=== 所有集群备份完成 ==="
```

## Related

- [[12-可靠性/02-灾难恢复/index.md|灾难恢复]]
- [[12-可靠性/04-混沌工程/index.md|混沌工程]]
- [[13-生产运维/03-事件响应/index.md|事件响应]]
