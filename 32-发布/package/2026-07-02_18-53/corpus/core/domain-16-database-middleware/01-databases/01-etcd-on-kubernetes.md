---
title: Kubernetes 上的 etcd 生产运维指南
description: 在 Kubernetes 上运行 etcd 的生产运维手册，覆盖仲裁机制、磁盘延迟、备份/恢复、成员替换、TLS/证书轮换与可观测性。
summary: 面向 SRE 与平台工程师的 Kubernetes 上 etcd 生产运维指南，包含 quorum 管理、磁盘延迟优化、备份与恢复、成员替换、TLS/证书轮换、监控告警、常见故障 remediation 及与集群控制面 runbook 的交叉引用。
category: database-middleware
tags:
- production
- best-practices
- playbook
- database-middleware
- etcd
- kubernetes
- quorum
- backup
- restore
- tls
- certificate
- observability
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- DBA
estimated_read_time: 30min
intent_queries:
- Kubernetes 上的 etcd 生产运维指南是什么
- 如何备份和恢复 etcd
- etcd 成员如何替换
- etcd 证书如何轮换
- etcd 磁盘延迟如何优化
- etcd quorum 丢失如何处理
trigger_keywords:
- etcd
- quorum
- etcd backup
- etcd restore
- etcd member
- etcd tls
- etcd certificate
- etcd observability
- NOSPACE
prerequisites:
- kubectl-basics
- etcd-basics
- linux-basics
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


# Kubernetes 上的 etcd 生产运维指南

> **适用场景**: 在 Kubernetes 集群内部或外部独立部署 etcd，作为 Kubernetes 数据存储、分布式配置中心或服务发现后端。  
> **目标读者**: SRE、平台工程师、DBA。  
> **最后更新**: 2026-07-01

etcd 是 Kubernetes 控制平面的“心脏”，也是许多分布式系统的核心协调组件。etcd 的稳定性直接决定集群的可用性。生产环境中，etcd 的常见问题包括：磁盘延迟高导致 leader 切换、成员故障导致 quorum 丢失、备份失效、证书过期等。本指南覆盖 etcd 的 quorum、磁盘、备份恢复、成员替换、TLS/证书轮换与可观测性，并关联控制面与可靠性工程域的现有 runbook。

---

## 1. 适用场景与范围

本指南适用于：

- Kubernetes 外部独立 etcd 集群的运维（kubeadm external etcd 模式）。
- 使用 etcd Operator（如 etcd-operator、bitnami etcd Helm chart）在 Kubernetes 内运行 etcd。
- etcd 作为应用配置中心（如 CoreDNS、Traefik、Vitess）的后端。

覆盖范围：etcd 仲裁机制、磁盘性能要求、备份/恢复、成员替换、TLS 证书轮换、监控告警、故障排查、与控制面 runbook 的交叉引用。

---

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 etcdctl（版本必须与 etcd server 一致）
ETCDCTL_VERSION=v3.5.15
wget https://github.com/etcd-io/etcd/releases/download/${ETCDCTL_VERSION}/etcd-${ETCDCTL_VERSION}-linux-amd64.tar.gz
tar xzf etcd-${ETCDCTL_VERSION}-linux-amd64.tar.gz
sudo mv etcd-${ETCDCTL_VERSION}-linux-amd64/etcdctl /usr/local/bin/
etcdctl version

# 配置 etcdctl endpoint（外部 etcd 示例）
export ETCDCTL_ENDPOINTS=https://192.168.1.10:2379,https://192.168.1.11:2379,https://192.168.1.12:2379
export ETCDCTL_CACERT=/etc/etcd/pki/ca.crt
export ETCDCTL_CERT=/etc/etcd/pki/etcdctl.crt
export ETCDCTL_KEY=/etc/etcd/pki/etcdctl.key
export ETCDCTL_API=3

# Kubeadm 集群中访问 etcd
kubectl -n kube-system exec -it etcd-control-plane -- \
  etcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
          --cert=/etc/kubernetes/pki/etcd/server.crt \
          --key=/etc/kubernetes/pki/etcd/server.key \
          endpoint status --write-out=table
```
---

## 3. 核心概念与架构

### 3.1 etcd Quorum

etcd 使用 Raft 共识算法，要求多数成员可用才能提供服务：

| 成员数 | 可容忍故障数 | 推荐场景 |
|--------|-------------|---------|
| 3 | 1 | 生产最小规模 |
| 5 | 2 | 高可用大规模 |
| 7 | 3 | 超大规模（一般不建议） |

> 优先使用奇数成员。4 成员只能容忍 1 个故障，与 3 成员相同，但成本更高。

### 3.2 磁盘延迟要求

etcd 对磁盘延迟极其敏感，建议使用 SSD/NVMe：

| 指标 | 建议阈值 | 说明 |
|------|---------|------|
| fsync 延迟 P99 | < 10ms | 超过 25ms 可能触发 leader 选举 |
| fsync 延迟 P999 | < 25ms | 持续高于此值需立即排查磁盘 |
| 磁盘 IOPS | ≥ 3000 | 随集群规模与写频率增加 |

### 3.3 WAL、Snapshot 与 Backend

- **WAL**: 记录所有写请求，必须先 fsync 到磁盘才返回客户端。
- **Snapshot**: 周期性将内存中的 mvcc 数据写入磁盘，用于恢复与压缩。
- **Backend (BoltDB)**: 实际存储 key-value 数据，受 `--quota-backend-bytes` 限制。

---

## 4. 标准操作流程

### 4.1 每日健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看集群成员与 leader
etcdctl member list -w table

# 2. 查看 endpoint 健康与状态
etcdctl endpoint health
etcdctl endpoint status -w table

# 3. 查看告警
etcdctl alarm list

# 4. 查看数据库大小
etcdctl endpoint status -w json | jq -r '.[] | .Status.dbSize'

# 5. 查看 leader 变更次数（metrics）
curl -s http://localhost:2379/metrics | grep etcd_server_leader_changes_seen_total
```
### 4.2 备份 etcd

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# etcd-backup.sh
set -euo pipefail

DATE=$(date +%Y%m%d-%H%M%S)
BACKUP_DIR="/backup/etcd"
mkdir -p "$BACKUP_DIR"

# 创建快照
etcdctl snapshot save "${BACKUP_DIR}/etcd-${DATE}.db"

# 验证快照完整性
etcdctl snapshot status "${BACKUP_DIR}/etcd-${DATE}.db"

# 备份证书
mkdir -p "${BACKUP_DIR}/pki-${DATE}"
cp -r /etc/etcd/pki/* "${BACKUP_DIR}/pki-${DATE}/" 2>/dev/null || true

# 保留最近 7 天
find "$BACKUP_DIR" -name 'etcd-*.db' -mtime +7 -delete
find "$BACKUP_DIR" -type d -name 'pki-*' -mtime +7 -delete

# 可选：上传到对象存储
aws s3 cp "${BACKUP_DIR}/etcd-${DATE}.db" s3://my-etcd-backups/
aws s3 cp --recursive "${BACKUP_DIR}/pki-${DATE}" s3://my-etcd-backups/pki-${DATE}/
```
### 4.3 恢复 etcd

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 停止所有 etcd 成员
sudo systemctl stop etcd

# 2. 在其中一个节点恢复快照
etcdctl snapshot restore /backup/etcd-latest.db \
  --name etcd-0 \
  --initial-cluster "etcd-0=https://192.168.1.10:2380,etcd-1=https://192.168.1.11:2380,etcd-2=https://192.168.1.12:2380" \
  --initial-cluster-token prod-etcd \
  --initial-advertise-peer-urls https://192.168.1.10:2380 \
  --data-dir=/var/lib/etcd

# 3. 将恢复后的数据目录同步到其他节点
rsync -avz --delete /var/lib/etcd/ etcd-1:/var/lib/etcd/
rsync -avz --delete /var/lib/etcd/ etcd-2:/var/lib/etcd/

# 4. 启动所有成员
sudo systemctl start etcd

# 5. 验证
etcdctl endpoint health
etcdctl endpoint status -w table
```
### 4.4 替换故障成员

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认故障成员 ID
etcdctl member list -w table

# 2. 移除故障成员
etcdctl member remove <member-id>

# 3. 在新节点添加成员
etcdctl member add etcd-3 --peer-urls=https://192.168.1.13:2380

# 4. 在新节点启动 etcd（使用添加时输出的环境变量）
# ETCD_NAME=etcd-3
# ETCD_INITIAL_CLUSTER=etcd-0=https://192.168.1.10:2380,...,etcd-3=https://192.168.1.13:2380
# ETCD_INITIAL_CLUSTER_STATE=existing
sudo systemctl start etcd

# 5. 验证
etcdctl member list -w table
etcdctl endpoint health
```
### 4.5 TLS 证书轮换

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查证书有效期（提前 90 天告警）
for cert in /etc/etcd/pki/*.crt; do
  echo "$cert: $(openssl x509 -in "$cert" -noout -dates | grep notAfter)"
done

# 2. 生成新证书（以 cfssl 为例）
cfssl gencert -ca=ca.crt -ca-key=ca.key -config=ca-config.json \
  -profile=server server-csr.json | cfssljson -bare server

# 3. 滚动替换：一次只替换一个成员的证书，验证健康后再替换下一个
# 复制新证书到 etcd-0，重启 etcd-0
sudo systemctl restart etcd
etcdctl endpoint health

# 4. 依次替换其余成员

# 若 CA 过期，需重新生成 CA 并滚动替换所有证书，过程更复杂，建议参考证书轮换 runbook
```
### 4.6 数据库压缩与碎片整理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看修订版本与数据库大小
etcdctl endpoint status -w json | jq -r '.[] | .Status.header.revision, .Status.dbSize'

# 2. 手动压缩历史版本（保留最近 1000 个修订）
REV=$(etcdctl endpoint status -w json | jq -r '.[] | .Status.header.revision' | head -1)
etcdctl compact "$((REV - 1000))"

# 3. 碎片整理（每个节点单独执行）
etcdctl defrag

# 4. 建议启用自动压缩
# etcd 启动参数添加：--auto-compaction-mode=revision --auto-compaction-retention=1000
```
---

## 5. 关键检查点与验证命令

| 检查项 | 验证命令 | 通过标准 |
|--------|---------|---------|
| 成员健康 | `etcdctl endpoint health` | 所有 endpoint healthy |
| Leader 存在 | `etcdctl endpoint status -w table` | 有且只有一个 leader |
| 数据库大小 | `etcdctl endpoint status -w json \| jq '.[].Status.dbSize'` | < 8GB（默认 quota） |
| 磁盘 fsync | `etcd_disk_wal_fsync_duration_seconds` P99 | < 10ms |
| 证书有效期 | `openssl x509 -in server.crt -noout -dates` | > 90 天 |
| 备份成功 | `etcdctl snapshot status /backup/etcd-latest.db` | 无错误 |
| 告警列表 | `etcdctl alarm list` | 无 NOSPACE/ERRORS |
| leader 变更 | `etcd_server_leader_changes_seen_total` | 24h 内 < 5 次 |

---

## 6. 常见故障与 Remediation

### 6.1 etcd leader 频繁切换

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查磁盘延迟
kubectl top nodes
iostat -x 1

# 2. 检查网络延迟
ping <peer-ip>

# 3. 检查 etcd 日志
journalctl -u etcd -n 500 | grep -i "leader"

# 修复：升级磁盘、优化网络、调整 heartbeat/election timeout
# etcd 启动参数：--heartbeat-interval=100 --election-timeout=1000
```
### 6.2 NOSPACE 告警

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看告警
etcdctl alarm list

# 2. 压缩并整理碎片
etcdctl compact <revision>
etcdctl defrag

# 3. 提升 quota
etcdctl alarm disarm
# 修改 etcd 启动参数 --quota-backend-bytes=17179869184（16GB）
```
### 6.3 证书过期导致 etcd 不可用

```bash
# 1. 检查证书有效期
openssl x509 -in /etc/etcd/pki/server.crt -noout -dates

# 2. 如果 CA 未过期，仅替换 server/peer 证书
# 如果 CA 过期，需重新生成所有证书并滚动替换

# 3. 临时使用 --client-cert-auth=false 恢复访问（仅限 break-glass）
# 修改 etcd 启动参数后重启
```

### 6.4 Quorum 丢失

```bash
# 若仅剩一个成员可用，无法恢复 quorum，需从备份恢复
# 1. 停止所有成员
# 2. 选择最新备份执行 snapshot restore
# 3. 重建集群并验证
# 详细步骤参考 domain-09 的 etcd-corruption-recovery-playbook
```

### 6.5 请求延迟高 / `etcd_request_duration_seconds` 高

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 etcd 日志与 metrics
curl -s http://localhost:2379/metrics | grep etcd_request_duration_seconds

# 2. 检查是否有大量 ListAll 请求
kubectl get --raw /metrics | grep apiserver_request_duration_seconds

# 3. 优化：提升磁盘、扩容 etcd 节点、调整 API Server 缓存、减少事件监听
```
---

## 7. 风险与注意事项

1. **不要直接删除 etcd 数据目录**: 除非执行 restore，否则会导致数据丢失。
2. **备份必须异地保留**: 本地备份无法应对机房级故障。
3. **恢复操作会丢失增量数据**: 从快照恢复会回滚到快照时间点，之后的数据需从其他方式补齐。
4. **证书轮换需滚动进行**: 同时替换所有成员证书可能导致集群不可用。
5. **etcd 不建议与业务混布**: 生产环境 etcd 应使用独立节点或低负载控制平面节点。
6. **避免频繁 compact/defrag**: 压缩与整理会短暂增加 I/O，建议在低峰期执行。
7. **监控 leader 变更**: leader 频繁切换往往是磁盘或网络问题的前兆，应立即排查。


## 4.13 etcd 备份保留与合规

etcd 备份的保留策略应满足业务恢复目标与合规要求：

- 保留最近 7 天的每日备份，最近 24 小时的每小时备份用于快速恢复。
- 保留每月一次的关键时点备份，保留期不少于 1 年。
- 备份应加密存储，并限制访问权限。
- 定期验证备份可恢复性，避免备份文件损坏导致无法恢复。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证备份文件完整性
etcdctl snapshot status /backup/etcd-latest.db

# 查看 S3 备份列表
aws s3 ls s3://my-etcd-backups/ --recursive | sort
```
## 4.14 etcd 与 API Server 协同调优

Kubernetes API Server 的某些参数会影响 etcd 负载：

- `--request-timeout`: 控制请求超时，避免长请求占用 etcd 连接。
- `--watch-cache`: 启用 watch 缓存可减少对 etcd 的 List/Watch 压力。
- `--etcd-compaction-interval`: 控制 API Server 触发 etcd 压缩的频率，默认 5 分钟。
- 大规模集群应合理设置 `--event-ttl`，避免事件无限增长。

```bash
# 查看 API Server 启动参数
ps aux | grep kube-apiserver | grep -E 'request-timeout|watch-cache|etcd-compaction-interval|event-ttl'
```

---

## 8. 相关 Runbook / 推荐阅读

### 同域核心文档

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-16-database-middleware/03-production-readiness-operations-guide|Database & Middleware 生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-16-database-middleware/01-databases/03-database-middleware-kubernetes|数据库中间件 Kubernetes 部署概览]]

### 控制面与性能调优

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-16-database-middleware/03-production-readiness-operations-guide|集群基础生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-01-cluster-fundamentals/06-kubernetes-production-architecture-blueprint|Kubernetes 生产架构蓝图]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-01-cluster-fundamentals/07-performance-tuning/01-apiserver-etcd-performance-tuning|API Server 与 etcd 性能调优]]

### 备份恢复与灾备

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-16-database-middleware/03-production-readiness-operations-guide|可靠性工程生产就绪运维指南]]
- [[domain-09-reliability-engineering/备份恢复/01-etcd-backup-restore.md|etcd 备份与恢复]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/10-disaster-recovery-playbooks/02-etcd-corruption-recovery-playbook|etcd 损坏恢复 Runbook]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/10-disaster-recovery-playbooks/05-control-plane-loss-recovery-playbook|控制面丢失恢复 Runbook]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-09-reliability-engineering/10-disaster-recovery-playbooks/08-certificate-expiry-mass-rotation-playbook|证书过期批量轮换 Runbook]]

### 工单案例

- [[domain-11-production-operations/工单案例/ticket-case-009-etcd-disk-full-apiserver-slow.md|工单案例 009：etcd 磁盘满导致 API Server 变慢]]

---

*本指南应与 etcd 官方运维文档结合使用。建议每日自动执行健康检查与备份，每月进行一次恢复演练，每季度评估一次磁盘性能与证书有效期。*


<!-- risk-assessed -->
