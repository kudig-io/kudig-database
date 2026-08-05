---
title: 存储数据保护与灾难恢复
summary: 存储数据保护与灾难恢复：数据保护是 Kubernetes 存储体系中最后一道防线。本文涵盖 Velero 最佳实践、不可变备份、勒索软件防护、多层灾难恢复策略以及
  RTO/RPO 目标设定。
category: concepts
tags:
- storage
- backup
- disaster-recovery
- velero
- k8s
tier: core
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 存储数据保护与灾难恢复

## 概述

数据保护是 Kubernetes 存储体系中最后一道防线。本文涵盖 Velero 最佳实践、不可变备份、勒索软件防护、多层灾难恢复策略以及 RTO/RPO 目标设定。

相关：[[concepts/csi-drivers.md|csi drivers]] | [[concepts/storage-tool-evolution.md|storage tool evolution]] | [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|index]]

---

## 1. Velero 1.14–1.15 最佳实践

### 1.1 CSI 快照优先

Velero 1.14+ 默认使用 CSI 快照而非 file-system backup，速度更快且对应用零干扰。

```yaml
apiVersion: velero.io/v1
kind: Backup
metadata:
  name: daily-csi-backup
  namespace: velero
spec:
  includedNamespaces: ["production"]
  snapshotMoveData: true              # CSI 快照 + 数据移动
  defaultVolumesToFsBackup: false     # 明确禁用 fs-backup 回退
  storageLocation: default
  ttl: 720h                           # 保留 30 天
```

### 1.2 定时备份

```yaml
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: production-hourly
  namespace: velero
spec:
  schedule: "0 * * * *"              # 每小时
  template:
    includedNamespaces: ["production"]
    snapshotMoveData: true
    storageLocation: default
    ttl: 336h                         # 14 天保留
  useOwnerReferencesInBackup: false   # 避免级联删除
---
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: production-daily
  namespace: velero
spec:
  schedule: "0 2 * * *"              # 每天凌晨 2 点
  template:
    includedNamespaces: ["production", "staging"]
    snapshotMoveData: true
    storageLocation: default-remote
    ttl: 2160h                        # 90 天保留
```

### 1.3 Backup Storage Location (BSL) 配置

```yaml
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: k8s-backups-primary
    prefix: velero
  config:
    region: us-east-1
    profile: velero-backup
  accessMode: ReadWrite
  default: true
---
# 远程 BSL（异地冗余）
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: default-remote
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: k8s-backups-dr
    prefix: velero
  config:
    region: us-west-2
    profile: velero-backup-dr
  accessMode: ReadWrite
```

---

## 2. 不可变备份

### 2.1 S3 Object Lock（WORM 模式）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建启用 Object Lock 的 S3 Bucket
aws s3api create-bucket \
  --bucket k8s-backups-immutable \
  --object-lock-enabled-for-bucket

# 配置默认保留策略（Governance 模式，30 天）
aws s3api put-object-lock-configuration \
  --bucket k8s-backups-immutable \
  --object-lock-configuration '{
    "ObjectLockEnabled": true,
    "Rule": {
      "DefaultRetention": {
        "Mode": "COMPLIANCE",
        "Days": 30
      }
    }
  }'
```
**保留模式说明**：

| 模式 | 特性 | 适用场景 |
|-----|------|---------|
| Governance | 特权用户可覆盖 | 开发/测试环境 |
| Compliance | 任何人（含 root）不可删除 | 合规生产环境 |

### 2.2 跨账户备份

```json
// S3 Bucket Policy：允许跨账户访问
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "CrossAccountBackupWrite",
      "Effect": "Allow",
      "Principal": { "AWS": "arn:aws:iam::BACKUP_ACCOUNT:root" },
      "Action": ["s3:PutObject", "s3:GetObject"],
      "Resource": "arn:aws:s3:::k8s-backups-immutable/*"
    }
  ]
}
```

### 2.3 MFA Delete

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用 MFA Delete（需要版本控制）
aws s3api put-bucket-versioning \
  --bucket k8s-backups-immutable \
  --versioning-configuration \
    Status=Enabled,MFADelete=Enabled \
  --mfa "arn:aws:iam::ACCOUNT:mfa/root-account-mfa-device 123456"
```
---

## 3. 勒索软件防护

### 3.1 备份大小异常检测

```python
# 备份大小异常检测脚本（CronJob 部署）
import boto3
from datetime import datetime, timedelta

def check_backup_anomaly(bucket, prefix, threshold_ratio=0.5):
    """检测备份大小突变（骤降 > 50% 可能表示被加密/破坏）"""
    s3 = boto3.client('s3')
    backups = []
    for obj in s3.list_objects_v2(Bucket=bucket, Prefix=prefix)['Contents']:
        backups.append((obj['Key'], obj['Size']))

    if len(backups) < 2:
        return

    latest = backups[-1][1]
    avg_size = sum(s[1] for s in backups[-7:]) / min(7, len(backups))

    if latest < avg_size * threshold_ratio:
        alert(f"⚠️ 备份大小异常：最新 {latest} vs 平均 {avg_size}")
```

### 3.2 校验和验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Velero 备份完成后校验
velero backup describe daily-csi-backup --details

# 使用 sha256 校验备份文件完整性
aws s3 cp s3://k8s-backups-immutable/velero/backups/daily-csi-backup/ - \
  | sha256sum > backup-checksum.txt

# 定期恢复验证（见下方 3.3）
```
### 3.3 隔离恢复命名空间

在专用命名空间中执行恢复验证，避免影响生产：

```yaml
apiVersion: velero.io/v1
kind: Restore
metadata:
  name: verify-backup-20260524
  namespace: velero
spec:
  backupName: daily-csi-backup
  includedNamespaces: ["production"]
  namespaceMapping:
    production: restore-verification    # 恢复到隔离命名空间
  restorePVs: true
  hooks:
    resources:
    - name: verify-db
      includedNamespaces: ["restore-verification"]
      postHooks:
      - exec:
          command: ["/bin/sh", "-c", "pg_isready -h localhost"]
          container: postgres
          onError: Continue
          timeout: 60s
```

**自动化恢复验证 CronJob**：

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: backup-restore-verify
  namespace: velero
spec:
  schedule: "0 4 * * 0"           # 每周日凌晨 4 点
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: velero-restore-verifier
          containers:
          - name: verifier
            image: velero/velero:v1.15.0
            command:
            - /bin/sh
            - -c
            - |
              velero restore create verify-$(date +%Y%m%d) \
                --from-backup $(velero backup get -o name | head -1) \
                --namespace-mappings production:restore-verify \
                --wait
              # 验证完成后清理
              velero restore delete verify-$(date +%Y%m%d) --confirm
          restartPolicy: OnFailure
```

---

## 4. 多层 DR 策略

### 4.1 四层防护架构

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────┐
│                   应用状态层                          │
│  数据库 WAL 归档 / 应用级复制 (如 PostgreSQL 流复制)    │
├─────────────────────────────────────────────────────┤
│                   PV 快照层                           │
│  CSI VolumeSnapshot → 异地快照复制                    │
├─────────────────────────────────────────────────────┤
│                   GitOps 层                           │
│  ArgoCD / Flux → Git 仓库 (基础设施即代码)             │
├─────────────────────────────────────────────────────┤
│                   etcd 备份层                         │
│  etcdctl snapshot / Velero CRD 备份                   │
└─────────────────────────────────────────────────────┘
```
### 4.2 etcd 备份

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# etcd 快照（CronJob）
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d-%H%M).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```
### 4.3 GitOps 层

所有 Kubernetes 清单（包括 StorageClass、PVC 定义、Velero Schedules）均应版本化存储在 Git 中，确保集群可从零重建。

---

## 5. RTO/RPO 目标

| 优先级 | RTO（恢复时间目标） | RPO（恢复点目标） | 策略 |
|-------|-------------------|------------------|------|
| Critical（P0） | < 1 小时 | < 15 分钟 | CSI 快照 + 同步复制 + 热备集群 |
| Standard（P1） | < 4 小时 | < 6 小时 | 定时 Velero 备份 + 跨区域快照 |
| Best Effort（P2） | < 24 小时 | < 24 小时 | 每日备份 + GitOps 重建 |

### 分层策略详解

**Critical（数据库、支付系统）**：
- 同步/半同步数据库复制
- CSI 快照每 15 分钟
- Velero 备份每小时
- 预配置的热备集群（ArggoCD 自动同步）

**Standard（业务应用、API 服务）**：
- Velero CSI 快照每 6 小时
- 跨区域快照复制
- GitOps 定义的完整环境，4 小时内可重建

**Best Effort（内部工具、开发环境）**：
- 每日 Velero 备份
- Git 仓库为唯一真实来源
- 24 小时内从 GitOps 重建

---

## 6. 多集群 DR 工具

### 6.1 Velero（多集群）

```bash
# 集群 A 备份，集群 B 恢复（共享 BSL）
# 集群 A
velero backup create cross-cluster-backup --snapshot-volumes

# 集群 B（连接同一 BSL）
velero restore create --from-backup cross-cluster-backup
```

### 6.2 Kasten K10

Kasten K10 提供企业级数据管理：

```yaml
apiVersion: config.kio.kasten.io/v1alpha1
kind: Policy
metadata:
  name: multi-cluster-dr
spec:
  actions:
  - action: backup
    backupParameters:
      filters:
        includeResources:
        - matchExpressions:
          - key: kasten.io/app
            operator: In
            values: ["critical"]
  schedules:
  - schedule: "0 */4 * * *"
  selector:
    matchExpressions:
    - key: kasten.io/cluster
      operator: In
      values: ["production-us-east", "production-eu-west"]
```

### 6.3 ArgoCD ApplicationSets

使用 ApplicationSet 实现多集群应用同步恢复：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: production-dr
spec:
  generators:
  - clusters:
      selector:
        matchLabels:
          env: dr-target
  template:
    metadata:
      name: 'production-{{name}}'
    spec:
      project: production
      source:
        repoURL: https://github.com/org/k8s-manifests
        targetRevision: main
        path: production
      destination:
        server: '{{server}}'
        namespace: production
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
```

---

## 7. 3-2-1 备份规则

> **3** 份数据副本，存储在 **2** 种不同介质上，其中 **1** 份在异地。

### Kubernetes 实现

```
副本 1: 生产 PV 数据（本地/云 SSD）
副本 2: Velero 备份 → 同区域对象存储（S3/GCS/ABS）
副本 3: 跨区域复制 → 异地对象存储 + Object Lock
```

**增强版 3-2-1-1-0**：
- **3** 份副本
- **2** 种介质
- **1** 份异地
- **1** 份离线/不可变（Object Lock）
- **0** 个未经验证的备份（定期恢复测试）

### 验证清单

- [ ] Velero 每日备份正常完成
- [ ] CSI 快照跨区域复制已配置
- [ ] Object Lock / WORM 策略已启用
- [ ] 跨账户备份隔离已实现
- [ ] 每周自动恢复验证通过
- [ ] 备份大小异常检测告警正常
- [ ] RTO/RPO 目标经演练验证
- [ ] GitOps 仓库包含完整环境定义
- [ ] DR Runbook 已编写并定期更新

---

## 参考资料

- [Velero 官方文档](https://velero.io/docs/)
- [Kasten K10 文档](https://docs.kasten.io/)
- [S3 Object Lock](https://docs.aws.amazon.com/AmazonS3/latest/userguide/object-lock.html)
- [KEP-3205: VolumeGroupSnapshot](https://github.com/kubernetes/enhancements/tree/master/keps/sig-storage/3205-volume-group-snapshot)

## Related

- [[concepts/csi-drivers.md|csi drivers]] — CSI 驱动规范与实现
- [[concepts/multi-cluster-dr-automation.md|multi cluster dr automation]] — 多集群灾备与自动化
- [[concepts/chaos-engineering-platforms.md|chaos engineering platforms]] — 混沌工程平台


<!-- risk-assessed -->
