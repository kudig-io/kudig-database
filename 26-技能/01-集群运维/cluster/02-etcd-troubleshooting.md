---
title: etcd 集群故障诊断与恢复
description: 针对 etcd 失去 quorum、性能劣化、磁盘配额超限、数据损坏与备份恢复的完整诊断技能，含症状识别、快速分级、证据三元组、修复操作与灾难恢复流程
summary: etcd 是 Kubernetes 唯一的持久化状态存储，etcd 故障直接导致集群写失败或完全不可用。本技能提供从健康检查到灾难恢复的生产级路径
category: skill
tags:
- k8s
- cluster
- etcd
- quorum
- backup
- restore
- defrag
- compaction
- troubleshooting
- sop
- runbook
sources:
- 故障诊断/FTA故障树/kubernetes-fta-full-analysis.md
- code/etcd-3.7.0/
- code/apiserver-master/
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- etcd 故障如何恢复
- etcd 失去 quorum 怎么办
- etcd database space exceeded 如何处理
- etcd 备份与恢复步骤
- 集群写操作全部失败什么原因
trigger_keywords:
- etcd
- quorum
- 法定人数
- mvcc database space exceeded
- etcd 备份
- etcd 恢复
- snapshot restore
- defrag
- compaction
- 写操作失败
prerequisites:
- kubectl-basics
- etcd-basics
- cluster-architecture
skill_id: SKILL-CLUSTER-002
skill_name: etcd 集群故障诊断与恢复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.30.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L1-manual-first
fta_path: TE-C -> IE-C.2 -> BE-C.2/BE-C.3
---

> **生产环境安全提示**
>
> 命令风险等级标注：🔴 高风险、🟡 中风险、🟢 低风险/只读。
>
> **etcd 数据操作最高警告**：etcd 存储集群全部状态。`snapshot restore`、`defrag`、成员增删等操作若误用将导致**不可逆数据丢失**。任何写操作前**必须先做 snapshot 备份**，并由高级工程师双人复核。本技能执行模式 **L1-manual-first**，恢复动作严禁 Agent 自动执行。

# etcd 集群故障诊断与恢复

> **Skill ID**: SKILL-CLUSTER-002
> **Agent 执行模式**: L1-manual-first
> **FTA 路径**: TE-C → IE-C.2 → BE-C.2（quorum）/ BE-C.3（配额）

---

## 1. 概述

etcd 是 Kubernetes 唯一的持久化状态存储，采用 Raft 一致性协议，需**多数派（quorum）**存活才能写入。3 节点集群容忍 1 节点故障，5 节点容忍 2 节点。etcd 故障表现为集群写操作失败甚至完全不可用。

**覆盖范围**：失去 quorum、Raft 选主抖动、性能劣化（磁盘慢/网络慢）、`mvcc: database space exceeded` 配额超限、碎片膨胀、数据损坏、备份与灾难恢复。

**前置条件**：控制面节点 SSH 权限、etcd 客户端证书、`etcdctl` 工具。

**边界**：apiserver 本身问题 → [01-apiserver-controlplane.md](01-apiserver-controlplane.md)。

---

## 2. 症状识别

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 写操作全部失败，读可能正常 | `kubectl apply` 超时，`kubectl get` 部分可用 | 0.85 | 需确认非 apiserver 问题 |
| S2 | etcd 报 `mvcc: database space exceeded` | apiserver/etcd 日志 | 0.95 | 配额超限（默认 2GB/8GB） |
| S3 | etcd endpoint health 不健康 | `etcdctl endpoint health` | 0.90 | 单成员不健康未必失 quorum |
| S4 | 无 leader / leader 频繁切换 | `etcdctl endpoint status` LEADER 列 | 0.85 | 网络抖动/磁盘慢导致 |
| S5 | etcd 请求延迟高 | `etcd_disk_wal_fsync_duration` P99 高 | 0.80 | 磁盘 IO 瓶颈 |
| S6 | 成员数不足多数派 | `etcdctl member list` 存活数 | 0.95 | 失去 quorum 判定关键 |

---

## 3. 快速分级

| 级别 | 判定条件 | 响应时限 | 处置 |
|:---:|---------|:---:|------|
| **P0** | 失去 quorum（存活成员 < 多数派），集群无法写 | 立即 | 灾难恢复流程，双人复核 |
| **P1** | 单成员故障但仍有 quorum；或配额超限拒写 | ≤15min | 恢复成员 / defrag + 解除告警 |
| **P2** | 性能劣化，延迟高但可用 | ≤1h | 排查磁盘/网络，compaction+defrag |
| **P3** | 单次抖动已恢复 | ≤1d | 观察 fsync 延迟指标 |

> **quorum 判定**：3 成员存活 ≥2 为健康；存活 ≤1 为 **P0 失去 quorum**。

---

## 4. 诊断工作流

### Phase 1: 快速定位（只读）

**D1.1**: 成员健康与状态（etcdctl 别名建议先导出证书环境变量）

```bash
# 🟢 低风险：只读
export ETCDCTL_API=3
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

etcdctl member list -w table
etcdctl endpoint health --cluster
etcdctl endpoint status --cluster -w table
```

- 关注 `endpoint status` 的：LEADER（是否有 leader）、DB SIZE、RAFT TERM/INDEX 是否一致。

**D1.2**: 确认配额告警（S2 分支）

```bash
# 🟢 低风险：只读
etcdctl alarm list
# 输出 NOSPACE 即配额超限
```

### Phase 2: 深度检查（只读）

**D2.1**: 磁盘/网络性能

```bash
# 🟢 低风险：只读
etcdctl endpoint status -w json | grep -o '"dbSize":[0-9]*'
# WAL fsync 延迟（Prometheus）见证据三元组
```

**D2.2**: 数据库大小与碎片

```bash
# 🟢 低风险：只读
etcdctl endpoint status --cluster -w table   # 看 DB SIZE 是否接近配额
```

### 4.6 证据三元组

```promql
# 🟢 etcd 有无 leader（0 表示无 leader）
etcd_server_has_leader == 0

# 🟢 leader 切换频率
rate(etcd_server_leader_changes_seen_total[5m]) > 0

# 🟢 WAL fsync 延迟 P99（> 10ms 需关注磁盘）
histogram_quantile(0.99, rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) > 0.01

# 🟢 db 大小接近配额
etcd_mvcc_db_total_size_in_bytes / etcd_server_quota_backend_bytes > 0.9
```

| 维度 | 来源 | 取值 |
|------|------|------|
| Metrics | etcd /metrics | has_leader、leader_changes、fsync 延迟、db size |
| Logs | etcd 容器日志 | `mvcc: database space exceeded` / `slow fdatasync` / `lost leader` |
| Events | apiserver 日志 | `etcdserver: request timed out` |

---

## 5. 根因分类

| RC-ID | 根因 | 概率 | 关键证据 | FTA | 修复 | 风险 |
|-------|------|------|---------|-----|------|------|
| RC-001 | 配额超限 (NOSPACE) | 28% | `mvcc: database space exceeded` | BE-C.3 | compaction+defrag+解除告警 | 🟡 |
| RC-002 | 失去 quorum（多成员宕机） | 22% | 存活成员 < 多数派 | BE-C.2 | 从快照灾难恢复 | 🔴 |
| RC-003 | 磁盘 IO 慢 | 18% | fsync P99 高 | BE-C.2 | 换 SSD/隔离 IO | 🟡 |
| RC-004 | 碎片膨胀 | 12% | db size 大但 key 少 | BE-C.3 | defrag | 🟡 |
| RC-005 | 网络分区/延迟 | 10% | leader 频繁切换 | BE-C.2 | 修网络 | 🟡 |
| RC-006 | 单成员数据损坏 | 6% | 成员启动报 corrupt | BE-C.2 | 移除并重建成员 | 🔴 |
| RC-007 | 证书过期 | 4% | etcd 日志 x509 | BE-C.4 | 轮换 etcd 证书（转 03） | 🟡 |

---

## 6. 修复操作

**REM-001（🟡 中风险）：compaction + defrag + 解除 NOSPACE 告警**

```bash
# 🟡 中风险：defrag 期间该成员短暂不可用，逐个成员串行执行
# 1. 获取当前 revision 并压缩历史
rev=$(etcdctl endpoint status -w json | grep -o '"revision":[0-9]*' | head -1 | cut -d: -f2)
etcdctl compact $rev
# 2. 碎片整理（逐成员，避免同时执行）
etcdctl defrag --cluster
# 3. 解除 NOSPACE 告警
etcdctl alarm disarm
```

**REM-002（🔴 高风险，需高级审批）：从快照灾难恢复（失去 quorum）**

```bash
# 🔴 高风险：全集群数据回滚到快照时刻，操作前确认快照完整性
# 前置：停止所有 etcd 与 apiserver 静态 Pod
# 1. 用最新快照在每个成员恢复数据目录（示例单成员）
etcdutl snapshot restore /var/lib/etcd/backup/snapshot.db \
  --name=<member-name> \
  --initial-cluster=<member1=https://ip1:2380,...> \
  --initial-advertise-peer-urls=https://<ip>:2380 \
  --data-dir=/var/lib/etcd-restored
# 2. 更新 etcd 静态 Pod 指向新 data-dir，逐节点拉起
# 3. 恢复 apiserver 静态 Pod，验证集群
```

**定期备份（🟢 低风险，预防）**

```bash
# 🟢 低风险：只读备份
etcdctl snapshot save /var/lib/etcd/backup/snapshot-$(date +%F-%H%M).db
etcdctl snapshot status /var/lib/etcd/backup/snapshot-*.db -w table
```

> 🔴 灾难恢复是最后手段。恢复会丢失快照时刻之后的所有变更；执行前务必确认 quorum 确实无法通过恢复成员重建。

---

## 7. 验证确认

| 阶段 | 判据 | 通过标准 |
|------|------|---------|
| 即时验证 | `etcdctl endpoint health --cluster` | 全部 healthy 且有 leader |
| 短期监控 | `etcd_server_has_leader` | 持续 == 1，无频繁切换 |
| 解决标准 | 集群写操作恢复 | `kubectl apply` 成功；NOSPACE 告警清除 |
| 回归检测 | db size 与 fsync 延迟 | db size 稳定，fsync P99 < 10ms |

---

## 8. 升级协议

- 失去 quorum（P0）→ 立即升级 etcd/平台专家，禁止 Agent 任何写操作。
- 配额超限（P1）→ 可按 REM-001 处置，defrag 需在低峰串行执行。
- 交接信息包：`member list`/`endpoint status` 表、`alarm list`、最新快照路径与 `snapshot status`、fsync 延迟曲线。

---

## 9. 版本兼容矩阵

> 基于 `code/etcd-3.7.0` 快照。

| 特性 | etcd 3.4 | 3.5 | 3.6/3.7 | 说明 |
|------|:----:|:----:|:----:|------|
| `etcdctl snapshot restore` | ✅ | ✅ | ⚠️ 迁移至 `etcdutl` | 3.6+ 恢复命令改用 `etcdutl snapshot restore` |
| 默认配额 backend | 2GB | 2GB | 可配置 | 生产建议显式设 `--quota-backend-bytes=8589934592` |
| `defrag --cluster` | ✅ | ✅ | ✅ | 全版本可用，需串行执行 |

> [存疑：`etcdctl` 与 `etcdutl` 的精确拆分版本以 etcd 官方发布说明为准；不同 K8s 发行版内置 etcd 版本不同，须以节点实际 `etcdctl version` 为准]

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊 | 纠正 |
|------|------|
| db size 大直接删数据 | 应先 compaction 再 defrag，切勿手动删 data-dir |
| 单成员不健康即灾难恢复 | 有 quorum 时应恢复单成员，不要全量 restore |
| defrag 全成员并行 | 会同时不可用致失 quorum，必须串行 |

### 10.2 生产案例

**案例: etcd 配额超限导致集群停写**

| 时间 | 事件 |
|------|------|
| T0 | 大量 Event/ConfigMap 写入，etcd db 达 2GB |
| T1 | apiserver 报 `etcdserver: mvcc: database space exceeded` |
| 根因 | 未启用自动 compaction，历史版本堆积（RC-001） |
| 修复 | 🟡 compact 到当前 revision + defrag + alarm disarm，并配置自动 compaction |

### 10.3 混沌验证

| 注入场景 | 方法（测试集群） | 应命中 | 验证标准 |
|---------|----------------|-------|---------|
| 失去 quorum | 停止 3 成员中的 2 个 | RC-002 | 写失败，has_leader=0 |
| 配额超限 | 设小 quota 并灌数据 | RC-001 | NOSPACE 告警 |

---

## 11. 云厂商特异性

| 厂商 | 差异 |
|------|------|
| 阿里云 ACK | 托管集群 etcd 由平台运维与备份，用户无需直接操作；专有版可访问 |
| AWS EKS | etcd 完全托管，用户不可见 |
| 自建 kubeadm | etcd 为静态 Pod，需自行配置备份与监控 |

---

## 12. 自动化集成接口

```json
{
  "skill_id": "SKILL-CLUSTER-002",
  "symptom": "etcd_quorum_lost",
  "alive_members": 1,
  "quorum": 2,
  "root_cause": "RC-002",
  "action": "escalate_disaster_recovery",
  "requires_approval": true,
  "risk": "critical"
}
```

- 🟢 自动执行：所有只读健康检查、快照备份
- 🔴 禁止自动：defrag、compact、snapshot restore、成员增删

---

## 相关链接

- [[26-技能/01-集群运维/cluster/README.md|Cluster 集群级故障诊断技能集]]
- [[26-技能/01-集群运维/cluster/01-apiserver-controlplane.md|控制平面不可用诊断]]
- [[26-技能/01-集群运维/cluster/03-cluster-cert-upgrade.md|证书与升级]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[etcd]] — 集群数据存储
- [[kube-apiserver]] — API Server
