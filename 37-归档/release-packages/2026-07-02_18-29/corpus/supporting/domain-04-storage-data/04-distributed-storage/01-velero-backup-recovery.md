---
title: Velero 阿里云专有云备份恢复实战
description: 在阿里云与专有云 Kubernetes 上部署 Velero，完成命名空间级备份、ESSD 云盘 PV 快照、OSS 对象存储归档、定时备份策略、跨集群恢复与灾难恢复演练
category: storage
tags:
- k8s
- velero
- backup
- restore
- disaster-recovery
- alicloud
- apsara-stack
- oss
- snapshot
- csi
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 存储工程师
- 运维工程师
- 技术支持
estimated_read_time: 30min
intent_queries:
- Velero 阿里云备份恢复如何配置
- 专有云 K8s 如何使用 Velero 备份 PV
- Velero 定时备份与灾难恢复最佳实践
trigger_keywords:
- Velero
- 备份恢复
- 灾难恢复
- 命名空间备份
- PV 快照
- OSS 备份
prerequisites:
- kubectl-basics
- storage-basics
- oss-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-06-26"
updated: "2026-06-26"
summary: '4. [配置 BackupStorageLocation 与 VolumeSnapshotLocation](#配置-backupstoragelocation-与-volumesnapshotlocation)'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Velero 阿里云专有云备份恢复实战

> **适用版本**: Kubernetes v1.28 - v1.32 | **Velero**: v1.12+ | **阿里云插件**: v1.8+ | **最后更新**: 2026-06
> **文档定位**: 聚焦 Velero 在阿里云公有云 ACK 与专有云 Apsara Stack 环境下的安装、配置、备份、恢复与演练。所有云厂商内容以 **阿里云 / 专有云** 为主，AWS/GCP/Azure 差异仅在附录对照。

<!-- chunk: 目录 -->
## 目录

1. [架构与组件](#架构与组件)
2. [前置条件](#前置条件)
3. [安装 Velero](#安装-velero)
4. [配置 BackupStorageLocation 与 VolumeSnapshotLocation](#配置-backupstoragelocation-与-volumesnapshotlocation)
5. [备份命名空间与集群资源](#备份命名空间与集群资源)
6. [PV 快照：阿里云云盘 CSI](#pv-快照阿里云云盘-csi)
7. [恢复演练](#恢复演练)
8. [定时备份策略](#定时备份策略)
9. [灾难恢复](#灾难恢复)
10. [监控与告警](#监控与告警)
11. [故障排查](#故障排查)
12. [最佳实践检查清单](#最佳实践检查清单)

---

<!-- chunk: 1. 架构与组件 -->
## 1. 架构与组件

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         源集群 (ACK / 专有云 K8s)                        │
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │                        Velero Server                             │   │
│   │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │   │
│   │  │   Backup     │  │   Restore    │  │   Schedule   │           │   │
│   │  │  Controller  │  │  Controller  │  │  Controller  │           │   │
│   │  └──────┬───────┘  └──────────────┘  └──────────────┘           │   │
│   │         │                                                        │   │
│   │         ▼                                                        │   │
│   │  ┌──────────────────────────────────────────────────────────┐   │   │
│   │  │              BackupStorageLocation (BSL)                  │   │   │
│   │  │         阿里云 OSS / 专有云 OSS                           │   │   │
│   │  └──────────────────────────────────────────────────────────┘   │   │
│   │         │                                                        │   │
│   │         ▼                                                        │   │
│   │  ┌──────────────────────────────────────────────────────────┐   │   │
│   │  │            VolumeSnapshotLocation (VSL)                   │   │   │
│   │  │         阿里云云盘快照 / CSI VolumeSnapshot               │   │   │
│   │  └──────────────────────────────────────────────────────────┘   │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼  备份元数据 + 对象存储 + 快照
┌─────────────────────────────────────────────────────────────────────────┐
│                          目标集群 (恢复/灾备)                            │
│                     相同 BSL/VSL 即可拉取备份恢复                         │
└─────────────────────────────────────────────────────────────────────────┘
```

| 组件 | 作用 | 阿里云/专有云对应 |
|:---|:---|:---|
| Velero Server | 管理 Backup/Restore/Schedule CR | Deployment，部署在 `velero` namespace |
| Velero CLI | 本地操作备份恢复 | 与 Server 版本严格一致 |
| BSL | 存放 Kubernetes 资源 YAML、Pod 日志等元数据 | 阿里云 OSS Bucket / 专有云 OSS Bucket |
| VSL | 触发并追踪块存储快照 | 阿里云云盘快照 / CSI `VolumeSnapshotClass` |
| CSI Plugin | 创建/删除 VolumeSnapshot | `alicloud-disk-snapshot` |

---

<!-- chunk: 2. 前置条件 -->
## 2. 前置条件

- 集群已安装 CSI 插件并启用 `VolumeSnapshot` CRD（ACK 默认开启）
- 已创建 OSS Bucket，并配置 RAM/STS 访问凭证
- Velero Server 所在节点可访问 OSS Endpoint 与 ECS OpenAPI（公网或内网）
- 专有云环境需确认 ASO/OSS Endpoint 为天基内网地址
- 目标 Bucket 已开启版本控制或跨区域复制（生产建议）

### 2.1 确认 CSI 快照能力

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get crd volumesnapshotclasses.snapshot.storage.k8s.io
kubectl get crd volumesnapshots.snapshot.storage.k8s.io
kubectl get volumesnapshotclass
```
---

<!-- chunk: 3. 安装 Velero -->
## 3. 安装 Velero

### 3.1 下载 Velero CLI

```bash
VERSION="v1.13.2"
wget https://github.com/vmware-tanzu/velero/releases/download/${VERSION}/velero-${VERSION}-linux-amd64.tar.gz
tar -xzf velero-${VERSION}-linux-amd64.tar.gz
mv velero-${VERSION}-linux-amd64/velero /usr/local/bin/
velero version --client-only
```

### 3.2 创建最小权限 RAM 凭证

```bash
export ALIBABA_CLOUD_ACCESS_KEY_ID="LTAIxxxxxxxxxxxxxxxx"   # 替换为实际 AccessKey ID
export ALIBABA_CLOUD_ACCESS_KEY_SECRET="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"   # 替换为实际 AccessKey Secret

cat > credentials-velero <<EOF
ALIBABA_CLOUD_ACCESS_KEY_ID=${ALIBABA_CLOUD_ACCESS_KEY_ID}
ALIBABA_CLOUD_ACCESS_KEY_SECRET=${ALIBABA_CLOUD_ACCESS_KEY_SECRET}
EOF
chmod 600 credentials-velero
```

**RAM Policy 最小权限**：

```json
{
  "Version": "1",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "oss:PutObject",
        "oss:GetObject",
        "oss:DeleteObject",
        "oss:ListObjects",
        "oss:ListBuckets",
        "oss:GetBucketLocation",
        "ecs:CreateSnapshot",
        "ecs:DeleteSnapshot",
        "ecs:DescribeSnapshots",
        "ecs:DescribeSnapshotLinks",
        "ecs:DescribeDisks",
        "ecs:DescribeInstances"
      ],
      "Resource": "*"
    }
  ]
}
```

### 3.3 阿里云 ACK 安装

```bash
velero install \
  --provider alibabacloud \
  --plugins registry.cn-hangzhou.aliyuncs.com/acs/velero-plugin-alibabacloud:v1.8 \
  --bucket velero-backup-bucket \
  --prefix ack-prod-cluster \
  --secret-file ./credentials-velero \
  --backup-location-config region=cn-hangzhou,endpoint=oss-cn-hangzhou-internal.aliyuncs.com \
  --snapshot-location-config region=cn-hangzhou \
  --use-volume-snapshots=true \
  --use-node-agent \
  --features=EnableCSI
```

### 3.4 专有云 Apsara Stack 安装

```bash
# 请根据实际环境替换为专有云内部镜像仓库与 OSS endpoint
export APSARA_REGISTRY="registry.apsara-stack.example"
export APSARA_REGION="hangzhou"
export APSARA_OSS_ENDPOINT="oss.apsara-stack.example"

velero install \
  --provider alibabacloud \
  --plugins ${APSARA_REGISTRY}/acs/velero-plugin-alibabacloud:v1.8 \
  --bucket velero-backup-bucket \
  --prefix apsara-prod-cluster \
  --secret-file ./credentials-velero \
  --backup-location-config region=${APSARA_REGION},endpoint=${APSARA_OSS_ENDPOINT} \
  --snapshot-location-config region=${APSARA_REGION} \
  --use-volume-snapshots=true \
  --use-node-agent \
  --features=EnableCSI
```

### 3.5 安装验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n velero
velero backup-location get
velero snapshot-location get
velero version
```
---

<!-- chunk: 4. BSL 与 VSL 配置 -->
## 4. 配置 BackupStorageLocation 与 VolumeSnapshotLocation

### 4.1 BackupStorageLocation

```yaml
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: alibabacloud
  objectStorage:
    bucket: velero-backup-bucket
    prefix: ack-prod-cluster
  config:
    region: cn-hangzhou
    endpoint: oss-cn-hangzhou-internal.aliyuncs.com
```

### 4.2 VolumeSnapshotLocation

```yaml
apiVersion: velero.io/v1
kind: VolumeSnapshotLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: alibabacloud
  config:
    region: cn-hangzhou
```

---

<!-- chunk: 5. 备份命名空间与集群资源 -->
## 5. 备份命名空间与集群资源

### 5.1 备份单个命名空间

```bash
velero backup create ns-prod-$(date +%Y%m%d-%H%M) \
  --include-namespaces production \
  --default-volumes-to-fs-backup \
  --wait
```

### 5.2 备份多个命名空间并排除临时资源

```bash
velero backup create daily-critical-$(date +%Y%m%d-%H%M) \
  --include-namespaces production,staging \
  --exclude-resources events,events.events.k8s.io,pods \
  --default-volumes-to-fs-backup \
  --ttl 720h \
  --storage-location default \
  --volume-snapshot-locations default
```

### 5.3 基于标签备份

```bash
velero backup create app-payment-$(date +%Y%m%d-%H%M) \
  --selector "app=payment,tier=database" \
  --include-namespaces production \
  --default-volumes-to-fs-backup
```

### 5.4 备份 Hook（以 MySQL 为例）

```yaml
apiVersion: velero.io/v1
kind: Backup
metadata:
  name: mysql-backup-with-hooks
  namespace: velero
spec:
  includedNamespaces:
    - production
  labelSelector:
    matchLabels:
      app: mysql
  hooks:
    resources:
      - name: mysql-backup-hook
        includedNamespaces:
          - production
        labelSelector:
          matchLabels:
            app: mysql
        pre:
          - exec:
              container: mysql
              command:
                - /bin/bash
                - -c
                - "mysql -u root -p$MYSQL_ROOT_PASSWORD -e 'FLUSH TABLES WITH READ LOCK; FLUSH LOGS;'"
              onError: Fail
              timeout: 60s
        post:
          - exec:
              container: mysql
              command:
                - /bin/bash
                - -c
                - "mysql -u root -p$MYSQL_ROOT_PASSWORD -e 'UNLOCK TABLES;'"
              onError: Continue
              timeout: 30s
```

### 5.5 验证备份

```bash
velero backup get
velero backup describe daily-critical-20260626-0200 --details
velero backup logs daily-critical-20260626-0200
```

---

<!-- chunk: 6. PV 快照 -->
## 6. PV 快照：阿里云云盘 CSI

### 6.1 确认 VolumeSnapshotClass

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get volumesnapshotclass
# 预期输出包含 alicloud-disk-snapshot
```
### 6.2 备份时使用 CSI 快照

```bash
velero backup create pv-snapshot-backup-$(date +%Y%m%d-%H%M) \
  --include-namespaces production \
  --snapshot-volumes \
  --volume-snapshot-locations default \
  --csi-snapshot-timeout 30m
```

### 6.3 使用 fs-backup 替代快照

```bash
velero backup create fs-backup-$(date +%Y%m%d-%H%M) \
  --include-namespaces production \
  --default-volumes-to-fs-backup
```

| 方式 | 适用场景 | RPO | 恢复速度 | 阿里云对应 |
|:---|:---|:---:|:---:|:---|
| CSI 快照 | ESSD 云盘、数据量大 | 低 | 快 | 云盘快照 |
| fs-backup | NAS/OSS/本地盘、跨地域 | 中 | 慢 | restic/kopia 上传 OSS |
| 混合模式 | 关键数据库 + 配置文件 | 低 | 中 | 快照 + fs-backup |

---

<!-- chunk: 7. 恢复演练 -->
## 7. 恢复演练

### 7.1 查看可用备份

```bash
velero backup get
velero backup describe daily-full-backup-20260626 --details
```

### 7.2 完整恢复到原集群

```bash
velero restore create restore-prod-$(date +%Y%m%d-%H%M) \
  --from-backup daily-full-backup-20260626 \
  --wait
```

### 7.3 选择性恢复

```bash
# 仅恢复指定命名空间
velero restore create restore-ns-prod \
  --from-backup daily-full-backup-20260626 \
  --include-namespaces production

# 仅恢复指定资源类型
velero restore create restore-workloads \
  --from-backup daily-full-backup-20260626 \
  --include-resources deployments.apps,services \
  --include-namespaces production

# 恢复到新命名空间（演练常用）
velero restore create restore-drill \
  --from-backup daily-full-backup-20260626 \
  --namespace-mappings production:production-drill \
  --include-namespaces production

# 排除 PVC（仅恢复配置）
velero restore create restore-config-only \
  --from-backup daily-full-backup-20260626 \
  --exclude-resources persistentvolumeclaims
```

### 7.4 恢复后验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
velero restore describe restore-prod-20260626-1432
velero restore logs restore-prod-20260626-1432

kubectl get pods -n production
kubectl get pvc -n production
kubectl get pv | grep production
```
---

<!-- chunk: 8. 定时备份策略 -->
## 8. 定时备份策略

### 8.1 每日全量 + 每小时关键资源

```yaml
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-full-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"
  template:
    includedNamespaces:
      - production
      - staging
    excludedResources:
      - events
      - events.events.k8s.io
    storageLocation: default
    volumeSnapshotLocations:
      - default
    ttl: 720h
    snapshotVolumes: true
    defaultVolumesToFsBackup: false
---
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: hourly-critical-backup
  namespace: velero
spec:
  schedule: "0 * * * *"
  template:
    includedNamespaces:
      - production
    labelSelector:
      matchLabels:
        backup: critical
    storageLocation: default
    ttl: 168h
    snapshotVolumes: true
```

### 8.2 备份保留策略对照表

| 备份类型 | 频率 | TTL | 快照策略 | 适用对象 |
|:---|:---|:---:|:---|:---|
| 全量备份 | 每日 02:00 | 30 天 | 启用快照 | 生产全量 |
| 关键备份 | 每小时 | 7 天 | 启用快照 | 标签 `backup=critical` |
| 配置备份 | 每 6 小时 | 14 天 | 不快照 | ConfigMap/Secret/Deployment |
| 演练副本 | 手动 | 3 天 | 按需 | 恢复演练 |

---

<!-- chunk: 9. 灾难恢复 -->
## 9. 灾难恢复

### 9.1 跨集群恢复流程

```
源集群异常
    │
    ▼
准备目标集群（ACK 新集群 / 专有云灾备集群）
    │
    ▼
目标集群安装 Velero，使用相同 BSL/VSL
    │
    ▼
velero backup get 同步备份列表
    │
    ▼
velero restore create --from-backup daily-full-backup-20260626
    │
    ▼
验证应用 + 存储 + 网络入口
    │
    ▼
切换 DNS / SLB / 专线流量
```

### 9.2 目标集群安装

```bash
velero install \
  --provider alibabacloud \
  --plugins registry.cn-hangzhou.aliyuncs.com/acs/velero-plugin-alibabacloud:v1.8 \
  --bucket velero-backup-bucket \
  --prefix ack-prod-cluster \
  --secret-file ./credentials-velero \
  --backup-location-config region=cn-hangzhou,endpoint=oss-cn-hangzhou-internal.aliyuncs.com \
  --snapshot-location-config region=cn-hangzhou \
  --use-volume-snapshots=true \
  --features=EnableCSI
```

### 9.3 执行跨集群恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
velero backup get

velero restore create dr-restore-$(date +%Y%m%d-%H%M) \
  --from-backup daily-full-backup-20260626-0200 \
  --include-namespaces production \
  --wait

kubectl get pods -n production
kubectl get pvc -n production
kubectl get svc -n production
```
### 9.4 专有云灾备特别注意事项

| 场景 | 风险 | 应对 |
|:---|:---|:---|
| 天基内网 OSS 不可达 | 恢复中断 | 使用 ASO 确认 OSS 服务状态，必要时切换公网 Endpoint |
| 云盘快照跨区域不可用 | PV 恢复失败 | 灾备集群与源集群在同一 Region，或使用 OSS 复制 |
| API Server 证书过期 | Velero 无法连接 | 先轮转证书，再执行恢复 |
| RAM 权限不足 | 快照创建失败 | 按第 3.2 节 RAM Policy 复核权限 |

---

<!-- chunk: 10. 监控与告警 -->
## 10. 监控与告警

```yaml
groups:
  - name: velero-alerts
    rules:
      - alert: VeleroBackupFailed
        expr: |
          increase(velero_backup_failure_total[1h]) > 0
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Velero 备份失败"
          description: "备份 {{ $labels.backup }} 在最近 1 小时内失败。"

      - alert: VeleroBackupMissing
        expr: |
          time() - velero_backup_last_successful_timestamp > 86400
        for: 1h
        labels:
          severity: warning
        annotations:
          summary: "Velero 备份超过 24 小时未成功"

      - alert: VeleroRestoreFailed
        expr: |
          increase(velero_restore_failure_total[1h]) > 0
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Velero 恢复失败"
```

---

<!-- chunk: 11. 故障排查 -->
## 11. 故障排查

| 现象 | 根因 | 修复命令 |
|:---|:---|:---|
| BSL PHASE=Unavailable | OSS Endpoint 或凭证错误 | `velero backup-location set --config endpoint=...` |
| 备份 Completed 但无快照 | VSL 未配置或 CSI 不支持 | 检查 `VolumeSnapshotClass` 与 `--snapshot-volumes` |
| 恢复快照 Pending | 云盘快照未就绪 | `kubectl get volumesnapshot -A` |
| OSS 上传慢 | 跨公网或 Bucket 地域不一致 | 使用内网 Endpoint，确保 Bucket 与集群同 Region |
| PodVolumeBackup 失败 | 节点 Agent 未运行 | `kubectl get pods -n velero -l name=node-agent` |

---

<!-- chunk: 12. 最佳实践检查清单 -->
## 12. 最佳实践检查清单

| 检查项 | 要求 | 验证命令 |
|:---|:---|:---|
| Velero Server 运行正常 | Pod 全部 Ready | `kubectl get pods -n velero` |
| BSL 可用 | PHASE 为 Available | `velero backup-location get` |
| VSL 可用 | PHASE 为 Available | `velero snapshot-location get` |
| 定时备份存在 | 至少 1 条 Schedule | `velero schedule get` |
| 最近一次备份成功 | PHASE 为 Completed | `velero backup get` |
| 快照可创建 | CSI VolumeSnapshotClass 存在 | `kubectl get volumesnapshotclass` |
| 恢复演练月度执行 | 演练记录归档 | 查看运维日志 |
| 备份保留策略执行 | 过期备份自动清理 | `velero backup get` 无过期项 |
| 凭证轮换 | AccessKey 按周期轮换 | RAM 控制台检查 |
| 跨集群恢复验证 | 灾备集群可拉取备份 | 季度演练 |

---

## 11. 备份合规与审计

### 11.1 阿里云 RAM 最小权限审计

定期使用阿里云 RAM 的权限分析功能，确认 Velero 使用的 AccessKey 或 RAM Role 未授予超出必要的权限：

```bash
aliyun ram GetPolicy --PolicyName VeleroBackupPolicy
aliyun ram ListUsersForGroup --GroupName velero-backup-operators
```

### 11.2 备份操作日志

Velero 所有 backup/restore 操作均会记录到 OSS 元数据中，建议同时启用 Kubernetes Audit Log，将 `velero.io` 相关 API 调用记录到 SLS：

```yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  - level: RequestResponse
    verbs: ["create", "delete", "patch"]
    resources:
      - group: "velero.io"
        resources: ["backups", "restores", "schedules"]
```

### 11.3 专有云变更单集成

在专有云环境中，任何可能影响数据保护的变更（如升级 Velero、修改 OSS Bucket 权限、调整备份策略）都应关联 ASO 变更单：

```bash
# 在 ASO 控制台提交变更单，填写：
# - 变更内容：Velero 版本升级 v1.14 → v1.15
# - 影响范围：备份系统
# - 回滚方案：使用旧版本 CLI 恢复最后一次成功备份
# - 验证项：执行一次测试备份与恢复
```

### 11.4 备份成本优化

阿里云 OSS 费用包括存储费、请求费与出站流量费。优化建议：

- 将 7 天内的备份保留在标准存储，超过 7 天的自动转低频访问；
- 对非关键命名空间关闭 PV 快照，仅备份 Kubernetes 资源 YAML；
- 合并多个小时级增量备份为每日全量，减少请求次数；
- 跨 Region 恢复时优先使用 OSS 传输加速内网 endpoint，避免公网流量费。

```bash
# 配置 OSS 生命周期规则示例
aliyun oss lifecycle --method put oss://${BUCKET} lifecycle.xml
```

---

## Related

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-04-storage-data/01-k8s-storage/03-storage-backup-disaster-recovery|10 - 存储备份与灾难恢复]]
- [[domain-04-storage-data/README|Storage Domain 存储领域知识库]]
- [[domain-12-cloud-providers/阿里云/apsara-stack-components|专有云组件索引]]

## See Also

- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-04-storage-data/04-distributed-storage/02-rook-ceph-production|Rook-Ceph 生产指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-04-storage-data/04-distributed-storage/03-longhorn-production|Longhorn 生产指南]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-04-storage-data/05-stateful-app-storage/01-stateful-app-storage-patterns|有状态应用存储模式]]


<!-- risk-assessed -->
