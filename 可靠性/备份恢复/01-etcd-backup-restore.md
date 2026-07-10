---
title: etcd 备份恢复：snapshot、恢复、定时任务
description: 面向阿里云专有云 K8s 运维工单智能体的 etcd 备份与恢复实战手册，覆盖 etcdctl snapshot、定时 CronJob 备份、灾难恢复演练及
  ASO 控制台操作。
summary: 面向阿里云专有云 K8s 运维工单智能体的 etcd 备份与恢复实战手册，覆盖 etcdctl snapshot、定时 CronJob 备份、灾难恢复演练及
  ASO 控制台操作。
category: reliability-engineering
tags:
- etcd
- backup-restore
- snapshot
- cronjob
- aso
- ack
- control-plane
- disaster-recovery
- sre
- operations
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 值班工程师
estimated_read_time: 15min
intent_queries:
- 如何备份 etcd 集群
- etcd snapshot 恢复步骤
- 阿里云专有云 etcd 备份策略
- etcd 定时备份 CronJob 配置
- etcd 灾难恢复最佳实践
trigger_keywords:
- etcd backup
- etcd restore
- etcd snapshot
- 控制面备份
- 专有云 etcd
prerequisites:
- etcd-basics
- kubectl-basics
- control-plane-basics
- linux-basics
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




# etcd 备份恢复：snapshot、恢复、定时任务

> **适用范围**: Kubernetes v1.28-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐
> **适用场景**: 阿里云 ACK 托管版/专有云服务热线下的 etcd 备份、恢复与演练。

## 目录

- [1. 概述与背景](#1-概述与背景)
- [2. etcd 在阿里云 K8s 中的部署形态](#2-etcd-在阿里云-k8s-中的部署形态)
- [3. 备份策略设计](#3-备份策略设计)
- [4. 手动备份：etcdctl snapshot](#4-手动备份etcdctl-snapshot)
  - [4.4 ACK 托管版 etcd 快照下载](#44-ack-托管版-etcd-快照下载)
- [5. 定时备份：CronJob 自动化](#5-定时备份cronjob-自动化)
  - [5.3 备份文件加密与访问控制](#53-备份文件加密与访问控制)
  - [5.4 监控备份任务](#54-监控备份任务)
- [6. 恢复流程](#6-恢复流程)
- [7. 验证与演练](#7-验证与演练)
- [8. 常见问题与故障排查](#8-常见问题与故障排查)
- [9. etcd 维护：压缩与碎片整理](#9-etcd-维护压缩与碎片整理)
- [10. 检查清单](#10-检查清单)
- [11. Related](#11-related)

## 1. 概述与背景

etcd 是 Kubernetes 控制面的唯一状态存储，所有 API 对象、事件、Secret、CRD 都持久化在 etcd 中。一旦 etcd 数据损坏或丢失，整个集群的资源视图将不可恢复。对于阿里云专有云（ASO）和托管版 ACK 环境，虽然 ACK 托管版由云侧负责 etcd 托管运维，但在专有云中客户往往需要自主完成 etcd 的备份、恢复与演练，以满足等保、金融合规及业务连续性要求。

本文档聚焦以下三类运维场景：

1. **例行备份**：通过 `etcdctl snapshot save` 或 CronJob 定时生成一致性快照。
2. **灾难恢复**：在 etcd 数据损坏、节点全部故障、误删 Namespace 等场景下，从快照恢复集群。
3. **演练验证**：定期开展恢复演练，确保备份可用、RTO/RPO 目标可达成。

> **核心原则**：备份必须跨可用区/跨介质存放；恢复演练必须在与生产隔离的环境中验证；任何恢复操作前必须停止 kube-apiserver 写入。

## 2. etcd 在阿里云 K8s 中的部署形态

| 部署形态 | etcd 管理方 | 备份责任 | 典型场景 |
|---|---|---|---|
| ACK 托管版 | 阿里云 | 云侧自动备份，可下载 snapshot | 公有云、金融云 |
| ACK 专有版 / ASO | 客户 / 联合运维 | 客户主导备份策略与恢复演练 | 专有云、政务云、私有部署 |
| 自建 Kubeadm | 客户 | 客户完全负责 | 测试、边缘场景 |

在专有云中，etcd 通常以静态 Pod 方式部署在 Master 节点，由 `kubelet` 通过 `/etc/kubernetes/manifests/etcd.yaml` 拉起。备份操作需要：

- 访问 etcd 客户端证书（通常位于 Master 节点的 `/etc/kubernetes/pki/etcd/`）。
- 访问 etcd 服务端点（通常为 `https://127.0.0.1:2379` 或 Master 内网 IP）。
- 具备写入备份存储的权限（OSS、NAS、本地冗余盘等）。

## 3. 备份策略设计

### 3.1 备份频率与保留策略

| 备份类型 | 频率 | 保留周期 | 存储位置 | 用途 |
|---|---|---|---|---|
| 全量 snapshot | 每小时 | 7 天 | 同城 OSS + 异地 OSS | 快速回滚 |
| 全量 snapshot | 每日凌晨 02:00 | 30 天 | 异地 OSS + 磁带/对象归档 | 合规审计 |
| 配置变更前 snapshot | 变更前手动触发 | 永久保留关键版本 | 版本化 OSS | 变更回滚 |

### 3.2 RTO/RPO 目标

- **RPO**：建议 ≤ 1 小时，关键金融类集群建议 ≤ 15 分钟。
- **RTO**：单 Master etcd 恢复 ≤ 30 分钟；全集群重建 ≤ 4 小时。

## 4. 手动备份：etcdctl snapshot

### 4.1 确认 etcd 集群健康状态

在执行备份前，必须先确认 etcd 成员健康，避免在集群分裂或节点异常时生成不一致快照。以下命令通过 etcdctl 检查成员列表与集群健康度：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 设置 etcdctl 访问参数
export ETCDCTL_API=3
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

# 查看成员列表与健康状态
etcdctl member list -w table
etcdctl endpoint health --cluster -w table
```
### 4.2 执行一致性快照

快照会生成 etcd 当前数据目录的一致性时间点副本。执行 snapshot 时应将输出定向到具有冗余能力的存储路径，并同时记录快照元数据：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
SNAP_DATE=$(date +%Y%m%d-%H%M%S)
SNAP_FILE=/backup/etcd/etcd-snapshot-${SNAP_DATE}.db
META_FILE=/backup/etcd/etcd-snapshot-${SNAP_DATE}.meta

# 生成 snapshot
etcdctl snapshot save ${SNAP_FILE}

# 记录快照元数据：大小、hash、etcd 版本
sha256sum ${SNAP_FILE} > ${META_FILE}
etcdctl version >> ${META_FILE}
etcdctl endpoint status --cluster -w json >> ${META_FILE}

# 上传到 OSS（使用 aliyun CLI）
aliyun oss cp ${SNAP_FILE} oss://my-k8s-backup/etcd/$(hostname)/
aliyun oss cp ${META_FILE} oss://my-k8s-backup/etcd/$(hostname)/
```
### 4.3 校验快照完整性

生成快照后必须校验，防止静默损坏导致恢复失败。`snapshot status` 会输出快照的 hash、revision、total keys 等关键信息：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 校验 snapshot 状态
etcdctl snapshot status ${SNAP_FILE} -w table

# 预期输出示例
# +----------+----------+------------+------------+
# |   HASH   | REVISION | TOTAL KEYS | TOTAL SIZE |
# +----------+----------+------------+------------+
# | 9f8e7d6c | 12345678 |     54321  |   256 MB   |
# +----------+----------+------------+------------+
```
### 4.4 ACK 托管版 etcd 快照下载

对于 ACK 托管版集群，云侧会按策略自动执行 etcd 快照。用户可通过 ACK 控制台或 OpenAPI 下载快照到本地或 OSS，用于本地验证与二次灾备：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 通过 aliyun CLI 查询集群可下载的 etcd 快照列表
aliyun cs GET /clusters/<cluster-id>/etcdbackup

# 下载指定 snapshot 到本地
aliyun cs GET /clusters/<cluster-id>/etcdbackup/<backup-id> \
  --body "{\"download\":true}" > etcd-snapshot-ack.db

# 校验下载文件的完整性
etcdctl snapshot status etcd-snapshot-ack.db -w table
```
> **注意**：下载的 snapshot 仅用于验证与异地归档，恢复操作通常由阿里云托管服务负责，非特殊场景不建议客户自行恢复托管版 etcd。

## 5. 定时备份：CronJob 自动化

在专有云中，推荐在独立的 `backup-system` Namespace 中部署 CronJob，避免与业务工作负载争用资源。CronJob 通过挂载 Master 节点 etcd 证书与备份存储 PVC/OSS 完成自动化备份。

### 5.1 备份 CronJob 配置

以下 YAML 每小时执行一次 snapshot，并将结果上传到 OSS。使用 OSS 作为目标存储可实现跨 AZ 容灾，且便于与 ASO 备份策略联动：

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: etcd-hourly-snapshot
  namespace: backup-system
spec:
  schedule: "0 * * * *"
  concurrencyPolicy: Forbid
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 5
  jobTemplate:
    spec:
      backoffLimit: 2
      activeDeadlineSeconds: 600
      template:
        spec:
          nodeSelector:
            node-role.kubernetes.io/master: ""
          tolerations:
          - key: node-role.kubernetes.io/master
            effect: NoSchedule
          containers:
          - name: etcd-backup
            image: registry.cn-hangzhou.aliyuncs.com/acs/etcd:3.5.15
            command:
            - /bin/sh
            - -c
            - |
              set -euo pipefail
              SNAP_DATE=$(date +%Y%m%d-%H%M%S)
              SNAP_FILE=/backups/etcd-snapshot-${SNAP_DATE}.db
              META_FILE=/backups/etcd-snapshot-${SNAP_DATE}.meta
              etcdctl snapshot save ${SNAP_FILE}
              sha256sum ${SNAP_FILE} > ${META_FILE}
              etcdctl version >> ${META_FILE}
              # 上传到 OSS（容器内已配置 aliyun CLI 凭证）
              aliyun oss cp ${SNAP_FILE} oss://my-k8s-backup/etcd/$(hostname)/
              aliyun oss cp ${META_FILE} oss://my-k8s-backup/etcd/$(hostname)/
              # 清理 7 天前的本地快照
              find /backups -name "etcd-snapshot-*.db" -mtime +7 -delete
            env:
            - name: ETCDCTL_API
              value: "3"
            - name: ETCDCTL_ENDPOINTS
              value: "https://127.0.0.1:2379"
            volumeMounts:
            - name: etcd-certs
              mountPath: /etc/kubernetes/pki/etcd
              readOnly: true
            - name: backup-storage
              mountPath: /backups
          volumes:
          - name: etcd-certs
            hostPath:
              path: /etc/kubernetes/pki/etcd
              type: Directory
          - name: backup-storage
            persistentVolumeClaim:
              claimName: etcd-backup-pvc
          restartPolicy: OnFailure
```

### 5.2 备份保留与清理策略

| 存储层 | 保留策略 | 清理方式 |
|---|---|---|
| 本地 PVC | 最近 7 天 | CronJob 内 `find -mtime +7 -delete` |
| 同城 OSS | 最近 30 天 | OSS Lifecycle Rule |
| 异地 OSS | 最近 180 天 | OSS Lifecycle Rule + 归档存储 |

### 5.3 备份文件加密与访问控制

etcd snapshot 包含整个集群的 Secret、Token 与配置，属于高敏感数据。上传到 OSS 前建议进行客户端加密，并通过 Bucket Policy 限制访问：

```bash
# 使用 AES-256-GCM 对 snapshot 加密（密码由 KMS/HSM 托管）
openssl enc -aes-256-cbc -salt -in ${SNAP_FILE} -out ${SNAP_FILE}.enc -pass pass:${SNAP_ENC_KEY}

# 上传加密后的文件
aliyun oss cp ${SNAP_FILE}.enc oss://my-k8s-backup/etcd/$(hostname)/

# 配置 OSS Bucket Policy，仅允许指定 RAM Role 访问
aliyun oss bucket-policy --method put oss://my-k8s-backup \
  file://oss-restrict-policy.json
```

### 5.4 监控备份任务

通过 Prometheus 监控 CronJob 失败次数与上次成功时间，结合 Alertmanager 发送告警：

```yaml
# PrometheusRule 示例
groups:
- name: etcd-backup
  rules:
  - alert: EtcdBackupJobFailed
    expr: kube_cronjob_status_last_schedule_time{job="etcd-hourly-snapshot"} - kube_job_status_succeeded{job=~"etcd-hourly-snapshot-.*"} > 3600
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "etcd 定时备份任务超过 1 小时未成功"
      description: "请检查 backup-system Namespace 下 CronJob 与 Pod 日志"
```

## 6. 恢复流程

### 6.1 恢复前的关键决策

| 场景 | 恢复策略 | 影响 |
|---|---|---|
| 单节点 etcd 数据损坏 | 替换该节点并从其他成员同步 | 低 |
| 多数节点故障但 snapshot 可用 | 从 snapshot 重建整个集群 | 高，需停服 |
| 误删 Namespace/CRD | 从 snapshot 恢复后比对差异 | 中 |
| etcd 版本升级失败 | 回退到升级前 snapshot | 高 |

### 6.2 从 snapshot 恢复 etcd 集群

恢复 etcd 会覆盖数据目录，必须先停止所有控制面组件，防止 apiserver 继续写入：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

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
# 1. 停止 apiserver、controller-manager、scheduler
# 在 Kubeadm/专有云中，通常通过移动静态 Pod 清单实现
sudo mv /etc/kubernetes/manifests/kube-apiserver.yaml /var/tmp/
sudo mv /etc/kubernetes/manifests/kube-controller-manager.yaml /var/tmp/
sudo mv /etc/kubernetes/manifests/kube-scheduler.yaml /var/tmp/

# 2. 停止 etcd 静态 Pod
sudo mv /etc/kubernetes/manifests/etcd.yaml /var/tmp/
sleep 30

# 3. 清理现有 etcd 数据目录
sudo rm -rf /var/lib/etcd/  # ⚠️ 删除系统/数据文件

# 4. 从 snapshot 恢复
etcdctl snapshot restore /backup/etcd-snapshot-20260629-020000.db \
  --name master-0 \
  --initial-cluster master-0=https://10.0.0.10:2380,master-1=https://10.0.0.11:2380,master-2=https://10.0.0.12:2380 \
  --initial-cluster-token etcd-cluster-1 \
  --initial-advertise-peer-urls https://10.0.0.10:2380 \
  --data-dir /var/lib/etcd

# 5. 恢复静态 Pod 清单
sudo mv /var/tmp/etcd.yaml /etc/kubernetes/manifests/
sudo mv /var/tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
sudo mv /var/tmp/kube-controller-manager.yaml /etc/kubernetes/manifests/
sudo mv /var/tmp/kube-scheduler.yaml /etc/kubernetes/manifests/
```
### 6.3 多节点集群恢复要点

对于三节点 etcd 集群，建议先在首节点完成 restore 并启动，再逐个加入其他节点。新节点加入时使用 `etcdctl member add` 而非 restore，避免破坏集群 token：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在已有 etcd 节点上添加新成员
etcdctl member add master-1 --peer-urls=https://10.0.0.11:2380

# 在新节点上启动 etcd，使用已有集群 token
etcd --name master-1 \
  --initial-advertise-peer-urls https://10.0.0.11:2380 \
  --listen-peer-urls https://10.0.0.11:2380 \
  --listen-client-urls https://10.0.0.11:2379,https://127.0.0.1:2379 \
  --advertise-client-urls https://10.0.0.11:2379 \
  --initial-cluster master-0=https://10.0.0.10:2380,master-1=https://10.0.0.11:2380 \
  --initial-cluster-state existing \
  --data-dir /var/lib/etcd
```
## 7. 验证与演练

### 7.1 恢复后验证命令

恢复完成后，需要验证 apiserver 可连接、关键资源存在、etcd 集群健康：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 apiserver
kubectl cluster-info
kubectl get nodes

# 验证 etcd 集群健康
etcdctl endpoint health --cluster -w table

# 验证关键资源
kubectl get ns
kubectl get deployments --all-namespaces | head -20
```
### 7.2 季度恢复演练模板

| 阶段 | 任务 | 责任人 | 验收标准 |
|---|---|---|---|
| 演练前 | 选择非生产克隆环境，通知相关方 | SRE | 环境隔离 |
| 备份验证 | 随机抽取最近 3 个 snapshot，校验 hash | SRE | 全部通过 |
| 恢复执行 | 按手册恢复 etcd 集群 | 值班工程师 | RTO 达标 |
| 业务验证 | 部署测试应用，验证 ConfigMap/Secret/Service | QA | 功能正常 |
| 复盘归档 | 记录问题、更新手册 | SRE Lead | 改进项闭环 |

## 8. 常见问题与故障排查

| 现象 | 根因 | 处理方案 |
|---|---|---|
| `snapshot status` 报 checksum 错误 | 快照文件损坏或传输不完整 | 重新下载并比对 OSS ETag/SHA256 |
| 恢复后 apiserver 无法启动 | 证书与 snapshot 不匹配 | 确认恢复时使用的 CA/证书与 snapshot 一致 |
| etcd 集群出现 split brain | member 启动参数不一致 | 统一 initial-cluster-token 与 peer URLs |
| 备份 CronJob 持续失败 | 证书挂载错误或 OSS 权限不足 | 检查 volumeMounts 与 RAM/OSS 策略 |
| snapshot 文件过大 | 事件未清理或 CRD 过多 | 调整 etcd compaction 与 defrag 策略 |
| 恢复后部分 CRD 丢失 | snapshot 版本与 apiserver 版本不匹配 | 确认 etcd 与 apiserver 大版本一致 |
| 备份任务在 Master 节点无法调度 | nodeSelector 或 toleration 配置错误 | 确认 Master 标签与污点容忍 |

## 9. etcd 维护：压缩与碎片整理

长期运行的 etcd 会产生大量历史 revision，导致 snapshot 体积膨胀、查询延迟增加。建议每周执行一次 compaction 与 defrag，并在低峰期进行：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取当前最大 revision
CURRENT_REV=$(etcdctl endpoint status --write-out=json | jq -r '.[].Status.header.revision')

# 执行 compaction，仅保留当前 revision（历史版本将被清理）
etcdctl compaction ${CURRENT_REV}

# 执行 defrag（需逐节点进行，避免集群不可用）
# 注意：defrag 期间该节点会短暂不可用，建议在维护窗口执行
etcdctl defrag

# 验证 defrag 后存储空间
etcdctl endpoint status --cluster -w table
```
### 9.1 自动维护策略

可通过 Kubernetes CronJob 在维护窗口自动执行 compaction/defrag，但需确保：

- 单节点逐个执行，避免同时 defrag 多个节点导致集群不可用。
- 执行前已生成最新 snapshot，防止误操作。
- 监控 etcd 集群健康状态，异常时立即中止。

## 10. 检查清单

- [ ] 已确认 etcd 证书路径与访问权限
- [ ] 已验证 etcd 集群健康状态
- [ ] 已生成 snapshot 并校验 hash
- [ ] 已配置 CronJob 定时备份并设置并发控制
- [ ] 已配置 OSS Lifecycle Rule 实现多副本保留
- [ ] 已对 snapshot 进行加密并限制 OSS 访问
- [ ] 已配置 Prometheus 告警监控备份任务
- [ ] 已制定恢复操作 SOP 并经过演练验证
- [ ] 已记录 RTO/RPO 目标并与业务方对齐
- [ ] 已定期执行 etcd compaction/defrag
- [ ] 已禁止在 etcd 节点直接执行非授权写操作

## 11. Related

- [[集群基础/控制平面/11-etcd-deep-dive.md|etcd 深度解析]]
- [[集群基础/控制平面/19-etcd-operations.md|etcd 运维操作指南]]
- [[集群基础/控制平面/10-plane-backup-disaster-recovery.md|控制面备份与灾难恢复]]
- [[可靠性/备份恢复/16-enterprise-backup-strategy.md|企业级备份策略]]
- [[可靠性/灾难恢复/99-velero-backup-recovery-guide.md|Velero 备份恢复指南]]
- [[存储/K8s存储/10-storage-backup-disaster-recovery.md|存储备份与灾难恢复]]


<!-- risk-assessed -->
