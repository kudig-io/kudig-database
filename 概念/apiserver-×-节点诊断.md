---
title: apiserver × 节点诊断
summary: apiserver × 节点诊断：apiserver与节点诊断是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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
- target: '[[实体/helm.md]]'
  type: uses
- target: '[[实体/prometheus.md]]'
  type: uses
- target: '[[实体/argocd.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# apiserver × 节点诊断

## 概述
节点（Node）是 Kubernetes 的物理/虚拟计算单元。apiserver 通过 Node Controller 持续监控所有节点的健康状态——kubelet 定期通过 apiserver 更新 Node 的 `status.conditions` 和 `status.allocatable`。节点诊断的所有核心操作（`kubectl describe node`、`kubectl cordon`、`kubectl drain`、`kubectl uncordon`）都通过 apiserver 执行。当节点与 apiserver 之间的心跳中断时，Node Controller 触发 Pod 驱逐流程，这是生产环境最常见的故障场景之一。

## 技术关联机制

1. **Node 心跳机制**：每个节点上的 kubelet 定期（默认每 10 秒）向 apiserver 发送 `POST /api/v1/nodes/<node>/status` 更新 NodeStatus。apiserver 将更新写入 etcd，Node Controller（运行在 controller-manager 中）通过 informer watch 到 NodeStatus 变更。如果心跳超过 `node-monitor_grace_period`（默认 40s）未更新，Node Controller 将 Node 标记为 `NotReady`。超过 `pod_eviction_timeout`（默认 5min）后，Node Controller 开始驱逐该节点上的 Pod。

2. **Node Status Conditions**：Node 的 `status.conditions` 包含多个健康指标：`Ready`（节点是否可调度）、`MemoryPressure`（内存不足）、`DiskPressure`（磁盘不足）、`PIDPressure`（进程数过多）、`NetworkUnavailable`（网络配置错误）。这些 condition 由 kubelet 上报给 apiserver，Node Controller 和调度器据此做决策。

3. **Cordon 与 Drain 的 API 操作**：`kubectl cordon` 将 Node 的 `spec.unschedulable` 设为 true，阻止调度器分配新 Pod。`kubectl drain` 在 cordon 基础上，逐个驱逐（Evict）现有 Pod——通过向 apiserver 发送 `Eviction` 子资源请求，遵守 PodDisruptionBudget 约束。Drain 操作依赖 apiserver 与 kubelet 的通信链路，如果 apiserver 到节点 kubelet（10250 端口）不通，drain 会超时。

4. **Node 资源上报**：kubelet 定期上报 Node 的 `capacity`（总资源）和 `allocatable`（可分配资源）。调度器从 apiserver 读取这些数据做调度决策。如果 kubelet 上报的 allocatable 不准确（如容器运行时资源泄漏），可能导致过度调度。

## 实践场景

- **节点 NotReady 排查**：通过 `kubectl describe node` 查看 Conditions 和 Events，判断是网络分区、kubelet 崩溃还是资源耗尽
- **维护性驱逐**：节点升级内核/替换硬件前执行 `kubectl drain`，安全迁移所有 Pod
- **资源碎片诊断**：通过 `kubectl describe node` 查看 Allocated resources，识别资源碎片导致的调度失败
- **节点自动扩缩容反馈**：Cluster Autoscaler 通过 apiserver 监控 Node 状态和 Pending Pod，自动增减节点

## 常见问题

### 问题1：节点状态在 Ready 和 NotReady 间频繁切换
**症状**：Node 状态频繁 flapping，Pod 反复被驱逐和重新调度
**根因**：kubelet 到 apiserver 的网络不稳定；或节点资源（CPU/Memory）接近阈值导致心跳延迟
**修复**：检查节点网络连通性和延迟；增加节点资源或迁移部分 Pod 降低负载；调整 `node-monitor-grace-period`

### 问题2：kubectl drain 卡住无法完成
**症状**：`kubectl drain <node>` 长时间等待，部分 Pod 无法驱逐
**根因**：PodDisruptionBudget 阻止驱逐；或 Pod 使用了 `emptyDir` 且非 ReplicaSet 管理（如 standalone Pod）；或 DaemonSet Pod
**修复**：使用 `--disable-eviction` 强制删除（⚠️ 绕过 PDB）；使用 `--ignore-daemonsets` 忽略 DaemonSet；检查 PDB 配置

### 问题3：节点资源耗尽导致 Pod 被 OOMKilled
**症状**：节点上的 Pod 频繁 OOMKilled 或被驱逐
**根因**：节点 Memory/Disk 资源耗尽，kubelet 触发节点压力驱逐（Node-pressure Eviction）
**修复**：`kubectl describe node` 查看 conditions 中的 MemoryPressure/DiskPressure；迁移部分 Pod 降低节点负载；增加节点资源或扩容节点数

## 关键命令

```bash
# 🟢 查看所有节点状态
kubectl get nodes -o wide

# 🟢 查看节点详细状态（含 conditions、capacity、allocatable）
kubectl describe node <node-name>

# # 🟢 查看节点上的 Pod 资源分配
kubectl describe node <node-name> | grep -A 20 "Allocated resources"

# 🟡 Cordon 节点（阻止调度）
kubectl cordon <node-name>

# 🟡 Drain 节点（驱逐 Pod + cordon）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 🟡 Uncordon 节点（恢复调度）
kubectl uncordon <node-name>

# 🟢 检查节点 Events
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name>
```

## 权衡取舍

| 维度 | apiserver 倾向 | 节点诊断 倾向 | 权衡点 |
|------|---------------|-------------|--------|
| 心跳频率 | 低频减少 API 负载 | 高频快速发现故障 | 集群负载 vs 故障检测速度 |
| 驱逐超时 | 长 timeout 防止误驱逐 | 短 timeout 快速恢复 | 稳定性 vs 恢复速度 |
| 资源粒度 | 粗粒度减少上报开销 | 细粒度精准调度 | API 压力 vs 调度精度 |
| Drain 策略 | 严格遵守 PDB | 快速驱逐提高效率 | 安全性 vs 运维效率 |

## 最佳实践
1. 监控 Node Ready 状态变化，配置告警在节点 NotReady 超过 1 分钟时通知
2. 为关键服务配置 PodDisruptionBudget，防止 drain 操作同时驱逐过多副本
3. 定期检查节点的 allocatable 资源使用率，设置 80% 水位告警预防资源耗尽
4. 使用 `kubectl drain` 时始终携带 `--ignore-daemonsets` 和 `--delete-emptydir-data` 避免卡住

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- apiserver
- 节点诊断
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
