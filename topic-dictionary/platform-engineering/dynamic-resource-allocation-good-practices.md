---
title: 动态资源分配（DRA）集群管理员最佳实践
description: '# 动态资源分配（DRA）集群管理员最佳实践'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- kubelet
- scheduler
- controller-manager
- daemonset
- job
- rbac
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 动态资源分配（DRA）集群管理员最佳实践 是什么
- 如何 动态资源分配（DRA）集群管理员最佳实践
trigger_keywords:
- 动态资源分配
- DRA
- 集群管理员最佳实践
- dictionary
title_en: Clusters
---


# 动态资源分配（DRA）集群管理员最佳实践

## 概述

动态资源分配（Dynamic Resource Allocation, DRA）是 Kubernetes 中用于管理专用硬件资源（如 GPU、FPGA 等）的一套机制。本文档面向集群管理员，介绍在配置和使用 DRA 时的最佳实践，包括驱动部署、升级、监控和性能调优等方面的建议。

## 核心概念/原理

- **DRA 驱动（DRA drivers）**：第三方应用程序，运行在集群每个节点上，负责与节点硬件和 Kubernetes 原生 DRA 组件交互。
- **DeviceClasses 和 ResourceSlices**：通常由管理员和 DRA 驱动管理，用于描述可用硬件资源。
- **ResourceClaim 和 ResourceClaimTemplate**：命名空间作用域的 API 对象，供集群运维人员或应用开发者在部署 Pod 时声明所需的专用资源。
- **RBAC 授权**：应使用 RBAC 等授权工具，根据用户角色控制对不同 DRA API 的访问权限。

## 关键机制或特性

### DRA 驱动部署与维护

- **部署方式**：通常以 DaemonSet 部署到全部或部分节点（通过 node selector 等机制）。
- **无缝升级（Seamless upgrades）**：部分 DRA 驱动支持通过 `kubeletplugin` 接口实现无缝升级，允许两个版本的驱动在短时间内共存。该功能需要 kubelet 1.33+，并且驱动需支持该特性。
  - 若无法使用无缝升级，升级期间可能出现：依赖 ResourceClaim 的新 Pod 无法启动、已停止 Pod 的资源清理被延迟、运行中的 Pod 不受影响。
- **健康检查**：DRA 驱动应暴露 gRPC 健康检查端点，建议配置为 DaemonSet 的 liveness probe，以便在驱动异常时自动重启，减少调度延迟和排障时间。
- **节点排空顺序**：在自定义节点排空逻辑时，应尽可能**最后**排空 DRA 驱动，确保驱动有机会为已分配资源的 Pod 执行清理（unprepare）。

### 高负载环境监控与调优

DRA 调度相比普通 Pod 调度会增加 API server 调用、内存和 CPU 消耗。在大规模环境中，应特别关注以下组件的调优：

#### kube-controller-manager

ResourceClaim 控制器由 kube-controller-manager 内部管理。关键指标：

- `workqueue_adds_total{name="resource_claim"}`：ResourceClaim 控制器的工作队列添加速率。
- `workqueue_depth{endpoint="kube-controller-manager", name="resource_claim"}`：工作队列深度，反映是否存在积压。
- `workqueue_work_duration_seconds{name="resource_claim"}`：处理工作的耗时分布。

根据测试（100 节点、720 长生命周期 Pod、80 搅动 Pod、Job 创建 QPS 为 10），`kube-controller-manager` 的 QPS 可设为 75、Burst 设为 150 作为下限参考。

#### kube-scheduler

- `scheduler_pod_scheduling_sli_duration_seconds`：调度端到端耗时。
- `scheduler_scheduling_algorithm_duration_seconds`：调度算法延迟。

#### kubelet

- `dra_operations_duration_seconds{operation_name="PrepareResources"}`：NodePrepareResources 操作耗时。
- `dra_operations_duration_seconds{operation_name="UnprepareResources"}`：NodeUnprepareResources 操作耗时。

#### DRA kubeletplugin

- `dra_grpc_operations_duration_seconds{method_name=~".*NodePrepareResources"}`：gRPC PrepareResources 耗时。
- `dra_grpc_operations_duration_seconds{method_name=~".*NodeUnprepareResources"}`：gRPC UnprepareResources 耗时。

## 使用场景

- **GPU/FPGA 等专用硬件调度**：为 AI/ML、视频转码、科学计算等工作负载动态分配加速器。
- **多租户硬件资源隔离**：通过 DRA 机制精细控制硬件资源的分配和回收。
- **大规模集群的硬件管理**：在高节点数、高 Pod 创建率环境中，稳定、高效地管理专用硬件资源。

## 最佳实践/注意事项

- 使用 RBAC 严格控制 DeviceClasses、ResourceSlices、ResourceClaims 和 ResourceClaimTemplates 的访问权限。
- 选择支持无缝升级的 DRA 驱动，并在 kubelet 1.33+ 环境中启用，以最小化升级期间的调度中断。
- 为 DRA 驱动配置 liveness probe，基于其 gRPC 健康检查端点，确保驱动高可用。
- 自定义 drain 逻辑时，确保在终止 DRA 驱动之前，先删除或确认没有已分配/预留的 ResourceClaim。
- 在大规模集群中，监控并适当提升 kube-controller-manager 和 kube-scheduler 的 QPS/Burst 配置。
- 持续关注 DRA 相关的工作队列指标和调度延迟指标，及时发现性能瓶颈。

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| DRA 驱动未注册设备 | ResourceSlice 未创建 | `kubectl get resourceslices` |
| Pod Pending 等待 ResourceClaim | 设备数量不足 | `kubectl describe resourceclaim`；检查节点可用设备 |
| 管理员访问被拒绝 | DRAAdminAccess 特性门控未启用 | 确认 apiserver 特性门控配置 |

## 生产检查清单

- [ ] DRA 驱动正确创建和更新 ResourceSlice
- [ ] 多租户集群限制管理员访问权限
- [ ] 设备健康状态纳入监控
- [ ] 避免 `spec.nodeName` 绕过调度器

## 命令快速参考

```bash
# 查看 ResourceSlice
kubectl get resourceslices -o wide

# 查看 DeviceClass
kubectl get deviceclasses

# 查看 ResourceClaim
kubectl get resourceclaims -A
```

## 交叉引用

- [Device Plugins](./device-plugins.md) — 传统设备插件方式
- [Operator 模式](./operator-pattern.md) — DRA 驱动通常以 Operator 形式部署

## 参考链接

- [Good practices for Dynamic Resource Allocation as a Cluster Admin - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/dra/)
