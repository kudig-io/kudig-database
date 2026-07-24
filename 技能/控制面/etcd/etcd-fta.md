---
title: etcd 异常诊断技能
description: etcd 集群异常的完整故障诊断技能，覆盖成员可用性、Raft 共识、磁盘 IO、网络与时钟、证书与访问控制、性能与碎片化等场景
summary: etcd 故障诊断，覆盖成员/Raft/磁盘/网络/证书/性能 6 大类 15+ 根因
category: skill
tags:
- k8s
- etcd
- raft
- control-plane
- troubleshooting
- fta
- cluster
- performance
sources:
- 故障诊断/FTA故障树/list/etcd-fta.md
- 故障诊断/高级排障/structural-05-control-plane-components/
- code/etcd-3.7.0/
created: '2026-05-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- etcd 集群不可用怎么排查
- etcd 性能劣化如何诊断
- etcd 磁盘空间不足怎么处理
- etcd 证书过期如何修复
- etcd leader 选举异常排查
trigger_keywords:
- etcd
- Raft
- leader
- quorum
- WAL
- fsync
- 碎片化
- 证书过期
- etcdctl
prerequisites:
- kubectl-basics
- etcd-basics
- linux-io-basics
fta_id: FTA-ETCD-001
component: etcd
severity: critical
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# etcd 异常诊断技能

## 1. 概述

### 覆盖范围

本技能覆盖 etcd 在生产环境中的全部常见故障：

- **成员可用性**：成员宕机、leader 选举异常、脑裂
- **磁盘与 IO**：磁盘满、WAL fsync 延迟、数据损坏
- **网络与时钟**：成员间网络问题、时钟漂移、端口阻断
- **证书与访问控制**：证书过期、认证失败、权限不足
- **性能与碎片化**：碎片化、请求压力、压缩问题

### 适用场景

| 适用 | 不适用 |
|------|--------|
| etcd 集群不可用/性能劣化 | 应用层 KV 使用错误 |
| API Server 连接 etcd 失败 | 非 etcd 原因的 API Server 故障 |
| etcd 磁盘/网络/证书问题 | 操作系统级硬件故障（非 etcd 层面） |
| Raft 共识异常 | etcd 版本升级操作（→ 升级 SOP） |

### 前置条件

- 具备 etcd 节点 SSH 权限
- 持有 etcd 客户端证书（`/etc/kubernetes/pki/etcd/`）
- 了解集群 etcd 拓扑（3/5 节点、静态 Pod 或独立部署）

---

## 2. 症状识别

| 症状 ID | 症状描述 | 工单关键词 | 确认命令 |
|---------|---------|-----------|---------|
| S1 | API Server 报 etcd 连接超时 | "etcd timeout"、"连接失败" | `kubectl get --raw /healthz/etcd` |
| S2 | etcdctl 命令执行超时/无响应 | "etcd 慢"、"命令卡住" | `etcdctl endpoint status --write-out=table` |
| S3 | etcd 日志频繁 leader changed | "选举"、"leader 切换" | `journalctl -u etcd | grep "leader changed"` |
| S4 | etcd 报 database space exceeded | "空间不足"、"NOSPACE" | `etcdctl endpoint status` 查看 DB SIZE |
| S5 | kubectl 命令响应极慢 | "集群卡"、"API 慢" | `kubectl get --raw /healthz?verbose` |
| S6 | etcd 成员 unhealthy | "成员异常"、"节点掉线" | `etcdctl endpoint health --cluster` |

### 排除标准

- 若仅 API Server 慢但 etcd 健康 → 转 API Server 排查
- 若节点 NotReady 导致 etcd Pod 异常 → 先恢复节点
- 若网络策略阻断 API Server → etcd 端口 → 转网络排查

---

## 3. 快速分级

| 严重性 | 定义 | 响应策略 |
|--------|------|---------|
| P0 | etcd 集群失去 quorum（多数成员不可用） | 立即恢复，5min 内响应，全员上线 |
| P1 | 单成员异常/性能严重劣化（> 500ms 延迟） | 15min 内隔离异常成员 |
| P2 | 磁盘空间告警/碎片化偏高 | 计划维护窗口处理 |
| P3 | 证书即将过期/性能轻微下降 | 提前规划变更 |

---

## 4. 诊断工作流

### Phase 1：快速检查（< 2 分钟）

#### D1.1 检查 etcd 集群健康状态

```bash
# 🟢 低风险：只读/信息收集
ETCDCTL_API=3 etcdctl endpoint health --cluster \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```

**判断逻辑**：
- 全部 healthy → etcd 本身正常，排查 API Server 连接
- 部分 unhealthy → 转对应成员深度检查
- 全部 unhealthy → P0，立即升级

#### D1.2 检查 etcd 成员状态

```bash
# 🟢 低风险：只读/信息收集
ETCDCTL_API=3 etcdctl endpoint status --cluster --write-out=table \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```

**关键指标**：
- `IS LEADER`：确认 leader 分布
- `DB SIZE`：数据库大小（告警阈值通常 2GB/8GB）
- `RAFT TERM`：term 频繁变化说明选举不稳定

#### D1.3 检查 etcd Pod 状态（静态 Pod 部署）

```bash
# 🟢 低风险：只读/信息收集
kubectl get pods -n kube-system -l component=etcd -o wide
kubectl logs -n kube-system etcd-<node-name> --tail=50 | grep -E "error|warn|panic"
```

### Phase 2：深度检查（< 10 分钟）

#### D2.1 磁盘 IO 性能检查

```bash
# 🟢 低风险：只读（需在 etcd 节点执行）
# WAL fsync 延迟（应 < 10ms）
iostat -x 1 5 | grep -E "Device|sda|nvme"
# etcd 磁盘使用
df -h /var/lib/etcd
du -sh /var/lib/etcd/member/
```

#### D2.2 网络连通性检查

```bash
# 🟢 低风险：只读
# peer 端口连通性
curl -k https://<peer-ip>:2380/health
# 网络延迟
ping -c 10 <peer-ip>
# 时钟同步
chronyc tracking  # 或 timedatectl status
```

#### D2.3 证书有效期检查

```bash
# 🟢 低风险：只读
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates
openssl x509 -in /etc/kubernetes/pki/etcd/peer.crt -noout -dates
openssl x509 -in /etc/kubernetes/pki/etcd/ca.crt -noout -dates
```

#### D2.4 碎片化与压缩检查

```bash
# 🟢 低风险：只读
ETCDCTL_API=3 etcdctl endpoint status --write-out=table  # 对比 DB SIZE vs DB SIZE IN USE
# 检查自动压缩配置
cat /etc/kubernetes/manifests/etcd.yaml | grep -E "auto-compaction|quota-backend"
```

### Phase 3：主动探测（需审批）

#### D3.1 手动压缩与碎片整理

```bash
# 🔴 高风险：压缩不可逆，碎片整理期间短暂阻塞写入
# 获取当前 revision
rev=$(ETCDCTL_API=3 etcdctl endpoint status --write-out="json" | jq '.[0].Status.header.revision')
# 压缩
ETCDCTL_API=3 etcdctl compact $rev
# 碎片整理（逐成员执行）
ETCDCTL_API=3 etcdctl defrag --endpoints=https://<member-ip>:2379
```

#### D3.2 成员移除与重新加入

```bash
# 🔴 高风险：操作不当可能破坏 quorum
ETCDCTL_API=3 etcdctl member list --write-out=table
ETCDCTL_API=3 etcdctl member remove <member-id>
# 重新加入需更新 --initial-cluster 配置
```

---

## 5. 根因分类

| 编号 | 根因 | 概率 | 关键证据 | FTA 映射 |
|------|------|------|----------|---------|
| RC-001 | 成员进程 OOM/崩溃 | 高 | 进程不存在/OOM 日志 | TE→IE-1→BE-1.1 |
| RC-002 | 节点宕机导致成员不可用 | 中 | 节点 NotReady + etcd Pod 消失 | TE→IE-1→BE-1.2 |
| RC-003 | Leader 选举超时/频繁切换 | 中 | 日志 "leader changed" 频繁 | TE→IE-1→BE-1.3 |
| RC-004 | 网络分区导致脑裂 | 低 | 多 leader/term 不一致 | TE→IE-1→BE-1.4 |
| RC-005 | 磁盘空间满/quota 超限 | 高 | "database space exceeded"/NOSPACE | TE→IE-2→BE-2.1 |
| RC-006 | WAL fsync 延迟过高 | 高 | "took too long for a write" 日志 | TE→IE-2→BE-2.2 |
| RC-007 | WAL/数据库文件损坏 | 低 | "database file is corrupted" | TE→IE-2→BE-2.3 |
| RC-008 | 成员间网络延迟/丢包 | 中 | peer 通信超时 | TE→IE-3→BE-3.1 |
| RC-009 | 时钟漂移超过容忍 | 中 | NTP 异常 + 选举超时 | TE→IE-3→BE-3.2 |
| RC-010 | peer/client 端口被防火墙阻断 | 中 | 2379/2380 端口不通 | TE→IE-3→BE-3.3 |
| RC-011 | 证书过期 | 高 | x509 certificate has expired | TE→IE-4→BE-4.1 |
| RC-012 | 客户端认证失败 | 中 | "authentication failed" | TE→IE-4→BE-4.2 |
| RC-013 | 碎片化严重（长期未压缩） | 中 | DB SIZE >> DB SIZE IN USE | TE→IE-5→BE-5.1 |
| RC-014 | Watch 连接过多/请求峰值 | 中 | 连接数异常/延迟飙升 | TE→IE-5→BE-5.2 |
| RC-015 | 自动压缩未启用 | 中 | 无 auto-compaction 参数 | TE→IE-5→BE-5.3 |

---

## 6. 修复操作

| 编号 | 对应根因 | 修复操作 | 风险等级 | 审批要求 |
|------|---------|---------|:--------:|---------|
| REM-001 | RC-001 | 增加 etcd 内存限制，重启 etcd Pod | 🟡 | 无需 |
| REM-002 | RC-002 | 恢复节点，etcd 成员自动重新加入 | 🟡 | 无需 |
| REM-003 | RC-003 | 检查网络延迟和磁盘 IO，调整 election-timeout | 🟡 | 变更审批 |
| REM-004 | RC-004 | 修复网络分区，等待 Raft 自动收敛 | 🔴 | 高级审批 |
| REM-005 | RC-005 | 执行压缩+碎片整理，或增大 quota-backend-bytes | 🔴 | 高级审批 |
| REM-006 | RC-006 | 迁移到 SSD/NVMe，减少磁盘 IO 竞争 | 🔴 | 高级审批 |
| REM-007 | RC-007 | 从快照恢复（→ backup-restore-etcd.md） | 🔴 | 高级审批 |
| REM-008 | RC-008 | 修复网络，确保 peer 间 RTT < 10ms | 🟡 | 无需 |
| REM-009 | RC-009 | 修复 NTP 服务，确保时钟偏差 < 100ms | 🟡 | 无需 |
| REM-010 | RC-010 | 防火墙放行 TCP 2379/2380 | 🟡 | 变更审批 |
| REM-011 | RC-011 | 使用 kubeadm certs renew 更新证书 | 🔴 | 高级审批 |
| REM-012 | RC-012 | 检查/重新签发客户端证书 | 🟡 | 变更审批 |
| REM-013 | RC-013 | 执行 `etcdctl defrag`（逐成员） | 🔴 | 高级审批 |
| REM-014 | RC-014 | 限制 Watch 连接数，排查异常客户端 | 🟡 | 无需 |
| REM-015 | RC-015 | 添加 `--auto-compaction-retention=1` 参数 | 🟡 | 变更审批 |

---

## 7. 验证确认

### 即时验证（修复后 1 分钟）

```bash
# 🟢 低风险
ETCDCTL_API=3 etcdctl endpoint health --cluster  # 全部 healthy
ETCDCTL_API=3 etcdctl endpoint status --write-out=table  # leader 稳定
kubectl get nodes  # 全部 Ready
```

### 短期监控（15-30 分钟）

- etcd WAL fsync 延迟 < 10ms（P99）
- 无 "leader changed" 日志
- API Server 响应时间正常（< 1s）
- DB SIZE 稳定不增长

### 解决标准

| 条件 | 判定 |
|------|------|
| 所有成员 endpoint healthy | ✅ |
| Leader 稳定（30min 内无切换） | ✅ |
| kubectl 命令响应 < 2s | ✅ |
| 无 NOSPACE/timeout 告警 | ✅ |

---

## 8. 升级协议

| 级别 | 自动升级条件 | 消息模板 | 交接信息 |
|------|------------|---------|---------|
| P0→全员 | quorum 丢失 | "【P0】etcd 集群失去 quorum，K8s 控制面不可用" | 成员状态 + 最近变更 + 备份时间点 |
| P0→专家 | 数据损坏 | "【P0】etcd 数据损坏，需从快照恢复" | 损坏日志 + 最近可用快照 |
| P1→SME | 单成员持续异常 > 15min | "【P1】etcd 成员 {member} 异常" | endpoint status + 磁盘/网络指标 |

---

## 9. 版本兼容矩阵

| etcd 版本 | K8s 版本 | 关键差异 |
|----------|---------|---------|
| 3.4.x | 1.18-1.21 | 默认 quota 2GB；`--experimental-compaction-batch-limit` |
| 3.5.x | 1.22-1.28 | 修复数据损坏 Bug（3.5.0-3.5.3 有已知问题）；默认 quota 8GB |
| 3.6.x | 1.29-1.32 | 新 Raft 实现；性能优化；`--experimental-stop-grpc-service-on-defrag` |
| 3.7.x | 1.34-1.36 | 进一步性能优化；改进的碎片整理 |

> [存疑：etcd 3.5.0-3.5.3 的数据损坏 Bug 是否在 3.5.4 完全修复，需确认 CVE 公告]

**通用提示**：排障前先确认 etcd 版本：
```bash
# 🟢 低风险
ETCDCTL_API=3 etcdctl version
# 或
kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].spec.containers[0].image}'
```

---

## 10. 知识进化

### 常见误诊模式

| 误诊模式 | 表现 | 正确做法 |
|---------|------|---------|
| 将 API Server 慢误判为 etcd 问题 | kubectl 慢但 etcd 延迟正常 | 先 `etcdctl endpoint status` 确认 etcd 健康 |
| 将磁盘 IO 竞争误判为 etcd Bug | 周期性延迟升高 | 检查同节点其他 IO 密集进程 |
| 将证书过期误判为网络问题 | 连接被拒绝 | 先 `openssl x509 -dates` 检查证书有效期 |
| 碎片整理期间误判为故障 | defrag 时短暂不可用 | 提前通知，逐成员执行 |

### 变更记录

| 版本 | 日期 | 变更内容 | 触发原因 |
|------|------|---------|---------|
| 1.0.0 | 2026-05-23 | 初版 FTA 故障树 | 技能库初始化 |
| 2.0.0 | 2026-07-23 | 重构为 12 章节标准结构，补全根因/修复/验证/版本矩阵 | 技能建设最佳实践对标 |

---

## 生产级观测与证据

### 关键事件/日志

| 日志模式 | 含义 | 对应根因 |
|---------|------|---------|
| `etcdserver: request timed out` | 请求超时 | RC-006/008 |
| `leader changed` 频繁 | 选举不稳定 | RC-003 |
| `mvcc: database space exceeded` | 空间超限 | RC-005 |
| `took too long for a write` | WAL 写入慢 | RC-006 |
| `x509: certificate has expired` | 证书过期 | RC-011 |
| `rafthttp: failed to dial` | peer 连接失败 | RC-008/010 |

### 关键指标（Prometheus）

| 指标 | 用途 | 告警阈值 |
|------|------|---------|
| `etcd_disk_wal_fsync_duration_seconds` | WAL fsync 延迟 | P99 > 10ms |
| `etcd_disk_backend_commit_duration_seconds` | 后端提交延迟 | P99 > 25ms |
| `etcd_server_leader_changes_seen_total` | leader 切换次数 | > 3/hour |
| `etcd_mvcc_db_total_size_in_bytes` | 数据库总大小 | > 6GB（quota 8GB） |
| `etcd_network_peer_round_trip_time_seconds` | peer RTT | P99 > 50ms |
| `etcd_server_proposals_failed_total` | 提案失败数 | > 0 持续增长 |

---

## 生产案例

### 案例 1: etcd 磁盘满导致集群只读

| 时间 | 事件 |
|------|------|
| 02:00 | 监控告警：etcd NOSPACE |
| 02:05 | `etcdctl endpoint status` 显示 DB SIZE 7.9GB（quota 8GB） |
| 02:10 | 确认长期未执行压缩，碎片率 60% |
| 02:15 | 🔴 REM-005 执行 compact + defrag（逐成员） |
| 02:30 | DB SIZE 降至 2.1GB，集群恢复写入 |

**根因**: RC-005 + RC-015。自动压缩未启用，长期积累导致空间耗尽。

### 案例 2: etcd 证书过期导致 API Server 连接失败

**现象**: 集群突然所有 kubectl 命令报 "Unable to connect to the server"

**诊断**: `openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates` 显示已过期

**修复**: 🔴 REM-011 `kubeadm certs renew etcd-server`，重启 etcd 和 API Server

### 案例 3: 磁盘 IO 竞争导致 etcd 延迟飙升

**现象**: 每天 03:00-04:00 集群响应变慢

**诊断**: 同节点备份任务（mysqldump）占满磁盘 IO，etcd WAL fsync P99 > 500ms

**修复**: 🟡 REM-006 将 etcd 数据目录迁移到独立 SSD

---

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]] — 方法论基础
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]] — 执行引擎
- [[技能/故障诊断-控制面/etcd/backup-restore-etcd.md|etcd 备份恢复]] — 同域技能
- [[技能/故障诊断-控制面/etcd/backup-restore-fta.md|备份恢复故障树]] — 同域技能
- [[技能/故障诊断-控制面/apiserver/apiserver-fta.md|API Server 故障树]] — 跨域关联
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]] — 知识索引

<!-- risk-assessed -->
