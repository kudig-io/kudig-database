---
title: 日志架构（Logging Architecture）
description: '# 日志架构（Logging Architecture）'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- scheduler
- daemonset
- agent
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 日志架构（Logging Architecture） 是什么
- 如何 日志架构（Logging Architecture）
trigger_keywords:
- 日志架构
- Logging
- Architecture
- dictionary
title_en: Logging
---


# 日志架构（Logging Architecture）

## 概述

应用日志是理解集群内部运行情况、调试问题和监控集群活动的重要手段。容器化应用最常见的日志记录方式是写入标准输出（`stdout`）和标准错误（`stderr`）。然而，仅靠容器引擎的原生功能通常不足以构建完整的日志解决方案。Kubernetes 引入了**集群级日志（cluster-level logging）**的概念，要求日志拥有独立于节点、Pod 和容器的存储和生命周期。

## 核心概念/原理

- **容器日志捕获**：容器运行时通过 CRI 日志格式捕获容器的 `stdout` 和 `stderr`，kubelet 将这些日志提供给 `kubectl logs` 使用。
- **集群级日志架构**：需要一个独立的后端来存储、分析和查询日志，Kubernetes 本身不提供原生日志存储方案。
- **日志轮转**：kubelet 负责管理容器日志的轮转，防止日志无限增长占满磁盘。
- **系统组件日志**：一部分组件以容器方式运行（如 scheduler、kube-proxy），另一部分直接运行在宿主机上（如 kubelet、容器运行时）。

## 关键机制或特性

### Pod 和容器日志

Kubernetes 捕获每个运行中容器的日志。可以通过以下命令查看：

```bash
kubectl logs <pod-name>
kubectl logs <pod-name> -c <container-name>
kubectl logs <pod-name> --previous
```

FEATURE STATE: `Kubernetes v1.32 [alpha]`

启用 `PodLogsQuerySplitStreams` 特性门控后，可以通过 Pod API 直接分别获取 `stdout` 和 `stderr` 流：

```bash
kubectl get --raw "/api/v1/namespaces/default/pods/<pod-name>/log?stream=Stderr"
```

### 日志轮转

FEATURE STATE: `Kubernetes v1.21 [stable]`

kubelet 通过以下配置控制日志轮转：

- `containerLogMaxSize`（默认 10Mi）：单个日志文件的最大大小。
- `containerLogMaxFiles`（默认 5）：每个容器允许的最大日志文件数。
- `containerLogMaxWorkers`：并发日志轮转的最大 worker 数。
- `containerLogMonitorInterval`：日志轮转监控检查间隔。

注意：`kubectl logs` 默认只返回最新日志文件的内容。

### 系统组件日志位置

- **Linux（systemd）**：kubelet 和容器运行时写入 `journald`，可用 `journalctl` 查看；非 systemd 环境写入 `/var/log` 下的 `.log` 文件。
- **Windows**：默认写入 `C:\var\logs`，部分部署工具使用 `C:\var\log\kubelet`。
- **容器内运行的组件**：直接写入 `/var/log` 下的 `.log` 文件，绕过默认容器日志机制。

### 集群级日志架构方案

#### 1. 节点级日志代理（Node-level logging agent）

在每个节点上运行日志代理（如 Fluent Bit、Fluentd，通常以 DaemonSet 部署），收集节点上所有容器的日志并转发到集中式日志存储。这是最常见的方式，无需修改应用。

#### 2. Sidecar 容器

- **流式 sidecar**：读取应用日志文件并输出到自己的 `stdout`/`stderr`，利用 kubelet 和节点日志代理收集。
- **带日志代理的 sidecar**：在 Pod 内运行独立的日志代理（如 fluentd），针对特定应用进行日志路由和转换。
  - 注意：这种方式资源消耗较大，且无法通过 `kubectl logs` 访问这些日志。

#### 3. 应用直接暴露/推送日志

应用直接将日志发送到日志后端。此方式超出了 Kubernetes 本身的范围。

## 使用场景

- **日常应用调试**：通过 `kubectl logs` 快速查看应用输出。
- **故障排查**：在容器崩溃、Pod 被驱逐或节点故障后，通过集群级日志后端检索历史日志。
- **安全审计**：收集审计日志和系统组件日志，用于合规性分析和威胁检测。
- **多格式日志分离**：通过 sidecar 将不同格式的日志流分离到不同的收集通道。

## 最佳实践/注意事项

- 优先使用节点级日志代理（DaemonSet）实现集群级日志收集，侵入性最小。
- 如果应用只写单个日志文件，建议直接将输出重定向到 `/dev/stdout`，避免使用流式 sidecar 造成存储翻倍。
- 为系统组件日志配置日志轮转，防止 `/var/log` 目录占满磁盘。
- 容器内运行的 Kubernetes 组件若将日志映射到节点共享卷，需要自行确保日志轮转机制生效。
- 使用 sidecar 运行独立日志代理时，注意资源消耗增加，且 `kubectl logs` 不可用。
- kubelet 的 `podLogsDir` 参数可自定义 Pod 日志目录，但修改需谨慎，因为许多工具默认依赖 `/var/log/pods`。

## 参考链接

- [Logging Architecture - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/logging/)
