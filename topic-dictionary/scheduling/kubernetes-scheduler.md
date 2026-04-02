# Kubernetes Scheduler

## 概述

在 Kubernetes 中，调度（Scheduling）是指将 Pod 与节点（Node）进行匹配，以便 Kubelet 能够运行它们的过程。kube-scheduler 是 Kubernetes 的默认调度器，作为控制平面的一部分运行。它负责为新创建的或未调度的 Pod 选择最优的节点。

## 核心概念/原理

调度器会持续监视那些尚未分配节点的新建 Pod。对于每个发现的 Pod，调度器负责找到最适合运行它的节点。这个决策过程主要考虑以下两个阶段：

1. **过滤（Filtering）**：找到一组可以运行该 Pod 的可行节点（feasible nodes）。例如，`PodFitsResources` 过滤器会检查候选节点是否有足够的可用资源来满足 Pod 的资源请求。如果过滤后节点列表为空，则该 Pod 暂时不可调度。
2. **评分（Scoring）**：对通过过滤的节点进行排名，选择最合适的节点。调度器根据活跃的评分规则为每个节点分配一个分数，最终选择分数最高的节点。如果有多个节点分数相同，则随机选择一个。

调度决策会考虑的因素包括：个体和集体的资源需求、硬件/软件/策略约束、亲和性与反亲和性规范、数据本地性、工作负载间的干扰等。

## 关键机制或特性

- **Binding（绑定）**：调度器选定节点后，会通知 API server 这一决策。
- **Scheduling Policies（调度策略）**：允许配置 Predicates（用于过滤）和 Priorities（用于评分）。
- **Scheduling Profiles（调度配置文件）**：允许配置实现不同调度阶段的插件，包括 `QueueSort`、`Filter`、`Score`、`Bind`、`Reserve`、`Permit` 等。还可以配置 kube-scheduler 运行不同的配置文件。
- **可替换性**：kube-scheduler 的设计允许用户在需要时编写自己的调度组件来替代默认调度器。

## 使用场景

- 理解 Pod 为什么被放置在特定节点上。
- 计划实现自定义调度器。
- 优化大规模集群中的 Pod 放置策略。

## 最佳实践/注意事项

- 调度器在选择节点时采用两阶段操作，先过滤后评分。
- 如果没有节点满足 Pod 的调度需求，Pod 将保持未调度状态，直到有合适的节点出现。
- 可以通过配置多个调度配置文件来适应不同类型的工作负载。

## 参考链接

- [Kubernetes 官方文档 - kube-scheduler](https://kubernetes.io/docs/concepts/scheduling-eviction/kube-scheduler/)
