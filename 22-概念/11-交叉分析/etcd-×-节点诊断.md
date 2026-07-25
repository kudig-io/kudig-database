---
title: etcd × 节点诊断
summary: etcd × 节点诊断：etcd与节点诊断是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- troubleshooting
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-07
relationships:
- target: '[[23-实体/08-交付与制品/helm.md]]'
  type: uses
- target: '[[23-实体/07-可观测性/prometheus.md]]'
  type: uses
- target: '[[23-实体/08-交付与制品/argocd.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# etcd × 节点诊断

## 概述
节点（Node）的健康状态——包括 Ready 条件、资源容量、已分配资源、taints——都存储在 etcd 中，由 kubelet 定期回写。Node Controller 通过 watch etcd 中的 Node 对象来检测节点故障和触发 Pod 驱逐。当 etcd 性能不佳时，节点心跳回写延迟会导致 Node Controller 误判节点 NotReady，进而触发不必要的 Pod 驱逐。同时，etcd 自身如果运行在某个节点上（自建集群的控制面节点），该节点的故障直接威胁 etcd 可用性。

## 技术关联机制

1. **节点心跳在 etcd 中的存储**：kubelet 每 10 秒通过 apiserver 向 etcd 更新 Node 的 `status.conditions` 和 `status.allocatable`。这些心跳更新是 etcd 的高频写入来源——100 节点的集群每秒约 10 次 NodeStatus 写入。如果 etcd 磁盘 I/O 不足，心跳写入积压导致 Node Controller 在 `node-monitor-grace-period`（默认 40s）内未收到心跳，错误标记节点 NotReady。

2. **Node Controller 的 etcd watch 依赖**：Node Controller 通过 informer watch etcd 中的 Node 对象变化。当节点心跳停止（网络分区或 kubelet 崩溃），etcd 中的 NodeStatus 停止更新，Node Controller 在 grace period 后检测到 `HeartbeatTimeout`，将 Node Ready condition 设为 `Unknown`。超过 `pod-eviction-timeout`（默认 5min）后，Controller 开始向 etcd 写入 Pod 删除请求，触发 Pod 在其他节点重建。

3. **etcd 运行节点的故障影响**：在自建集群中，etcd 通常以静态 Pod 形式运行在控制面节点上。如果控制面节点硬件故障导致 etcd 不可用，apiserver 也随之不可用，Node Controller 无法工作——已故障 worker 节点上的 Pod 不会被驱逐。这就是为什么生产环境 etcd 必须以 3/5 节点集群部署，确保单节点故障不影响 Quorum。

4. **节点级 etcd 诊断**：当 etcd 出现性能问题时，需要在运行 etcd 的节点上进行诊断——检查磁盘 I/O（`iostat`）、检查 etcd 进程资源使用（`top`）、检查 etcd 日志（`journalctl`）。这些操作需要 SSH 到控制面节点，无法通过 kubectl 完成。

## 实践场景

- **etcd 延迟导致误驱逐**：etcd 磁盘 I/O 瓶颈导致节点心跳回写延迟，Node Controller 误判节点 NotReady 并驱逐 Pod
- **控制面节点故障**：运行 etcd 的控制面节点故障，需要确保剩余 etcd 节点构成 Quorum
- **节点级 etcd 性能诊断**：etcd 性能下降时 SSH 到控制面节点检查磁盘 IOPS 和延迟
- **etcd 磁盘满**：控制面节点磁盘空间不足导致 etcd 无法写入，所有节点心跳停止

## 常见问题

### 问题1：etcd 性能问题导致节点被误判 NotReady
**症状**：节点实际正常但频繁被标记 NotReady，Pod 被不必要地驱逐和重新调度
**根因**：etcd 写入延迟高导致 kubelet 的心跳回写延迟，超过 `node-monitor-grace-period`
**修复**：提升 etcd 磁盘性能；适当增大 `node-monitor-grace-period`（如 60s）

### 问题2：控制面节点故障导致 etcd Quorum 丢失
**症状**：一个控制面节点宕机后整个集群不可用
**根因**：3 节点 etcd 集群中 2 个节点在同一物理机上故障；或 etcd 部署在单个节点上
**修复**：确保 etcd 节点分布在不同的物理机/AZ；使用 5 节点 etcd 集群提升容错能力

### 问题3：etcd 磁盘满导致所有节点心跳失败
**症状**：所有节点状态变为 Unknown，apiserver 报 `etcdserver: mvcc: database space exceeded`
**根因**：控制面节点磁盘空间耗尽，etcd 无法写入任何数据
**修复**：紧急扩容磁盘；执行 etcd compaction 释放空间；清理不需要的资源和日志

## 关键命令

```bash
# 🟢 检查 etcd 健康（影响节点心跳处理）
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint health --write-out=table

# 🟢 监控 etcd 磁盘写入延迟（节点心跳的关键瓶颈）
kubectl get --raw /metrics | grep etcd_disk_wal_fsync_duration_seconds

# 🟢 查看所有节点状态
kubectl get nodes -o wide

# 🟢 SSH 到控制面节点检查 etcd 性能（节点级诊断）
ssh <control-plane-node>
iostat -x 1 5  # 检查磁盘 I/O
df -h /var/lib/etcd  # 检查磁盘空间

# 🟢 检查 etcd 数据库大小
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint status --write-out=table

# 🟢 查看 Node Controller 日志（检测节点心跳超时）
kubectl logs -n kube-system kube-controller-manager-<node> | grep -i "heartbeat\|notready"
```

## 权衡取舍

| 维度 | etcd 倾向 | 节点诊断 倾向 | 权衡点 |
|------|----------|-------------|--------|
| 心跳频率 | 低频减少 etcd 写入 | 高频快速发现故障 | etcd 负载 vs 故障检测速度 |
| Grace Period | 长 grace 避免误判 | 短 grace 快速响应 | 稳定性 vs 响应速度 |
| etcd 节点分布 | 集中部署简化管理 | 分散部署提升容错 | 管理复杂 vs 高可用性 |
| 诊断方式 | kubectl 依赖 etcd | SSH 直接诊断不依赖 etcd | 便利性 vs 可靠性 |

## 最佳实践
1. 将 etcd 节点分布在不同物理机/AZ 上，确保单点故障不影响 Quorum
2. 为 etcd 使用专用 SSD 磁盘，确保心跳回写延迟不超过 10ms
3. 监控 etcd 磁盘空间使用率，设置 70% 水位告警防止磁盘满
4. 建立 SSH 直连控制面节点的诊断能力，在 apiserver/etcd 故障时执行节点级诊断

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[etcd]]
- 节点诊断
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
