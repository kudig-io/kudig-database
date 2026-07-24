---
title: etcd × 滚动更新
summary: etcd × 滚动更新：etcd与滚动更新是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- release
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-07
relationships:
- target: '[[实体/helm.md]]'
  type: uses
- target: '[[实体/prometheus.md]]'
  type: uses
- target: '[[实体/argocd.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# etcd × 滚动更新

## 概述
滚动更新过程中，Deployment Controller 频繁向 etcd 读写状态信息——每次 Pod 创建/删除、ReplicaSet 副本数变更、Deployment status 更新都是 etcd 操作。etcd 的写入性能直接决定了滚动更新的速度。同时，etcd 自身的滚动更新（etcd 集群成员升级）也是生产环境最敏感的运维操作之一——不当操作可能导致 Quorum 丢失和数据不一致。

## 技术关联机制

1. **应用滚动更新对 etcd 的写入压力**：一个 100 副本 Deployment 的滚动更新（maxSurge=25%）在更新过程中涉及约 125 次 Pod 创建和 100 次 Pod 删除。每次操作伴随 ReplicaSet status 更新和 Deployment status 更新，总计约 400+ 次 etcd 写入。如果 etcd 的 fsync 延迟为 10ms，仅写入开销就约 4 秒；若延迟为 100ms（磁盘瓶颈），则需 40 秒。

2. **etcd 自身的滚动更新**：etcd 集群通常以 3 或 5 节点部署。升级 etcd 版本或维护时需要逐个（rolling）更新成员——移除旧成员 → 添加新成员 → 等待数据同步 → 处理下一个成员。关键是始终维护 Quorum：3 节点集群一次只能移除 1 个成员，5 节点一次最多移除 2 个。违反 Quorum 原则会导致集群不可用。

3. **etcd 滚动更新中的 Raft 数据同步**：新 etcd 成员加入集群后，需要从 leader 同步完整数据（snapshot + 后续 entries）。这个同步过程可能耗时数分钟（取决于数据量和网络带宽）。在同步完成前，新成员不参与 Quorum 计数（learner 模式）。如果数据同步期间 leader 切换，可能导致同步中断需要重新开始。

4. **滚动更新期间的 etcd compaction 影响**：大规模滚动更新产生的资源变更（大量 revision 增长）可能触发 etcd auto-compaction。compaction 会截断历史 revision，导致 Deployment Controller 的 informer watch 失效（需要全量 relist），对 etcd 造成瞬时高读负载，进一步影响更新速度。

## 实践场景

- **大规模 Deployment 滚动更新的 etcd 瓶颈**：500 副本 Deployment 滚动更新时，etcd 写入延迟飙升导致更新时间从预期 5 分钟增加到 30 分钟
- **etcd 集群版本升级**：从 etcd 3.4 升级到 3.5，逐个成员替换，确保 Quorum 安全
- **etcd 磁盘维护**：逐个 etcd 成员替换磁盘/扩容，每个成员替换后等待数据完全同步
- **滚动更新期间的 etcd compaction 风暴**：大规模更新触发 compaction，导致 Controller relist 风暴，需要分批执行更新

## 常见问题

### 问题1：大规模滚动更新期间 etcd 延迟飙升
**症状**：500 副本 Deployment 滚动更新时 apiserver 响应变慢，etcd fsync 延迟从 5ms 飙到 200ms
**根因**：大量 Pod 创建/删除的写入请求超过 etcd 磁盘 I/O 能力
**修复**：使用较小的 maxSurge（如 10%）控制并发；使用更高性能的 SSD；分批次更新

### 问题2：etcd 滚动更新导致 Quorum 丢失
**症状**：同时替换 2 个 etcd 成员后集群不可用，apiserver 报 `etcdserver: leader changed`
**根因**：3 节点集群同时移除 2 个成员，剩余 1 个成员无法构成 Quorum（需要 2/3）
**修复**：严格逐个替换 etcd 成员；每个成员替换后验证集群健康再处理下一个

### 问题3：etcd 新成员数据同步缓慢
**症状**：新 etcd 成员加入后数据同步耗时超过 30 分钟
**根因**：etcd 数据量大（接近 2GB）；或网络带宽不足
**修复**：使用 `etcdctl snapshot restore` 预加载基础数据减少同步量；确保节点间网络带宽 >1Gbps

## 关键命令

```bash
# 🟢 监控 etcd 写入延迟（滚动更新期间的关键指标）
kubectl get --raw /metrics | grep etcd_disk_wal_fsync_duration_seconds

# 🟢 查看 etcd 集群成员状态
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> member list --write-out=table

# 🟢 检查 Deployment 滚动更新进度
kubectl rollout status deployment/<name> -n <ns>

# 🟡 etcd 成员安全替换（先添加新成员）
ETCDCTL_API=3 etcdctl member add etcd-new --peerUrls=https://<new-node>:2380

# 🟡 移除旧 etcd 成员（确认新成员同步完成后）
ETCDCTL_API=3 etcdctl member remove <old-member-id>

# 🟢 验证 etcd 集群健康（每次成员变更后必须检查）
ETCDCTL_API=3 etcdctl --endpoints=<all-endpoints> endpoint health --write-out=table
```

## 权衡取舍

| 维度 | etcd 倾向 | 滚动更新 倾向 | 权衡点 |
|------|----------|-------------|--------|
| 更新并发 | 低并发减少 etcd 写入压力 | 高并发快速完成发布 | etcd 稳定 vs 发布速度 |
| etcd 成员更新 | 严格逐个保证 Quorum | 并行替换加速升级 | 安全性 vs 运维效率 |
| Compaction 时机 | 频繁 compaction 节省空间 | 延迟 compaction 避免 relist | 存储效率 vs 运行稳定 |
| 数据同步 | 快速同步缩短窗口期 | 充分同步保证一致性 | 运维效率 vs 数据安全 |

## 最佳实践
1. 大规模 Deployment（>100 副本）滚动更新时设置较小的 maxSurge（如 10%），避免 etcd 写入风暴
2. etcd 集群成员更新严格逐个执行，每次验证集群健康（`endpoint health`）后再处理下一个
3. 在低峰期执行大规模滚动更新，减少 etcd 高负载对其他集群操作的影响
4. 监控 etcd fsync 延迟，超过 25ms（P99）时暂停或减缓滚动更新节奏

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[etcd]]
- 滚动更新
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
