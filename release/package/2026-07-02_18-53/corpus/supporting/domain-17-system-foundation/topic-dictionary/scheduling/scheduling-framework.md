---
title: Scheduling Framework
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
- gpu
- nvidia
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Scheduling Framework 是什么
- 如何 Scheduling Framework
trigger_keywords:
- Scheduling
- Framework
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Scheduling Framework

## 概述

调度框架（Scheduling Framework）是 [[Kubernetes|Kubernetes]] 调度器的可插拔架构。它由一组直接编译到调度器中的"插件"API 组成。这些 API 允许将大多数调度功能实现为插件，同时保持调度核心轻量且可维护。该功能在 Kubernetes v1.19 中达到 stable 状态。

## 核心概念/原理

每次调度一个 Pod 的尝试分为两个阶段：**调度周期（scheduling cycle）** 和 **绑定周期（binding cycle）**。

- **调度周期**：为 Pod 选择一个节点。
- **绑定周期**：将该决策应用到集群中。

调度周期和绑定周期合称为"调度上下文"。调度周期串行运行，而绑定周期可以并发运行。如果 Pod 被确定为不可调度或发生内部错误，调度或绑定周期可以被中止，Pod 会返回到队列中重试。

## 关键机制或特性

### 扩展点（Extension Points）

1. **PreEnqueue**：在将 Pod 添加到内部活动队列之前调用。只有所有 PreEnqueue 插件返回 `Success`，Pod 才能进入活动队列。
2. **EnqueueExtension**：插件可以控制是否根据集群变化重试被该插件拒绝的 Pod 的调度。
3. **QueueingHint**（v1.34+ stable）：决定 Pod 是否可以重新排队到活动队列或退避队列的回调函数。
4. **QueueSort**：用于对调度队列中的 Pod 进行排序。一次只能启用一个队列排序插件。
5. **PreFilter**：用于预处理 Pod 信息或检查集群/Pod 必须满足的某些条件。
6. **Filter**：用于过滤掉无法运行 Pod 的节点。如果某个 Filter 插件将节点标记为不可行，则不会为该节点调用剩余的 Filter 插件。
7. **PostFilter**：仅在未找到可行节点时调用。典型的实现是抢占（preemption），尝试通过驱逐其他 Pod 使当前 Pod 可调度。
8. **PreScore**：执行"预评分"工作，生成 [[Score|Score]] 插件可共享的状态。
9. **Score**：对通过过滤阶段的节点进行排名。调度器会调用每个评分插件为每个节点打分。
10. **NormalizeScore**：在计算最终节点排名之前修改分数。
11. **Reserve**：在调度器实际绑定 Pod 之前执行，防止竞争条件。包含 `Reserve` 和 `Unreserve` 两个方法。
12. **Permit**：在调度周期结束时调用，可以阻止或延迟绑定到候选节点。支持 approve、deny 或 wait（带超时）。
13. **PreBind**：在 Pod 绑定之前执行所需的工作（如准备网络卷）。
14. **Bind**：将 Pod 绑定到节点。如果某个 Bind 插件选择处理该 Pod，则跳过剩余的 Bind 插件。
15. **PostBind**：在 Pod 成功绑定后调用，用于清理相关资源。

### 容量评分（Capacity Scoring）

v1.33+ alpha（`StorageCapacityScoring` 特性门控）：扩展 VolumeBinding 插件，根据节点上的存储容量对节点进行评分，适用于支持 Storagege Capacity（存储容量）|Storage Capacity]] 的 CSI 卷。

## 使用场景

- 需要实现自定义调度逻辑时，可以通过调度框架编写插件。
- 企业需要根据不同的工作负载类型配置不同的调度策略时，可以配置多个调度配置文件（scheduler profiles）。
- 需要在绑定前执行额外准备工作（如动态卷准备）时，可以实现 PreBind 插件。

## 最佳实践/注意事项

- `Unreserve` 方法的实现必须是幂等的，且不能失败。
- 调度周期串行执行，绑定周期可并发执行。
- 大多数调度插件在 Kubernetes v1.18+ 中默认启用。
- 可以配置多个调度配置文件以适应不同类型的负载。

## 生产 YAML 示例

### 自定义调度配置文件（多插件组合）

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
  - schedulerName: default-scheduler
    plugins:
      preFilter:
        enabled:
          - name: NodeResourcesFit
          - name: NodePorts
          - name: VolumeBinding
      filter:
        enabled:
          - name: NodeResourcesFit
          - name: NodePorts
          - name: NodeAffinity
          - name: VolumeBinding
          - name: TaintToleration
      preScore:
        enabled:
          - name: InterPodAffinity
          - name: TaintToleration
      score:
        enabled:
          - name: NodeResourcesBalancedAllocation
            weight: 1
          - name: ImageLocality
            weight: 1
          - name: InterPodAffinity
            weight: 1
          - name: NodeAffinity
            weight: 1
          - name: TaintToleration
            weight: 3
      reserve:
        enabled:
          - name: VolumeBinding
      preBind:
        enabled:
          - name: VolumeBinding
      bind:
        enabled:
          - name: DefaultBinder
  # GPU 工作负载专用配置文件
  - schedulerName: gpu-scheduler
    plugins:
      score:
        enabled:
          - name: NodeResourcesFit
            weight: 5                      # GPU 节点优先按资源利用率评分
        disabled:
          - name: NodeResourcesBalancedAllocation
    pluginConfig:
      - name: NodeResourcesFit
        args:
          scoringStrategy:
            type: MostAllocated            # GPU 节点采用装箱策略
            resources:
              - name: nvidia.com/gpu
                weight: 10
              - name: cpu
                weight: 1
              - name: memory
                weight: 1
```

### 调度框架扩展点流程图

```
Pod 入队 → [PreEnqueue] → 活动队列 → [QueueSort] → 调度周期开始
  ↓
[PreFilter] → [Filter] → 可行节点列表
  ↓
[PostFilter]（仅在无可行节点时）→ 尝试抢占
  ↓
[PreScore] → [Score] → [NormalizeScore] → 选择最优节点
  ↓
[Reserve] → [Permit] → 绑定周期开始
  ↓
[PreBind] → [Bind] → [PostBind] → 完成
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 自定义插件导致调度器崩溃 | 插件实现有 bug 或 panic | 查看调度器日志 `kubectl logs -n kube-system -l component=kube-scheduler` |
| Reserve 后绑定失败 | Unreserve 未正确清理状态 | 确认 Unreserve 方法幂等且不返回错误 |
| Permit 阶段 Pod 超时 | wait 超时时间设置过短 | 调整 Permit 插件的超时参数 |
| PostFilter 未触发抢占 | PostFilter 插件被禁用 | 确认 `DefaultPreemption` 插件已启用 |
| VolumeBinding PreBind 失败 | PV 准备未就绪 | 检查 PVC 绑定状态和 CSI 驱动日志 |

## 生产检查清单

- [ ] 为不同工作负载类型配置独立的 schedulerName 和 profile
- [ ] 确认 Unreserve 方法实现幂等且不会失败
- [ ] 禁用不需要的插件减少调度开销
- [ ] 启用 VolumeBinding 插件确保存储卷在绑定前就绪
- [ ] 为 GPU 工作负载配置 MostAllocated 装箱策略
- [ ] 监控各扩展点的延迟指标：`scheduler_plugin_execution_duration_seconds`
- [ ] 测试自定义插件在异常场景下的行为

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看调度器启用的插件
kubectl logs -n kube-system -l component=kube-scheduler --tail=50 | grep -i plugin

# 查看插件执行延迟
curl -sk https://localhost:10259/metrics | grep scheduler_plugin_execution_duration

# 查看各阶段耗时
curl -sk https://localhost:10259/metrics | grep scheduler_framework_extension_point_duration

# 查看 Pod 使用的调度器
kubectl get pod <pod-name> -o jsonpath='{.spec.schedulerName}'

# 验证多配置文件生效
kubectl get pods --all-namespaces -o custom-columns='NAME:.metadata.name,SCHEDULER:.spec.schedulerName'
```
## 交叉引用

- [Kubernetes 调度器](./kubernetes-scheduler.md) — 调度器整体架构
- [调度器性能调优](./scheduler-performance-tuning.md) — 插件层面的性能优化
- [资源装箱](./resource-bin-packing.md) — NodeResourcesFit 插件的评分策略配置
- [Pod 优先级与抢占](./pod-priority-and-preemption.md) — PostFilter 阶段的抢占实现
- [动态资源分配](./dynamic-resource-allocation.md) — DRA 与 Reserve/PreBind 阶段的交互

## 参考链接

- [Kubernetes 官方文档 - Scheduling Framework](https://kubernetes.io/docs/concepts/scheduling-eviction/scheduling-framework/)

## Related

- [[domain-17-system-foundation/topic-dictionary/scheduling/affinity.md|亲和性]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/anti-affinity.md|反亲和性]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/api-initiated-eviction.md|API-initiated Eviction]]


<!-- risk-assessed -->
