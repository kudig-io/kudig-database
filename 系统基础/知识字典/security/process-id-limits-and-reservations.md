---
title: Process ID Limits And Reservations（进程 ID 限制与预留）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
tier: peripheral
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Process ID Limits And Reservations（进程 ID 限制与预留） 是什么
- 如何 Process ID Limits And Reservations（进程 ID 限制与预留）
trigger_keywords:
- Process
- ID
- Limits
- And
- Reservations
- 进程
- 限制与预留
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Process ID Limits And Reservations（进程 ID 限制与预留）

## 概述

进程 ID（PIDs）是节点上的基本资源。[[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 允许限制单个 Pod 可使用的 PID 数量，同时也可为节点预留一定数量的可分配 PID，供操作系统和 Kubernetes 守护进程使用。PID 耗尽很容易在未触及其他资源限制的情况下发生，进而导致主机守护进程（如 [[kubelet|kubelet]]、kube-proxy、容器运行时）无法运行，引发节点不稳定。

## 核心概念/原理

- **PID 是节点级稀缺资源**：Linux 系统的 PID 数量存在上限（如 `/proc/sys/kernel/pid_max`）。如果 Pod 无限制地创建进程，可能迅速耗尽节点 PID，影响系统进程和其他工作负载。
- **在 kubelet 级别配置**：与 CPU、内存等资源不同，Pod 的 PID 限制不是在 Pod 的 `.spec` 中定义，而是通过 kubelet 的命令行参数或配置文件在**节点级别**进行设置。
- **双重保护**：
  - **Per-Pod PID limiting**：限制单个 Pod 可使用的 PID 数量，防止一个 Pod 影响其他 Pod。
  - **Node PID reservation**：为系统进程和 Kubernetes 守护进程预留 PID，防止工作负载将节点 PID 耗尽。

## 关键机制或特性

### 1. 节点 PID 预留（Node PID limits）

通过 kubelet 的 `--system-reserved` 和 `--kube-reserved` 命令行选项中的 `pid=<number>` 参数进行配置：
- `--system-reserved=pid=<number>`：为操作系统整体预留的 PID 数量。
- `--kube-reserved=pid=<number>`：为 Kubernetes 系统守护进程（kubelet 等）预留的 PID 数量。

### 2. Pod PID 限制（Pod PID limits）

通过以下方式配置每个 Pod 的最大 PID 数：
- kubelet 命令行参数：`--pod-max-pids`
- kubelet 配置文件字段：`PodPidsLimit`

每个节点可以有不同的 PID 限制。例如，若节点主机 OS 最大 PID 为 262144，且预期运行少于 250 个 Pod，可为每个 Pod 分配 1000 个 PID 的预算。

### 3. 基于 PID 的驱逐（PID based eviction）

kubelet 支持使用 `pid.available` 驱逐信号来配置 Pod 的 PID 使用阈值。当 Pod 异常消耗大量 PID 时，kubelet 可启动驱逐流程。驱逐信号值是**周期性计算**的，它本身并不强制限制 PID 使用。真正的硬限制由 per-Pod 和 per-Node 的 PID limiting 设置。

### 4. 硬限制行为

一旦达到硬限制，工作负载在尝试获取新 PID 时将失败。根据工作负载对这些失败的反应以及 Pod 的存活探针和就绪探针配置，Pod 可能会被重新调度，也可能不会。

## 使用场景

- **防止 fork bomb 攻击**：限制单个 Pod 的 PID 使用，防止简单的 fork bomb 影响整个集群的节点稳定性。
- **保护系统守护进程**：为 kubelet、kube-proxy 和容器运行时预留足够的 PID，确保关键系统服务始终可用。
- **多租户节点隔离**：在共享节点上运行多个 Pod 时，确保某个异常 Pod 不会耗尽节点 PID，从而影响同节点上的其他租户工作负载。

## 最佳实践/注意事项

- **统一节点配置**：由于 PID 限制是在 kubelet 级别配置的，不同节点可能有不同的限制。为简化管理，建议所有节点使用相同的 PID 资源限制和预留值。
- **检查系统默认 PID 上限**：某些 Linux 安装将默认 PID 上限设置得较低（如 32768）。在大型集群或高密度部署场景下，建议提高 `/proc/sys/kernel/pid_max` 的值。
- **Per-Pod 限制的局限性**：Per-Pod PID 限制可以保护 Pod 之间的相互影响，但**不能保证**所有调度到该节点的 Pod 总和不会导致节点整体 PID 耗尽，也**不能保护**节点代理自身免受 PID 耗尽影响。因此，必须配合 Node PID reservation 使用。
- **PID 限制与资源请求/限制的关系**：PID limiting 是计算资源请求和限制的重要补充，但配置方式完全不同（kubelet 配置 vs Pod spec 配置），目前不支持在 Pod 级别定义 PID limit。
- **驱逐信号的补充作用**：虽然 `pid.available` 驱逐信号有助于在 PID 异常增长时进行干预，但由于其计算是周期性的，对于 PID 快速增长的情况，仍可能无法及时阻止节点进入不稳定状态。硬限制才是最终防线。

## 参考链接

- [Kubernetes 官方文档 - Process ID Limits And Reservations](https://kubernetes.io/docs/concepts/policy/pid-limiting/)

## Related

- [[系统基础/topic-dictionary/security/admission-controller.md|准入控制器]]
- [[系统基础/topic-dictionary/security/application-security-checklist.md|应用安全清单]]
- [[系统基础/topic-dictionary/security/athenz.md|Athenz 身份认证与授权]]


<!-- risk-assessed -->
