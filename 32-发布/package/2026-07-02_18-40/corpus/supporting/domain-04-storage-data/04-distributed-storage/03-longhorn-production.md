---
title: Longhorn Kubernetes 生产部署与运维
description: 在阿里云与专有云 Kubernetes 上部署 Longhorn 分布式块存储，覆盖架构、安装、卷管理、快照/备份、节点故障恢复、CSI
  集成、升级流程与生产注意事项
summary: 在阿里云与专有云 Kubernetes 上部署 Longhorn 分布式块存储，覆盖架构、安装、卷管理、快照/备份、节点故障恢复、CSI 集成、升级流程与生产注意事项
category: storage
tags:
- k8s
- longhorn
- distributed-storage
- csi
- snapshot
- backup
- disaster-recovery
- alicloud
- apsara-stack
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 存储工程师
- 运维工程师
estimated_read_time: 30min
intent_queries:
- Longhorn K8s 生产安装
- Longhorn 备份恢复与节点故障恢复
- 阿里云 K8s Longhorn 最佳实践
trigger_keywords:
- Longhorn
- 分布式存储
- CSI
- 卷管理
- 快照
- 备份
prerequisites:
- kubectl-basics
- storage-basics
- helm-basics
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




# Longhorn Kubernetes 生产部署与运维

> **适用版本**: Kubernetes v1.28 - v1.32 | **Longhorn**: v1.6+ | **最后更新**: 2026-06
> **文档定位**: Longhorn 是轻量级分布式块存储，适合中小规模 K8s 集群、开发测试环境或专有云边缘场景。阿里云 ACK 生产环境建议优先使用阿里云云盘 CSI，Longhorn 作为补充方案。

<!-- chunk: 目录 -->
## 目录

1. [架构概述](#架构概述)
2. [前置条件](#前置条件)
3. [安装 Longhorn](#安装-longhorn)
4. [卷管理](#卷管理)
5. [快照与备份](#快照与备份)
6. [节点故障恢复](#节点故障恢复)
7. [CSI 集成与扩容](#csi-集成与扩容)
8. [升级流程](#升级流程)
9. [生产注意事项](#生产注意事项)
10. [监控与告警](#监控与告警)
11. [故障排查](#故障排查)
12. [最佳实践检查清单](#最佳实践检查清单)

---

<!-- chunk: 1. 架构概述 -->
## 1. 架构概述

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          Kubernetes Cluster                              │
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │                      Longhorn Manager                            │   │
│   │           管理 Volume / Engine / Replica / Node                  │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │                      Longhorn UI                                 │   │
│   │              Web 控制台：卷、快照、备份、节点                    │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
│   ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                   │
│   │   Engine    │  │  Replica 1  │  │  Replica 2  │  (默认 3 副本)    │
│   │  iSCSI 目标  │  │  数据副本   │  │  数据副本   │                   │
│   └─────────────┘  └─────────────┘  └─────────────┘                   │
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │              Longhorn CSI Plugin (provisioner/node)             │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │           BackupTarget: 阿里云 OSS / 专有云 OSS / NFS           │   │
│   └─────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

| 组件 | 作用 | 部署方式 |
|:---|:---|:---|
| Longhorn Manager | 控制平面，处理 Volume CR | DaemonSet |
| Longhorn Engine | 每个 Volume 一个，提供 iSCSI 目标 | 由 Manager 调度 |
| Replica | 数据副本，默认 3 副本 | 分布在不同节点 |
| Longhorn UI | 可视化管理 | Deployment + Service |
| CSI Plugin | 对接 K8s PVC/PV | DaemonSet + Deployment |
| Instance Manager | 管理 Engine/Replica 进程 | DaemonSet |

---

<!-- chunk: 2. 前置条件 -->
## 2. 前置条件

### 2.1 节点要求

| 项目 | 最低要求 | 生产建议 |
|:---|:---|:---|
| 节点数 | 3 | ≥ 3，副本跨节点分布 |
| CPU/内存 | 2C4G | 4C8G 以上 |
| 数据盘 | 50GB | SSD，用于 `/var/lib/longhorn` |
| Kernel | 3.10+ | 4.x+，支持 iscsi_tcp |
| iscsid | 已安装并运行 | `systemctl status iscsid` |

### 2.2 安装 open-iscsi

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# CentOS/RHEL
yum install -y iscsi-initiator-utils
systemctl enable --now iscsid

# Ubuntu/Debian
apt-get install -y open-iscsi
systemctl enable --now iscsid

# 验证
systemctl status iscsid
iscsiadm -m session
```
### 2.3 检查挂载

```bash
mount | grep /var/lib/longhorn
# 确保 /var/lib/longhorn 有独立数据盘挂载，避免撑爆系统盘
```

---

<!-- chunk: 3. 安装 Longhorn -->
## 3. 安装 Longhorn

### 3.1 使用 Helm 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add longhorn https://charts.longhorn.io
helm repo update

helm install longhorn longhorn/longhorn \
  --namespace longhorn-system \
  --create-namespace \
  --version 1.6.0 \
  --set defaultSettings.replicaCount=3 \
  --set defaultSettings.backupTarget="s3://longhorn-backup-bucket@oss-cn-hangzhou/" \
  --set defaultSettings.backupTargetCredentialSecret="oss-backup-secret"
```
### 3.2 配置 OSS 备份 Secret

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
export ALIBABA_CLOUD_ACCESS_KEY_ID="LTAIxxxxxxxxxxxxxxxx"   # 替换为实际 AccessKey ID
export ALIBABA_CLOUD_ACCESS_KEY_SECRET="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"   # 替换为实际 AccessKey Secret

cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Secret
metadata:
  name: oss-backup-secret
  namespace: longhorn-system
type: Opaque
stringData:
  AWS_ACCESS_KEY_ID: ${ALIBABA_CLOUD_ACCESS_KEY_ID}
  AWS_SECRET_ACCESS_KEY: ${ALIBABA_CLOUD_ACCESS_KEY_SECRET}
  AWS_ENDPOINTS: https://oss-cn-hangzhou-internal.aliyuncs.com
  AWS_REGION: cn-hangzhou
EOF
```
### 3.3 验证安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get pods -n longhorn-system
kubectl get svc -n longhorn-system

# 访问 UI
kubectl port-forward svc/longhorn-frontend 8080:80 -n longhorn-system
```
### 3.4 专有云安装调整

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm install longhorn ./longhorn \
  --namespace longhorn-system \
  --create-namespace \
  --set global.image.registry=harbor.apsara-stack.local/longhorn \
  --set defaultSettings.replicaCount=3 \
  --set defaultSettings.backupTarget="s3://longhorn-backup-bucket@oss-private-region/" \
  --set defaultSettings.backupTargetCredentialSecret="oss-backup-secret"
```
---

<!-- chunk: 4. 卷管理 -->
## 4. 卷管理

### 4.1 创建 StorageClass

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: longhorn
provisioner: driver.longhorn.io
allowVolumeExpansion: true
reclaimPolicy: Delete
volumeBindingMode: Immediate
parameters:
  numberOfReplicas: "3"
  staleReplicaTimeout: "2880"
  fromBackup: ""
  fsType: "ext4"
```

### 4.2 创建 PVC

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: longhorn-pvc
  namespace: production
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: longhorn
  resources:
    requests:
      storage: 10Gi
```

### 4.3 查看卷状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pvc -n production
kubectl get pv
kubectl get volume -n longhorn-system
```
### 4.4 通过 Longhorn UI 管理卷

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl port-forward svc/longhorn-frontend 8080:80 -n longhorn-system
# 浏览器访问 http://localhost:8080
```
---

<!-- chunk: 5. 快照与备份 -->
## 5. 快照与备份

### 5.1 创建快照

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: longhorn-snapshot
  namespace: production
spec:
  volumeSnapshotClassName: longhorn-snapshot
  source:
    persistentVolumeClaimName: longhorn-pvc
EOF
```
### 5.2 创建备份

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: longhorn.io/v1beta2
kind: RecurringJob
metadata:
  name: daily-backup
  namespace: longhorn-system
spec:
  cron: "0 2 * * *"
  task: backup
  groups:
    - default
  retain: 7
  concurrency: 2
EOF
```
### 5.3 从备份恢复

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: longhorn-pvc-restored
  namespace: production
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: longhorn
  dataSource:
    name: longhorn-snapshot
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  resources:
    requests:
      storage: 10Gi
EOF
```
---

<!-- chunk: 6. 节点故障恢复 -->
## 6. 节点故障恢复

### 6.1 节点临时故障

```
节点宕机 5 分钟内
    │
    ▼
Longhorn 标记该节点为 NotReady
    │
    ▼
在其他节点重建缺失 Replica
    │
    ▼
节点恢复后，Longhorn 自动同步并清理多余 Replica
```

### 6.2 节点永久故障

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
# 1. 确认节点已不可恢复
kubectl get nodes

# 2. 在 Longhorn UI 中禁用故障节点
# Nodes -> node-worker-03 -> Edit Node -> Scheduling: Disabled

# 3. 从集群移除节点
kubectl drain node-worker-03 --ignore-daemonsets --delete-emptydir-data
kubectl delete node node-worker-03

# 4. Longhorn 自动在其他节点重建 Replica
# 等待 Degraded 卷恢复健康
```
### 6.3 验证卷健康

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get volume -n longhorn-system
# 确认 state: attached, robustness: healthy
```
---

<!-- chunk: 7. CSI 集成与扩容 -->
## 7. CSI 集成与扩容

### 7.1 CSI Snapshot 支持

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get volumesnapshotclass
# 预期输出
NAME                DRIVER                   DELETIONPOLICY
longhorn-snapshot   driver.longhorn.io       Delete
```
### 7.2 CSI 扩容

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

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
# 编辑 PVC，增大 storage 请求
kubectl edit pvc longhorn-pvc -n production
# Longhorn 自动扩容文件系统
```
### 7.3 StorageClass 参数

| 参数 | 默认值 | 说明 |
|:---|:---:|:---|
| numberOfReplicas | 3 | 副本数 |
| staleReplicaTimeout | 2880 | 过时副本超时时间（分钟） |
| fromBackup | "" | 从备份创建 |
| fsType | ext4 | 文件系统类型 |
| dataLocality | disabled | 数据本地性策略 |

---

<!-- chunk: 8. 升级流程 -->
## 8. 升级流程

### 8.1 升级前准备

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 查看当前版本
helm list -n longhorn-system

# 2. 创建全量备份
kubectl apply -f - <<EOF
apiVersion: longhorn.io/v1beta2
kind: RecurringJob
metadata:
  name: pre-upgrade-backup
  namespace: longhorn-system
spec:
  cron: "0 0 1 1 *"
  task: backup
  groups:
    - default
  retain: 1
EOF

# 3. 确认所有卷 Healthy
kubectl get volume -n longhorn-system
```
### 8.2 执行升级

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm upgrade longhorn longhorn/longhorn \
  --namespace longhorn-system \
  --version 1.7.0 \
  --reuse-values \
  --wait \
  --timeout 10m
```
### 8.3 升级后验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n longhorn-system
kubectl get volume -n longhorn-system
kubectl get settings -n longhorn-system
```
---

<!-- chunk: 9. 生产注意事项 -->
## 9. 生产注意事项

| 场景 | 风险 | 建议 |
|:---|:---|:---|
| 单节点故障 | 副本降级 | 默认 3 副本，跨可用区部署节点 |
| 网络分区 | 脑裂 | 配置 Pod Disruption Budget 与节点亲和性 |
| 系统盘撑爆 | Longhorn 默认使用 /var/lib/longhorn | 独立挂载数据盘 |
| 备份目标不可用 | 备份失败 | 使用阿里云 OSS 内网 Endpoint，配置告警 |
| 大卷重建慢 | 恢复时间长 | 控制单卷大小，使用快照分级 |
| 升级 Longhorn | 卷短暂不可用 | 按官方升级文档逐步执行 |

### 9.1 阿里云/专有云场景

| 场景 | 推荐方案 |
|:---|:---|
| ACK 生产核心存储 | 阿里云云盘 CSI（更高 IOPS、SLA） |
| 自建 K8s / 边缘节点 | Longhorn（轻量、易运维） |
| 专有云内部测试环境 | Longhorn + 专有云 OSS 备份 |
| 跨区域灾备 | Longhorn 备份到异地 OSS Bucket |

---

<!-- chunk: 10. 监控与告警 -->
## 10. 监控与告警

```yaml
groups:
  - name: longhorn-alerts
    rules:
      - alert: LonghornVolumeDegraded
        expr: longhorn_volume_robustness == 2
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Longhorn 卷处于 Degraded 状态"

      - alert: LonghornVolumeFault
        expr: longhorn_volume_robustness == 3
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Longhorn 卷处于 Fault 状态"

      - alert: LonghornNodeDown
        expr: longhorn_node_status{condition="ready"} == 0
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Longhorn 节点不可用"

      - alert: LonghornBackupFailed
        expr: increase(longhorn_backup_failed_total[1h]) > 0
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Longhorn 备份失败"
```

---

<!-- chunk: 11. 故障排查 -->
## 11. 故障排查

| 问题 | 排查命令 | 解决方案 |
|:---|:---|:---|
| Pod 无法挂载卷 | `kubectl describe pod order-service-7d9f4b8c5-x2k9m` | 检查 iscsid、Longhorn Manager、Replica 状态 |
| 卷显示 Degraded | UI 或 `kubectl get volume` | 等待重建或检查节点状态 |
| 备份失败 | `kubectl logs -n longhorn-system -l app=longhorn-manager` | 检查 OSS 凭证与 Endpoint |
| UI 无法访问 | `kubectl get svc -n longhorn-system` | 配置 NodePort/Ingress |
| 升级卡住 | `kubectl get pods -n longhorn-system` | 查看 Instance Manager 日志 |

---

<!-- chunk: 12. 最佳实践检查清单 -->
## 12. 最佳实践检查清单

| 检查项 | 要求 | 验证命令 |
|:---|:---|:---|
| Longhorn Pod 全部 Running | 无 CrashLoopBackOff | `kubectl get pods -n longhorn-system` |
| iscsid 运行正常 | 所有节点 | `systemctl status iscsid` |
| 数据盘独立挂载 | 非系统盘 | `df -h /var/lib/longhorn` |
| 默认副本数 3 | 生产环境 | UI -> Settings -> General |
| 备份目标可达 | OSS 内网 Endpoint | UI -> Backup |
| 卷全部 Healthy | 无 Degraded/Fault | `kubectl get volume -n longhorn-system` |
| 节点调度策略正确 | 跨节点分布 | UI -> Node |
| 快照保留策略 | 防止 OSS 成本膨胀 | RecurringJob retain 配置 |
| 升级前快照 | 可回滚 | 升级前创建全量备份 |
| UI 访问已限制 | 仅白名单 IP 或 VPN | Ingress/NetworkPolicy |

---

## Related

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-04-storage-data/01-k8s-storage/03-storage-backup-disaster-recovery|10 - 存储备份与灾难恢复]]
- [[domain-04-storage-data/README.md|Storage Domain 存储领域知识库]]

## See Also

- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-04-storage-data/04-distributed-storage/01-velero-backup-recovery|Velero 阿里云专有云备份恢复实战]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-04-storage-data/04-distributed-storage/02-rook-ceph-production|Rook-Ceph 生产指南]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-04-storage-data/05-stateful-app-storage/01-stateful-app-storage-patterns|有状态应用存储模式]]

---

## Longhorn 与阿里云 ACK 存储互补方案

在阿里云 ACK 环境中，Longhorn 不应替代 ESSD/NAS/OSS 等托管存储，而应作为补充：

| 存储类型 | 优势 | Longhorn 是否替代 |
|:---|:---|:---:|
| 阿里云 ESSD | 低延迟、高 IOPS、托管快照 | 否 |
| 阿里云 NAS | 多 Pod 共享读写（RWX） | 否 |
| 阿里云 OSS | 低成本归档、海量对象 | 否 |
| Longhorn | 跨节点副本、快速快照、边缘场景 | 补充 |

适用 Longhorn 的场景包括：边缘节点池无法使用托管存储；开发与测试环境需要低成本块存储；需要秒级快照与跨节点副本的轻量级有状态应用；专有云 Apsara Stack 未部署统一块存储服务。

### 混合使用示例

```yaml
# 核心数据库使用 ESSD
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data
spec:
  storageClassName: alicloud-disk-topology
  resources:
    requests:
      storage: 500Gi
---
# 边缘缓存使用 Longhorn
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: edge-cache
spec:
  storageClassName: longhorn
  resources:
    requests:
      storage: 100Gi
```

---

## Longhorn 升级与维护窗口

升级 Longhorn 前，建议先阅读 Release Notes，并在维护窗口执行。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 查看当前版本
helm -n longhorn-system list

# 2. 更新 Helm 仓库
helm repo update

# 3. 获取新 values
helm get values longhorn -n longhorn-system > longhorn-values.yaml

# 4. 执行升级
helm upgrade longhorn longhorn/longhorn \
  -n longhorn-system \
  -f longhorn-values.yaml \
  --version 1.6.1 \
  --wait

# 5. 验证 Pod 与卷状态
kubectl -n longhorn-system get pods
kubectl -n longhorn-system get volumes
```
---

## Longhorn 数据本地化与副本自动平衡

Longhorn 支持 `dataLocality` 参数，可让卷副本优先分布在 Pod 所在节点，降低跨节点读写的网络开销。

| dataLocality 值 | 行为 | 适用场景 |
|:---|:---|:---|
| disabled | 不保证数据本地化 | 通用场景 |
| best-effort | 尽量将副本放在 Pod 所在节点 | 网络敏感型应用 |
| strict-local | 必须本地有副本，否则 Pod 无法调度 | 极高性能要求 |

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: longhorn-local
provisioner: driver.longhorn.io
parameters:
  numberOfReplicas: "3"
  dataLocality: "best-effort"
  replicaAutoBalance: "best-effort"
reclaimPolicy: Delete
allowVolumeExpansion: true
```

`replicaAutoBalance` 可在节点故障或新增节点后自动重新平衡副本位置，保持高可用与负载均衡。

---

## Longhorn 监控大盘与日常巡检

建议将 Longhorn 指标接入 Grafana，重点关注以下大盘：

- 卷健康状态：Degraded、Faulted、Healthy 数量趋势。
- 节点磁盘使用率：默认数据路径 `/var/lib/longhorn` 空间。
- 备份成功率：按 RecurringJob 分组统计。
- IO 延迟与吞吐：识别性能瓶颈节点。

日常巡检命令：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点磁盘使用情况
kubectl -n longhorn-system get nodes.longhorn.io -o yaml | grep -A5 diskStatus

# 查看所有卷状态
kubectl -n longhorn-system get volumes

# 查看最近备份
kubectl -n longhorn-system get backups
```
```

<!-- risk-assessed -->
