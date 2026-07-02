---
title: etcd 数据损坏检测与恢复全流程
description: 'etcd 数据损坏的检测方法、snapshot 恢复步骤、成员重建流程及数据校验验证'
summary: 'etcd 数据损坏的检测方法、snapshot 恢复步骤、成员重建流程及数据校验验证'
category: reliability-engineering
tags:
- disaster-recovery
- k8s
- sre
- etcd
- data-integrity
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
- etcd 数据损坏检测与恢复 是什么
- 如何 etcd 数据损坏检测与恢复
- etcd snapshot 恢复流程
trigger_keywords:
- etcd
- corruption
- snapshot
- restore
- data-integrity
prerequisites:
- kubectl-basics
- etcd-basics
- sre-practices
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

# etcd 数据损坏检测与恢复全流程

## 概述

etcd 是 Kubernetes 集群的核心数据存储层，所有集群状态（Secret、ConfigMap、Deployment、Service 等）均持久化在 etcd 中。当 etcd 数据发生损坏时，可能导致集群无法调度、API Server 拒绝请求甚至整个集群不可用。

本手册覆盖以下场景：

- **数据损坏检测**：通过 endpoint health、db consistency check 等手段识别损坏
- **Snapshot 恢复**：从定期备份的 snapshot 文件恢复集群状态
- **成员重建**：当单个 etcd 成员数据不可恢复时，如何安全替换
- **Learner 节点恢复**：使用 learner 模式安全加入新成员
- **数据校验验证**：恢复后的数据完整性与一致性验证

### 损坏类型分类

| 类型 | 症状 | 严重程度 |
|------|------|----------|
| 物理损坏（磁盘故障） | etcd 启动失败，日志报 `database file is corrupted` | P0 |
| 逻辑损坏（不一致） | 成员间 Raft 日志不一致，leader 选举失败 | P0 |
| 部分数据丢失 | 特定 key 读取异常，revision 不连续 | P1 |
| 成员间数据漂移 | 不同成员返回不同版本的数据 | P1 |

## 详细步骤

### 第一阶段：损坏检测

#### 1.1 etcd Endpoint 健康检查

```bash
# 检查所有 endpoint 健康状态
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health

# 输出示例（正常）：
# https://127.0.0.1:2379 is healthy: successfully committed proposal: took = 1.234ms

# 输出示例（异常）：
# https://127.0.0.1:2379 is unhealthy: context deadline exceeded
```

```bash
# 检查 endpoint 状态（包含 leader、DB 大小等详细信息）
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=table
```

#### 1.2 数据库一致性检查

```bash
# 停止 etcd 后执行离线一致性检查
# ⚠️ 此操作需要先停止 etcd 进程
systemctl stop etcd

# 执行离线数据校验
ETCDCTL_API=3 etcdctl check db \
  --data-dir=/var/lib/etcd \
  --initial-advertise-peer-urls=https://10.0.0.1:2380

# 输出示例（正常）：
# checking db...
# finished checking db, no errors

# 输出示例（损坏）：
# checking db...
# failed to check db: database file is corrupted
```

```bash
# 检查 Raft 日志一致性
ETCDCTL_API=3 etcdctl check datascale \
  --data-dir=/var/lib/etcd
```

#### 1.3 成员间数据对比

```bash
# 获取所有成员列表
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379,https://10.0.0.3:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member list --write-out=table

# 对比各成员的 revision（应一致）
for ep in https://10.0.0.1:2379 https://10.0.0.2:2379 https://10.0.0.3:2379; do
  echo "=== $ep ==="
  ETCDCTL_API=3 etcdctl \
    --endpoints=$ep \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key \
    endpoint status --write-out=json | jq '.[0].Status.header.revision'
done
```

#### 1.4 日志分析

```bash
# 检查 etcd 日志中的损坏关键词
journalctl -u etcd --since "1 hour ago" | grep -iE \
  "corrupt|inconsistent|data loss|hash mismatch|alarm|compaction"

# 常见错误日志：
# "etcdmain: database file is corrupted"
# "etcdserver: alarm activated: CORRUPT"
# "rafthttp: failed to read message on stream"
# "etcdserver: failed to purge snap file"
```

### 第二阶段：Snapshot 恢复

#### 2.1 确认是否有可用备份

```bash
# 检查自动备份（如使用 kubeadm 或自定义 cronjob）
ls -lh /var/lib/etcd-snapshot/

# 检查 Velero 备份（如已部署）
velero backup get | grep etcd

# 手动创建紧急备份（如 etcd 仍可读取）
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  snapshot save /tmp/emergency-snapshot-$(date +%Y%m%d%H%M%S).db
```

#### 2.2 验证 Snapshot 完整性

```bash
# 验证 snapshot 文件完整性
ETCDCTL_API=3 etcdctl snapshot status \
  /var/lib/etcd-snapshot/latest.db \
  --write-out=table

# 输出示例：
# +----------+----------+------------+------------+
# |   HASH   | REVISION | TOTAL KEYS | TOTAL SIZE |
# +----------+----------+------------+------------+
# | a1b2c3d4 |  1234567 |       5892 |     128 MB |
# +----------+----------+------------+------------+
```

#### 2.3 从 Snapshot 恢复（单节点）

```bash
# ⚠️ 以下操作会覆盖当前 etcd 数据，执行前确认已保存紧急备份

# 停止 etcd
systemctl stop etcd

# 备份当前数据目录
mv /var/lib/etcd /var/lib/etcd.broken.$(date +%Y%m%d%H%M%S)

# 从 snapshot 恢复
ETCDCTL_API=3 etcdctl snapshot restore \
  /var/lib/etcd-snapshot/latest.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3=https://10.0.0.3:2380 \
  --initial-advertise-peer-urls=https://10.0.0.1:2380 \
  --initial-cluster-token=etcd-cluster-k8s

# 设置正确的文件权限
chown -R etcd:etcd /var/lib/etcd

# 启动 etcd
systemctl start etcd
```

#### 2.4 从 Snapshot 恢复（集群模式 - 3 节点）

```bash
# 对每个 etcd 节点执行以下操作（按顺序逐个执行）

# 节点 1
ETCDCTL_API=3 etcdctl snapshot restore \
  /var/lib/etcd-snapshot/latest.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3=https://10.0.0.3:2380 \
  --initial-advertise-peer-urls=https://10.0.0.1:2380 \
  --initial-cluster-token=etcd-cluster-k8s-recovery

# 节点 2
ETCDCTL_API=3 etcdctl snapshot restore \
  /var/lib/etcd-snapshot/latest.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-2 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3=https://10.0.0.3:2380 \
  --initial-advertise-peer-urls=https://10.0.0.2:2380 \
  --initial-cluster-token=etcd-cluster-k8s-recovery

# 节点 3
ETCDCTL_API=3 etcdctl snapshot restore \
  /var/lib/etcd-snapshot/latest.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-3 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3=https://10.0.0.3:2380 \
  --initial-advertise-peer-urls=https://10.0.0.3:2380 \
  --initial-cluster-token=etcd-cluster-k8s-recovery

# 在所有节点上设置权限并启动
# chown -R etcd:etcd /var/lib/etcd
# systemctl start etcd
```

### 第三阶段：成员重建

#### 3.1 移除损坏的成员

```bash
# 查看当前成员列表
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member list --write-out=table

# 移除损坏的成员（假设 member ID 为 abc123）
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member remove abc123
```

#### 3.2 添加新成员

```bash
# 添加新成员（Learner 模式，推荐用于安全恢复）
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member add etcd-3-new \
  --learner=true \
  --peer-urls=https://10.0.0.3:2380

# 输出示例：
# Member abc123def456 added to cluster xyz789
# ETCD_NAME="etcd-3-new"
# ETCD_INITIAL_CLUSTER="etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3-new=https://10.0.0.3:2380"
# ETCD_INITIAL_CLUSTER_STATE="existing"
```

#### 3.3 Learner 节点恢复流程

```bash
# 在新节点上执行以下操作

# 清理旧数据
rm -rf /var/lib/etcd

# 创建 etcd 配置
cat > /etc/kubernetes/etcd/etcd.conf << EOF
ETCD_NAME=etcd-3-new
ETCD_DATA_DIR=/var/lib/etcd
ETCD_INITIAL_CLUSTER=etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3-new=https://10.0.0.3:2380
ETCD_INITIAL_CLUSTER_STATE=existing
ETCD_INITIAL_ADVERTISE_PEER_URLS=https://10.0.0.3:2380
ETCD_ADVERTISE_CLIENT_URLS=https://10.0.0.3:2379
ETCD_LISTEN_CLIENT_URLS=https://10.0.0.3:2379,https://127.0.0.1:2379
ETCD_LISTEN_PEER_URLS=https://10.0.0.3:2380
EOF

# 启动 etcd
systemctl start etcd

# 验证 learner 状态
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.3:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=table
```

#### 3.4 Learner 转正（Promote）

```bash
# 等待 learner 同步完成后转正
# 查看 learner 的同步状态
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member list --write-out=table

# 获取 learner 的 member ID（从上面输出中查找 isLearner=true 的条目）
# 假设 learner member ID 为 def789

# 将 learner 转为正式成员
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member promote def789

# 输出：Member def789 promoted in cluster xyz789
```

### 第四阶段：数据校验验证

#### 4.1 集群健康验证

```bash
# 验证所有 endpoint 健康
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379,https://10.0.0.3:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health --write-out=table

# 验证集群成员状态
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379,https://10.0.0.3:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=table
```

#### 4.2 Revision 一致性验证

```bash
# 对比所有节点的 revision（应完全一致）
for ep in https://10.0.0.1:2379 https://10.0.0.2:2379 https://10.0.0.3:2379; do
  REV=$(ETCDCTL_API=3 etcdctl \
    --endpoints=$ep \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key \
    endpoint status --write-out=json | jq -r '.[0].Status.header.revision')
  echo "$ep → revision: $REV"
done
```

#### 4.3 Kubernetes 资源完整性验证

```bash
# 验证核心资源可正常读取
kubectl get nodes
kubectl get namespaces
kubectl get pods -A
kubectl get secrets -n kube-system

# 验证 ServiceAccount token 可用
kubectl auth can-i --list

# 验证 Deployment、StatefulSet 等控制器状态
kubectl get deployments -A
kubectl get statefulsets -A

# 检查是否有异常的资源丢失
kubectl get all -A -o yaml | wc -l
```

#### 4.4 Alarm 清除

```bash
# 检查是否有未清除的 alarm
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  alarm list

# 清除 CORRUPT alarm（如果已通过 snapshot 恢复解决）
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  alarm disarm
```

## 生产最佳实践

### 备份策略

- **自动快照**：每小时自动创建 etcd snapshot，保留最近 72 小时
- **异地备份**：将 snapshot 复制到对象存储（S3/OSS/GCS），保留 30 天
- **备份验证**：每周执行一次备份恢复演练，确保备份可用

```bash
# 示例：CronJob 定期备份 etcd
# 使用 etcdctl snapshot save 结合 kubectl 创建 CronJob
```

### 监控告警

- 监控 etcd `etcd_server_has_leader` 指标，leader 丢失立即告警
- 监控 `etcd_disk_wal_fsync_duration_seconds`，P99 > 10ms 需关注磁盘性能
- 监控 `etcd_server_leader_changes_seen_total`，频繁 leader 切换需排查网络
- 监控 `etcd_mvcc_db_total_size_in_bytes`，接近 2GB 需执行 compaction

### 容量规划

| 参数 | 推荐值 | 说明 |
|------|--------|------|
| etcd 磁盘类型 | NVMe SSD | WAL fsync 延迟直接影响性能 |
| 磁盘空间 | ≥ 100GB | 需要 compaction + defrag 空间 |
| DB 大小告警阈值 | 2GB | 超过后执行 compaction + defrag |
| 成员数量 | 3 或 5 | 奇数个，容忍 (n-1)/2 故障 |

### 操作规范

- **Compaction**：定期执行 `etcdctl compact`，避免 MVCC 历史数据膨胀
- **Defrag**：compaction 后执行 `etcdctl defrag`，释放磁盘空间
- **禁用自动 compaction**：Kubernetes 默认 auto-compaction-retention=0，建议设置为 5m 或按需

## 故障排查

### 场景 1：etcd 启动失败报 `database file is corrupted`

```bash
# 原因：磁盘故障或异常断电导致数据文件损坏
# 解决：从 snapshot 恢复（参考第二阶段）
# 预防：使用 RAID 或云盘快照保护磁盘
```

### 场景 2：etcd 报 `CORRUPT alarm`

```bash
# 检查 alarm 状态
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  alarm list

# 原因：Raft 日志 hash 校验不一致，某成员数据可能被篡改或损坏
# 解决：
# 1. 确认哪个成员数据损坏（对比 revision）
# 2. 移除损坏成员并重建（参考第三阶段）
# 3. 清除 alarm
```

### 场景 3：成员间 revision 不一致

```bash
# 原因：网络分区导致部分写入未同步
# 解决：
# 1. 确认 leader 的 revision 是正确的
# 2. 停止 revision 异常的成员
# 3. 删除其数据目录并以 learner 重新加入
```

### 场景 4：etcd compaction 后空间未释放

```bash
# 执行在线 defrag
ETCDCTL_API=3 etcdctl \
  --endpoints=https://10.0.0.1:2379,https://10.0.0.2:2379,https://10.0.0.3:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  defrag

# 注意：defrag 会短暂阻塞该节点的请求，逐个执行
```

## 参考链接

- [etcd 官方文档 - Disaster Recovery](https://etcd.io/docs/latest/op-guide/recovery/)
- [etcd 官方文档 - Data Corruption](https://etcd.io/docs/latest/op-guide/data_corruption/)
- [Kubernetes etcd 备份与恢复](https://kubernetes.io/docs/tasks/administer-cluster/configure-upgrade-etcd/)
- [kubeadm etcd 备份最佳实践](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/)
- [CNCF etcd 最佳实践指南](https://www.cncf.io/blog/)

---

*本手册适用于 Kubernetes 1.28-1.32 版本。执行恢复操作前，请确保已备份当前数据。*
