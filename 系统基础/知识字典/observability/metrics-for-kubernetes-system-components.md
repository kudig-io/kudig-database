---
title: Kubernetes 系统组件指标
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- rbac
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 系统组件指标 是什么
- 如何 Kubernetes 系统组件指标
trigger_keywords:
- Kubernetes
- 系统组件指标
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 系统组件指标

## 概述

Kubernetes 系统组件指标能够帮助我们深入了解集群内部的运行状况，对于构建监控仪表板和告警系统尤为重要。Kubernetes 组件以 [[Prometheus|Prometheus]] 文本格式暴露指标，便于人和机器共同阅读和处理。

## 核心概念/原理

- **指标端点**：大多数组件默认在 HTTP 服务器的 `/metrics` 端点暴露指标。未默认暴露的组件可通过 `--bind-address` 启用。
- **Prometheus 格式**：结构化的纯文本格式，包含指标名称、标签、类型和帮助信息。
- **RBAC 授权**：如果集群启用了 RBAC，读取指标需要具备访问 `/metrics` 的权限（`nonResourceURLs` 的 `get` 权限）。
- **指标生命周期**：Alpha → Beta → Stable → Deprecated → Hidden → Deleted，不同阶段有不同的稳定性保证。

## 关键机制或特性

### 主要组件指标端点

- **kube-controller-manager**：提供控制器性能和健康指标，如 [[系统基础/知识字典/fundamentals/etcd.md|etcd]] 请求延迟、云提供商 API 延迟等。
- **kube-scheduler**：暴露可选指标，报告运行中 Pod 的请求资源和限制（`/metrics/resources`）。
- **[[kubelet|kubelet]]**：除 `/metrics` 外，还提供 `/metrics/cadvisor`、`/metrics/resource` 和 `/metrics/probes`。
- **kube-apiserver**、**kube-proxy**：均在 `/metrics` 端点暴露指标。

### kubelet PSI 指标

FEATURE STATE: `Kubernetes v1.34 [beta]`

kubelet 支持收集 Linux 内核的 **Pressure Stall Information (PSI)** 指标，用于监控 CPU、内存和 I/O 的压力情况。指标在 `/metrics/cadvisor` 暴露，包括：

- `container_pressure_cpu_stalled_seconds_total`
- `container_pressure_cpu_waiting_seconds_total`
- `container_pressure_memory_stalled_seconds_total`
- `container_pressure_memory_waiting_seconds_total`
- `container_pressure_io_stalled_seconds_total`
- `container_pressure_io_waiting_seconds_total`

要求：Linux 内核 4.20+、cgroup v2。

### 指标生命周期管理

| 阶段 | 说明 |
|------|------|
| Alpha | 无稳定性保证，可随时修改或删除 |
| Beta | 标签不可删除，可新增标签 |
| Stable | 保证不修改名称、类型，不删除 |
| Deprecated | 计划删除，但仍可用，带弃用版本注解 |
| Hidden | 不再发布，但可通过标志启用 |
| Deleted | 完全删除，不可用 |

### 隐藏指标启用

通过 `--show-hidden-metrics-for-version` 标志可启用上一版本中隐藏的指标，作为管理员的应急手段。该标志值只能为上一小版本（如 `1.29`）。

### 指标禁用与基数控制

- **禁用指标**：通过 `--disabled-metrics=metric1,metric2` 显式关闭特定指标。
- **标签值白名单**：通过 `--allow-metric-labels` 限制指标标签的取值范围，防止高基数指标导致内存问题。

## 使用场景

- **集群健康监控**：通过 Prometheus 等工具抓取指标，构建实时仪表板。
- **容量规划**：利用 scheduler 的 `/metrics/resources` 评估资源请求和限制分布。
- **性能调优**：通过 controller-manager 和 kubelet PSI 指标识别瓶颈和压力点。
- **告警与自动化**：基于指标设置告警阈值，触发自动修复或扩缩容。

## 最佳实践/注意事项

- 生产环境中配置 Prometheus 或其他 metrics scraper 定期收集指标并存储到时序数据库。
- 关注指标的生命周期阶段，避免依赖 Alpha 指标构建关键告警。
- 在升级集群前，检查是否有依赖的指标被弃用或隐藏，必要时使用 `--show-hidden-metrics-for-version` 过渡。
- 对高基数指标使用 `--allow-metric-labels` 进行限制，防止组件内存异常增长。
- 监控 `cardinality_enforcement_unexpected_categorizations_total` 元指标，了解基数控制的触发情况。

## 参考链接

- [Metrics For Kubernetes System Components - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/system-metrics/)

## Related

- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
