---
title: KubeEdge
description: KubeEdge 是 CNCF 孵化项目，将 Kubernetes 的能力扩展到边缘计算场景。它在云边之间建立安全通信通道，让边缘节点可以离线自治运行，适合
  I...
summary: KubeEdge 是 CNCF 孵化项目，将 Kubernetes 的能力扩展到边缘计算场景。它在云边之间建立安全通信通道，让边缘节点可以离线自治运行，适合
  I...
category: dictionary
tags:
- k8s
- glossary
- kubeedge
- edge-computing
- cncf
- iot
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeEdge 是什么
- KubeEdge 详解
trigger_keywords:
- KubeEdge
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KubeEdge

> **英文名**: KubeEdge

## 概述

KubeEdge 是 CNCF 孵化项目，将 Kubernetes 的能力扩展到边缘计算场景。它在云边之间建立安全通信通道，让边缘节点可以离线自治运行，适合 IoT、CDN、零售等边缘场景。

## 核心概念/原理

### 核心架构

| 组件 | 位置 | 功能 |
|------|------|------|
| CloudCore | 云端（K8s 集群） | 管理边缘节点和下发配置 |
| EdgeCore | 边缘节点 | 运行 Pod、设备管理、离线自治 |
| EdgeMesh | 边缘 | 边缘节点间的服务网格 |
| Device Controller | 边缘 | IoT 设备管理 |

### 云边协同

- **配置下发**：云端创建资源，自动同步到边缘。
- **状态上报**：边缘节点状态异步上报到云端。
- **离线自治**：边缘节点断网后继续运行，恢复后自动同步。

## 关键机制或特性

- **离线自治**：边缘节点网络中断后仍可运行工作负载。
- **轻量级**：EdgeCore 资源占用极小（适合 ARM 设备）。
- **设备管理**：通过 Device CRD 管理 IoT 设备（MQTT/Modbus）。
- **EdgeMesh**：边缘节点间的服务发现和负载均衡。
- 支持 ARM64 架构。

## 使用场景与最佳实践

- IoT/边缘场景使用 KubeEdge 将 K8s 能力下沉到边缘。
- 利用离线自治能力应对不稳定的边缘网络。
- 使用 Device Controller 统一管理 IoT 设备。
- 边缘节点优先部署 DaemonSet 类型的监控和日志 Agent。
- 合理规划云边网络带宽，避免大量资源同步。

## 参考链接

- [KubeEdge Official](https://kubeedge.io/)

## Related

- [[系统基础/知识字典/fundamentals/node.md|Node]]
- [[系统基础/知识字典/fundamentals/cluster.md|Cluster]]
- [[系统基础/知识字典/workloads/daemonset.md|DaemonSet]]
- [[系统基础/知识字典/networking/service.md|Service]]
- [[系统基础/知识字典/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->
