---
title: etcd 维护专项文档
description: '**文档类型**: 运维维护手册 | **适用版本**: K8s 1.28-1.33 | **最后更新**: 2026-05'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- etcd
- prometheus
- job
- rag
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- etcd 维护专项文档 是什么
- 如何 etcd 维护专项文档
- etcd 维护专项文档 故障排查
- etcd 维护专项文档 排障步骤
trigger_keywords:
- etcd
- 维护专项文档
- structural
- trouble
- shooting
---


# etcd 维护专项文档

> **文档类型**: 运维维护手册 | **适用版本**: K8s 1.28-1.33 | **最后更新**: 2026-05
> **使用场景**: Agent 处理 etcd 相关运维问题（磁盘空间、备份恢复、领导选举、成员变更）

---

## 1. etcd 核心概念速查

### 1.1 关键指标

| 指标 | 正常值 | 告警阈值 | 说明 |
|------|--------|---------|------|
| `db.size` | < quota 的 50% | > quota 的 80% | 逻辑数据库大小 |
| `actual.db.size` | ≈ db.size | 远小于 db.size 说明有压缩需求 | 物理文件大小 |
| `wal.fsync.duration` | < 10ms | > 100ms | WAL 写入延迟（磁盘性能敏感） |
| `leader.footprint` | 稳定 | 持续增长 | 领导者复制压力 |
| `applied_index` | 持续增长 | 停滞 | 复制状态 |
| `commited_index` | ≥ applied_index | 差距持续扩大 | 提交状态 |

### 1.2 常用诊断命令

```bash
# 基本健康检查
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health

# 查看集群成员
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  member list -w table

# 查看 leader
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint status

# 检查配额
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  get "" --prefix=true --count-only  # 历史版本数量

# 查看 db 大小
du -sh /var/lib/etcd/
```

---

## 2. 故障场景

### 2.1 磁盘空间不释放（最常见）

**故障现象**: 删除大量历史资源后，`du -sh /var/lib/etcd/` 显示空间未减少，`db.size` 仍然很大

**根因**: etcd 使用 B+tree 存储，删除操作只标记 tombstone，不立即压缩空间

**排查步骤**：
```bash
# 1. 检查当前 db 大小
du -sh /var/lib/etcd/member/

# 2. 检查 logical vs physical size 差距
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  check datascale --write-out=table
# 如 "Actual db size" 远小于 "Total db size" 说明需要 defrag

# 3. 执行在线 defrag（不影响集群）
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  defrag --timeout=5m

# 4. 验证 defrag 效果
du -sh /var/lib/etcd/member/
```

**修复步骤**：
```bash
# 多节点集群：逐节点 defrag（不要同时！）
# 1. 在节点 1 执行 defrag
ETCDCTL_API=3 etcdctl --endpoints=https://node-1:2379 defrag

# 2. 等待完成后检查
du -sh /var/lib/etcd/

# 3. 确认无异常后再处理下一个节点
```

**自动 defrag 方案**：
```yaml
# 使用 etcd-backup-restore 工具自动 defrag
# 或配置 etcd-exporter + Prometheus 告警触发 defrag
```

---

### 2.2 etcd space quota exceeded

**故障现象**: API Server 报 "etcdserver: mvcc: database space exceeded"，写入被拒绝

**排查步骤**：
```bash
# 1. 确认配额状态
etcdctl --endpoints=https://127.0.0.1:2379 endpoint status | grep -i quota

# 2. 检查当前使用量
COMPACT_REV=$(ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  get "" --prefix=true --keys-only | tail -1 | awk '{print $1+1}')

echo "Current revision: $COMPACT_REV"
```

**修复步骤**：
```bash
# 紧急扩容配额（不推荐长期使用）
# 编辑 etcd 配置 /etc/kubernetes/etcd.config.yaml
# 添加: quota-backend-bytes: 17179869184  (16GB)

# 标准修复：
# 1. Compact 历史版本（保留最近 N 个版本）
COMPACT_REV=$(ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  get "" --prefix=true --keys-only | tail -1 | awk '{print $1+1}')

ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  compact $COMPACT_REV

# 2. defrag
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  defrag

# 3. 解除 alarm
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  alarm disarm

# 4. 验证
kubectl get pods -n kube-system | grep etcd  # 确认所有 etcd pod 健康
```

**预防措施**：
```bash
# 配置定期 compact（建议每周一次）
# 使用 cron 或 Kubernetes Job
0 3 * * 0 ETCDCTL_API=3 etcdctl defrag --endpoints=https://127.0.0.1:2379 ...
```

---

### 2.3 Leadership election 失败（leader 频繁切换）

**故障现象**: etcd 日志显示 "raft term changed" 或 "lost leader"，集群不稳定

**排查步骤**：
```bash
# 1. 查看当前 leader
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 endpoint status -w json | jq '.leader'

# 2. 检查网络延迟（etcd 对网络延迟极敏感）
ping -c 20 <other-etcd-node-ip>
# 期望 RTT < 5ms

# 3. 检查磁盘 I/O 延迟（etcd 要求 WAL 写入 < 10ms）
iostat -x 1 10
# avgqu-sz > 1 说明有 I/O 等待

# 4. 检查 CPU 使用率
top
# 高 CPU 导致 heartbeat 延迟
```

**修复步骤**：
```bash
# 方案 1: 降低心跳间隔（临时）
# 编辑 /etc/kubernetes/etcd.config.yaml
# 添加: heartbeat-interval: 500  (默认 1000ms)
# 重启 etcd: systemctl restart etcd

# 方案 2: 使用更快磁盘（SSD NVMe）
# 将 /var/lib/etcd 迁移到 NVMe 盘

# 方案 3: 转移 leader 到稳定节点
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  move-leader <new-leader-ip>
```

---

### 2.4 Member add 失败

**故障现象**: `etcdctl member add` 成功但新节点无法加入，日志报 "conflicting cluster ID" 或 "peer cluster not found"

**排查步骤**：
```bash
# 1. 查看现有成员
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 member list -w table

# 2. 检查新节点的 cluster 配置是否与现有集群一致
# 在新节点上:
cat /etc/kubernetes/etcd/etcd.conf.yaml | grep ETCD_INITIAL_CLUSTER
# 确认与现有集群的 INITIAL_CLUSTER 一致

# 3. 清理新节点的残留数据
sudo rm -rf /var/lib/etcd/
sudo systemctl restart etcd
```

**正确的 add member 流程**：
```bash
# 1. 在现有集群添加新成员
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  member add <new-node-name> --peer-urls=http://<new-node-ip>:2380

# 2. 记录返回的启动命令（包含新的 cluster token）

# 3. 在新节点执行（使用新 token）
kubeadm join phase etcd https://<existing-ip>:2379 --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --control-plane --certificate-key <cert-key>
```

---

### 2.5 Member remove 失败（节点退役）

**故障现象**: `etcdctl member remove` 卡住或超时

**排查步骤**：
```bash
# 1. 查看 member 状态
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 member list -w table

# 2. 如果 member 处于 unstarted 状态，直接从 cluster 移除
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  member remove <member-id>

# 3. 如果 remove 超时，使用 --force 选项
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  member remove <member-id> --force

# 4. 在被移除的节点上清理残留
sudo rm -rf /var/lib/etcd/
sudo systemctl stop etcd
```

---

### 2.6 Snapshot backup 验证失败

**故障现象**: `etcdctl snapshot save` 成功，但 restore 时报错 "snapshot file is not valid"

**排查步骤**：
```bash
# 1. 检查 snapshot 文件完整性
ls -lh /backup/etcd-*.db

# 2. 验证 snapshot 元数据
ETCDCTL_API=3 etcdctl --write-out=table snapshot status /backup/etcd-latest.db
# 期望: hash 不为空，revision > 0

# 3. 检查 snapshot 的 cluster ID
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-latest.db -w json | jq '.metadata.cluster_id'

# 4. 与当前集群的 cluster ID 对比
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 endpoint status -w json | jq '.[0].metadata.cluster_id'

# 5. 如 cluster ID 不匹配，说明 snapshot 来自不同的集群，不能直接 restore
```

**正确的 backup/restore 流程**：
```bash
# ========== BACKUP ==========
# 1. 创建 snapshot
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d-%H%M%S).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 2. 验证 snapshot
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-latest.db -w table
# 期望: hash 不为空，revision > 0，db-size > 0

# ========== RESTORE ==========
# 3. 停止 etcd
systemctl stop etcd

# 4. 备份现有数据（重要）
sudo mv /var/lib/etcd /var/lib/etcd-old-$(date +%Y%m%d)

# 5. 从 snapshot 恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-latest.db \
  --data-dir=/var/lib/etcd \
  --name=<node-name> \
  --initial-cluster=<cluster-init> \
  --initial-cluster-token=<token> \
  --initial-advertise-peer-urls=http://<node-ip>:2380

# 6. 启动 etcd
systemctl start etcd

# 7. 验证
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 endpoint health
```

---

## 3. 性能调优

### 3.1 磁盘 I/O 优化

```bash
# 检查当前 I/O 延迟
iostat -x 1 10 | grep -E "avgqu-sz|avg-cpu"
# avgqu-sz > 1 且 avg-cpu > 80% 说明磁盘瓶颈

# 建议存储配置
# - 使用 SSD NVMe（延迟 < 1ms）
# - 禁止 noatime（减少写入）
# - 隔离 WAL 日志到独立磁盘（避免写入竞争）
```

### 3.2 网络优化

```bash
# 检查节点间 RTT 延迟
ping -c 50 <other-etcd-node-ip> | tail -1
# 目标: RTT < 1ms（同机房），< 5ms（跨机房）

# 避免跨地域部署 etcd（延迟太高影响写入性能）
```

### 3.3 参数调优

```yaml
# /etc/kubernetes/etcd.config.yaml 关键参数
max-snapshots: 5          # 快照保留数量
max-wals: 5                # WAL 保留数量
quota-backend-bytes: 8589934592  # 8GB（根据磁盘容量调整）
heartbeat-interval: 500   # 心跳间隔（ms）
election-timeout: 5000     # 选举超时（ms）
snapshot-count: 10000      # 触发快照的事务数
auto-compaction-mode: periodic  # 定期压缩
auto-compaction-retention: "1h"  # 保留 1 小时历史
```

---

## 4. 监控指标

### 4.1 Prometheus 告警规则

```yaml
groups:
- name: etcd-alerts
  rules:
  - alert: EtcdDatabaseQuotaExceeded
    expr: etcd_mvcc_db_total_size_in_bytes > etcd_server_quota_backend_bytes * 0.8
    for: 1m
    labels:
      severity: critical
    annotations:
      description: "etcd database usage exceeds 80% of quota"

  - alert: EtcdDbSizeGrowingRapidly
    expr: rate(etcd_mvcc_db_total_size_in_bytes[5m]) > 100000000
    for: 5m
    labels:
      severity: warning
    annotations:
      description: "etcd db size growing rapidly"

  - alert: EtcdHighFsyncDuration
    expr: histogram_quantile(0.99, rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) > 0.1
    for: 5m
    labels:
      severity: warning
    annotations:
      description: "etcd WAL fsync duration > 100ms"

  - alert: EtcdHighCommitDuration
    expr: histogram_quantile(0.99, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m])) > 0.25
    for: 5m
    labels:
      severity: warning
    annotations:
      description: "etcd backend commit duration > 250ms"
```

---

## 5. 故障排查决策树

```
etcd 异常
  ├── 无法写入 (space exceeded)
  │   ├── compact + defrag + alarm disarm
  │   └── 扩容 quota-backend-bytes（紧急）
  ├── 磁盘空间不释放
  │   ├── defrag 即可
  │   └── 定期 defrag 任务
  ├── Leader 频繁切换
  │   ├── 检查网络延迟 (ping)
  │   ├── 检查磁盘 I/O (iostat)
  │   └── 转移 leader
  ├── 新节点无法加入
  │   ├── 清理残留数据
  │   └── 检查 INITIAL_CLUSTER 配置
  └── Snapshot 恢复失败
      ├── 验证 snapshot 完整性
      └── 检查 cluster ID 是否匹配
```

---

```yaml
---
id: ETCD-MAINTENANCE-001
domain: control-plane
type: maintenance-guide
tags: [etcd, maintenance, disk-space, backup-restore, leader-election, agent-corpus, k8s-1.28-1.33]
intent_queries:
  - "etcd 磁盘空间不释放怎么办"
  - "etcd space quota exceeded 怎么处理"
  - "etcd snapshot backup 怎么验证"
  - "etcd leader 频繁切换怎么排查"
  - "etcd member add 失败怎么解决"
difficulty: advanced
target_roles: [sre, ops-engineer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - domain-3-control-plane/11-etcd-deep-dive.md
  - domain-3-control-plane/10-plane-backup-disaster-recovery.md
  - topic-fta/list/etcd-fta.md
---
```