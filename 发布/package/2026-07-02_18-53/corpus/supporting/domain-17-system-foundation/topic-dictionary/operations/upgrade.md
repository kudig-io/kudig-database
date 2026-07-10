---
title: 升级
description: Upgrade（升级）是指将 Kubernetes 集群或组件从旧版本升级到新版本的过程。Kubernetes 支持滚动升级策略，确保升级过程中集群持续可用。...
summary: Upgrade（升级）是指将 Kubernetes 集群或组件从旧版本升级到新版本的过程。Kubernetes 支持滚动升级策略，确保升级过程中集群持续可用。...
category: dictionary
tags:
- k8s
- glossary
- operations
- upgrade
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 升级 是什么
- Upgrade 详解
trigger_keywords:
- 升级
- Upgrade
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 升级

> **英文名**: Upgrade

## 概述

Upgrade（升级）是指将 Kubernetes 集群或组件从旧版本升级到新版本的过程。Kubernetes 支持滚动升级策略，确保升级过程中集群持续可用。

## 核心概念/原理

### 升级路径

Kubernetes 支持跨一个小版本升级（如 1.30 → 1.31），不支持跨多个版本。

```
推荐路径：1.30.x → 1.31.x → 1.32.x
不推荐：1.30.x → 1.32.x（跳版本）
```

### 升级顺序

```
1. 升级控制平面组件（API Server → Controller Manager → Scheduler）
2. 升级 kubelet（逐个节点 cordon + drain + 升级 + uncordon）
3. 升级 CoreDNS、kube-proxy 等系统组件
4. 验证集群健康状态
```

### kubeadm 升级

```bash
# 检查可用版本
kubeadm upgrade plan

# 升级控制平面
kubeadm upgrade apply v1.32.0

# 升级节点
kubeadm upgrade node
```

## 关键机制或特性

- 升级前必须备份 etcd 数据。
- 控制平面先升级，kubelet 后升级（kubelet 版本不能高于 API Server）。
- 升级期间 kubelet 版本可以比 API Server 低 2 个小版本（版本偏差策略）。
- 云厂商托管集群（EKS、ACK、GKE）通常提供自动化升级。

## 使用场景与最佳实践

- 在非生产环境充分测试升级后再在生产环境执行。
- 制定详细的升级计划和回滚方案。
- 选择维护窗口执行升级，减少对业务的影响。
- 监控升级过程中的关键指标（Pod 状态、API Server 延迟、etcd 健康）。

## 参考链接

- [Upgrade - Official Documentation](https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-upgrade/)

## Related

- [[domain-17-system-foundation/知识字典/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/知识字典/tooling/helm.md|Helm]]
- [[domain-17-system-foundation/知识字典/tooling/kustomize.md|Kustomize]]
- [[domain-17-system-foundation/知识字典/operations/cordon.md|Cordon]]
- [[domain-17-system-foundation/知识字典/operations/uncordon.md|Uncordon]]


<!-- risk-assessed -->
