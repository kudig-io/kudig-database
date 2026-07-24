---
title: etcd × Deployment
summary: etcd × Deployment：etcd与Deployment是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- workloads
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

# etcd × Deployment

## 概述
etcd 是 Kubernetes 所有集群状态的唯一持久化存储后端，每一个 Deployment 对象——包括其 spec（期望状态）和 status（实际状态）——都作为 key-value 存储在 etcd 中。Deployment Controller 的所有协调操作都基于从 etcd 读取的数据。当 etcd 出现性能问题（磁盘 I/O 瓶颈、网络延迟、内存不足）时，Deployment 的创建、更新和扩缩容操作会变慢，极端情况下可能导致整个发布流程卡住。

## 技术关联机制

1. **Deployment 对象在 etcd 中的存储**：每个 Deployment 对象以 `/registry/deployments/<namespace>/<name>` 为 key 存储在 etcd 中，value 是 protobuf 序列化的 Deployment 对象。相关的 ReplicaSet 和 Pod 也存储在 etcd 中。一个典型的 10 副本 Deployment 及其关联资源在 etcd 中大约占用数十 KB。在万级 Deployment 的集群中，etcd 的存储容量需要规划。

2. **写路径**：用户执行 `kubectl apply -f deployment.yaml` → apiserver 接收请求 → 准入控制 → **apiserver 通过 Raft 协议将 Deployment 对象写入 etcd**。这个写操作需要 etcd Quorum 中多数节点确认（2/3 或 3/5），网络延迟直接影响写入延迟。如果 etcd 磁盘 fsync 慢（如使用普通 HDD），写延迟可能从正常的 5-10ms 飙升到数百 ms。

3. **读路径与 Watch**：Deployment Controller 通过 informer 从 apiserver（进而从 etcd）List 和 Watch Deployment 对象。apiserver 对 etcd 的读操作通过 gRPC 调用完成。Watch 操作利用 etcd 的 revision 机制实现增量通知——Controller 指定 `revision=N`，etcd 返回 N 之后的所有变更事件。如果 etcd 压缩了历史 revision（`--auto-compaction`），过期的 watch 会收到 `compacted` 错误，需要重新全量 List。

4. **etcd 性能对 Deployment 滚动更新的影响**：滚动更新期间，Deployment Controller 频繁更新 Deployment 的 status（updatedReplicas, readyReplicas 等字段），每次更新都是一次 etcd 写操作。如果 etcd 磁盘 IOPS 不足，status 回写延迟会导致 Controller 决策滞后——表现为 `kubectl rollout status` 长时间无进展。

## 实践场景

- **大规模 Deployment 扩容**：将 replicas 从 100 扩到 1000 时，etcd 需要处理大量 Pod 对象的写入，磁盘 I/O 可能成为瓶颈
- **etcd 压缩与 Deployment watch**：etcd 的 auto-compaction 截断历史 revision，如果 Deployment Controller 的 informer 落后于 compaction 阈值，会触发全量 relist，对 etcd 造成瞬时读压力
- **etcd 磁盘满故障**：etcd 磁盘空间耗尽时无法写入，所有 Deployment 创建/更新/扩缩操作全部失败
- **Deployment 密集集群的 etcd 容量规划**：每个 Deployment + ReplicaSet + Pod 在 etcd 中约 10-50KB，10000 个 Deployment 约需 500MB etcd 存储

## 常见问题

### 问题1：Deployment 创建延迟高
**症状**：`kubectl apply` 响应时间从正常的 <1s 增加到 5-10s
**根因**：etcd 磁盘 I/O 瓶颈导致 Raft 写入慢；或 etcd 集群网络延迟高
**修复**：使用 SSD/NVMe 作为 etcd 存储；检查 etcd 集群成员间的网络延迟；监控 `etcd_disk_wal_fsync_duration_seconds`

### 问题2：Deployment Controller 响应缓慢
**症状**：修改 Deployment 后 Pod 创建延迟数十秒甚至数分钟
**根因**：etcd 读延迟高导致 informer watch 事件延迟；或 etcd compaction 触发全量 relist 风暴
**修复**：检查 etcd 的 `etcd_network_peer_round_trip_time_seconds`；调整 `--auto-compaction-retention`；确保 etcd 内存充足

### 问题3：etcd 磁盘满导致 Deployment 操作全部失败
**症状**：所有 `kubectl apply/scale` 返回 `rpc error: code = ResourceExhausted`
**根因**：etcd 数据目录磁盘空间耗尽，可能由大量 Event 对象或 quota 配置不当导致
**修复**：紧急扩容 etcd 磁盘；清理不需要的资源；调整 `--quota-backend-bytes`；配置 etcd auto-defrag

## 关键命令

```bash
# 🟢 检查 etcd 集群健康
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint health --write-out=table

# 🟢 查看 etcd 性能指标（在 apiserver metrics 中）
kubectl get --raw /metrics | grep etcd_request_duration_seconds

# 🟢 查看 etcd 存储使用情况
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint status --write-out=table

# 🟢 监控 etcd 磁盘写入延迟
kubectl get --raw /metrics | grep etcd_disk_wal_fsync_duration_seconds

# 🟢 检查 Deployment 在 etcd 中的数量
kubectl get --raw /metrics | grep apiserver_storage_objects | grep deployment
```

## 权衡取舍

| 维度 | etcd 倾向 | Deployment 倾向 | 权衡点 |
|------|----------|----------------|--------|
| 写入频率 | 低频写入减少磁盘 I/O | 高频 status 更新快速反馈 | 磁盘寿命 vs 可观测性 |
| 对象数量 | 少对象减少存储和 relist 开销 | 多 Deployment 支撑业务需求 | 存储成本 vs 业务灵活 |
| Compaction 频率 | 高频压缩节省空间 | 低频压缩减少 relist 风暴 | 存储效率 vs 运行稳定 |
| Quorum 大小 | 大 Quorum 更高可用 | 小 Quorum 更低延迟 | 可用性 vs 性能 |

## 最佳实践
1. 为 etcd 使用专用 SSD/NVMe 磁盘，确保 fsync 延迟 <10ms
2. 监控 etcd 关键指标：`etcd_disk_wal_fsync_duration_seconds` P99 < 25ms，`etcd_server_slow_apply_total` = 0
3. 配置 etcd auto-compaction 和 auto-defrag，避免存储无限增长影响性能
4. 为大规模集群（>5000 Deployment）规划 etcd 存储容量，定期检查 `apiserver_storage_objects`

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[etcd]]
- [[Deployment]]
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/etcd-×-StatefulSet.md|etcd-×-StatefulSet]]
- [[概念/Deployment-×-PVC.md|Deployment-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
