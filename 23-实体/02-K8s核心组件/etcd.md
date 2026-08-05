---
title: etcd (entities)
description: '- etcd 深度解析'
summary: etcd is the backing datastore for Kubernetes. All cluster state ([[pods|Pods]],
  Services, [[configmaps|ConfigMaps]], [[secrets|Secrets]], etc.) is persisted to
  etcd. It uses Raft consensus for faul...
category: entities
tags:
- k8s
- etcd
- raft
- mvcc
- database
- control-plane
- apiserver
- operator
- rag
- scheduler
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd 是什么
- 如何 etcd
trigger_keywords:
- etcd
prerequisites:
- kubectl-basics
- ebpf-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# etcd

## Overview

etcd is the backing datastore for Kubernetes. All cluster state ([[pods|Pods]], Services, [[configmaps|ConfigMaps]], [[secrets|Secrets]], etc.) is persisted to etcd. It uses Raft consensus for fault-tolerant replication and MVCC (Multi-Version Concurrency Control) for watchable history.

## Key Properties

| Property | Value |
|----------|-------|
| **Consensus** | Raft (Leader/Follower/Candidate) |
| **Storage** | B+ Tree with MVCC revision chains |
| **Watch** | Real-time event streaming by revision |
| **Ports** | 2379 (client gRPC), 2380 (peer replication) |
| **Data Path** | `/registry/{resource-type}/{namespace}/{name}` |
| **Quota** | Default 2GB (`--quota-backend-bytes=8GB` for production) |

## Raft Consensus

- Odd number of nodes (3, 5, or 7)
- Tolerates f failures with 2f+1 nodes
- Leader handles all writes; followers replicate log
- Configurable heartbeat (100ms) and election timeout (1000ms)

## MVCC and Watch

Every write increments a global revision number. Watch streams track from a specific revision and receive events for changes. etcd compaction removes old revisions periodically (default every 5 minutes) to reclaim space.

## Production Requirements

- **Storage**: SSD or NVMe for low fsync latency (<10ms p99)
- **Backup**: Hourly snapshots with `etcdctl snapshot save`
- **Defragmentation**: Regular `etcdctl defrag` to reclaim space after compaction
- **Monitoring**: Watch disk commit duration, db size, leader changes, proposal failures

## 运维操作

### 常用命令

```bash
# 🟢 查看集群状态
ETCDCTL_API=3 etcdctl endpoint status --cluster -w table

# 🟢 查看集群成员
etcdctl member list -w table

# 🟢 查看健康状态
etcdctl endpoint health --cluster

# 🟢 查看 Leader
etcdctl endpoint status --cluster -w table | grep -v false

# 🟢 查看数据库大小
etcdctl endpoint status --cluster -w json | jq '.[].Status.dbSize'

# 🟢 查看告警
etcdctl alarm list

# 🟡 压缩历史版本
etcdctl compact $(etcdctl endpoint status --cluster -w json | jq '.[0].Status.header.revision')

# 🟡 碎片整理 (会短暂阻塞)
etcdctl defrag --cluster

# 🔴 快照备份
etcdctl snapshot save /backup/etcd-$(date +%Y%m%d).db

# 🔴 快照恢复
etcdctl snapshot restore /backup/etcd-xxx.db \
  --data-dir=/var/lib/etcd-restored \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://10.0.1.1:2380

# 🔴 移除成员
etcdctl member remove <member-id>

# 🔴 添加成员
etcdctl member add etcd-4 --peer-urls=https://10.0.1.4:2380
```

### K8s 中的 etcd 操作

```bash
# 🟢 查看 etcd Pod
kubectl get pods -n kube-system -l component=etcd

# 🟢 查看 etcd 日志
kubectl logs -n kube-system etcd-<node-name> --tail=50

# 🟢 通过 kube-apiserver 查看 etcd 健康
kubectl get --raw /healthz/etcd

# 🟢 查看 etcd 指标
kubectl exec -n kube-system etcd-<node> -- \
  curl -s --cert /etc/kubernetes/pki/etcd/server.crt \
  --key /etc/kubernetes/pki/etcd/server.key \
  https://127.0.0.1:2379/metrics | grep etcd_disk
```

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Leader 频繁切换 | 磁盘 IO 慢/网络抖动 | 检查磁盘延迟、网络 |
| 数据库空间不足 | 未压缩/未碎片整理 | compact + defrag |
| 写入延迟高 | 磁盘 fsync 慢 | 使用 SSD/NVMe |
| 成员不健康 | 证书过期/网络断开 | 检查证书和网络 |
| NOSPACE 告警 | 超过 quota | 压缩+碎片+增大 quota |
| 数据丢失 | 多数节点失败 | 从快照恢复 |

### 排查流程

```
1. 检查集群健康
   etcdctl endpoint health --cluster
       │
2. 检查 Leader 和成员状态
   etcdctl endpoint status --cluster -w table
       │
3. 检查告警
   etcdctl alarm list
       │
4. 检查磁盘延迟
   etcdctl endpoint status --cluster -w json | jq '.[].Status.dbSize'
   # 查看 metrics: etcd_disk_wal_fsync_duration_seconds
       │
5. 检查日志
   journalctl -u etcd --since "10 min ago"
```

## 监控指标

### 关键指标与告警

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| etcd_disk_wal_fsync_duration_seconds | WAL fsync 延迟 | P99 > 10ms |
| etcd_disk_backend_commit_duration_seconds | Backend 提交延迟 | P99 > 25ms |
| etcd_server_proposals_failed_total | 提案失败 | > 0 |
| etcd_server_leader_changes_seen_total | Leader 切换 | > 3/hour |
| etcd_mvcc_db_total_size_in_bytes | 数据库大小 | > 80% quota |
| etcd_network_peer_round_trip_time_seconds | Peer RTT | P99 > 50ms |

### Prometheus 告警规则

```yaml
groups:
- name: etcd-alerts
  rules:
  - alert: EtcdHighDiskLatency
    expr: histogram_quantile(0.99, rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) > 0.01
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "etcd WAL fsync P99 延迟超过 10ms"

  - alert: EtcdDatabaseSpaceLow
    expr: etcd_mvcc_db_total_size_in_bytes / etcd_server_quota_backend_bytes > 0.8
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "etcd 数据库空间使用超过 80%"

  - alert: EtcdLeaderChanges
    expr: rate(etcd_server_leader_changes_seen_total[1h]) > 3
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "etcd Leader 频繁切换"
```

## 生产最佳实践

1. **使用 SSD/NVMe** - etcd 对磁盘延迟极其敏感
2. **独立部署** - 不与其他工作负载共享磁盘
3. **定期备份** - 每小时快照，异地存储
4. **自动压缩** - `--auto-compaction-retention=1h`
5. **定期碎片整理** - 压缩后执行 defrag
6. **监控告警** - 磁盘延迟、Leader 切换、空间使用
7. **奇数节点** - 3/5/7 节点，容忍 1/2/3 失败
8. **网络隔离** - Peer 通信使用专用网络

## 检查清单

- [ ] 理解 Raft 共识算法
- [ ] 掌握 etcdctl 常用命令
- [ ] 能执行备份和恢复
- [ ] 掌握故障排查流程
- [ ] 理解压缩和碎片整理
- [ ] 能配置监控告警
- [ ] 了解生产最佳实践

## Related
- [[22-概念/11-交叉分析/etcd × Operator 模式.md|etcd × Operator 模式]] — 综合
- [[22-概念/11-交叉分析/etcd × 可观测性.md|etcd × 可观测性]] — 综合

- [[grpc]] — gRPC
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[22-概念/08-可靠性与运维/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[26-技能/02-控制面/etcd/backup-restore-etcd.md|backup-restore-etcd]] — Backup and Restore etcd
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[22-概念/01-核心架构/watch-mechanism.md|Watch Mechanism]]
- [[22-概念/08-可靠性与运维/high-availability-patterns.md|High Availability Patterns]]
- [[26-技能/02-控制面/etcd/backup-restore-etcd.md|Backup and Restore etcd]]
- [[23-实体/02-K8s核心组件/kube-apiserver.md|kube-apiserver]]
- [[22-概念/01-核心架构/etcd Operational Reference.md|etcd Operational Reference]]

- etcd 深度解析
- 19-etcd-operations
- 07-distributed-consensus-etcd
- [[19-故障诊断/01-核心排障/02-control-plane-etcd-troubleshooting.md|02-control-plane-etcd-troubleshooting]]
- [[19-故障诊断/04-高级排障/structural-10-etcd-maintenance|10-etcd-maintenance]]
- [[19-故障诊断/06-FTA故障树/list/etcd-fta.md|etcd 异常故障树分析]]
- [[19-故障诊断/04-高级排障/structural-01-control-plane/02-etcd-troubleshooting.md|02-etcd-troubleshooting]]
- RELEASE-NOTES-0.2
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.5.md|RELEASE-NOTES-3.5]]
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-2.0.md|RELEASE-NOTES-2.0]]
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-2.1.md|RELEASE-NOTES-2.1]]
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- RELEASE-NOTES-0.3
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.4.md|RELEASE-NOTES-3.4]]
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-2.2.md|RELEASE-NOTES-2.2]]
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.3.md|RELEASE-NOTES-3.3]]
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.1
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.6.md|RELEASE-NOTES-3.6]]
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-2.3.md|RELEASE-NOTES-2.3]]
- [[37-归档/release-notes/core-deps/etcd/RELEASE-NOTES-3.2.md|RELEASE-NOTES-3.2]]
- Wiki Digest — Daily (2026-05-21) — Cross-reference
- [[23-实体/15-参考与索引/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]] — Cross-reference
- [[23-实体/15-参考与索引/specialized-workloads-terms.md|K8s 专用工作负载术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-design-principles-deep-dive.md|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[23-实体/15-参考与索引/workloads-terms.md|K8s 工作负载术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-structured-troubleshooting.md|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[23-实体/15-参考与索引/fundamentals-terms.md|K8s 基础概念术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-architecture-fundamentals.md|K8s 架构基础与核心组件原理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|发布说明阅读指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-advanced-ecosystem.md|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-control-plane-deep-dive.md|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[23-实体/15-参考与索引/kubectl-quick-reference.md|Kubectl Quick Reference]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-deployment-create.md|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-production-operations.md|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-cluster-delete.md|Kubernetes 集群删除操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-cluster-create.md|Kubernetes 集群创建操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[23-实体/15-参考与索引/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-cluster-cert.md|Kubernetes 集群证书管理操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-node-create.md|Kubernetes 节点管理操作指南]] — Cross-reference
- [[23-实体/15-参考与索引/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]] — Cross-reference
- [[23-实体/15-参考与索引/multi-cloud-terms.md|K8s 多云架构术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/kudig-man-pages-index.md|KUDIG Man Pages Index]] — Cross-reference
- [[23-实体/15-参考与索引/version-upgrade-guide.md|版本升级指南]] — Cross-reference
- [[23-实体/15-参考与索引/operations-terms.md|K8s 运维运营术语参考]] — Cross-reference
- [[22-概念/08-可靠性与运维/kubeadm-cluster-operations.md|kubeadm 集群运维全景]] — Cross-reference
- [[22-概念/11-交叉分析/etcd × 高可用模式.md|etcd × 高可用模式]] — Cross-reference
- [[19-故障诊断/01-核心排障/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]] — Cross-reference
- [[22-概念/08-可靠性与运维/k8s-mttr-benchmark.md|K8s 问题分布与 MTTR 基准]] — Cross-reference
- [[22-概念/08-可靠性与运维/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]] — Cross-reference
- [[22-概念/11-交叉分析/声明式 API × 控制器模式.md|声明式 API × 控制器模式]] — Cross-reference
- [[22-概念/02-工作负载/deployment-controller-architecture.md|Deployment 控制器架构]] — Cross-reference
- [[22-概念/05-安全/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]] — Cross-reference
- [[22-概念/10-最佳实践/bp-infrastructure.md|最佳实践：Infrastructure]] — Cross-reference
- [[22-概念/01-核心架构/declarative-api.md|Declarative API]] — Cross-reference
- [[22-概念/01-核心架构/core-dependency-version-matrix.md|核心依赖版本矩阵]] — Cross-reference
- [[22-概念/12-研究/kubernetes-version-evolution.md|Kubernetes 版本演进]] — Cross-reference
- [[22-概念/12-研究/ai-agent-openclaw-workspace.md|OpenClaw 工作空间配置]] — Cross-reference
- [[22-概念/09-平台与发布/infrastructure-as-code.md|Infrastructure as Code]] — Cross-reference
- [[35-元数据/metadata/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[22-概念/01-核心架构/eventual-consistency.md|Eventual Consistency in Kubernetes]] — Cross-reference
- [[22-概念/10-最佳实践/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[22-概念/01-核心架构/Kubernetes Core Concepts.md|Kubernetes Core Concepts]] — Cross-reference
- [[22-概念/03-网络/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-01-day-one-checklist.md|Day 1: 新人首日检查清单]] — Cross-reference
- [[26-技能/02-控制面/etcd/最佳实践/k8s-disaster-recovery-guide.md|Kubernetes 灾难恢复最佳实践]] — Cross-reference
- [[26-技能/03-节点/node/诊断排障/ts-node-components.md|节点组件故障排查]] — Cross-reference
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]] — Cross-reference
- [[26-技能/07-安全/rbac/诊断排障/ts-security-auth.md|安全认证故障排查]] — Cross-reference
- [[26-技能/02-控制面/crd-operator/运维操作/develop-crd-operator.md|Develop CRD Operator]] — Cross-reference
- [[26-技能/03-节点/node/运维操作/node-drain-and-maintenance.md|节点驱逐与维护]] — Cross-reference
- [[26-技能/04-工作负载/statefulset/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-deletion.md|kubeadm 集群删除操作]] — Cross-reference
- [[26-技能/01-集群运维/kubeadm/kubeadm-ha-cluster-setup.md|kubeadm 高可用集群搭建]] — Cross-reference
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/k8s-cluster-configuration-guide.md|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[26-技能/02-控制面/apiserver/诊断排障/ts-control-plane.md|控制平面故障排查]] — Cross-reference
- [[26-技能/08-可观测性/monitoring/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]] — Cross-reference
- [[26-技能/01-集群运维/gitops-argocd/诊断排障/ts-gitops-devops.md|GitOps/DevOps 排查]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-02-first-ticket-guide.md|Day 2: 第一个工单处理指南]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-oncall-quick-qa.md|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/skill-MOC.md|topic-skills MOC]] — Cross-reference
- [[26-技能/04-工作负载/pod/方法论/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]] — Cross-reference
- [[26-技能/06-存储/csi-storage/诊断排障/ts-storage.md|存储故障排查]] — Cross-reference
- [[26-技能/01-集群运维/cluster-upgrade/诊断排障/ts-cluster-operations.md|集群运维故障排查]] — Cross-reference
- [[26-技能/03-节点/node/skill-notready/skill-assets-escalation-template.md|Escalation Template]] — Cross-reference
- [[01-集群基础/03-控制平面/11-etcd-deep-dive.md|etcd 深度解析]] — Cross-reference
- [[01-集群基础/03-控制平面/12-apiserver-deep-dive.md|kube-apiserver 深度解析]] — Cross-reference
- Domain-3: Kubernetes控制平面 — Cross-reference
- [[23-实体/15-参考与索引/core-deps-changelog.md|核心依赖变更日志索引]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
