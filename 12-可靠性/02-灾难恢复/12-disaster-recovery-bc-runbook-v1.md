---
title: Kubernetes 灾难恢复与业务连续性 Runbook
description: 覆盖 RTO/RPO 定义、etcd quorum loss 恢复、Velero 全集群恢复、AZ/Region 故障切换、DR 演练节奏与业务连续性模板的权威生产级手册
summary: 覆盖 RTO/RPO 定义、etcd quorum loss 恢复、Velero 全集群恢复、AZ/Region 故障切换、DR 演练节奏与业务连续性模板的权威生产级手册
category: reliability-engineering
tags:
- production
- best-practices
- playbook
- disaster-recovery
- business-continuity
- reliability-engineering
- sre
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 灾难恢复与业务连续性 Runbook 是什么
- 如何制定 K8s DR/BC 方案
- etcd quorum loss 如何恢复
- Velero 全集群恢复步骤
- AZ/Region 故障如何切换
trigger_keywords:
- 灾难恢复
- 业务连续性
- DR
- BC
- RTO
- RPO
- etcd quorum
- Velero restore
- AZ failover
prerequisites:
- kubectl-basics
- etcd-basics
- velero-basics
- sre-practices
- cloud-cli-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes 灾难恢复与业务连续性 Runbook

> **核心原则**：DR 不是备份清单，而是可重复、可验证、有人负责的恢复工程。任何未在 30 天内演练过的恢复流程都不应被视为可靠。

## 1. 适用场景与范围

本 Runbook 适用于以下生产灾难场景：

- **etcd 仲裁（quorum）丢失**：控制面 etcd 集群因网络分区、磁盘故障或成员宕机导致无法选出 leader，API Server 不可用。
- **控制面全损**：API Server / etcd / scheduler 全部不可恢复，需要从 snapshot 重建控制面。
- **集群级资源误删或损坏**：Namespace、CRD、RBAC、ConfigMap、Secret 等被批量删除或篡改。
- **可用区（AZ）级故障**：单个 AZ 的节点、负载均衡、存储同时不可用。
- **区域（Region）级灾难**：整个地理区域的基础设施中断，需要切换到备用区域。
- **业务连续性（BC）启动**：灾难超出技术恢复范围，需要按业务影响进行决策、沟通与降级服务。

**范围边界**：

- 本 Runbook 给出统一决策流程、RTO/RPO 框架、etcd 与 Velero 恢复入口、AZ/Region 切换动作和演练模板。
- 具体的 etcd 数据损坏恢复、控制面全损重建、单 AZ 切换、应用级数据库 DR 分别参见下方“相关 Runbook”。
- 云厂商具体命令（如 AWS ALB、阿里云 SLB、GKE Multi-Cluster Ingress）以各云 Runbook 为准。

## 2. 前置条件与工具

### 2.1 信息清单（必须提前维护）

| 项目 | 示例 | 维护位置 |
|------|------|----------|
| etcd 端点与成员 | `https://10.0.0.1:2379,https://10.0.0.2:2379,https://10.0.0.3:2379` | `_meta/journal/etcd-inventory.md` |
| Velero BackupStorageLocation | `s3://k8s-dr-<region>/velero` | `velero backup-location get` |
| VolumeSnapshotClass | `csi-snapclass-fast` | `kubectl get volumesnapshotclass` |
| 备用集群 kubeconfig | `dr-cluster/kubeconfig` | 安全密码库 |
| 全局负载均衡 / DNS | `app.example.com CNAME gslb.example.com` | DNS 控制台 |
| 关键命名空间列表 | `production, monitoring, ingress-nginx` | GitOps 仓库 |

### 2.2 工具与权限

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 客户端工具
kubectl version --client
etcdctl version
velero version
aws --version   # 或 aliyun / gcloud / az

# 权限要求：灾难恢复 Namespace 必须 cluster-admin
kubectl auth can-i '*' '*' --all-namespaces
```
### 2.3 备份就绪检查（每日自动巡检）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# etcd 备份状态
ls -lh /backup/etcd/
etcdctl snapshot status /backup/etcd/latest.db --write-out=table

# Velero 定时备份
velero backup get | grep -v Completed
velero schedule get

# 备份异地复制检查（以 S3 跨区域复制为例）
aws s3 ls s3://k8s-dr-secondary/velero/backups/
```
### 2.4 备份架构与复制策略

生产环境的备份必须满足“3-2-1”原则：至少 3 份数据、使用 2 种不同介质、其中 1 份位于异地。针对 Kubernetes 建议：

- **etcd 快照**：每小时通过 CronJob 或 systemd timer 生成 snapshot，本地保留 72 小时，并通过对象存储生命周期策略同步到异地 bucket，保留 30 天。
- **Velero 资源与卷备份**：BackupStorageLocation 指向主区域对象存储，同时启用跨区域复制（S3 CRR / OSS 跨区域复制 / GCS Dual-Region）到 DR 区域。
- **不可变备份**：对关键备份启用对象锁定（S3 Object Lock / OSS WORM），防止勒索软件或误操作删除备份。
- **备份加密**：使用 KMS 服务端加密，Velero `BackupStorageLocation` 中指定 `serverSideEncryption: aws:kms` 与 `kmsKeyId`。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查备份是否启用跨区域复制完成
aws s3api head-object --bucket k8s-dr-secondary --key velero/backups/prod-daily-20260701-0200/prod-daily-20260701-0200.tar.gz
# 预期返回 ReplicateStatus 为 COMPLETED
```
### 2.5 关键告警规则

以下 PrometheusRule 用于在备份失败或证书即将过期时第一时间通知值班人员：

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dr-critical-alerts
  namespace: monitoring
spec:
  groups:
  - name: dr
    rules:
    - alert: VeleroBackupFailed
      expr: velero_backup_failure_total{schedule!=""} > 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Velero 定时备份失败"
        description: "备份 {{ $labels.schedule }} 最近 5 分钟失败次数 > 0"
    - alert: EtcdBackupStale
      expr: time() - etcd_backup_last_success_timestamp_seconds > 7200
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "etcd 备份超过 2 小时未成功"
        description: "请检查 etcd snapshot CronJob 或对象存储复制链路"
    - alert: KubeadmCertExpiryWarning
      expr: kubeadm_cert_expiry_seconds < 2592000
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "K8s 内部证书有效期小于 30 天"
```

> `etcd_backup_last_success_timestamp_seconds` 需通过自定义 exporter 或 Pushgateway 上报；也可用 Velero 原生指标替代。

## 3. 标准操作流程

### Phase 0：事件定级与 RTO/RPO 确认

收到灾难告警后，首先按以下矩阵定级，并确认本次事件的 RTO/RPO 目标：

| 等级 | 触发条件 | RTO 目标 | RPO 目标 | 决策人 |
|------|----------|---------|---------|--------|
| P0 | 全集群不可用 / Region 级灾难 | ≤ 30 min | 0（etcd）/ ≤ 5 min（应用数据） | 值班主任 + 技术 VP |
| P1 | 控制面不可用 / 多 AZ 影响 | ≤ 15 min | 0 | SRE 负责人 |
| P2 | 单 AZ 故障 / 核心命名空间丢失 | ≤ 30 min | ≤ 5 min | 值班 SRE |
| P3 | 非核心命名空间可恢复性故障 | ≤ 4 h | ≤ 1 h | 值班 SRE |

> **RTO（Recovery Time Objective）**：从灾难发生到业务恢复可用允许的最长时间。
> **RPO（Recovery Point Objective）**：灾难发生后允许丢失的数据时间窗口。etcd 通常要求 RPO=0，应用数据依赖 Velero 备份频率。

RTO/RPO 不应由技术团队单方面设定，而应由业务方根据每分钟的收入损失、合规要求与客户体验影响共同确定。技术团队负责将业务目标转化为备份频率、冗余容量与自动化恢复能力。

### Phase 1：信息采集与影响评估（0–5 min）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 API Server 可用性
kubectl get --raw /healthz --request-timeout=5s

# 2. 检查 etcd 成员健康
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379,https://10.0.0.3:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health --write-out=table

# 3. 检查节点可用区分布
kubectl get nodes -L topology.kubernetes.io/zone,node.kubernetes.io/instance-type

# 4. 检查 Pod 异常分布
kubectl get pods -A -o wide | grep -v Running | grep -v Completed | head -n 50

# 5. 检查 Velero 备份可用性
velero backup get --output json | jq '.items[] | {name: .metadata.name, status: .status.phase, age: .metadata.creationTimestamp}'
```
### Phase 2：etcd Quorum 丢失恢复

如果 `endpoint health` 显示超过半数成员 unhealthy，进入 quorum 恢复流程。

#### 2.1 可保留多数成员数据时（推荐）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认 healthy 成员
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member list --write-out=table

# 移除异常成员
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member remove <bad-member-id>

# 以 Learner 模式重新加入新成员
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member add etcd-3-new --learner=true --peer-urls=https://10.0.0.3:2380

# 在新节点上启动 etcd（ETCD_INITIAL_CLUSTER_STATE=existing）
# 同步完成后 promote 为正式成员
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member promote <learner-id>
```
#### 2.2 所有成员数据不一致或损坏时

如果无法确定哪个成员数据正确，或所有成员都损坏，使用最近 snapshot 重建整个 etcd 集群：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 停止所有 etcd 成员
systemctl stop etcd  # 在所有控制面节点执行

# 逐个节点从 snapshot 恢复（注意：每个节点需使用自己的 advertise URL）
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd/latest.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3=https://10.0.0.3:2380 \
  --initial-advertise-peer-urls=https://10.0.0.1:2380 \
  --initial-cluster-token=etcd-cluster-k8s-dr

chown -R etcd:etcd /var/lib/etcd
systemctl start etcd
```
> 详细步骤与数据校验参见 [[12-可靠性/02-灾难恢复/13-etcd-corruption-recovery-playbook.md|etcd 数据损坏检测与恢复全流程]]。

### Phase 3：Velero 全集群恢复

当 etcd 恢复后仍缺少 Kubernetes 资源，或灾难为“集群资源被误删/新集群重建”时，使用 Velero 恢复。

#### 3.1 恢复前准备

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认备份清单
velero backup describe prod-daily-20260701-0200 --details

# 确认 BackupStorageLocation 可访问
velero backup-location get

# 若恢复到新集群，需安装同版本 Velero 并配置相同 BSL
velero install \
  --provider aws \
  --plugins velero/velero-plugin-for-aws:v1.11.0 \
  --bucket k8s-dr-secondary \
  --prefix velero \
  --backup-location-config region=ap-southeast-1,s3ForcePathStyle="true",s3Url=https://s3.ap-southeast-1.amazonaws.com \
  --snapshot-location-config region=ap-southeast-1 \
  --secret-file ./cloud-credentials
```
#### 3.2 执行全集群恢复

```bash
# 场景 A：全量恢复到原集群（覆盖式，谨慎使用）
velero restore create full-restore-$(date +%Y%m%d-%H%M%S) \
  --from-backup prod-daily-20260701-0200 \
  --include-cluster-resources=true \
  --restore-volumes=true \
  --wait

# 场景 B：恢复到隔离命名空间做验证（推荐先演练）
velero restore create drill-restore-$(date +%Y%m%d-%H%M%S) \
  --from-backup prod-daily-20260701-0200 \
  --include-namespaces production,monitoring \
  --namespace-mappings production:prod-drill,monitoring:mon-drill \
  --restore-volumes=true \
  --wait
```

#### 3.3 有状态应用恢复注意事项

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 若备份使用 CSI 快照，确认 VolumeSnapshotClass 存在
kubectl get volumesnapshotclass

# 恢复后检查 PVC 绑定
kubectl get pvc -n production

# 若跨 AZ 恢复后 Pod 无法挂载，检查 snapshot 来源 AZ 与节点 AZ 是否一致
kubectl get pv -o json | jq '.items[].spec.csi.volumeAttributes'
```
#### 3.4 Velero 定时备份策略示例

为达到 RPO ≤ 5 min 的目标，建议对关键命名空间每小时执行一次资源备份，并配合 CSI 快照保护持久卷。以下 Schedule 将备份保留 30 天，并自动清理过期备份：

```yaml
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: prod-critical-hourly
  namespace: velero
spec:
  schedule: "0 * * * *"
  template:
    includedNamespaces:
    - production
    - monitoring
    - ingress-nginx
    snapshotVolumes: true
    storageLocation: default
    volumeSnapshotLocations:
    - aws-east
    ttl: 720h0m0s
    labelSelector:
      matchExpressions:
      - key: app.kubernetes.io/part-of
        operator: Exists
```

> 对 RPO 要求更严的有状态应用，应在应用层增加连续数据保护（CDP）或数据库原生复制，Velero 仅作为 Kubernetes 资源与卷级保护的底线。

### Phase 4：AZ / Region 故障切换

#### 4.1 单 AZ 故障

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
# 1. 确认故障 AZ
FAILED_AZ=$(kubectl get nodes -o json | jq -r '.items[] | select(.status.conditions[] | select(.type=="Ready" and .status!="True")) | .metadata.labels["topology.kubernetes.io/zone"]' | sort -u | head -n1)

# 2. 对故障 AZ 节点设置不可调度并驱逐工作负载
kubectl cordon -l topology.kubernetes.io/zone=${FAILED_AZ}
kubectl drain -l topology.kubernetes.io/zone=${FAILED_AZ} \
  --ignore-daemonsets --delete-emptydir-data --force \
  --pod-selector='app notin (csi-node,node-exporter)'

# 3. 在健康 AZ 扩容关键服务
kubectl scale deployment/order-service --replicas=30 -n production

# 4. 确认拓扑分布约束生效
kubectl get pods -n production -o wide | awk '{print $NF}' | sort | uniq -c
```
拓扑分布约束示例（应在平时配置）：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
  namespace: production
spec:
  template:
    spec:
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: order-service
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values: ["order-service"]
              topologyKey: kubernetes.io/hostname
```

#### 4.2 Region 级故障切换

Region 级切换前提是备用集群已就绪且 Velero 备份已复制到异地对象存储。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 在 DR 集群执行恢复（仅恢复关键命名空间以压缩 RTO）
velero restore create region-dr-$(date +%Y%m%d-%H%M%S) \
  --from-backup prod-daily-20260701-0200 \
  --include-namespaces production,monitoring,ingress-nginx \
  --restore-volumes=true \
  --wait

# 2. 切换全局 DNS / GSLB 到 DR 集群入口
# AWS Route53 示例
aws route53 change-resource-record-sets \
  --hosted-zone-id Z1234567890ABC \
  --change-batch file://dr-failover.json

# 3. 验证入口解析
dig +short app.example.com
curl -sf -o /dev/null -w "%{http_code}" https://app.example.com/health
```
`dr-failover.json` 示例：

```json
{
  "Changes": [{
    "Action": "UPSERT",
    "ResourceRecordSet": {
      "Name": "app.example.com",
      "Type": "CNAME",
      "TTL": 60,
      "ResourceRecords": [{"Value": "dr-ingress.example.com"}]
    }
  }]
}
```

#### 4.3 故障回切（Failback）

当主 Region 恢复后，应按以下顺序回切，避免数据冲突：

1. **数据一致性校验**：对比主从数据库、对象存储、消息队列的 offset，确认无双向写入造成的冲突。
2. **增量同步补平**：若 DR 期间产生新数据，使用数据库原生复制或对象存储同步工具将增量写回主 Region。
3. **低流量切回**：先将 5% 流量切回主 Region，观察 10 分钟核心指标。
4. **全量切回并关闭 DR**：确认主 Region 稳定后，将 DNS/GSLB 切回主入口，并保留 DR 集群 24 小时待命。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 低流量切回主 Region（以 Route53 加权记录为例）
cat > failback-10pct.json <<'EOF'
{
  "Changes": [{
    "Action": "UPSERT",
    "ResourceRecordSet": {
      "Name": "app.example.com",
      "Type": "CNAME",
      "SetIdentifier": "primary",
      "Weight": 10,
      "TTL": 60,
      "ResourceRecords": [{"Value": "primary-ingress.example.com"}]
    }
  },{
    "Action": "UPSERT",
    "ResourceRecordSet": {
      "Name": "app.example.com",
      "Type": "CNAME",
      "SetIdentifier": "dr",
      "Weight": 90,
      "TTL": 60,
      "ResourceRecords": [{"Value": "dr-ingress.example.com"}]
    }
  }]
}
EOF
aws route53 change-resource-record-sets --hosted-zone-id Z1234567890ABC --change-batch file://failback-10pct.json
```
### Phase 5：DR 演练节奏与度量

DR 演练是验证本 Runbook 有效性的唯一方式。建议按以下节奏执行：

| 演练类型 | 频率 | 场景 | 目标 | 参与方 |
|----------|------|------|------|--------|
| 桌面推演 | 每月 | AZ 故障、etcd 单成员失效 | 熟悉流程、更新联系人 | SRE |
| 隔离环境恢复演练 | 每季度 | Velero 命名空间级恢复、etcd snapshot 恢复 | 验证备份可用性、RTO ≤ 目标 | SRE + 应用团队 |
| 生产环境 Game Day | 每半年 | 单 AZ 流量切换、Region 级切换 | 实测 RTO/RPO、验证监控告警 | 全技术团队 + 业务方 |
| 全链路 BC 演练 | 每年 | 机房级灾难、长时间不可用 | 验证降级服务、沟通流程、外部依赖 | 公司级危机管理小组 |

演练完成后必须记录以下指标：

```bash
# 实际 RTO：从告警触发到业务健康检查通过的时间
echo "RTO=$(($(date -d '2026-07-01T03:18:00Z' +%s) - $(date -d '2026-07-01T03:00:00Z' +%s))) 秒"

# 实际 RPO：从备份时间戳到灾难发生时间的时间差
velero backup describe prod-daily-20260701-0200 | grep 'Start Time'
```

演练报告应包含：发现的问题、改进项、Owner、截止日期，并归档到 `_meta/journal/dr-drill-YYYY-MM-DD.md`。

### Phase 6：业务连续性（BC）决策模板

当技术恢复无法在原 RTO 内完成时，值班负责人应启动 BC 流程并填写以下模板。

**BC 启动决策树**：

- 技术恢复进度 > 50% RTO 且仍有明确路径 → 继续恢复，每 5 分钟同步进展。
- 技术恢复进度 < 50% RTO 但路径不明 → 准备切换到 DR 集群或降级服务。
- 已确认数据不可恢复或核心外部依赖中断 → 立即启动降级服务并通知业务方。

```markdown
## BC 决策记录

- 事件编号：INC-2026-0701-001
- 发现时间：2026-07-01 03:00 UTC
- 影响业务与范围：订单核心链路、支付回调、库存扣减
- 影响用户数 / 订单量：约 120 万用户 / 每分钟 3,000 单
- 当前 RTO 状态：已用 18 min / 目标 30 min
- 当前 RPO 状态：etcd RPO ≈ 0，应用数据库 RPO ≈ 2 s
- 数据一致性状态：主库同步正常，DR 集群延迟 2 s
- 决策选项与风险评估：
  1. 继续原地恢复（预计再需 15 min，无数据丢失，风险：超时）
  2. 切换至 DR 集群（预计 5 min，可能丢失 2 s 数据，风险：切换后需数据 reconciliation）
  3. 启动只读降级服务（预计 2 min，无数据丢失，风险：交易暂停）
- 最终决策与理由：选择选项 2，因 RTO 窗口即将耗尽且 DR 集群已验证可用。
- 决策人：值班经理 xxx
- 沟通对象与方式：客服（IM 群）、运营（电话）、管理层（邮件 + 短信）、客户（状态页）
- 下一步动作：
  - 03:20 完成 DNS 切换
  - 03:25 验证核心交易链路
  - 03:30 发布状态页更新
- 复盘预约时间：2026-07-03 10:00 UTC
```

### Phase 7：恢复后稳定性观察

技术恢复完成并不意味着灾难结束，必须进入稳定性观察期：

1. **灰度放量**：从 1% 流量开始，每 5 分钟按 5% / 25% / 50% / 100% 阶梯放大，期间持续监控错误率与 P99 延迟。
2. **缓存预热**：Redis、CDN、本地缓存可能在恢复后命中率骤降，应提前触发预热 Job 或临时提升缓存容量。
3. **下游依赖重连**：数据库连接池、消息队列消费者、外部支付接口可能出现瞬时超限，需观察并适时重启相关 Pod。
4. **告警静默恢复**：恢复期间为减少噪音设置的告警静默应在 30 分钟内全部解除。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 观察核心指标（示例 Prometheus 查询）
# 错误率
kubectl exec -it deploy/prometheus -n monitoring -- \
  curl -s 'http://localhost:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[1m]))/sum(rate(http_requests_total[1m]))'

# P99 延迟
kubectl exec -it deploy/prometheus -n monitoring -- \
  curl -s 'http://localhost:9090/api/v1/query?query=histogram_quantile(0.99,sum(rate(http_request_duration_seconds_bucket[1m]))by(le))'
```
## 4. 关键检查点与验证命令

每完成一个 Phase，必须验证以下检查点：

| 检查项 | 命令 | 通过标准 |
|--------|------|----------|
| etcd 成员健康 | `etcdctl endpoint health --write-out=table` | 所有 endpoint healthy |
| Revision 一致性 | 循环各 endpoint 取 revision | 差异 = 0 |
| API Server 响应 | `kubectl get --raw /healthz` | 返回 `ok` |
| 核心 Pod 运行 | `kubectl get pods -n kube-system` | 全部 Running / Completed |
| Velero 恢复状态 | `velero restore get` | Phase = Completed |
| PVC 绑定 | `kubectl get pvc -n production` | 全部 Bound |
| 入口可达 | `curl -I https://app.example.com/health` | HTTP 200 |
| 业务 SLO | Prometheus 查询 | 错误率 / P99 在 SLO 内 |
| DNS 解析 | `dig app.example.com` | 指向预期入口 |
| 证书有效期 | `kubeadm certs check-expiration` | 全部 > 7 天 |

## 5. 回滚 / 应急方案

- **Velero 恢复异常**：立即停止恢复，删除 restore 对象，回退 DNS，并从更早的备份重试。
  ```bash
  velero restore delete <restore-name> --confirm
  kubectl delete ns production --wait=false
  velero restore create --from-backup prod-daily-20260630-0200 ...
  ```
- **etcd 恢复后 split-brain**：比较各成员 revision，选择 revision 最高的唯一 leader；其余成员清理数据目录并以 learner 重新加入。
- **Region 切换后数据冲突**：切换回主 Region 前，先执行数据库/消息队列的数据核对与冲突解决；禁止双向写入。
- **无法恢复且备份不可用**：启动“控制面全损重建”流程，按 [[12-可靠性/02-灾难恢复/16-control-plane-loss-recovery-playbook.md|控制面全部丢失的灾难恢复]] 从 snapshot 重建集群。

## 6. 风险与注意事项

1. **Split-brain**：etcd 成员恢复过程中必须保证 `initial-cluster-token` 唯一，避免旧成员重新加入形成两个集群。恢复完成后应对比所有成员 revision，确认一致。
2. **版本偏差**：Velero 恢复要求目标集群与备份集群 Kubernetes 小版本一致（如 1.31→1.31），CRD 版本也必须兼容。跨小版本恢复可能导致资源无法解析或控制器行为异常。
3. **AZ 绑定 PV**：云厂商卷快照通常与 AZ 绑定，跨 AZ 恢复时需确认 StorageClass `volumeBindingMode: WaitForFirstConsumer` 或手动指定 AZ。否则 Pod 可能持续 Pending。
4. **DNS TTL**：故障切换前应将关键域名 TTL 降至 60s 以下，否则客户端缓存会延长实际切换时间。切换完成后可逐步恢复原始 TTL。
5. **Secret / 证书**：跨集群恢复后，Ingress TLS、ServiceAccount token、cloud provider credentials 可能失效，需提前准备。建议将证书与外部凭证保存在独立 Vault，并在恢复后统一轮换。
6. **StatefulSet 启动顺序**：数据库主从、Kafka、ZooKeeper 等需要按顺序启动并验证，不可直接批量 apply。应先启动依赖底层存储的服务，再启动消费者。
7. **资源配额与许可证**：DR 集群的节点规格、IP 地址池、LoadBalancer 配额、软件 License 必须在平时预留。灾难时临时扩容往往受云厂商配额限制。
8. **备份不可恢复**：定期执行恢复演练，验证 snapshot 与 Velero 备份真实可用。很多团队在真正灾难时才发现备份文件损坏或权限不足。
9. **监控告警盲区**：灾难期间部分告警可能因 Prometheus、Alertmanager 本身故障而静默。应确保关键告警具备跨集群或短信/电话通道。
10. **人为误操作放大灾难**：恢复命令往往具有高破坏性，必须在执行前二次确认，并在隔离环境先演练。禁止在睡眠不足或缺乏备份的情况下直接在生产环境执行 etcd restore。

## 7. 相关 Runbook / 推荐阅读

- [[12-可靠性/00-总览/99-production-readiness-operations-guide.md|可靠性工程生产就绪运维指南]]
- [[12-可靠性/02-灾难恢复/10-dr-scenarios-catalog.md|灾备场景目录]]
- [[12-可靠性/02-灾难恢复/11-az-failure-playbook.md|可用区故障恢复手册]]
- [[12-可靠性/02-灾难恢复/13-etcd-corruption-recovery-playbook.md|etcd 数据损坏检测与恢复全流程]]
- [[12-可靠性/02-灾难恢复/16-control-plane-loss-recovery-playbook.md|控制面全部丢失的灾难恢复]]
- [[12-可靠性/01-备份恢复/01-etcd-backup-restore.md|etcd 备份与恢复]]
- [[12-可靠性/02-灾难恢复/99-velero-backup-recovery-guide.md|Velero 企业级备份恢复实践指南]]
- [[12-可靠性/02-灾难恢复/18-cross-region-disaster-recovery.md|跨区域灾难恢复]]
- [[12-可靠性/02-灾难恢复/17-disaster-recovery-drills.md|灾难恢复演练]]
- [[13-生产运维/00-总览/99-production-readiness-operations-guide.md|生产运维生产就绪指南]]
- [[11-发布变更/00-总览/99-production-readiness-operations-guide.md|发布变更管理生产就绪指南]]
- [[18-云厂商/00-总览/99-production-readiness-operations-guide.md|云厂商生产就绪指南]]
- [[01-集群基础/00-总览/99-production-readiness-operations-guide.md|集群基础生产就绪指南]]
- [[06-存储/00-总览/99-production-readiness-operations-guide.md|存储数据生产就绪指南]]

---

*本 Runbook 应与每季度 DR 演练结合使用。演练结果、RTO/RPO 实测数据和改进项应归档到 `_meta/journal/`，并在下次生产就绪评审中复核。*


<!-- risk-assessed -->
