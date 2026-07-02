---
title: Scheduler Performance Tuning
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Scheduler Performance Tuning 是什么
- 如何 Scheduler Performance Tuning
trigger_keywords:
- Scheduler
- Performance
- Tuning
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Scheduler Performance Tuning

## 概述

kube-scheduler 是 [[Kubernetes|Kubernetes]] 的默认调度器，负责将 Pod 放置到集群的节点上。在大型集群中，可以通过调整调度器的行为来平衡调度延迟（新 Pod 快速放置）和准确性（调度器很少做出差的放置决策）。

## 核心概念/原理

### percentageOfNodesToScore

这是主要的性能调优参数，通过 `KubeSchedulerConfiguration` 设置。它决定了调度器在调度 Pod 时需要评分的节点数量阈值，表示为集群中所有节点总数的百分比。

- 取值范围：0 到 100 的整数。
- 0 表示使用编译时的默认值。
- 如果设置超过 100，调度器会当作 100 处理。

### 默认阈值

如果不指定阈值，Kubernetes 使用线性公式计算：
- 100 节点集群：50%
- 5000 节点集群：10%
- 自动值的下限为 5%

这意味着无论集群多大，调度器至少会评分 5% 的节点，除非显式将 `percentageOfNodesToScore` 设置为小于 5。

### 节点迭代方式

为了公平地考虑所有节点，调度器以轮询（round robin）方式迭代节点。如果节点分布在多个区域中，调度器会跨区域迭代节点，以确保来自不同区域的节点都能进入可行性检查。

## 关键机制或特性

- **提前停止搜索**：当调度器找到足够多的可行节点（超过配置的百分比）时，它会停止搜索更多可行节点，直接进入评分阶段。这在大型集群中可以节省大量时间。
- **轮询机制**：调度器维护一个节点数组，对每个 Pod 从上次停止的位置继续检查，确保所有节点都有公平的机会被考虑。
- **跨区域迭代**：如果节点分布在多个 zone，调度器会按 zone 交错迭代（如 zone1-node1, zone2-node1, zone1-node2, zone2-node2...）。
- **Opportunistic Batching**（v1.35+ beta）：在调度大规模工作负载时，允许调度器在调度周期之间重用过滤和评分结果，大幅提高调度速度。
  - 适用条件：Pod 没有 Pod 间亲和性/反亲和性、没有拓扑分布约束、没有 DRA ResourceClaim，且独占调度在节点上。
  - 缓存每 0.5 秒过期。

## 使用场景

- 数千节点的大型集群需要优化调度器吞吐量。
- 批量创建大量相似 Pod 时，需要减少调度延迟。
- 在调度速度和放置质量之间寻求平衡。

## 最佳实践/注意事项

- 对于几百个节点或更少的集群，保持默认值即可，修改配置不太可能显著改善性能。
- 避免将 `percentageOfNodesToScore` 设置得过低（建议不低于 10%），否则调度器可能频繁做出较差的 Pod 放置决策。
- 除非调度器吞吐量对应用至关重要且节点评分不重要（即只要能运行在任何可行节点上即可），否则不要将百分比设得太低。
- 检查的可行节点越少，某些可能获得更高评分的节点可能根本不会进入评分阶段，导致次优放置。
- 启用 Opportunistic Batching 时，需要满足特定配置条件（禁用默认拓扑分布、禁用 DRAExtendedResource 等）。

## 生产 YAML 示例

### 大规模集群调度器性能配置

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
percentageOfNodesToScore: 15               # 5000+ 节点集群建议 10-20%
profiles:
  - schedulerName: default-scheduler
    plugins:
      score:
        enabled:
          - name: NodeResourcesBalancedAllocation
            weight: 1
          - name: ImageLocality
            weight: 1
    pluginConfig:
      - name: DefaultPreemption
        args:
          minCandidateNodesPercentage: 10   # 抢占时评估的最小候选节点百分比
          minCandidateNodesAbsolute: 100
```

### 启用 Opportunistic Batching（v1.35+ beta）

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
percentageOfNodesToScore: 10
profiles:
  - schedulerName: default-scheduler
    plugins:
      preScore:
        disabled:
          - name: PodTopologySpread         # Opportunistic Batching 要求禁用
      score:
        disabled:
          - name: PodTopologySpread
    pluginConfig:
      - name: DefaultPreemption
        args:
          minCandidateNodesPercentage: 5
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 调度延迟 > 1s | percentageOfNodesToScore 过高或集群规模大 | 降低 `percentageOfNodesToScore`；检查 `scheduler_scheduling_attempt_duration_seconds` |
| Pod 被调度到非最优节点 | percentageOfNodesToScore 过低 | 提高百分比或检查 Score 插件配置 |
| 批量创建 Pod 时调度吞吐量低 | 未启用 Opportunistic Batching | 确认 v1.35+；检查是否满足 batching 前置条件 |
| Batching 缓存命中率低 | Pod 间亲和性 / 拓扑分布约束阻止 batching | 对批量 Pod 移除 podAffinity 和 topologySpreadConstraints |
| 调度器 CPU 使用率异常高 | Filter 阶段插件过多或节点数过大 | 使用 profiling 工具分析；禁用不必要的 Filter 插件 |

## 生产检查清单

- [ ] 100 节点以下集群保持默认值，无需调整
- [ ] 1000+ 节点集群设置 `percentageOfNodesToScore` 为 10-25%
- [ ] 5000+ 节点集群设置 `percentageOfNodesToScore` 为 5-15%
- [ ] 监控 `scheduler_scheduling_attempt_duration_seconds` p99 延迟
- [ ] 监控 `scheduler_schedule_attempts_total` 吞吐量
- [ ] 批量工作负载场景评估启用 Opportunistic Batching
- [ ] 禁用不需要的 Score 插件减少评分开销
- [ ] 配置调度器 HA（多副本 + Leader Election）

## 命令快速参考

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看调度器当前配置
kubectl get cm -n kube-system kube-scheduler-config -o yaml

# 查看调度延迟指标
kubectl port-forward -n kube-system svc/kube-scheduler 10259:10259
curl -sk https://localhost:10259/metrics | grep scheduler_scheduling_attempt_duration

# 查看调度吞吐量
curl -sk https://localhost:10259/metrics | grep scheduler_schedule_attempts_total

# 查看 pending Pod 队列深度
curl -sk https://localhost:10259/metrics | grep scheduler_pending_pods

# 查看调度器 Leader 信息
kubectl get leases -n kube-system kube-scheduler -o yaml

# 查看调度器资源使用
kubectl top pod -n kube-system -l component=kube-scheduler
```
## 交叉引用

- [Kubernetes 调度器](./kubernetes-scheduler.md) — 过滤与评分两阶段流程
- [调度框架](./scheduling-framework.md) — 各扩展点的性能影响
- [资源装箱](./resource-bin-packing.md) — MostAllocated 策略的评分效率
- [Pod 拓扑分布约束](./pod-topology-spread-constraints.md) — 拓扑分布与 Batching 的冲突

## 参考链接

- [Kubernetes 官方文档 - Scheduler Performance Tuning](https://kubernetes.io/docs/concepts/scheduling-eviction/scheduler-perf-tuning/)

## Related
- [[domain-19-landscape-references/topic-index/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]


<!-- risk-assessed -->
