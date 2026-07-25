---
title: 节点关闭（Node Shutdowns）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- controller-manager
- pdb
- statefulset
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 节点关闭（Node Shutdowns） 是什么
- 如何 节点关闭（Node Shutdowns）
trigger_keywords:
- 节点关闭
- Node
- Shutdowns
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 节点关闭（Node Shutdowns）

## 概述

在 [[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]] 集群中，节点可能会因为计划内维护或意外原因（如断电）而关闭。如果节点在关闭前未被清空（drain），可能导致工作负载失败。节点关闭分为**优雅关闭（graceful）**和**非优雅关闭（non-graceful）**两种类型。Kubernetes 提供了相应的机制来尽量降低节点关闭对工作负载的影响。

## 核心概念/原理

- **优雅节点关闭**：[[kubelet|kubelet]] 尝试检测系统关闭信号，按照正常的 Pod 终止流程停止节点上的 Pod，并在关闭期间拒绝接收新 Pod。
- **非优雅节点关闭**：kubelet 的关闭管理器未检测到关闭事件，Pod 可能长时间停留在 Terminating 状态，[[StatefulSet|StatefulSet]] 无法在新节点重建同名 Pod，卷也无法重新挂载。
- **systemd 抑制锁（inhibitor locks）**：Linux 上的优雅关闭依赖 systemd 的抑制锁来延迟关机，为 Pod 终止争取时间。
- **Windows 服务控制处理程序**：Windows 上的优雅关闭依赖 kubelet 以 Windows 服务运行，通过注册服务控制处理程序来延迟预关闭事件。

## 关键机制或特性

### 优雅节点关闭配置

通过 `KubeletConfiguration` 中的以下选项配置：

- `shutdownGracePeriod`：节点延迟关闭的总时长（普通 Pod + 关键 Pod 的优雅终止总时间）。
- `shutdownGracePeriodCriticalPods`：用于终止关键 Pod 的时长，必须小于 `shutdownGracePeriod`。

例如，`shutdownGracePeriod=30s`、`shutdownGracePeriodCriticalPods=10s`，则前 20 秒用于普通 Pod，后 10 秒用于关键 Pod。

### 基于 Pod 优先级的优雅关闭

FEATURE STATE: `Kubernetes v1.24 [beta]`（默认启用）

通过 `shutdownGracePeriodByPodPriority` 配置，可以按 Pod 的 PriorityClass 值分阶段关闭，实现更细粒度的关闭控制。需要启用 `GracefulNodeShutdownBasedOnPodPriority` 特性门控。

### 非优雅节点关闭处理

FEATURE STATE: `Kubernetes v1.28 [stable]`（默认启用）

当节点发生非优雅关闭时，可手动为节点添加污点 `node.kubernetes.io/out-of-[[Service|service]]`（效果为 `NoExecute` 或 `NoSchedule`），系统会强制删除无对应容忍的 Pod，并立即执行卷分离操作，使 Pod 能在其他节点快速恢复。

### 强制存储分离超时

如果 Pod 删除在 6 分钟内未成功，且节点不健康，Kubernetes 将强制分离卷。此行为可选，可通过 `kube-controller-manager` 的 `disable-force-detach-on-timeout` 配置禁用。

## 使用场景

- **计划内节点维护**：在关机前启用优雅关闭，确保工作负载有序终止。
- **意外断电或硬件问题**：通过非优雅关闭处理机制（out-of-service 污点）快速恢复 StatefulSet 和带状态应用。
- **关键业务保护**：通过基于优先级的关闭策略，优先保证高优先级业务的终止时间。

## 最佳实践/注意事项

- 在计划内维护前，优先使用 `kubectl drain` 清空节点，减少工作负载中断。
- 配置合理的 `shutdownGracePeriod` 和 `shutdownGracePeriodCriticalPods`，确保关键 Pod 有足够时间优雅终止。
- 添加 `node.kubernetes.io/out-of-service` 污点前，务必确认节点确实已关机或断电（而非正在重启）。
- Pod 开始终止后，即使节点关闭被取消，已终止的 Pod 也不会被 kubelet 恢复，需要重新调度。
- 使用非优雅节点关闭流程时需谨慎，操作不当可能导致数据损坏。
- 控制平面节点不建议配置 swap，且应确保关键系统守护进程不受 swap 影响。

## 故障排查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| Pod 长时间 Terminating（节点关机后） | 非优雅关闭，kubelet 未检测到 shutdown | `kubectl get pods -o wide --field-selector=status.phase=Running` | 添加 `node.kubernetes.io/out-of-service` 污点 |
| StatefulSet Pod 无法在新节点重建 | 卷未分离，仍绑定到旧节点 | `kubectl describe pv <pv-name>` | 使用 out-of-service 污点触发强制卷分离 |
| 优雅关闭期间 Pod 被强杀 | shutdownGracePeriod 设置过短 | `journalctl -u kubelet | grep shutdown` | 增大 `shutdownGracePeriod` 和 `shutdownGracePeriodCriticalPods` |
| 节点关机后 kubelet 未正确终止 Pod | systemd 抑制锁未生效 | `systemd-inhibit --list` | 确认 kubelet 注册了 shutdown inhibitor lock |
| 关键 Pod 优先级关闭策略未生效 | `GracefulNodeShutdownBasedOnPodPriority` 未启用 | `kubelet --feature-gates` | 确认特性门控已启用并配置 `shutdownGracePeriodByPodPriority` |

## 生产检查清单

- [ ] `shutdownGracePeriod` 和 `shutdownGracePeriodCriticalPods` 已合理配置
- [ ] 关键 Pod 设置了高 PriorityClass
- [ ] 计划内维护使用 `kubectl drain` 而非直接关机
- [ ] 非优雅关闭的 out-of-service 污点操作流程已文档化
- [ ] StatefulSet 的 PDB 已正确配置
- [ ] 强制卷分离超时（6 分钟）的行为已确认
- [ ] Windows 节点的 kubelet 以服务方式运行

## 命令快速参考

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl taint nodes`：变更污点影响 Pod 调度

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 优雅清空节点（计划内维护）
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data

# 标记节点为 out-of-service（非优雅关闭）
kubectl taint nodes <node> node.kubernetes.io/out-of-service=nodeshutdown:NoExecute

# 移除 out-of-service 污点（节点恢复后）
kubectl taint nodes <node> node.kubernetes.io/out-of-service-

# 检查节点 condition
kubectl get nodes -o custom-columns=NAME:.metadata.name,READY:.status.conditions[-1].status

# 查看 kubelet shutdown 日志
journalctl -u kubelet | grep -i shutdown

# 检查 systemd 抑制锁
systemd-inhibit --list
```
## 交叉引用

- [Node Shutdowns - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/node-shutdown/)
- 相关主题：[Disruptions](../workloads/disruptions.md) · [Taints and Tolerations](../scheduling/taints-and-tolerations.md) · [Pod Lifecycle](../workloads/pod-lifecycle.md)

## 参考链接

- [Node Shutdowns]()

## Related

- [[17-系统基础/06-知识字典/operations/argo.md|Argo]]
- [[17-系统基础/06-知识字典/operations/backup-disaster-recovery.md|备份与灾难恢复（Backup & Disaster Recovery）]]
- [[17-系统基础/06-知识字典/operations/capacity-planning-forecasting.md|13 - 容量规划与资源预测]]


<!-- risk-assessed -->
