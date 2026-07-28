---
title: 节点自动扩缩容（Node Autoscaling）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- scheduler
- hpa
- vpa
- pdb
- daemonset
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 节点自动扩缩容（Node Autoscaling） 是什么
- 如何 节点自动扩缩容（Node Autoscaling）
trigger_keywords:
- 节点自动扩缩容
- Node
- Autoscaling
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 节点自动扩缩容（Node Autoscaling）

## 概述

节点自动扩缩容（Node Autoscaling）能够根据集群中工作负载的需求，自动**供应（provision）**新节点或**整合（consolidate）**现有节点，以提供所需容量的同时优化成本。执行这些操作的组件称为**节点自动扩缩器（Node autoscalers）**。

## 核心概念/原理

- **节点供应（Node Provisioning）**：当集群中有 Pod 因资源不足而无法调度到现有节点时，自动扩缩器会创建新的节点来容纳这些 Pod。
- **节点整合（Node Consolidation）**：当集群中存在利用率较低的节点时，自动扩缩器会移除这些节点，并将上面的 Pod 重新调度到其他节点，从而提高整体利用率、降低成本。
- **自动扩缩器的工作输入**：
  - Pod 的调度约束（如资源请求、节点亲和性、存储卷需求等）。
  - 自动扩缩器配置施加的节点约束（如节点组、最小/最大节点数等）。
- **注意**：扩缩器仅根据 Pod 的 **resource requests** 做决策，而不是实际运行时的资源使用量。

## 关键机制或特性

### 节点供应

自动扩缩器通过调用云提供商 API 创建/删除支撑节点的资源（最常见的是虚拟机）。主要目标是为不可调度的 Pod 提供可调度性，同时尽量降低成本或在故障域间平衡节点数量。

**自动配置（Auto-provisioning）**：部分扩缩器支持无需预先完全配置节点规格，而是根据待调度 Pod 的需求动态选择节点配置。

### 节点整合

整合通过移除一组利用率低的节点来提升成本效益。移除非空节点是有影响的：节点上的 Pod 会被终止并可能需要重新创建，但**整合通常不应导致任何 Pod 处于 Pending 状态**。

- 空节点：仅运行 [[daemonset|DaemonSet]] 和静态 Pod 的节点，整合更简单直接。
- 非空节点：移除会导致 Pod 中断，但自动扩缩器会预测重新调度结果，确保 Pod 能安置在现有或新替换节点上。

### 自动扩缩器实现

目前由 SIG Autoscaling 维护的主流实现有：

#### Cluster Autoscaler

- 向预配置的**节点组（Node groups）**添加或移除节点。
- 不支持自动配置，所有可供应节点必须来自预配置节点组。
- 直接提供多种云提供商集成。

#### Karpenter

- 基于 `NodePool` 配置自动配置节点。
- 管理节点整个生命周期（包括自动刷新、自动升级等）。
- 直接与单个云资源（如单个 VM）交互，不依赖云提供商资源组。
- 目前主要集成 AWS 和 Azure。

### 与工作负载自动扩缩容结合

- **水平工作负载自动扩缩容（HPA）**：根据负载自动调整 Pod 副本数，配合节点自动扩缩容可在负载增加时自动增加节点，负载降低时自动释放节点。
- **垂直工作负载自动扩缩容（VPA）**：根据历史资源使用自动调整 Pod 的 resource requests，配合节点自动扩缩容可优化资源请求设置，提升成本效益。
  - **注意**：不建议为 DaemonSet Pod 启用 VPA，因为自动扩缩器需要预测 DaemonSet 在新节点上的资源占用，VPA 会导致预测不可靠。

### 其他相关组件

- **Descheduler**：基于自定义策略提供节点整合功能，也可优化 Pod/节点分布。
- **Cluster Proportional Autoscaler / Cluster Proportional Vertical Autoscaler**：基于集群节点数量进行工作负载扩缩容。

## 使用场景

- **弹性业务负载**：业务流量波动大，需要随 Pod 数量变化自动调整节点规模。
- **成本优化**：在低峰期自动缩减节点数量，减少闲置资源开销。
- **突发调度需求**：大数据批处理、CI/CD 等临时性任务需要快速扩展节点容量。
- **资源请求优化困难**：结合 VPA 自动调整请求值，再让节点自动扩缩容匹配实际容量。

## 最佳实践/注意事项

- 正确设置 Pod 的 resource requests 是保证集群成本效益的关键。
- 节点自动扩缩容与 HPA/VPA 配合使用，可实现端到端的弹性伸缩。
- 选择自动扩缩器时，根据云提供商支持情况和功能需求（是否需要节点生命周期管理）在 Cluster Autoscaler 和 Karpenter 之间做出选择。
- 在整合过程中，虽然扩缩器会预测调度结果，但实际调度不由扩缩器控制，仍可能出现少量 Pod Pending 的情况（例如整合过程中恰好有新 Pod 创建）。
- 不建议为 DaemonSet 启用垂直自动扩缩容，以免影响节点资源预测。

## 故障排查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| Pending Pod 长时间无节点扩容 | Autoscaler 未识别到 Pending Pod | `kubectl get [[Pods|pods]] --field-selector=status.phase=Pending` | 检查 Autoscaler 日志和节点组配置 |
| 扩容后节点 NotReady | 节点初始化超时或 [[kubelet|kubelet]] 配置错误 | `kubectl describe node <new-node>` | 检查 cloud-init 日志和 kubelet 状态 |
| 缩容未触发（闲置节点仍存在） | Pod 有 PDB 或 local storage 阻止驱逐 | `kubectl logs -n kube-system cluster-autoscaler-*` | 检查 PDB、annotation `cluster-autoscaler.[[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]].io/safe-to-evict` |
| Karpenter 选择了错误的实例类型 | NodePool 约束配置不当 | `kubectl get nodeclaim -o wide` | 调整 NodePool 的 instanceType 和 requirements |
| 节点频繁扩缩（抖动） | 扩缩阈值设置过于敏感 | 查看 Autoscaler `--scale-down-delay-after-add` | 增大 scale-down 延迟和 utilization 阈值 |
| DaemonSet 资源预测错误 | VPA 修改了 DaemonSet 的 requests | `kubectl get vpa -A` | 不要为 DaemonSet 启用 VPA |

## 生产检查清单

- [ ] 所有 Pod 已正确设置 resource requests
- [ ] 节点组 / NodePool 的 min/max 节点数已合理配置
- [ ] Cluster Autoscaler / Karpenter 日志可正常查看
- [ ] PDB 不会阻塞整个节点缩容
- [ ] 扩缩容延迟参数已根据业务特性调优
- [ ] 节点启动脚本（cloud-init / user-data）经过验证
- [ ] HPA + 节点自动扩缩容联动测试已通过
- [ ] Spot/Preemptible 节点配置了正确的 taints 和 labels

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Pending Pods
kubectl get pods -A --field-selector=status.phase=Pending

# 查看 Cluster Autoscaler 状态
kubectl get configmap -n kube-system cluster-autoscaler-status -o yaml

# 查看 Karpenter NodePool 和 NodeClaim
kubectl get nodepool
kubectl get nodeclaim -o wide

# 查看节点资源分配情况
kubectl describe nodes | grep -A 5 "Allocated resources"

# 手动标记节点可安全驱逐
kubectl annotate node <node> cluster-autoscaler.kubernetes.io/safe-to-evict="true"

# 查看 Autoscaler 日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=100
```
## 交叉引用

- [Node Autoscaling - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/node-autoscaling/)
- 相关主题：[Horizontal Pod Autoscaling](../workloads/horizontal-pod-autoscaling.md) · [Vertical Pod Autoscaling](../workloads/vertical-pod-autoscaling.md) · [Karpenter Autoscaling](../scheduling/karpenter-autoscaling.md)

## 参考链接

- [Node Autoscaling]()

## Related

- [[17-系统基础/06-知识字典/operations/argo.md|Argo]]
- [[17-系统基础/06-知识字典/operations/backup-disaster-recovery.md|备份与灾难恢复（Backup & Disaster Recovery）]]
- [[17-系统基础/06-知识字典/operations/capacity-planning-forecasting.md|13 - 容量规划与资源预测]]


<!-- risk-assessed -->
