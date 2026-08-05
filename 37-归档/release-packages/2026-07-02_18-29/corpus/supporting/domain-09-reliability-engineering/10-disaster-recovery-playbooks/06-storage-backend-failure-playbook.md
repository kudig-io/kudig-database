---
title: 存储后端故障恢复
description: 'Ceph/Longhorn 存储后端集群故障检测、OSD 恢复、Mon 选举修复及 PVC 数据抢救全流程'
summary: 'Ceph/Longhorn 存储后端集群故障检测、OSD 恢复、Mon 选举修复及 PVC 数据抢救全流程'
category: reliability-engineering
tags:
- disaster-recovery
- k8s
- sre
- ceph
- longhorn
- storage
- osd
- pvc
tier: critical
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
- 存储后端故障恢复 是什么
- 如何恢复 Ceph 集群故障
- Longhorn 引擎故障怎么处理
trigger_keywords:
- ceph
- osd
- longhorn
- pvc
- storage-backend
- mon
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


# 存储后端故障恢复

## 概述

存储后端是 Kubernetes 有状态工作负载的根基。当 Ceph 集群出现 OSD 全部 down、Mon 选举死锁，或 Longhorn 引擎卡在 degraded 状态时，依赖 PVC 的业务 Pod 将无法挂载卷或出现 I/O 错误。本手册覆盖从故障检测到数据抢救的完整恢复链路，适用于 Ceph（Rook-Ceph / 外部集群）和 Longhorn 两种主流存储后端。

---

## 1. Ceph 集群故障检测

### 1.1 集群整体状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 进入 Rook-Ceph toolbox Pod
kubectl -n rook-ceph exec -it deploy/rook-ceph-tools -- bash

# 查看集群健康状态
ceph health detail
ceph -s

# 输出解读：
#   HEALTH_OK    → 正常
#   HEALTH_WARN  → 部分降级，通常可自愈
#   HEALTH_ERR   → 严重故障，需立即介入
```
### 1.2 OSD 状态树

```bash
# 查看 OSD 拓扑与状态
ceph osd tree

# 关键状态标记：
#   up/in     → 正常
#   up/out    → OSD 运行但未加入 CRUSH（刚启动）
#   down/in   → OSD 宕机但仍占位（需恢复）
#   down/out  → OSD 宕机且已被踢出（数据需重平衡）

# 查看具体 OSD 的磁盘信息
ceph osd df tree
```

### 1.3 PG（Placement Group）状态

```bash
ceph pg stat
ceph pg dump_stuck unclean
ceph pg dump_stuck stale

# 关键状态：
#   active+clean        → 正常
#   active+degraded     → 副本不足，正在恢复
#   active+remapped     → PG 正在迁移
#   stale               → PG 无主（OSD 全部 down）
```

---

## 2. OSD 故障恢复

### 2.1 单个 OSD 宕机

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认哪个 OSD down 了
ceph osd tree | grep down

# 查看对应 Pod 状态（Rook-Ceph 环境）
kubectl -n rook-ceph get pod -l app=rook-ceph-osd

# 查看 OSD Pod 日志
kubectl -n rook-ceph logs <osd-pod-name> --tail=100

# 常见原因与处理：
#   1. 磁盘故障 → 更换磁盘后 OSD 自动重建
#   2. Pod 被驱逐 → 修复节点后 OSD Pod 自动重启
#   3. BlueStore 损坏 → 需要重建 OSD

# 强制标记 OSD 为 down（如果 Pod 已无法恢复）
ceph osd down <osd-id>

# 如果确认 OSD 数据丢失，标记为 out 触发数据重平衡
ceph osd out <osd-id>
```
### 2.2 多个 OSD 批量宕机

```bash
# 批量检查所有 down 状态的 OSD
ceph osd tree | grep -E "down"

# 降低重平衡速度以减轻网络压力（在恢复期间）
ceph osd set noout          # 暂停自动踢出
ceph tell 'osd.*' injectargs '--osd-max-backfills=1 --osd-recovery-max-active=1'

# 逐个恢复后取消限制
ceph osd unset noout
ceph tell 'osd.*' injectargs '--osd-max-backfills=4 --osd-recovery-max-active=3'
```

### 2.3 BlueStore 损坏修复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 BlueStore 状态
ceph-objectstore-tool --data-path /var/lib/ceph/osd/ceph-<id> --op fsck

# 如果 fsck 失败，尝试修复
ceph-objectstore-tool --data-path /var/lib/ceph/osd/ceph-<id> --op repair

# 最后手段：重建 OSD（数据将从其他副本恢复）
# Rook-Ceph 环境下删除对应 PVC 让 Operator 自动重建
kubectl -n rook-ceph delete pvc <osd-pvc-name>
```
---

## 3. Mon 选举问题处理

### 3.1 Mon 状态检查

```bash
ceph mon stat
ceph mon dump

# 查看 Mon 节点 quorum 状态
ceph quorum_status --format json-pretty | jq '.quorum_names'
```

### 3.2 Mon 选举死锁

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 症状：ceph health 显示 mons down, quorum 半数以下存活

# 检查所有 Mon Pod
kubectl -n rook-ceph get pod -l app=rook-ceph-mon

# 如果多数 Mon Pod 不可用，优先恢复节点或重启 Pod
kubectl -n rook-ceph delete pod <mon-pod-name>  # 触发重建

# 如果 Mon 数据损坏，需要从存活的 Mon 重建
# 删除损坏的 Mon 的 PVC
kubectl -n rook-ceph delete pvc <mon-pvc-name>
# Rook Operator 会自动重建 Mon
```
### 3.3 强制恢复单 Mon 集群

```bash
# 仅用于单节点 Mon 全部丢失的灾难场景
# 1. 从 OSD 数据中提取 monmap
ceph-mon --extract-monmap /tmp/monmap --mon-data /var/lib/ceph/mon/ceph-<id>

# 2. 修改 monmap 移除不可用节点
monmaptool /tmp/monmap --rm <dead-mon-host>

# 3. 注入修复后的 monmap
ceph-mon --inject-monmap /tmp/monmap --mon-data /var/lib/ceph/mon/ceph-<id>
```

---

## 4. Longhorn 引擎故障恢复

### 4.1 引擎状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有卷的状态
kubectl -n longhorn-system get lhv -o wide

# 查看具体卷的引擎和副本状态
kubectl -n longhorn-system get lhv <volume-name> -o yaml

# 关键字段：
#   state: attached/detached/creating/degraded/faulted
#   robustness: healthy/degraded/faulted
```
### 4.2 引擎 Degraded 恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查副本健康状况
kubectl -n longhorn-system get lhr -o wide

# 强制分离故障卷
kubectl -n longhorn-system patch lhv <volume-name> \
  --type=json -p='[{"op":"replace","path":"/spec/nodeID","value":""}]'

# 等待卷 detach 后重新 attach 到健康节点
kubectl -n longhorn-system patch lhv <volume-name> \
  --type=json -p='[{"op":"replace","path":"/spec/nodeID","value":"<healthy-node>"}]'
```
### 4.3 引擎 Faulted 恢复

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Faulted 状态意味着所有副本数据可能损坏
# 1. 检查是否有可用的快照
kubectl -n longhorn-system get lhs -l longhornvolume=<volume-name>

# 2. 从快照恢复
kubectl apply -f - <<EOF
apiVersion: longhorn.io/v1beta2
kind: Volume
metadata:
  name: <volume-name>-recovered
  namespace: longhorn-system
spec:
  fromBackup: ""
  frontend: blockdev
  replicaAutoBalance: best-effort
  size: "<original-size>"
  numberOfReplicas: 2
EOF
```
---

## 5. PVC 数据抢救方法

### 5.1 确定 PVC 绑定状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 Pending PVC
kubectl get pvc -A | grep Pending

# 查看 PVC 事件
kubectl describe pvc <pvc-name> -n <namespace>
```
### 5.2 Ceph RBD 快照恢复

```bash
# 创建快照（如果 OSD 部分可用）
rbd snap create <pool>/<image>@emergency-snap

# 克隆到新镜像
rbd clone <pool>/<image>@emergency-snap <pool>/<image>-recovered

# 导出镜像到文件（最后手段）
rbd export <pool>/<image> /tmp/volume-backup.img
```

### 5.3 通过临时 Pod 挂载抢救数据

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建一个临时 Pod 挂载故障 PVC
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: data-rescue-pod
  namespace: <target-namespace>
spec:
  containers:
  - name: rescue
    image: busybox
    command: ["sleep", "3600"]
    volumeMounts:
    - name: data
      mountPath: /data
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: <faulted-pvc>
EOF

# 挂载成功后拷贝数据
kubectl cp <target-namespace>/data-rescue-pod:/data /tmp/rescue-data
```
### 5.4 Longhorn 备份恢复

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 列出可用备份
kubectl -n longhorn-system get backup -l longhornvolume=<volume-name>

# 从备份恢复为新卷
kubectl apply -f - <<EOF
apiVersion: longhorn.io/v1beta2
kind: Backup
metadata:
  name: restore-<timestamp>
  namespace: longhorn-system
spec:
  backupName: <backup-name>
  volumeName: <new-volume-name>
EOF
```
---

## 6. 生产最佳实践

### 6.1 Ceph 集群规划

| 配置项 | 推荐值 | 说明 |
|--------|--------|------|
| Mon 数量 | 3 或 5（奇数） | 保证 quorum 容忍单节点故障 |
| OSD 副本数 | 3 | 至少容忍 1 个 OSD 故障 |
| PG 数量 | OSD 数 × 100 / 副本数 | 避免 PG 过少导致热点 |
| noout 超时 | 600s | 给运维留足处理时间 |

### 6.2 Longhorn 配置建议

- 副本数至少设为 2，关键卷设为 3
- 启用定期快照和备份到 S3/NFS 后端
- 配置 `replicaAutoBalance: best-effort` 自动均衡副本分布
- 存储网络与业务网络隔离，避免 I/O 抢占

### 6.3 监控告警

```yaml
# Prometheus 告警规则示例
groups:
- name: ceph-alerts
  rules:
  - alert: CephHealthError
    expr: ceph_health_status == 2
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "Ceph 集群进入 HEALTH_ERR 状态"

  - alert: CephOSDDown
    expr: ceph_osd_up == 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "OSD {{ $labels.osd }} 已宕机超过 5 分钟"

  - alert: LonghornVolumeFaulted
    expr: longhorn_volume_robustness == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "Longhorn 卷 {{ $labels.volume }} 进入 faulted 状态"
```

---

## 7. 故障排查

### 7.1 Ceph 常见问题

| 症状 | 可能原因 | 处理方法 |
|------|---------|---------|
| `ceph -s` 卡住无响应 | Mon 全部不可达 | 检查 Mon Pod 和网络连通性 |
| OSD 反复 crash | 磁盘故障或内核 bug | 检查 `dmesg` 和 OSD 日志 |
| PG stuck stale | OSD 全部 down | 恢复至少一个 OSD 或标记 PG 丢失 |
| 写入 I/O 超时 | 磁盘满或网络分区 | `ceph osd df` 检查容量 |

### 7.2 Longhorn 常见问题

| 症状 | 可能原因 | 处理方法 |
|------|---------|---------|
| 卷 stuck in attaching | 节点无可用磁盘空间 | 清理空间或迁移卷 |
| 引擎频繁 restart | 内存不足 | 增加引擎 Pod 内存限制 |
| 备份失败 | S3 凭证过期或网络不通 | 检查 backup target 配置 |
| 副本 rebuild 慢 | 存储网络带宽不足 | 调整并发数和网络 QoS |

### 7.3 PVC Pending 排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 StorageClass
kubectl get sc

# 2. 检查 Provisioner Pod
kubectl -n <provisioner-ns> get pod -l app=csi-provisioner

# 3. 检查 PVC 事件
kubectl describe pvc <pvc-name> -n <namespace>

# 4. 检查 CSI 驱动日志
kubectl -n rook-ceph logs deploy/csi-rbdplugin-provisioner -c csi-provisioner --tail=50
```
---

## 参考链接

- [Ceph 官方文档 - 故障排查](https://docs.ceph.com/en/latest/rados/troubleshooting/)
- [Rook-Ceph 文档 - 灾难恢复](https://rook.io/docs/rook/latest/Troubleshooting/disaster-recovery/)
- [Longhorn 文档 - 故障恢复](https://longhorn.io/docs/latest/recover/)
- [Kubernetes CSI 规范](https://kubernetes-csi.github.io/docs/)


<!-- risk-assessed -->
